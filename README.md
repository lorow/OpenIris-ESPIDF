| Supported Targets | ESP32-S3 · Project Babble · Venti · FaceFocusVR · ESP32CAM · ESP32-AIThinker · ESP32-M5Stack · ESP-EYE · Wroom s3 N8R2 · Wroom s3 QIO N8R2 · Wroom s3 N8R8 · WROVER|
| ----------------- | --------------------------------------- |

## OpenIris-ESPIDF

Firmware and tools for OpenIris — Wi‑Fi, UVC streaming, and a Python setup CLI.

---

## What’s inside

- ESP‑IDF firmware (C/C++) with modules for Camera, Wi‑Fi, UVC, REST/Serial commands, and more
- Python tools for setup over USB serial:
  - `tools/switchBoardType.py` — choose a board profile (builds the right sdkconfig)
  - `tools/setup_openiris.py` — interactive CLI for Wi‑Fi, MDNS/Name, Mode, LED PWM, Logs, and a Settings Summary
- Composite USB (UVC + CDC) when UVC mode is enabled (`GENERAL_INCLUDE_UVC_MODE`) for simultaneous video streaming and command channel
- LED current monitoring (if enabled via `MONITORING_LED_CURRENT`) with filtered mA readings
- Battery voltage monitoring (if enabled via `MONITORING_BATTERY_ENABLE`) with Li-ion SOC percentage calculation
- Configurable debug LED + external IR LED control with optional error mirroring (`LED_DEBUG_ENABLE`, `LED_EXTERNAL_AS_DEBUG`)
- Auto‑discovered per‑board configuration overlays under `boards/`
- Command framework (JSON over serial / CDC / REST) for mode switching, Wi‑Fi config, OTA credentials, LED brightness, info & monitoring
- Single source advertised name (`CONFIG_GENERAL_ADVERTISED_NAME`) used for both UVC device name and mDNS hostname (unless overridden at runtime)

---

## First-time setup on Windows (VS Code + ESP‑IDF extension)

If you’re starting fresh on Windows, this workflow is smooth and reliable:

1. Install tooling

- Git: https://git-scm.com/downloads/win
- Visual Studio Code: https://code.visualstudio.com/

2. Get the source code

