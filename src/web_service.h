#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "script_service.h"
#include "wifi_service.h"
#include "keymaps.h"

namespace web {

inline WebServer server(cfg::HTTP_PORT);


static const char SETUP_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HID Mimicker Setup</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:linear-gradient(135deg,#f5f7fa,#e4e9f2);min-height:100vh;padding:16px;color:#333}
.container{max-width:440px;margin:60px auto}
.card{background:#fff;border-radius:16px;padding:28px;box-shadow:0 2px 20px rgba(0,0,0,.08)}
h1{font-size:22px;color:#2c3e6b;margin-bottom:6px}
.subtitle{color:#888;margin-bottom:22px;font-size:14px}
label{display:block;margin-bottom:6px;font-size:13px;font-weight:600;color:#555}
select,input{width:100%;padding:10px 12px;border:1px solid #d0d5e0;border-radius:8px;font-size:14px;margin-bottom:14px;outline:none;background:#fff}
select:focus,input:focus{border-color:#3b82f6;box-shadow:0 0 0 2px rgba(59,130,246,.15)}
.btn{width:100%;padding:12px;background:#2c3e6b;color:#fff;border:none;border-radius:10px;font-size:14px;font-weight:600;cursor:pointer}
.btn:hover{background:#3b5290}
.btn:disabled{background:#d0d5e0;color:#999;cursor:not-allowed}
.msg{margin-top:14px;padding:12px;border-radius:8px;text-align:center;font-size:13px;display:none}
.msg.ok{display:block;background:#ecfdf5;color:#059669}
.msg.err{display:block;background:#fef2f2;color:#dc2626}
.refresh{background:none;border:none;color:#3b82f6;cursor:pointer;font-size:12px;margin-bottom:14px;padding:0}
</style></head><body><div class="container"><div class="card">
<h1>WiFi Setup</h1>
<div class="subtitle">Pick your network and enter the password.</div>
<label>Network</label>
<select id="ssid"><option>Scanning...</option></select>
<button class="refresh" onclick="loadNetworks()">Rescan</button>
<label>Password</label>
<input type="password" id="pass" placeholder="(blank for open networks)">
<button class="btn" id="save" onclick="save()">Save &amp; Connect</button>
<div class="msg" id="msg"></div>
</div></div>
<script>
function loadNetworks(){
  const sel=document.getElementById('ssid');
  sel.innerHTML='<option>Scanning...</option>';
  fetch('/wifi').then(r=>r.json()).then(d=>{
    sel.innerHTML='';
    d.networks.forEach(n=>{
      const o=document.createElement('option');
      o.value=n.ssid;
      o.textContent=n.ssid+' ('+n.rssi+'dBm)';
      sel.appendChild(o);
    });
    if(!d.networks.length){
      sel.innerHTML='<option>(none found)</option>';
    }
  }).catch(()=>{
    sel.innerHTML='<option>(scan failed)</option>';
  });
}
function save(){
  const ssid=document.getElementById('ssid').value;
  const pass=document.getElementById('pass').value;
  const btn=document.getElementById('save');
  const msg=document.getElementById('msg');
  btn.disabled=true;
  msg.className='msg';
  fetch('/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)})
  .then(r=>{
    if(!r.ok)throw 0;
    msg.className='msg ok';
    msg.textContent='Saved. Device restarting — reconnect to your normal WiFi to use the mimicker.';
  }).catch(()=>{
    msg.className='msg err';
    msg.textContent='Save failed';
    btn.disabled=false;
  });
}
loadNetworks();
</script></body></html>
)HTML";


static const char PAGE_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HID Mimicker</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:linear-gradient(135deg,#f5f7fa,#e4e9f2);min-height:100vh;padding:16px;color:#333}
.container{max-width:680px;margin:0 auto}
.card{background:#fff;border-radius:16px;padding:24px;box-shadow:0 2px 20px rgba(0,0,0,.08);margin-bottom:14px}
h1{font-size:20px;color:#2c3e6b;display:flex;align-items:center;gap:10px}
.header-row{display:flex;justify-content:space-between;align-items:center;gap:16px;flex-wrap:wrap}
.layout-select{display:flex;align-items:center;gap:8px}
.layout-select label{font-size:11px;color:#888;font-weight:600;text-transform:uppercase;letter-spacing:.5px}
.layout-select select{padding:6px 10px;border:1px solid #d0d5e0;border-radius:6px;font-size:13px;background:#fff;outline:none;cursor:pointer}
.layout-select select:focus{border-color:#3b82f6;box-shadow:0 0 0 2px rgba(59,130,246,.15)}
.palette{display:flex;flex-wrap:wrap;gap:6px;padding:12px;background:#f8f9fc;border-radius:12px;border:2px dashed #d0d5e0;margin-bottom:12px}
.block{display:inline-flex;align-items:center;gap:6px;padding:6px 14px;border-radius:8px;font-size:13px;font-weight:600;cursor:grab;user-select:none;color:#fff;transition:transform .15s}
.block:active{cursor:grabbing;transform:scale(.93)}
.block:hover{transform:translateY(-1px);box-shadow:0 4px 12px rgba(0,0,0,.15)}
.b-wait{background:#f59e0b}.b-key{background:#3b82f6}.b-str{background:#10b981}
.b-mouse{background:#8b5cf6}.b-click{background:#ef4444}.b-rpt{background:#06b6d4}
.builder{min-height:60px;padding:10px;background:#f8f9fc;border-radius:12px;border:2px dashed #d0d5e0;transition:background .2s}
.builder.drag-over{background:#e8ecf6;border-color:#3b82f6}
.builder:empty::after{content:'Drop blocks here';display:block;text-align:center;color:#bbb;padding:20px;font-size:14px}
.block-row{display:flex;align-items:center;gap:8px;padding:7px 10px;margin-bottom:5px;background:#fff;border-radius:8px;border:1px solid #e5e9f0}
.block-row:last-child{margin-bottom:0}
.block-row.dragging{opacity:.4}
.drag-handle{cursor:grab;color:#bbb;font-size:16px;padding:0 4px}
.block-tag{font-size:11px;font-weight:700;padding:2px 8px;border-radius:4px;color:#fff;min-width:48px;text-align:center}
.block-row .param{flex:1;min-width:0}
.block-row input,.block-row select{width:100%;padding:5px 8px;border:1px solid #d0d5e0;border-radius:6px;font-size:13px;outline:none;background:#fff}
.block-row input:focus,.block-row select:focus{border-color:#3b82f6;box-shadow:0 0 0 2px rgba(59,130,246,.15)}
.del-btn{background:none;border:none;color:#ccc;font-size:18px;cursor:pointer;padding:0 4px}
.del-btn:hover{color:#ef4444}
.controls{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
.btn{padding:10px 24px;border:none;border-radius:10px;font-size:14px;font-weight:600;cursor:pointer;transition:.2s;display:inline-flex;align-items:center;gap:6px}
.btn-start{background:#10b981;color:#fff}.btn-start:hover{background:#059669}
.btn-stop{background:#ef4444;color:#fff}.btn-stop:hover{background:#dc2626}
.btn:disabled{background:#d0d5e0;color:#999;cursor:not-allowed}
.status{margin-top:10px;padding:10px 14px;border-radius:10px;font-size:13px;display:flex;align-items:center;gap:8px}
.s-idle{background:#f8f9fc;color:#888}.s-ready{background:#ecfdf5;color:#059669}.s-running{background:#fef2f2;color:#dc2626}
.dot{width:8px;height:8px;border-radius:50%}
.d-idle{background:#bbb}.d-ready{background:#10b981}.d-running{background:#ef4444}
.raw{margin-top:14px}
.raw-label{display:flex;justify-content:space-between;align-items:center;margin-bottom:6px;gap:10px}
.raw-label .title{font-size:11px;color:#888;font-weight:600;text-transform:uppercase;letter-spacing:.5px}
.raw-meta{display:flex;align-items:center;gap:12px}
.raw-meta .size{font-size:11px;color:#888;font-variant-numeric:tabular-nums}
.raw-meta .size.warn{color:#f59e0b;font-weight:700}
.raw-meta .size.danger{color:#ef4444;font-weight:700}
.save-state{display:inline-flex;align-items:center;gap:5px;font-size:11px;color:#888}
.save-state .save-dot{width:7px;height:7px;border-radius:50%;flex-shrink:0;background:#9ca3af}
.save-state.saved .save-dot{background:#10b981}
.save-state.saving .save-dot{background:#f59e0b;animation:pulse 1s infinite}
.save-state.unsaved .save-dot{background:#9ca3af}
.save-state.error .save-dot{background:#ef4444}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.35}}
.raw-label button{background:none;border:none;color:#3b82f6;cursor:pointer;font-size:12px;padding:2px 6px;border-radius:4px}
.raw-label button:hover{background:#eff6ff}
.raw textarea{width:100%;font-family:'SF Mono',Menlo,Monaco,Consolas,monospace;font-size:12.5px;line-height:1.5;padding:10px 12px;border:1px solid #e5e9f0;border-radius:8px;background:#f8f9fc;resize:vertical;min-height:140px;outline:none;color:#333}
.raw textarea:focus{border-color:#3b82f6;background:#fff;box-shadow:0 0 0 2px rgba(59,130,246,.15)}
</style></head><body><div class="container">
<div class="card" style="padding:16px 24px">
<div class="header-row">
<h1>HID Mimicker</h1>
<div class="layout-select">
<label for="layout">Target layout</label>
<select id="layout">
<option value="US">US — English</option>
<option value="UK">UK — English</option>
<option value="DE">German (QWERTZ)</option>
<option value="FR">French (AZERTY)</option>
<option value="ES">Spanish</option>
<option value="IT">Italian</option>
<option value="PT">Portuguese</option>
<option value="TR_Q">Turkish-Q</option>
<option value="TR_F">Turkish-F</option>
</select>
</div>
</div>
</div>
<div class="card">
<div class="palette" id="palette">
<div class="block b-wait" draggable="true" data-cmd="WAIT">WAIT</div>
<div class="block b-key" draggable="true" data-cmd="KEY">KEY</div>
<div class="block b-key" draggable="true" data-cmd="KEYDOWN">KEYDOWN</div>
<div class="block b-key" draggable="true" data-cmd="KEYUP">KEYUP</div>
<div class="block b-str" draggable="true" data-cmd="STRING">STRING</div>
<div class="block b-str" draggable="true" data-cmd="STRINGLN">STRINGLN</div>
<div class="block b-mouse" draggable="true" data-cmd="MOUSE">MOUSE</div>
<div class="block b-mouse" draggable="true" data-cmd="WHEEL">WHEEL</div>
<div class="block b-click" draggable="true" data-cmd="CLICK">CLICK</div>
<div class="block b-click" draggable="true" data-cmd="PRESS">PRESS</div>
<div class="block b-click" draggable="true" data-cmd="RELEASE">RELEASE</div>
<div class="block b-rpt" draggable="true" data-cmd="REPEAT">REPEAT</div>
</div>
<div class="builder" id="builder"></div>
<div class="raw">
<div class="raw-label">
<span class="title">Script</span>
<div class="raw-meta">
<span class="size" id="size">0 / 16384</span>
<span class="save-state saved" id="saveState"><span class="save-dot"></span><span id="saveText">Saved</span></span>
<button onclick="copyRaw()">Copy</button>
</div>
</div>
<textarea id="rawText" spellcheck="false" placeholder="WAIT 1000&#10;STRING hello&#10;KEY ENTER"></textarea>
</div>
<div class="controls">
<button class="btn btn-start" id="btnStart" onclick="doStart()">Start</button>
<button class="btn btn-stop" id="btnStop" onclick="doStop()" disabled>Stop</button>
</div>
<div class="status s-idle" id="status">
<span class="dot d-idle" id="dot"></span><span id="msg">Drop blocks to begin</span>
</div>
</div></div>
<script>
const MAX_SCRIPT = 16384;
const builder = document.getElementById('builder');
const rawText = document.getElementById('rawText');
const sizeEl = document.getElementById('size');
const saveStateEl = document.getElementById('saveState');
const saveTextEl = document.getElementById('saveText');
const layoutEl = document.getElementById('layout');

const KEY_OPTIONS = [
  ['Modifiers',  ['SHIFT','CTRL','ALT','GUI']],
  ['Navigation', ['UP','DOWN','LEFT','RIGHT','HOME','END','PGUP','PGDOWN']],
  ['Editing',    ['ENTER','TAB','BACKSPACE','DELETE','INSERT','SPACE','ESC']],
  ['Function',   ['F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','F11','F12']],
  ['Special',    ['CAPSLOCK']],
];

const CFG = {
  WAIT:    {c:'b-wait', i:[{n:'ms',t:'number',v:'1000',min:0,max:60000}]},
  KEY:     {c:'b-key',  i:[{n:'key',t:'keysel',v:'ENTER'}]},
  KEYDOWN: {c:'b-key',  i:[{n:'key',t:'keysel',v:'CTRL'}]},
  KEYUP:   {c:'b-key',  i:[{n:'key',t:'keysel',v:'CTRL'}]},
  STRING:  {c:'b-str',  i:[{n:'text',t:'text',v:'hello'}]},
  STRINGLN:{c:'b-str',  i:[{n:'text',t:'text',v:'hello'}]},
  MOUSE:   {c:'b-mouse',i:[{n:'x',t:'number',v:'10',min:-127,max:127},{n:'y',t:'number',v:'10',min:-127,max:127}]},
  WHEEL:   {c:'b-mouse',i:[{n:'amt',t:'number',v:'1',min:-127,max:127}]},
  CLICK:   {c:'b-click',i:[{n:'btn',t:'sel',o:[{v:'L',l:'Left'},{v:'R',l:'Right'},{v:'M',l:'Middle'}],v:'L'}]},
  PRESS:   {c:'b-click',i:[{n:'btn',t:'sel',o:[{v:'L',l:'Left'},{v:'R',l:'Right'},{v:'M',l:'Middle'}],v:'L'}]},
  RELEASE: {c:'b-click',i:[{n:'btn',t:'sel',o:[{v:'L',l:'Left'},{v:'R',l:'Right'},{v:'M',l:'Middle'}],v:'L'}]},
  REPEAT:  {c:'b-rpt',  i:[]}
};

let initialized = false;
let saved = '';
let saveTimer = null;

function setSaveState(state){
  const labels = {saved:'Saved', saving:'Saving…', unsaved:'Unsaved', error:'Save failed'};
  saveStateEl.className = 'save-state ' + state;
  saveTextEl.textContent = labels[state];
}

function updateSize(){
  const len = rawText.value.length;
  sizeEl.textContent = len + ' / ' + MAX_SCRIPT;
  if (len >= MAX_SCRIPT) sizeEl.className = 'size danger';
  else if (len > MAX_SCRIPT * 0.9) sizeEl.className = 'size warn';
  else sizeEl.className = 'size';
}

function postState(body){
  return fetch('/state', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(body)});
}

function scheduleSave(){
  if (!initialized) return;
  setSaveState('unsaved');
  clearTimeout(saveTimer);
  saveTimer = setTimeout(flushSave, 600);
}

function flushSave(){
  clearTimeout(saveTimer);
  const s = rawText.value;
  if (s === saved) { setSaveState('saved'); return Promise.resolve(); }
  setSaveState('saving');
  return postState({script: s})
    .then(() => {
      saved = s;
      if (s === rawText.value) setSaveState('saved');
      else scheduleSave();
    })
    .catch(() => setSaveState('error'));
}

document.querySelectorAll('#palette .block').forEach(b => {
  b.addEventListener('dragstart', e => e.dataTransfer.setData('cmd', b.dataset.cmd));
  b.addEventListener('click', () => addBlock(b.dataset.cmd));
});
builder.addEventListener('dragover', e => { e.preventDefault(); builder.classList.add('drag-over'); });
builder.addEventListener('dragleave', () => builder.classList.remove('drag-over'));
builder.addEventListener('drop', e => {
  e.preventDefault(); builder.classList.remove('drag-over');
  const cmd = e.dataTransfer.getData('cmd'); if (cmd) addBlock(cmd);
});

function mkInp(x){
  if (x.t === 'keysel') {
    const s = document.createElement('select'); s.dataset.name = x.n;
    KEY_OPTIONS.forEach(([label, keys]) => {
      const og = document.createElement('optgroup'); og.label = label;
      keys.forEach(k => {
        const op = document.createElement('option');
        op.value = k; op.textContent = k;
        og.appendChild(op);
      });
      s.appendChild(og);
    });
    s.value = x.v;
    return s;
  }
  if (x.t === 'sel') {
    const s = document.createElement('select'); s.dataset.name = x.n;
    x.o.forEach(o => {
      const op = document.createElement('option');
      op.value = (typeof o === 'string') ? o : o.v;
      op.textContent = (typeof o === 'string') ? o : o.l;
      s.appendChild(op);
    });
    s.value = x.v;
    return s;
  }
  const i = document.createElement('input');
  i.type = x.t; i.dataset.name = x.n; i.value = x.v || '';
  if (x.min !== undefined) i.min = x.min;
  if (x.max !== undefined) i.max = x.max;
  return i;
}

function addBlock(cmd){
  const c = CFG[cmd]; if (!c) return;
  const row = document.createElement('div');
  row.className = 'block-row'; row.draggable = true; row.dataset.cmd = cmd;
  row.innerHTML = '<span class="drag-handle">⠇</span><span class="block-tag ' + c.c + '">' + cmd + '</span>';
  const pd = document.createElement('div'); pd.className = 'param';
  if (c.i.length === 1) pd.appendChild(mkInp(c.i[0]));
  else if (c.i.length > 1) {
    const fx = document.createElement('div'); fx.style.cssText = 'display:flex;gap:6px';
    c.i.forEach(x => { const inp = mkInp(x); inp.style.flex = '1'; fx.appendChild(inp); });
    pd.appendChild(fx);
  }
  row.appendChild(pd);
  const del = document.createElement('button');
  del.className = 'del-btn'; del.textContent = '×';
  del.onclick = () => { row.remove(); syncText(); };
  row.appendChild(del);
  row.addEventListener('input', syncText);
  row.addEventListener('change', syncText);
  row.addEventListener('dragstart', e => { row.classList.add('dragging'); e.dataTransfer.effectAllowed = 'move'; });
  row.addEventListener('dragend', () => { row.classList.remove('dragging'); syncText(); });
  row.addEventListener('dragover', e => {
    e.preventDefault();
    const dr = builder.querySelector('.dragging'); if (!dr || dr === row) return;
    const r = row.getBoundingClientRect();
    if (e.clientY < r.top + r.height / 2) builder.insertBefore(dr, row);
    else builder.insertBefore(dr, row.nextSibling);
  });
  builder.appendChild(row);
  syncText();
}

function syncText(){
  rawText.value = genScript();
  updateSize();
  scheduleSave();
}

function genScript(){
  const lines = [];
  builder.querySelectorAll('.block-row').forEach(r => {
    const cmd = r.dataset.cmd;
    const vals = Array.from(r.querySelectorAll('input,select')).map(i => i.value);
    lines.push(vals.length ? cmd + ' ' + vals.join(' ') : cmd);
  });
  return lines.join('\r\n');
}

function parseScript(text){
  builder.innerHTML = '';
  text.split('\n').forEach(line => {
    line = line.trim(); if (!line || line.startsWith('//')) return;
    const p = line.split(/\s+/); const cmd = p[0].toUpperCase();
    if (!CFG[cmd]) return;
    addBlock(cmd);
    const r = builder.lastElementChild;
    if (!r) return;
    const inputs = r.querySelectorAll('input,select');
    if (cmd === 'STRING' || cmd === 'STRINGLN') {
      if (inputs[0]) inputs[0].value = line.substring(cmd.length + 1);
    } else {
      inputs.forEach((i, idx) => { if (p[idx + 1] !== undefined) i.value = p[idx + 1]; });
    }
  });
}

function setStatus(s, t){
  document.getElementById('status').className = 'status s-' + s;
  document.getElementById('dot').className = 'dot d-' + s;
  document.getElementById('msg').textContent = t;
}

function fetchState(){ return fetch('/state').then(r => r.json()); }

function poll(){
  fetchState().then(d => {
    const empty = rawText.value.trim().length === 0;
    document.getElementById('btnStart').disabled = d.running || empty;
    document.getElementById('btnStop').disabled  = !d.running;
    if (d.running) setStatus('running', 'Running…');
    else if (empty) setStatus('idle', 'Drop blocks to begin');
    else setStatus('ready', 'Ready to start');
  }).catch(() => {});
}

async function doStart(){
  await flushSave();
  await postState({run:'start'});
  poll();
}

function doStop(){ postState({run:'stop'}).then(poll); }

function copyRaw(){
  rawText.select();
  document.execCommand('copy');
  rawText.setSelectionRange(0, 0); rawText.blur();
}

rawText.addEventListener('input', () => { updateSize(); scheduleSave(); });
rawText.addEventListener('change', () => parseScript(rawText.value));

layoutEl.addEventListener('change', () => postState({layout: layoutEl.value}));

fetchState().then(d => {
  saved = d.script || '';
  rawText.value = saved;
  parseScript(saved);
  rawText.value = saved;
  layoutEl.value = d.layout || 'US';
  updateSize();
  setSaveState('saved');
  initialized = true;
  poll();
});
setInterval(poll, 1500);
</script></body></html>
)HTML";


inline void handleRoot() {
  if (wifi::is_ap()) {
    server.send_P(200, "text/html", SETUP_HTML);
  } else {
    server.send_P(200, "text/html", PAGE_HTML);
  }
}


inline void handleGetState() {
  JsonDocument doc;
  doc["running"] = script::running;
  doc["layout"]  = keymap::name(script::getLayout());
  doc["script"]  = script::text;
  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}


inline void handlePostState() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "text/plain", "bad json");
    return;
  }
  if (doc["script"].is<const char*>()) {
    script::save(doc["script"].as<String>());
  }
  if (doc["layout"].is<const char*>()) {
    script::setLayout(keymap::from_name(doc["layout"].as<String>()));
  }
  if (doc["run"].is<const char*>()) {
    String r = doc["run"].as<String>();
    if (r == "start") script::start();
    else if (r == "stop") script::stop();
  }
  server.send(200, "text/plain", "ok");
}


inline void handleWifiScan() {
  int found = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray arr = doc["networks"].to<JsonArray>();
  for (int i = 0; i < found; i++) {
    JsonObject n = arr.add<JsonObject>();
    n["ssid"] = WiFi.SSID(i);
    n["rssi"] = WiFi.RSSI(i);
  }
  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
  WiFi.scanDelete();
}


inline void handleWifiSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "ssid required");
    return;
  }
  wifi::save_credentials(ssid, pass);
  server.send(200, "text/plain", "saved");
  delay(500);
  ESP.restart();
}


inline void init() {
  server.on("/",      HTTP_GET,  handleRoot);
  server.on("/state", HTTP_GET,  handleGetState);
  server.on("/state", HTTP_POST, handlePostState);
  server.on("/wifi",  HTTP_GET,  handleWifiScan);
  server.on("/wifi",  HTTP_POST, handleWifiSave);
  server.onNotFound(handleRoot);
  server.begin();
}


inline void loop() {
  server.handleClient();
}

} // namespace web
