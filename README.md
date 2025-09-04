| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

## OpenIris-ESPIDF

Firmware and tools for OpenIris — Wi‑Fi, UVC streaming, and a Python setup CLI.

---

## What’s inside
- ESP‑IDF firmware (C/C++) with modules for Camera, Wi‑Fi, UVC, REST/Serial commands, and more
- Python tools for setup over USB serial:
  - `tools/switchBoardType.py` — choose a board profile (builds the right sdkconfig)
  - `tools/openiris_setup.py` — interactive CLI for Wi‑Fi, MDNS/Name, Mode, LED PWM, Logs, and a Settings Summary

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
- If you see the extension’s home page instead, click “Configure extension”, pick “EXPRESS”, choose “GitHub” as the server and version “v5.4.2”.
- Then open the ESP‑IDF Explorer tab and click “Open ESP‑IDF Terminal”. We’ll use that for builds.

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
- Set the target (e.g., ESP32‑S3).
- Build, flash, and open the serial monitor.

### 3) Use the Python setup CLI (recommended)
Configure the device over USB serial.

Before you run it:
- If you still have the serial monitor open, close it (the port must be free).
- In VS Code, open the sidebar “ESP‑IDF: Explorer” and click “Open ESP‑IDF Terminal”. We’ll run the CLI there so Python and packages are in the right environment.

Then run:
```cmd
python .\tools\openiris_setup.py --port COMxx
```
Examples:
- Windows: `python .\tools\openiris_setup.py --port COM69`, …
- macOS: idk
- Linux: idk

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

---

## Common workflows
- Fast Wi‑Fi setup: in the CLI, go to “Wi‑Fi settings” → “Automatic setup”, then check “status”.
- Change name/MDNS: set the device name in the CLI, then replug USB — UVC will show the new name.
- Adjust brightness/LED: set LED PWM in the CLI.

---

## Project layout (short)
- `main/` — entry point
- `components/` — modules (Camera, WiFi, UVC, CommandManager, …)
- `tools/` — Python helper tools (board switch, setup CLI, scanner)

If you want to dig deeper: commands are mapped via the `CommandManager` under `components/CommandManager/...`.

---

## Troubleshooting
- UVC doesn’t appear on the host?
  - Switch mode to UVC via CLI tool, replug USB and wait 20s.

---

Feedback, issues, and PRs are welcome.