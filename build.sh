#!/bin/bash
# Build RT-Thread RPMsg Master for PolarFire SoC Icicle Kit
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ============================================================
# CONFIG: SoftConsole installation path
# ============================================================
# Replace __SOFTCONSOLE_PATH__ with your actual SoftConsole path.
#
# Examples:
#   Windows (MSYS2): D:/Microchip/SoftConsole-v2022.2-RISC-V-747
#   Linux:           /opt/microchip/SoftConsole
# ============================================================
SC_INSTALL_DIR="D:/Microchip/SoftConsole-v2022.2-RISC-V-747"

# MSYS2 path conversion: D:/Microchip/... -> /d/Microchip/...
# Required for make child shell compatibility on MSYS2.
# On Linux, the path is used as-is.
SC_INSTALL_DIR_MSYS=$(echo "$SC_INSTALL_DIR" | sed 's|^\([A-Za-z]\):/|/\1/|')
TOOLCHAIN_BIN="${SC_INSTALL_DIR_MSYS}/riscv-unknown-elf-gcc/bin"

export PATH="${TOOLCHAIN_BIN}:${SC_INSTALL_DIR}/python3/bin:${PATH}"

# Verify toolchain is reachable
if ! command -v riscv64-unknown-elf-gcc &>/dev/null; then
    echo "[ERROR] riscv64-unknown-elf-gcc not found in PATH"
    echo "  Checked: ${TOOLCHAIN_BIN}"
    echo "  Hint: verify SC_INSTALL_DIR in build.sh"
    exit 1
fi

echo "Toolchain: $(command -v riscv64-unknown-elf-gcc)"
echo ""

# ============================================================
# Build
# ============================================================
echo -e "\033[1;32m=== Cleaning previous build ===\033[0m"
make clean MASTER=1
rm -f payload.bin
rm -f Master-Default/*.elf Master-Default/*.bin

echo -e "\033[1;32m=== Building RT-Thread RPMsg Master ===\033[0m"
make MASTER=1

echo -e "\033[1;32m=== Generating HSS payload ===\033[0m"
# HSS payload generator path (relative to SC_INSTALL_DIR)
"${SC_INSTALL_DIR}/hss-payload-generator-v2026.04.1/hss-payload-generator/binaries/hss-payload-generator.exe" \
-c hss-payload-rtthread.yaml payload.bin

echo -e "\n\033[1;36m=== Build SUCCESSFUL! ===\033[0m"
echo "ELF:  Master-Default/mpfs-rpmsg-master.elf"
echo "BIN:  Master-Default/mpfs-rpmsg-master.bin"
echo "HSS payload: payload.bin"
echo ""

