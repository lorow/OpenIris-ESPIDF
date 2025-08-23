| Supported Targets (IDK lol) | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

## OpenIris-ESPIDF

Firmware and tools for OpenIris — Wi‑Fi, UVC streaming, and a handy Python setup CLI.

---

## What’s inside
- ESP‑IDF firmware (C/C++) with modules for Camera, Wi‑Fi, UVC, REST/Serial commands, and more
- Python tools to make setup easy over USB serial:
  - `tools/switchBoardType.py` — choose a board profile (loads the right sdkconfig)
  - `tools/openiris_setup.py` — interactive CLI for Wi‑Fi, MDNS/Name, Mode, LED PWM, Logs, and a Settings Summary
  - `tools/wifi_scanner.py` — optional Wi‑Fi scanner

---

## Prerequisites
- ESP‑IDF installed and available in your terminal (`idf.py` works)
- Python 3.10+ with `pip` (on Windows this is installed automatically when you set up the ESP‑IDF VS Code extension; see “First-time setup on Windows” below)
- USB cable to your board
- Optional: install Python dependencies for the tools

Windows (cmd):
```cmd
python -m pip install -r tools\requirements.txt
```
macOS/Linux (bash):
```bash
python3 -m pip install -r tools/requirements.txt
```
Note for Windows: If emojis don’t render nicely, it’s only cosmetic — everything still works.

---

## First-time setup on Windows (VS Code + ESP‑IDF extension)
If you’re starting fresh on Windows, this workflow is smooth and reliable:

1) Install tooling
- Git: https://git-scm.com/downloads/win
- Visual Studio Code: https://code.visualstudio.com/

2) Get the source code
- Create a folder where you want the repo (e.g., `D:\OpenIris-ESPIDF\`). In File Explorer, right‑click the folder and choose “Open in Terminal”.
- Clone and open in VS Code:
```cmd
git clone https://github.com/lorow/OpenIris-ESPIDF.git
cd OpenIris-ESPIDF
code .
```

3) Install the ESP‑IDF VS Code extension
- In VS Code, open the Extensions tab and install: https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension

4) Set the default terminal profile to Command Prompt
- Press Ctrl+Shift+P → search “Terminal: Select Default Profile” → choose “Command Prompt”.
- Restart VS Code from its normal shortcut (not from Git Bash). This avoids running ESP‑IDF in the wrong shell.

5) Configure ESP‑IDF in the extension
- On first launch, the extension may prompt to install ESP‑IDF and tools — follow the steps. It can take a while.
- If you see the extension’s home page instead, click “Configure extension”, pick “EXPRESS”, choose “GitHub” as the server and version “v5.3.3”.
- Then open the ESP‑IDF Explorer tab and click “Open ESP‑IDF Terminal”. We’ll use that for builds.

6) Default board profile and switching
- The project ships with multiple board profiles; one default may target internal “Project Babble” boards.
- If you have a Seeed XIAO ESP32S3 (or another board), switch the profile with the helper script:
```cmd
python .\tools\switchBoardType.py --board xiao-esp32s3 --diff
```
- You can re‑run this anytime to switch boards; `--diff` shows the changes.

After this, you’re ready for the Quick start below.

---

## Quick start

### 1) Pick your board (loads the default configuration)
Windows (cmd):
```cmd
python .\tools\switchBoardType.py --board xiao-esp32s3 --diff
```
macOS/Linux (bash):
```bash
python3 ./tools/switchBoardType.py --board xiao-esp32s3 --diff
```
- Set `--board` to your target board
- `--diff` shows what changed in the config

### 2) Build & flash
- Set the target (e.g., ESP32‑S3):
```cmd
idf.py set-target esp32s3
```
- Build, flash, and open the serial monitor:
```cmd
idf.py -p COM15 flash monitor
```
Replace `COM15` with your port (Windows: `COM…`, Linux: `/dev/ttyUSB…`, macOS: `/dev/cu.usbmodem…`).
Exit the monitor with `Ctrl+]`.

### 3) Use the Python setup CLI (recommended)
Configure the device conveniently over USB serial.

Windows (cmd):
```cmd
python .\tools\openiris_setup.py --port COM15
```
macOS/Linux (bash):
```bash
python3 ./tools/openiris_setup.py --port /dev/ttyUSB0
```
What the CLI can do:
- Wi‑Fi menu: automatic (scan → pick → password → connect → wait for IP) or manual (scan, show, configure, connect, status)
- Set MDNS/Device name (also used for the UVC device name)
- Switch mode (Wi‑Fi / UVC / Auto)
- Adjust LED PWM
- Show a Settings Summary (MAC, Wi‑Fi status, mode, PWM, …)
- View logs

Tip: On start, the CLI keeps the device in a setup state so replies are stable.

---

## Serial number & MAC
- Internally, the serial number is derived from the Wi‑Fi MAC address.
- The CLI displays the MAC by default (clearer); it’s the value used as the serial number.
- The UVC device name is based on the MDNS hostname.

---

## Common workflows
- Fast Wi‑Fi setup: in the CLI, go to “Wi‑Fi settings” → “Automatic setup”, then check “status”.
- Change name/MDNS: set the device name in the CLI, then replug USB — UVC will show the new name.
- Adjust brightness/LED: set LED PWM in the CLI.
- Quick health check: run “Get settings summary” after flashing to verify the device state.

---

## Project layout (short)
- `main/` — entry point
- `components/` — modules (Camera, WiFi, UVC, CommandManager, …)
- `tools/` — Python helper tools (board switch, setup CLI, scanner)

If you want to dig deeper: commands are mapped via the `CommandManager` under `components/CommandManager/...`.

---

## Troubleshooting
- No device/port found?
  - Pass `--port` explicitly (e.g., `--port COM15`).
  - Right after reset, the device emits heartbeats for a short time; the CLI finds it fastest during that window.
- UVC doesn’t appear on the host?
  - Switch mode to UVC and replug USB. On Windows the first driver init can take a moment.
- Wi‑Fi won’t connect?
  - Check password/channel. Use the automatic setup in the CLI and then inspect the status.
- ESP‑IDF not detected?
  - Ensure the ESP‑IDF environment is active in the terminal (IDF Command Prompt or the extension’s ESP‑IDF Terminal; alternatively source `export.ps1`/`export.sh`).
- Missing Python modules?
  - Run `pip install -r tools/requirements.txt`.

---

Have fun with OpenIris! Feedback, issues, and PRs are welcome.
