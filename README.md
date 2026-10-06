# 🍷 Winerunner

A fast, lightweight, and robust Wine game launcher and profile manager written in C.

`winerunner` replaces messy, hardcoded `.zshrc`/`.bashrc` aliases (like `alias nfs-mw='cd ~/Games/... && nohup wine explorer ...'`) with an organized, auto-configuring game runner.

---

## ⚡ Features & Improvements over Shell Aliases

| Feature | Old Shell Alias (`.zshrc`) | `winerunner` |
| :--- | :--- | :--- |
| **Configuration** | Manually writing shell aliases for each game | `winerunner add` with interactive wizard or CLI flags |
| **Screen Resolution** | Hardcoded (e.g., `1920x1080`) | Auto-detects display via `xrandr`, configurable per game |
| **Log Diagnostics** | Lost completely (`>/dev/null 2>&1`) | Logged to `~/.local/state/winerunner/logs/<game>.log` |
| **Process Control** | Uncontrolled background job | `winerunner list` displays running status & PID, `kill` cleanly stops |
| **Debug Mode** | Need to manually edit command in terminal | `winerunner <game> --foreground` or `--debug` anytime |
| **Prefixes & Envs** | Clutters alias line | Isolated `wineprefix` and environment variables per profile |
| **Shell Autocompletion** | None | Full tab-completion for games and commands in **Bash** and **Zsh** |

---

## 📦 Building and Installation

### Prerequisites
- `cmake` (>= 3.16)
- `gcc` or `clang` (C11 support)
- `wine` installed on your system

### 1. Build & Install to User Bin (`~/.local/bin`) — No root/sudo required
```bash
cd ~/codex/persona/winerunner
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Installs winerunner (and 'wineplay' symlink) into ~/.local/bin:
cmake --build build --target install-user
```

### 2. System-wide Install (Optional)
```bash
sudo cmake --install build
```

---

## 🎮 Usage Guide

### 1. Register a Game

#### Interactive Setup Wizard:
Simply run:
```bash
winerunner add
```
It will guide you through:
1. Game alias (e.g. `nfsmw`)
2. Path to executable (`~/Games/Nfsmw2005/speed.exe`)
3. Virtual desktop resolution (auto-detected from your screen)
4. Wine virtual desktop (`explorer /desktop=...`) toggle
5. Custom WINEPREFIX (optional)
6. Environment variables (optional, e.g. `DXVK_HUD=fps`)

#### Quick One-Liner:
```bash
winerunner add nfsmw ~/Games/Nfsmw2005/speed.exe
```

With custom options:
```bash
winerunner add nfsmw ~/Games/Nfsmw2005/speed.exe --res 1920x1080 --desktop-name speed
winerunner add cod4 ~/Games/Codmw4/iw3sp.exe --no-desktop
```

---

### 2. Launch a Game

Run detached in background (default):
```bash
winerunner nfsmw
```

Run in foreground to stream Wine console logs:
```bash
winerunner nfsmw --foreground
```

Run with temporary resolution override:
```bash
winerunner nfsmw --res 1280x720
```

---

### 3. List All Saved Games & Running Status
```bash
winerunner list
# or
winerunner ls
```
Output:
```text
REGISTERED GAMES (2)
ALIAS          STATUS           RESOLUTION   DESKTOP      EXECUTABLE PATH
──────────────────────────────────────────────────────────────────────────────────
nfsmw          ● Running [4123] 1920x1080    Yes (speed)   /home/lynn/Games/Nfsmw2005/speed.exe
cod4           ○ Stopped        1920x1080    No           /home/lynn/Games/Codmw4/iw3sp.exe
──────────────────────────────────────────────────────────────────────────────────
```

---

### 4. View Profile Details
```bash
winerunner info nfsmw
```

---

### 5. Inspect Logs
```bash
winerunner logs nfsmw
winerunner logs nfsmw -n 100
```

---

### 6. Terminate Game Processes
```bash
winerunner kill nfsmw
# or
winerunner stop nfsmw
```

---

### 7. Remove a Saved Game
```bash
winerunner remove nfsmw
# or
winerunner rm nfsmw
```

---

## 🛠️ Configuration File

Profiles are saved in standard INI format in `$XDG_CONFIG_HOME/winerunner/games.conf` (defaults to `~/.config/winerunner/games.conf`).

Example `games.conf`:
```ini
[nfsmw]
exe = /home/lynn/Games/Nfsmw2005/speed.exe
workdir = /home/lynn/Games/Nfsmw2005
resolution = 1920x1080
desktop_name = speed
virtual_desktop = true
wineprefix = 
env = 
extra_args = 
```
You can edit this file directly or view its location with:
```bash
winerunner config
```

---