- Create a folder where you want the repo (e.g., `D:\OpenIris-ESPIDF\`). In File Explorer, right‑click the folder and choose “Open in Terminal”.
- Clone and open in VS Code:

```cmd
git clone https://github.com/lorow/OpenIris-ESPIDF.git
cd OpenIris-ESPIDF
code .
```

3. Install the ESP‑IDF VS Code extension

- In VS Code, open the Extensions tab and install: https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension

4. Set the default terminal profile to Command Prompt

- Press Ctrl+Shift+P → search “Terminal: Select Default Profile” → choose “Command Prompt”.
- Restart VS Code from its normal shortcut (not from Git Bash). This avoids running ESP‑IDF in the wrong shell.

5. Configure ESP‑IDF in the extension

- On first launch, the extension may prompt to install ESP‑IDF and tools — follow the steps. It can take a while.
- If you see the extension’s home page instead, click “Configure extension”, pick “EXPRESS”, choose “GitHub” as the server and version “v5.4.2”.
- Then open the ESP‑IDF Explorer tab and click “Open ESP‑IDF Terminal”. We’ll use that for builds.

After this, you’re ready for the Quick start below.

---

## Quick start

### 1) Grab UV.

We're using UV to manage our tools, grab and install it from [here](https://docs.astral.sh/uv/getting-started/installation/).

Once installed, you'll be able to just run the commands below and UV will take care of setting up everything.

### 1) Pick your board (loads the default configuration)

Boards are auto‑discovered from the `boards/` directory. First list them, then pick one:

Windows (cmd):

```cmd
uv run .\tools\switchBoardType.py --list
uv run .\tools\switchBoardType.py --board seed_studio_xiao_esp32s3 --diff
```

macOS/Linux (bash):

```bash
uv run ./tools/switchBoardType.py --list
uv run ./tools/switchBoardType.py --board seed_studio_xiao_esp32s3 --diff
```

Notes:

- Use `--list` to see all detected board keys.
- Board key = relative path under `boards/` with `/` replaced by `_` (and duplicate tail segments collapsed, e.g. `project_babble/project_babble` -> `project_babble`).
- `--diff` shows what will change vs the current `sdkconfig`.
- You can also pass partial or path‑like inputs (e.g. `facefocusvr/eye_L`), the tool normalizes them.
- ESP32‑S3 WROOM‑1 (e.g. Freenove) users: pick the board by the marking on your module. **N8R2** (2MB quad PSRAM) → `wrooms3N8R2` or `wrooms3QION8R2`; **N8R8** (8MB octal PSRAM) → `wrooms3N8R8`. Flashing the wrong variant aborts at boot with `quad_psram: PSRAM ID read error` / `Failed to init external RAM!`.

### 2) Build & flash

- Set the target (e.g., ESP32‑S3).
- Build, flash, and open the serial monitor.
- (Optional) For UVC mode ensure `GENERAL_INCLUDE_UVC_MODE=y`. If you want device to boot directly into UVC: also set `START_IN_UVC_MODE=y`.
- Disable Wi‑Fi services for pure wired builds: `GENERAL_ENABLE_WIRELESS=n`.

### 3) Use the Python setup CLI (recommended)

Configure the device over USB serial.

Before you run it:

- If you still have the serial monitor open, close it (the port must be free).

Then run:

Windows (cmd):

```cmd
uv run .\tools\setup_openiris.py --port COMxx
```

macOS/Linux (bash):

```bash
uv run ./tools/setup_openiris.py --port /dev/tty<port>
```

Examples:

- Windows: `uv run .\tools\setup_openiris.py --port COM69`
- macOS: `uv run ./tools/setup_openiris.py --port /dev/ttyACM0`
- Linux: `uv run ./tools/setup_openiris.py --port /dev/ttyACM0`

What the CLI can do:

- Wi‑Fi menu: automatic (scan → pick → password → connect → wait for IP) or manual (scan, show, configure, connect, status)
- Set MDNS/Device name (also used for the UVC device name)
- Switch mode (Wi‑Fi / UVC / Setup)
- Adjust LED PWM
- Show a Settings Summary (MAC, Wi‑Fi status, mode, PWM, …)
- View logs

---

## Serial number & MAC

- Internally, the serial number is derived from the Wi‑Fi MAC address.
- The CLI displays the MAC by default (clearer); it’s the value used as the serial number.
- The UVC device name is based on the MDNS hostname.

## Advertised Name (UVC + mDNS)

`CONFIG_GENERAL_ADVERTISED_NAME` (Kconfig) defines the base name announced over:

- USB UVC descriptor (appears in OS camera list)
- mDNS hostname / service name

Runtime override: If the setup CLI (or a JSON command) provides a new device name, that value supersedes the compile-time default until next flash/reset of settings.

---

## Common workflows

- Fast Wi‑Fi setup: in the CLI, go to “Wi‑Fi settings” → “Automatic setup”, then check “status”.
- Change name/MDNS: set the device name in the CLI, then replug USB — UVC will show the new name.
- Adjust brightness/LED: set LED PWM in the CLI.
- Switch to UVC mode over commands (CDC/serial):
  `{"commands":[{"command":"switch_mode","data":{"mode":"uvc"}}]}` then reboot.
- Read filtered LED current (if enabled):
  `{"commands":[{"command":"get_led_current"}]}`
- Read battery status (if enabled):
  `{"commands":[{"command":"get_battery_status"}]}`

---

## Project layout (short)

- `main/` — entry point
- `components/` — modules (Camera, WiFi, UVC, CommandManager, …)
- `tools/` — Python helper tools (board switch, setup CLI, scanner)
- `tests/` - Hardware in the loop tests, with support for different boards and automatic skips if a board can't perform a given test
  If you want to dig deeper: commands are mapped via the `CommandManager` under `components/CommandManager/...`.

---

## Running Hardware In The Loop Tests

In order to run the tests, you'll need to setup a couple of things:

- copy the `.env.example` file in `/tests/` and rename it to `.env`. Then, open it and fill out the network details - SSID and Password.
- plug in your board to your pc and wait for it to boot.
- open the terminal (`ctrl/cmd + j` in VSC) and head over to `/tests/` directory.

Once there, you can invoke the tests with `UV` like so:

```cmd
uv run pytest --board=<your board name> --connection=COM<the number your board connected to>
```

This will auto select every test we have, connect to your board automatically and have pytest skip tests that don't fit your board.
For example, tests involving switching modes to UVC and testing commands over there are disabled for esp32 based boards as only esp32s3 can do it. Same goes for WiFi for FaceFocus Boards.

If you see any fails, you can try rerunning them one by one with:

```cmd
uv run pytest --board=<your board name> --connection=COM<the number your board connected to> -k name_of_the_test
```

Or rerun every single failed test like so:

```cmd
uv run pytest --board=<your board name> --connection=COM<the number your board connected to> --lf
```

Sometimes tests will fail due to timeouts, this is normal.

You should see the tests starting to go off, with any luck - all of them passing, your board should also start reacting to the changes - reboots, blinking lights etc are **expected** as we're performing hardware in the loop tests.

### Warning:

    After the testing session ends **WE WILL RESET THE BOARD**, any changes you've made yourself to it will be lost. This is done to ensure that the test we perform are unaffected by any changes done by said tests.
    If we skipped that, tests involving adding fake networks would break some that rely on the board connecting to real ones in a timely manner, for example.
    
    There is currently no way to skip that behavior.

## Troubleshooting

### USB Composite (UVC + CDC)

When UVC support is compiled in, the device enumerates as a composite USB device:

- UVC interface: video streaming (JPEG frames)
- CDC (virtual COM): command channel accepting newline‑terminated JSON objects

Example newline‑terminated JSON commands over CDC (one per line):

```
{"commands":[{"command":"ping"}]}
{"commands":[{"command":"get_who_am_i"}]}
{"commands":[{"command":"switch_mode","data":{"mode":"wifi"}}]}
```

Chained commands in a single request (processed in order):

```
{"commands":[
  {"command":"set_mdns","data":{"hostname":"tracker"}},
  {"command":"set_wifi","data":{"name":"main","ssid":"your_network","password":"password","channel":0,"power":0}}
]}
```

Responses are JSON blobs flushed immediately.

---

### Monitoring (LED Current & Battery)

**LED Current Monitoring**

Enabled with `MONITORING_LED_CURRENT=y` plus shunt/gain settings. The task samples every `CONFIG_MONITORING_LED_INTERVAL_MS` ms and maintains a filtered moving average over `CONFIG_MONITORING_LED_SAMPLES` samples. Use `get_led_current` command to query.

**Battery Monitoring**

Enabled with `MONITORING_BATTERY_ENABLE=y`. Supports voltage divider configuration for measuring Li-ion/Li-Po battery voltage:

| Kconfig | Description |
|---------|-------------|
| MONITORING_BATTERY_ADC_GPIO | GPIO pin connected to voltage divider output |
| MONITORING_BATTERY_DIVIDER_R_TOP_OHM | Top resistor value (battery side) |
| MONITORING_BATTERY_DIVIDER_R_BOTTOM_OHM | Bottom resistor value (GND side) |
| MONITORING_BATTERY_INTERVAL_MS | Sampling interval in milliseconds |
| MONITORING_BATTERY_SAMPLES | Moving average window size |

The firmware includes a Li-ion discharge curve lookup table for SOC (State of Charge) percentage calculation with linear interpolation. Use `get_battery_status` command to query voltage (mV) and percentage (%).

### Debug & External LED Configuration

| Kconfig                     | Effect                                                                                              |
| --------------------------- | --------------------------------------------------------------------------------------------------- |
| LED_DEBUG_ENABLE            | Enables/disables discrete status LED GPIO init & drive                                              |
| LED_EXTERNAL_CONTROL        | Enables PWM control for IR / external LED                                                           |
| LED_EXTERNAL_PWM_DUTY_CYCLE | Default duty % applied at boot (0–100)                                                              |
| LED_EXTERNAL_AS_DEBUG       | Mirrors only error patterns onto external LED (0%/50%) when debug LED absent or also for redundancy |

### Board Profiles

Each file under `boards/` overlays `sdkconfig.base_defaults`. The merge order: base → board file → (optional) dynamic Wi‑Fi overrides via `switchBoardType.py` flags. Duplicate trailing segment directories collapse to unique keys.

- UVC doesn’t appear on the host?
  - Switch mode to UVC via CLI tool, replug USB and wait 20s.

### Adding a new board configuration

1. Create a new config file under `boards/` (you can nest folders): for example `boards/my_family/my_variant`.
2. Populate it with only the `CONFIG_...` lines that differ from the shared defaults. Shared baseline lives in `boards/sdkconfig.base_defaults` and is always merged first.
3. The board key the script accepts will be the relative path with `/` turned into `_` (example: `boards/my_family/my_variant` -> `my_family_my_variant`).
4. Run `uv run tools/switchBoardType.py --list` to verify it’s detected, then switch using `-b my_family_my_variant`.
5. If you accidentally create two files that collapse to the same key the last one found wins—rename to keep keys unique.

Tips:

- Use `--diff` after adding a board to sanity‑check only the intended keys change.
- For Wi‑Fi overrides on first flash: add none—pass `--ssid` / `--password` when switching if needed.

---

## Troubleshooting

### LED Status / Error Patterns

The firmware uses a small set of LED patterns to indicate status and blocking errors. When `LED_DEBUG_ENABLE` is disabled and `LED_EXTERNAL_AS_DEBUG` is enabled the external IR LED mirrors ONLY error patterns (0%/50% duty). Non‑error patterns are not mirrored.

| State                    | Visual                                                    | Category     | Timing Pattern (ms)    | Meaning                                                   |
| ------------------------ | --------------------------------------------------------- | ------------ | ---------------------- | --------------------------------------------------------- |
| LedStateNone             | ![idle](docs/led_patterns/idle.svg)                       | idle         | (off)                  | No activity / heartbeat window waiting                    |
| LedStateStreaming        | ![stream](docs/led_patterns/streaming.svg)                | active       | steady on              | Streaming running (UVC or Wi‑Fi)                          |
| LedStateStoppedStreaming | ![stopped](docs/led_patterns/stopped.svg)                 | inactive     | steady off             | Streaming intentionally stopped                           |
| CameraError              | ![camera error](docs/led_patterns/camera_error.svg)       | error        | 300/300 300/700 (loop) | Camera init/runtime failure (check sensor, ribbon, power) |
| WiFiStateConnecting      | ![wifi connecting](docs/led_patterns/wifi_connecting.svg) | transitional | 400/400 (loop)         | Wi‑Fi associating / DHCP pending                          |
| WiFiStateConnected       | ![wifi connected](docs/led_patterns/wifi_connected.svg)   | notification | 150/150×3 then 600 off | Wi‑Fi connected successfully                              |
| WiFiStateError           | ![wifi error](docs/led_patterns/wifi_error.svg)           | error        | 200/100 500/300 (loop) | Wi‑Fi failed (auth timeout or no AP)                      |

---

Feedback, issues, and PRs are welcome.
