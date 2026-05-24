#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "script.h"

namespace web {

inline WebServer server(cfg::HTTP_PORT);

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
.btn-save{background:#2c3e6b;color:#fff}.btn-save:hover{background:#3b5290}
.btn-start{background:#10b981;color:#fff}.btn-start:hover{background:#059669}
.btn-stop{background:#ef4444;color:#fff}.btn-stop:hover{background:#dc2626}
.btn:disabled{background:#d0d5e0;color:#999;cursor:not-allowed}
.status{margin-top:10px;padding:10px 14px;border-radius:10px;font-size:13px;display:flex;align-items:center;gap:8px}
.s-idle{background:#f8f9fc;color:#888}.s-ready{background:#ecfdf5;color:#059669}.s-running{background:#fef2f2;color:#dc2626}
.dot{width:8px;height:8px;border-radius:50%}
.d-idle{background:#bbb}.d-ready{background:#10b981}.d-running{background:#ef4444}
.hint{margin-top:8px;font-size:11px;color:#999;text-align:center}
</style></head><body><div class="container">
<div class="card" style="padding:16px 24px"><h1>HID Mimicker</h1></div>
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
<div class="controls">
<button class="btn btn-save" onclick="doSave()">Save</button>
<button class="btn btn-start" id="btnStart" onclick="doStart()">Start</button>
<button class="btn btn-stop" id="btnStop" onclick="doStop()" disabled>Stop</button>
</div>
<div class="status s-idle" id="status">
<span class="dot d-idle" id="dot"></span><span id="msg">Drop blocks, Save, then Start</span>
</div>
<div class="hint">Tip: touch the wired pin to start/stop without the UI.</div>
</div></div>
<script>
const builder = document.getElementById('builder');
const CFG = {
  WAIT:{c:'b-wait',i:[{n:'ms',t:'number',v:'1000'}]},
  KEY:{c:'b-key',i:[{n:'key',t:'text',v:'A'}]},
  KEYDOWN:{c:'b-key',i:[{n:'key',t:'text',v:'W'}]},
  KEYUP:{c:'b-key',i:[{n:'key',t:'text',v:'W'}]},
  STRING:{c:'b-str',i:[{n:'text',t:'text',v:'hello'}]},
  STRINGLN:{c:'b-str',i:[{n:'text',t:'text',v:'hello'}]},
  MOUSE:{c:'b-mouse',i:[{n:'x',t:'number',v:'10'},{n:'y',t:'number',v:'10'}]},
  WHEEL:{c:'b-mouse',i:[{n:'amt',t:'number',v:'1'}]},
  CLICK:{c:'b-click',i:[{n:'btn',t:'sel',o:['L','R','M'],v:'L'}]},
  PRESS:{c:'b-click',i:[{n:'btn',t:'sel',o:['L','R','M'],v:'L'}]},
  RELEASE:{c:'b-click',i:[{n:'btn',t:'sel',o:['L','R','M'],v:'L'}]},
  REPEAT:{c:'b-rpt',i:[]}
};
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
  if (x.t === 'sel') {
    const s = document.createElement('select'); s.dataset.name = x.n;
    x.o.forEach(o => { const op = document.createElement('option'); op.value = o; op.textContent = o; s.appendChild(op); });
    s.value = x.v; return s;
  }
  const i = document.createElement('input'); i.type = x.t; i.dataset.name = x.n; i.value = x.v || ''; return i;
}
function addBlock(cmd){
  const c = CFG[cmd]; if (!c) return;
  const row = document.createElement('div');
  row.className = 'block-row'; row.draggable = true; row.dataset.cmd = cmd;
  row.innerHTML = `<span class="drag-handle">⠿</span><span class="block-tag ${c.c}">${cmd}</span>`;
  const pd = document.createElement('div'); pd.className = 'param';
  if (c.i.length === 1) pd.appendChild(mkInp(c.i[0]));
  else if (c.i.length > 1) {
    const fx = document.createElement('div'); fx.style.cssText = 'display:flex;gap:6px';
    c.i.forEach(x => { const inp = mkInp(x); inp.style.flex = '1'; fx.appendChild(inp); });
    pd.appendChild(fx);
  }
  row.appendChild(pd);
  const del = document.createElement('button');
  del.className = 'del-btn'; del.textContent = '×'; del.onclick = () => row.remove();
  row.appendChild(del);
  row.addEventListener('dragstart', e => { row.classList.add('dragging'); e.dataTransfer.effectAllowed = 'move'; });
  row.addEventListener('dragend', () => row.classList.remove('dragging'));
  row.addEventListener('dragover', e => {
    e.preventDefault();
    const dr = builder.querySelector('.dragging'); if (!dr || dr === row) return;
    const r = row.getBoundingClientRect();
    if (e.clientY < r.top + r.height / 2) builder.insertBefore(dr, row);
    else builder.insertBefore(dr, row.nextSibling);
  });
  builder.appendChild(row);
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
let saved = '';
function setStatus(s, t){
  document.getElementById('status').className = 'status s-' + s;
  document.getElementById('dot').className = 'dot d-' + s;
  document.getElementById('msg').textContent = t;
}
function poll(){
  fetch('/status').then(r => r.json()).then(d => {
    document.getElementById('btnStart').disabled = d.running;
    document.getElementById('btnStop').disabled  = !d.running;
    setStatus(d.running ? 'running' : 'idle',
              d.running ? 'Running...' : 'Drop blocks, Save, then Start');
  }).catch(() => {});
}
function doSave(){
  const s = genScript();
  fetch('/save', {method:'POST', body: s}).then(() => { saved = s; setStatus('ready', 'Saved'); });
}
function doStart(){
  const s = genScript();
  const p = s !== saved ? fetch('/save', {method:'POST', body: s}).then(() => saved = s) : Promise.resolve();
  p.then(() => fetch('/start', {method:'POST'})).then(poll);
}
function doStop(){ fetch('/stop', {method:'POST'}).then(poll); }
fetch('/script').then(r => r.text()).then(s => { saved = s; parseScript(s); });
setInterval(poll, 1500);
</script></body></html>
)HTML";

inline void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}


inline void handleScript() {
  server.send(200, "text/plain", script::text);
}


inline void handleSave() {
  script::save(server.arg("plain"));
  server.send(200, "text/plain", "ok");
}


inline void handleStart() {
  script::start();
  server.send(200, "text/plain", "started");
}


inline void handleStop() {
  script::stop();
  server.send(200, "text/plain", "stopped");
}


inline void handleIP() {
  server.send(200, "text/plain", WiFi.localIP().toString());
}


inline void handleStatus() {
  String body = "{\"running\":";
  body += script::running ? "true" : "false";
  body += "}";
  server.send(200, "application/json", body);
}


inline void init() {
  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/script", HTTP_GET,  handleScript);
  server.on("/save",   HTTP_POST, handleSave);
  server.on("/start",  HTTP_POST, handleStart);
  server.on("/stop",   HTTP_POST, handleStop);
  server.on("/status", HTTP_GET,  handleStatus);
  server.on("/ip",     HTTP_GET,  handleIP);
  server.begin();
}


inline void loop() {
  server.handleClient();
}

} // namespace web
