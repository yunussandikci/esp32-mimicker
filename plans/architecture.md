# ESP32 HID Mimicker — Architecture

## File Layout

```
src/
├── main.cpp            — wires services
├── config.h            — constants (no credentials)
├── wifi_service.h      — NVS creds + AP/STA mode, captive DNS
├── web_service.h       — HTTP server + two HTML pages
├── script_service.h    — non-blocking script engine (persisted to NVS)
└── leds_service.h      — onboard WS2812 status
```

Every module follows the `namespace foo { init(); loop(); }` shape so
`main.cpp::loop()` reads as a flat list of sibling calls.

## State Machine

```
BOOT
├─ NVS has creds?
│   ├─ NO  → AP_MODE       (LED: magenta — waiting for configuration)
│   └─ YES → STA_CONNECTING (LED: yellow)
│              ├─ Success  → STA_READY (LED: blue idle / green running)
│              └─ Timeout (15s) → wipe NVS, restart → AP_MODE
└─ STA_READY: connection drops for 30s → wipe NVS, restart → AP_MODE
```

The post-restart trip back through AP mode is the only "reconfigure WiFi"
affordance — no extra reset gesture needed.

## LED Palette (onboard WS2812 on GPIO 21)

| State | Color | Trigger |
|---|---|---|
| Connecting (STA) | 🟡 yellow | `wifi::init()` running |
| Setup (AP) | 🟣 magenta | No creds, or STA failed |
| Idle (STA) | 🔵 blue | Connected, no script running |
| Running | 🟢 green | `script::running == true` |

`leds::loop()` only drives blue/green; AP-magenta and connecting-yellow are
one-shots set during boot.

## HTTP Routes

Both modes share a single `WebServer` on port 80.

| Route | Method | AP mode | STA mode |
|---|---|---|---|
| `/` | GET | Setup page | Block builder |
| `/wifi` | GET | JSON scan results | (same) |
| `/wifi` | POST | Save creds → restart | (same) |
| `/script` | GET | Returns current script | (same) |
| `/save` | POST | Replaces script | (same) |
| `/start` `/stop` | POST | Toggles `script::running` | (same) |
| `/status` | GET | `{running}` JSON | (same) |
| `/ip` | GET | Plain-text IP | (same) |
| (unknown) | * | Falls through to `/` (captive portal) | 404 |

## NVS

Two `Preferences` namespaces:

- `"wifi"` — keys `"ssid"`, `"pass"`. Empty-string default acts as the "no creds saved" sentinel that triggers AP-setup mode.
- `"script"` — key `"text"`. Loaded into `script::text` at boot; written on every `script::save()`. Lets user scripts survive power-cycles.

`pio run -t erase` wipes both, forcing a fresh AP-setup flow and the default hello-world script.

## AP Mode Details

- Open WiFi (no password) so captive-portal detection on phones pops the
  setup page automatically
- DNS server on port 53 answers `*` queries with `192.168.4.1`
- Mode is `WIFI_AP_STA` (not `WIFI_AP`) so `WiFi.scanNetworks()` works for
  the setup-page dropdown
- `softAPIP()` is the default `192.168.4.1`

## Script Engine

Single non-blocking state machine in `script::loop()`. One line per loop
iteration; `WAIT` defers via `millis() < wait_until`. End-of-script
auto-stops (no separate "armed but finished" state). Commands:

| Command | Effect |
|---|---|
| `WAIT <ms>` | Defer next-line execution |
| `KEY <name>` | Press + release (auto-Shift for upper/symbol) |
| `KEYDOWN`/`KEYUP <name>` | Sticky hold/release |
| `STRING` / `STRINGLN <text>` | `Print` / `Println` |
| `MOUSE <dx> <dy>` | Relative move |
| `WHEEL <amt>` | Scroll |
| `CLICK` / `PRESS` / `RELEASE <L\|R\|M>` | Mouse buttons |
| `REPEAT` | Reset cursor to start |
| `// …` | Comment |

## USB Identity

Spoofed Logitech G413 (VID 0x046D, PID 0xC33A) — chosen because most
operating systems trust it without driver prompts.
