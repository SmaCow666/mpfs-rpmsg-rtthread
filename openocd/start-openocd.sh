#!/bin/bash
# ============================================================
# start-openocd.sh -- Launch OpenOCD for PolarFire SoC Icicle Kit
# (Linux / WSL environment)
#
# Usage:
#   ./start-openocd.sh           Debug U54_1 (default)
#   ./start-openocd.sh 2         Debug U54_2
#   ./start-openocd.sh 0         Debug E51
#
# Prerequisites:
#   SoftConsole installed, SC_INSTALL_DIR must match your setup.
# ============================================================

set -euo pipefail

SC_INSTALL_DIR="/opt/microchip/SoftConsole"
# For WSL users, change to:
# SC_INSTALL_DIR="/mnt/d/Microchip/SoftConsole-v2022.2-RISC-V-747"

HART="${1:-1}"

OPENOCD_BIN="${SC_INSTALL_DIR}/openocd/bin/openocd"
OPENOCD_SCRIPTS="${SC_INSTALL_DIR}/openocd/share/openocd/scripts"
PROJ_OPENOCD="$(cd "$(dirname "$0")" && pwd)"

if [ ! -x "$OPENOCD_BIN" ]; then
    echo "[ERROR] OpenOCD not found at: $OPENOCD_BIN"
    echo "[ERROR] Please check SC_INSTALL_DIR in this script."
    exit 1
fi

if [ ! -f "${PROJ_OPENOCD}/board/icicle-kit.cfg" ]; then
    echo "[ERROR] Board config not found at: ${PROJ_OPENOCD}/board/icicle-kit.cfg"
    exit 1
fi

echo "============================================================"
echo "PolarFire SoC Icicle Kit -- OpenOCD Launcher"
echo "  OpenOCD:  ${OPENOCD_BIN}"
echo "  HART:     ${HART} (0=E51, 1=U54_1, 2=U54_2)"
echo "  Board:    icicle-kit.cfg"
echo "============================================================"

LD_LIBRARY_PATH="${SC_INSTALL_DIR}/openocd/lib:${SC_INSTALL_DIR}/fpServer/lib:${LD_LIBRARY_PATH:-}" \
"${OPENOCD_BIN}" \
    -s "${OPENOCD_SCRIPTS}" \
    -s "${PROJ_OPENOCD}" \
    -c "set HART ${HART}" \
    -f board/icicle-kit.cfg
