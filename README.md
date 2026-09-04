# esp32-modulo-riego

ESP32 firmware that exposes the board's GPIO pins over a WebSocket connection,
so pins can be configured, written and read remotely as JSON commands.

Built with PlatformIO for the Arduino framework. It is the device side of a
small IoT system: the browser UI is
[esp32-control-panel](https://github.com/BHS551/esp32-control-panel) and the
AWS backend is
[esp32-control-server](https://github.com/BHS551/esp32-control-server).

## The problem it solves

Controlling a microcontroller from the internet usually means writing a custom
protocol and then re-flashing the board every time the control logic changes.

This firmware inverts that: it ships a **generic remote-GPIO protocol** and
keeps no application logic at all. Whether a pin drives an irrigation valve, a
lamp or a relay is decided by whatever sends the commands. The board is
flashed once and the behaviour lives on the other end of the socket.

## Protocol

The board opens a WebSocket client to the configured host and waits. Every
message is JSON with a `type` and a `body`.

**Commands in** (`type: "cmd"`):

```json
{ "type": "cmd", "body": { "type": "pinMode",      "pin": 18, "mode": "output" } }
{ "type": "cmd", "body": { "type": "digitalWrite", "pin": 18, "value": 1 } }
{ "type": "cmd", "body": { "type": "digitalRead",  "pin": 18 } }
```

`mode` accepts `output`, `input_pullup` or `input` — anything unrecognised
falls back to `input`.

**Messages out:**

```json
{ "action": "msg", "type": "status", "body": "ok" }        // command applied
{ "action": "msg", "type": "output", "body": 1 }           // digitalRead result
{ "action": "msg", "type": "error",  "body": "…" }         // rejected
```

## Key technical decisions

**Every failure answers over the socket.** A malformed JSON body, a missing
`type`, an unknown command — each sends an `error` message back rather than
failing silently. On a headless board with no display, an unanswered command is
indistinguishable from a dead board, so the firmware always says why.

**Parsing is validated before dispatch.** `deserializeJson` is checked, then the
message `type`, then the command body's shape, before any pin is touched. A
half-parsed command never reaches `digitalWrite`.

**Fixed-size buffers, no dynamic allocation.** `StaticJsonDocument<2048>` and a
256-byte message buffer keep memory deterministic — heap fragmentation is what
makes a long-running microcontroller fall over after days of uptime.

**`digitalRead` replies rather than acknowledging.** Writes answer `status: ok`;
reads answer `output` with the value, so the caller distinguishes "done" from
"here is the state".

**WiFiMulti over plain WiFi.** The board can be given several networks and
joins whichever is reachable.

## Building and flashing

Requires [PlatformIO](https://platformio.org/) and a `nodemcu-32s` board.

```bash
pio run                 # build
pio run -t upload       # flash
pio device monitor      # serial log, 921600 baud
```

Before flashing, set the values at the top of `src/main.cpp`:

| `#define` | Meaning |
|---|---|
| `WIFI_SSID` / `WIFI_PASS` | Network the board joins |
| `WS_HOST` | WebSocket host, e.g. an API Gateway endpoint |
| `WS_PORT` / `WS_URL` | Port (443) and path (`/dev`) |

Dependencies are resolved by PlatformIO from `platformio.ini`:
`links2004/WebSockets` and `bblanchon/ArduinoJson`.

> Set `upload_port` in `platformio.ini` to match your serial port; it is
> currently pinned to a Windows `COM` port.

> **Note.** This project is on the `master` branch; the repository's default
> branch holds only a stub README.
