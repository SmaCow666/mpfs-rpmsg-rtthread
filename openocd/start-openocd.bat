@echo off
REM ============================================================
REM start-openocd.bat -- Launch OpenOCD for PolarFire SoC Icicle Kit
REM
REM Usage:
REM   start-openocd.bat          Debug U54_1 (default)
REM   start-openocd.bat 2        Debug U54_2
REM   start-openocd.bat 0        Debug E51
REM
REM Prerequisites:
REM   SoftConsole-v2022.2-RISC-V-747 installed at D:\Microchip\
REM ============================================================

setlocal enabledelayedexpansion

set SC_INSTALL_DIR=D:\Microchip\SoftConsole-v2022.2-RISC-V-747
set HART=%1
if "%HART%"=="" set HART=1

set OPENOCD_BIN=%SC_INSTALL_DIR%\openocd\bin\openocd.exe
set OPENOCD_SCRIPTS=%SC_INSTALL_DIR%\openocd\share\openocd\scripts
set PROJ_OPENOCD=%~dp0

if not exist "%OPENOCD_BIN%" (
    echo [ERROR] OpenOCD not found at: %OPENOCD_BIN%
    echo [ERROR] Please check SC_INSTALL_DIR in this script.
    pause
    exit /b 1
)

set PATH=%SC_INSTALL_DIR%\fpServer;%PATH%

echo ============================================================
echo PolarFire SoC Icicle Kit -- OpenOCD Launcher
echo   OpenOCD:  %OPENOCD_BIN%
echo   HART:     %HART% (0=E51, 1=U54_1, 2=U54_2)
echo   Board:    icicle-kit.cfg
echo ============================================================

"%OPENOCD_BIN%" ^
    -s "%OPENOCD_SCRIPTS%" ^
    -s "%PROJ_OPENOCD%" ^
    -c "set HART %HART%" ^
    -f board/icicle-kit.cfg

echo.
echo OpenOCD terminated (exit code: %ERRORLEVEL%)
pause


