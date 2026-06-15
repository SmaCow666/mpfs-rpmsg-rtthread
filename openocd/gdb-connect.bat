@echo off
REM ============================================================
REM gdb-connect.bat -- GDB Debug for PolarFire SoC Icicle Kit
REM
REM Usage:   gdb-connect.bat [master|remote]
REM
============================================================
REM AMP Debug Flow (SoftConsole multi-core debug approach):
REM
REM   1. monitor reset halt        Reset all harts
REM   2. continue                  Release all harts -> HSS boots on E51
REM                                 HSS inits DDR -> releases U54_1
REM                                 U54_1 starts app at 0x91800000
REM   3. Wait 2-3s -> Ctrl+C       App is now running, halt it
REM   4. thread 2                  Switch to U54_1 (hart 1)
REM   5. load                      Load new ELF to DDR (overwrites old)
REM   6. set $pc = 0x91800000      Set PC to entry point
REM   7. b main / continue         Set breakpoint, run
REM ============================================================

setlocal enabledelayedexpansion

set SC_INSTALL_DIR=D:\Microchip\SoftConsole-v2022.2-RISC-V-747
set MODE=%1
if "%MODE%"=="" set MODE=master

set GDB=%SC_INSTALL_DIR%\riscv-unknown-elf-gcc\bin\riscv64-unknown-elf-gdb.exe
set GDB_PORT=3333
set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..
set ELF=%PROJECT_DIR%\Master-Default\mpfs-rpmsg-master.elf

if /i "%MODE%"=="remote" (
    set ELF=%PROJECT_DIR%\Remote-Default\mpfs-rpmsg-remote.elf
    if not "%2"=="" set GDB_PORT=%2
)

if not exist "!GDB!" (
    echo [ERROR] GDB not found at: !GDB!
    pause
    exit /b 1
)
if not exist "!ELF!" (
    echo [ERROR] ELF not found at: !ELF!
    echo [HINT]  Run build.sh first to build the project.
    pause
    exit /b 1
)

echo ============================================================
echo GDB Debug  --  PolarFire SoC RT-Thread  (AMP Debug Mode)
echo   Board:   Icicle Kit (MPFS250T)
echo   Target:  U54_1 (hart 1, thread 2)
echo   ELF:     !ELF!
echo   GDB:     localhost:!GDB_PORT!
echo ============================================================
echo.
echo [WORKFLOW]
echo  GDB will auto-execute:
echo    1. target remote          Connect to OpenOCD
echo    2. monitor reset halt     Global reset, all harts stop
echo    3. continue               Release all harts, HSS boots on E51
echo.
echo  [WAIT] Wait 2-3 seconds (HSS inits DDR + releases U54_1)
echo  [HALT] Then press Ctrl+C to return to GDB prompt
echo.
echo  After Ctrl+C:
echo    (gdb) thread 2                   Switch to U54_1 (hart 1)
echo    (gdb) load                       Load ELF to DDR
echo    (gdb) set $pc = 0x91800000       Set PC to entry
echo    (gdb) b main                     Set breakpoint
echo    (gdb) continue                   Run
echo.
echo GDB starting...
pause

"!GDB!" -ex "set architecture riscv:rv64" ^
        -ex "set mem inaccessible-by-default off" ^
        -ex "set riscv use_compressed_breakpoints no" ^
        -ex "set remote noack-packet on" ^
        -ex "target remote localhost:!GDB_PORT!" ^
        -ex "monitor reset halt" ^
        -ex "continue" ^
        "!ELF!"


