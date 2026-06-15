<#
.SYNOPSIS
    启动 OpenOCD 连接 PolarFire SoC Icicle Kit

.DESCRIPTION
    启动 OpenOCD，配置 FlashPro5/6 调试适配器，连接指定的 RISC-V 核心。
    默认调试 U54_1 (HART 1)，可通过参数切换。

.PARAMETER Hart
    目标核心编号: 0=E51, 1=U54_1(默认), 2=U54_2, 3=U54_3, 4=U54_4

.EXAMPLE
    .\start-openocd.ps1           # 启动 OpenOCD，U54_1
    .\start-openocd.ps1 -Hart 2   # 启动 OpenOCD，U54_2

.NOTES
    前提: SoftConsole-v2022.2-RISC-V-747 已安装在 D:\Microchip\
#>

# ============================================================
# CONFIG: Set your SoftConsole installation path below
# ============================================================
param(
    [ValidateRange(0,4)]
    [int]$Hart = 1
)

# ---- 配置区 ----
$SC_INSTALL_DIR = "D:\Microchip\SoftConsole-v2022.2-RISC-V-747"
# ----------------

# 脚本位于 openocd/ 目录下，自身就是项目 OpenOCD 配置目录
$ProjOcdDir = $PSScriptRoot
$OpenOcdExe = Join-Path $SC_INSTALL_DIR "openocd\bin\openocd.exe"
$OcdScripts = Join-Path $SC_INSTALL_DIR "openocd\share\openocd\scripts"

# 检查文件
if (-not (Test-Path $OpenOcdExe)) {
    Write-Host "[ERROR] OpenOCD not found: $OpenOcdExe" -ForegroundColor Red
    Write-Host "[ERROR] Check SC_INSTALL_DIR at the top of this script." -ForegroundColor Red
    exit 1
}
if (-not (Test-Path (Join-Path $ProjOcdDir "board\icicle-kit.cfg"))) {
    Write-Host "[ERROR] Board config not found: board/icicle-kit.cfg" -ForegroundColor Red
    exit 1
}

# 将 fpServer 目录加入 PATH（使 OpenOCD 能加载 fpcomm.dll FlashPro 库）
$env:PATH = "$SC_INSTALL_DIR\fpServer;$env:PATH"

$hartNames = @{0="E51";1="U54_1";2="U54_2";3="U54_3";4="U54_4"}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "PolarFire SoC Icicle Kit — OpenOCD Launcher" -ForegroundColor Cyan
Write-Host "  OpenOCD:  $OpenOcdExe" -ForegroundColor Cyan
Write-Host "  HART:     $Hart ($($hartNames[$Hart]))" -ForegroundColor Cyan
Write-Host "  Board:    icicle-kit.cfg" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# 启动 OpenOCD（前台运行，Ctrl+C 终止）
& $OpenOcdExe `
    -s "$OcdScripts" `
    -s "$ProjOcdDir" `
    -c "set HART $Hart" `
    -f "board/icicle-kit.cfg"

$exitCode = $LASTEXITCODE
Write-Host "`nOpenOCD terminated (exit code: $exitCode)" -ForegroundColor Yellow




