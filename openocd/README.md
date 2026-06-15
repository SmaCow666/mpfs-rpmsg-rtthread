# OpenOCD Debug Config -- PolarFire SoC Icicle Kit

## Directory Structure

```
openocd/
+---- README.md                       # This file
+---- start-openocd.bat               # [Launch] Windows cmd startup script
+---- start-openocd.ps1               # [Launch] PowerShell startup script
+---- start-openocd.sh                # [Launch] Linux/WSL startup script
+---- gdb-connect.bat                 # [Debug] GDB one-click connect + load ELF
|
+---- interface/
|   \---- flashpro5.cfg               # FlashPro5/6 adapter config
+---- target/
|   \---- mpfs250t.cfg                # MPFS250T target helpers (mem map + utilities)
+---- board/
|   \---- icicle-kit.cfg              # **Main entry** -- passed to OpenOCD at startup
\---- scripts/
    +---- debug-hart1.cfg             # Debug U54_1 (Master application)
    +---- debug-hart2.cfg             # Debug U54_2 (Remote application)
    \---- flash-hart1.cfg             # Flash U54_1 ELF to DDR
```
## Prerequisites

Must use **SoftConsole-bundled OpenOCD** because:
- FlashPro5/6 uses Microchip proprietary protocol
- Only SoftConsole OpenOCD integrates the fpServer library
- Standard open-source OpenOCD does **not** support FlashPro5/6

Verify OpenOCD version:
```powershell
D:\Microchip\SoftConsole-v2022.2-RISC-V-747\openocd\bin\openocd.exe -v
```
Expected: `xPack OpenOCD (Microchip SoftConsole build) ... 0.10.0+dev`

---

## Usage Flow

### Step 1: Launch OpenOCD

> Run from the **openocd/** directory. OpenOCD runs in the foreground.

#### Option A: PowerShell (Windows, recommended)

```powershell
cd D:\XuHuiNiu\CodeData\MPFS250T\polarfire-rtthread\openocd
.\start-openocd.ps1               # Debug U54_1 (default)
.\start-openocd.ps1 -Hart 2       # Debug U54_2
.\start-openocd.ps1 -Hart 0       # Debug E51
```

#### Option B: cmd (Command Prompt)

```cmd
cd D:\XuHuiNiu\CodeData\MPFS250T\polarfire-rtthread\openocd
start-openocd.bat                 # Debug U54_1 (default)
start-openocd.bat 2               # Debug U54_2
```

#### Option C: Linux / WSL

```sh
cd /path/to/polarfire-rtthread/openocd
./start-openocd.sh                # Debug U54_1 (default)
./start-openocd.sh 2              # Debug U54_2
```

> **Note**: In `start-openocd.sh`, `SC_INSTALL_DIR` defaults to `/opt/microchip/SoftConsole`.
> Adjust to match your installation path.

#### Successful startup output:

```
PolarFire SoC Icicle Kit -- OpenOCD Launcher
  OpenOCD:  D:\Microchip\...\openocd.exe
  HART:     1 (U54_1)
  Board:    icicle-kit.cfg
============================================================
xPack OpenOCD ... 0.10.0+dev
...
Info : Listening on port 3333 for gdb connections
Info : Listening on port 6666 for tcl connections
```

OpenOCD is now connected, waiting for GDB.

---

### Step 2: Connect GDB for Debugging

> Open **another terminal** and run from the **openocd/** directory.

#### Option A: One-click debug (recommended)

```cmd
cd D:\XuHuiNiu\CodeData\MPFS250T\polarfire-rtthread\openocd
gdb-connect.bat                   # Connect U54_1 Master
gdb-connect.bat remote            # Connect U54_2 Remote
```

This script automates:
1. Launch riscv64-unknown-elf-gdb
2. Connect to OpenOCD (localhost:3333)
3. Reset and halt the target
4. Load ELF to DDR (0x91800000)

#### Option B: Manual GDB

```cmd
D:\Microchip\SoftConsole-v2022.2-RISC-V-747\riscv-unknown-elf-gcc\bin\riscv64-unknown-elf-gdb.exe ^
  Master-Default\mpfs-rpmsg-master.elf
```

In GDB session:
```
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

---

### Step 3: Debug Commands

| Command | Description |
|---|---|
| continue or c | Resume execution |
| break function_name | Set software breakpoint |
| hbreak address | Set hardware breakpoint |
| stepi / nexti | Single-step instructions |
| info registers | View RISC-V registers |
| x/32wx 0x91800000 | View DDR memory |
| backtrace | View call stack |

---

## Switching Target Hart

| Hart | Core | Purpose |
|---|---|---|
| 0 | E51 | Monitor core (HSS) |
| **1** | **U54_1** | **RT-Thread Master (default)** |
| 2 | U54_2 | RT-Thread Remote |
| 3 | U54_3 | Application core |
| 4 | U54_4 | Application core |

Example:
```powershell
.\start-openocd.ps1 -Hart 2       # Debug U54_2
```

And connect Remote ELF:
```cmd
gdb-connect.bat remote
```

---

## Programming ELF to DDR

### Method A: Via GDB load (recommended)

Once OpenOCD connects, in GDB:
```
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

### Method B: Via flash-hart1.cfg script

Launch OpenOCD with the flash script:
```powershell
& "D:\Microchip\SoftConsole-v2022.2-RISC-V-747\openocd\bin\openocd.exe" `
    -s "D:\Microchip\SoftConsole-v2022.2-RISC-V-747\openocd\share\openocd\scripts" `
    -s ".\openocd" `
    -c "set HART 1" `
    -f board/icicle-kit.cfg `
    -f scripts/flash-hart1.cfg
```

### Method C: Via SD card HSS payload (no JTAG)

```sh
./build.sh                            # Build + generate payload.bin
dd if=payload.bin of=/dev/sda bs=1M   # Write to SD card
```

---

## Troubleshooting

| Problem | Cause | Solution |
|---|---|---|
| `invalid command name "?"` | cfg file has UTF-8 BOM | Save cfg files as ASCII (fixed) |
| `couldn't find board/microsemi-riscv.cfg` | OpenOCD search path missing SoftConsole scripts | Verify -s points to SoftConsole scripts dir |
| `fpServer: no device found` | FlashPro5/6 not connected or driver issue | Check USB connection; verify driver in Device Manager |
| `target failed to examine` | JTAG clock too fast or reset timing | Add `adapter speed 500` to cfg |
| `timeout waiting for hart` | Target in WFI state | Use `reset halt` first |
| `no elf file found` | Project not built yet | Run build.sh first |

---

## How Each Script Works

### start-openocd.ps1

```powershell
# 1. Set SoftConsole path
$SC_INSTALL_DIR = "D:\Microchip\SoftConsole-v2022.2-RISC-V-747"
# 2. Add fpServer to PATH (loads fpcomm.dll for FlashPro communication)
$env:PATH = "$SC_INSTALL_DIR\fpServer;$env:PATH"
# 3. Launch OpenOCD with HART param and project board cfg
& openocd.exe -s <SoftConsole scripts> -s <project openocd> -c "set HART 1" -f board/icicle-kit.cfg
```

### gdb-connect.bat

```cmd
riscv64-unknown-elf-gdb.exe
(gdb) target remote localhost:3333     # Connect to OpenOCD
(gdb) monitor reset halt               # Reset + halt
(gdb) load                              # Load ELF to DDR
```

