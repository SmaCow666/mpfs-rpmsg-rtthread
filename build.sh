#!/bin/bash
# ============================================================
# build.sh — One-click build for PolarFire SoC Icicle Kit
# ============================================================
# Builds FreeRTOS RPMsg firmware for PolarFire SoC Icicle Kit.
# Supports Master and Remote targets with optional HSS payload.
#
# Usage:
#   ./build.sh                    # Build FreeRTOS Master target
#   ./build.sh remote             # Build FreeRTOS Remote target
#   ./build.sh all                # Build both Master + Remote targets
#   ./build.sh clean              # Clean all build artifacts
#
# Requirements:
#   - RISC-V toolchain (riscv64-unknown-elf-gcc)
#   - make, python3
#   - (optional) hss-payload-generator for HSS payload generation
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ============================================================
# CONFIGURATION
# ============================================================

# --- SoftConsole installation path ---
# Set to the actual path on your system.
# MSYS2 path syntax: D:/Microchip/SoftConsole-v2022.2-RISC-V-747
# Linux path:        /opt/microchip/SoftConsole
SC_INSTALL_DIR="${SC_INSTALL_DIR:-}"

# If SC_INSTALL_DIR is not set, attempt auto-detection
if [ -z "$SC_INSTALL_DIR" ]; then
    # Common locations
    for candidate in \
        "/opt/microchip/SoftConsole"* \
        "/d/Microchip/SoftConsole"* \
        "C:/Microchip/SoftConsole"* \
        "D:/Microchip/SoftConsole"*; do
        if [ -d "$candidate" ]; then
            SC_INSTALL_DIR="$candidate"
            break
        fi
    done
fi

# --- HSS payload YAML ---
HSS_YAML="hss-payload-rtthread.yaml"
HSS_PAYLOAD_GEN="${SC_INSTALL_DIR}/hss-payload-generator/binaries/hss-payload-generator"
HSS_PAYLOAD_GEN_EXE="${HSS_PAYLOAD_GEN}.exe"

# --- Parallel jobs ---
JOBS=$(nproc 2>/dev/null || echo 4)

# ============================================================
# Functions
# ============================================================

setup_toolchain() {
    if [ -n "$SC_INSTALL_DIR" ] && [ -d "$SC_INSTALL_DIR" ]; then
        # Handle path for both Windows (MSYS2) and Linux
        local toolchain_dir
        if [ -d "${SC_INSTALL_DIR}/riscv-unknown-elf-gcc/bin" ]; then
            toolchain_dir="${SC_INSTALL_DIR}/riscv-unknown-elf-gcc/bin"
        elif [ -d "${SC_INSTALL_DIR}/toolchain/riscv-unknown-elf-gcc/bin" ]; then
            toolchain_dir="${SC_INSTALL_DIR}/toolchain/riscv-unknown-elf-gcc/bin"
        else
            echo "[WARN] Toolchain bin dir not found under SC_INSTALL_DIR"
            echo "  Searching common locations..."
            toolchain_dir=$(find "$SC_INSTALL_DIR" -name "riscv64-unknown-elf-gcc" -type f 2>/dev/null | head -1 | xargs dirname 2>/dev/null || true)
        fi

        if [ -n "$toolchain_dir" ] && [ -d "$toolchain_dir" ]; then
            export PATH="${toolchain_dir}:${PATH}"
        fi
    fi

    # Verify toolchain
    if ! command -v riscv64-unknown-elf-gcc &>/dev/null; then
        echo "[ERROR] riscv64-unknown-elf-gcc not found!"
        echo "  Set SC_INSTALL_DIR or add toolchain to PATH."
        echo "  Example: export SC_INSTALL_DIR=/opt/microchip/SoftConsole-v2022.2"
        exit 1
    fi

    echo "  Toolchain: $(command -v riscv64-unknown-elf-gcc)"
    echo "  Version:   $(riscv64-unknown-elf-gcc --version | head -1)"
}

build_target() {
    local target="$1"
    local variant="$2"
    local flags=""

    if [ "$target" = "master" ]; then
        flags="MASTER=1"
        local bindir="Master-Default"
        local elfname="mpfs-rpmsg-master.elf"
    else
        flags=""
        local bindir="Remote-Default"
        local elfname="mpfs-rpmsg-remote.elf"
    fi

    echo ""
    echo "=============================================="
    echo " Building: ${target} (${variant})"
    echo "=============================================="

    # Clean specific target
    make clean "$flags" 2>/dev/null || true
    rm -rf "${bindir}"

    # Build
    make "$flags" -j"$JOBS"

    # Verify
    if [ -f "${bindir}/${elfname}" ]; then
        local size
        size=$(riscv64-unknown-elf-size "${bindir}/${elfname}" 2>/dev/null | tail -1 | awk '{print $1 " (text) + " $2 " (data) + " $3 " (bss)"}')
        echo "  ${elfname}: ${size}"
    else
        echo "[ERROR] Build failed: ${bindir}/${elfname} not produced!"
        exit 1
    fi
}

generate_hss_payload() {
    if [ ! -f "$HSS_YAML" ]; then
        echo ""
        echo "  [SKIP] HSS payload: ${HSS_YAML} not found"
        return
    fi

    local gen=""
    if [ -x "$HSS_PAYLOAD_GEN" ]; then
        gen="$HSS_PAYLOAD_GEN"
    elif [ -x "$HSS_PAYLOAD_GEN_EXE" ]; then
        gen="$HSS_PAYLOAD_GEN_EXE"
    fi

    if [ -z "$gen" ]; then
        echo ""
        echo "  [SKIP] HSS payload: hss-payload-generator not found"
        return
    fi

    echo ""
    echo "=== Generating HSS payload ==="
    "$gen" -c "$HSS_YAML" payload.bin
    echo "  Payload: payload.bin ($(wc -c < payload.bin) bytes)"
}

show_help() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  (none)    Build FreeRTOS Master target (default)"
    echo "  master    Build FreeRTOS Master target"
    echo "  remote    Build FreeRTOS Remote target"
    echo "  all       Build both Master + Remote targets"
    echo "  clean     Clean all build artifacts"
    echo "  help      Show this help"
    exit 0
}

# ============================================================
# Main
# ============================================================

# Parse command
CMD="${1:-master}"
case "$CMD" in
    master|"")
        TARGETS="master"
        ;;
    remote)
        TARGETS="remote"
        ;;
    all)
        TARGETS="master remote"
        ;;
    clean)
        echo "=== Cleaning all build artifacts ==="
        make clean MASTER=1 2>/dev/null || true
        make clean 2>/dev/null || true
        rm -rf Master-Default Remote-Default payload.bin
        echo "Done."
        exit 0
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "[ERROR] Unknown command: $CMD"
        show_help
        ;;
esac

echo "=============================================="
echo " PolarFire SoC Firmware Builder"
echo "=============================================="
echo ""

# Setup environment
echo "--- Toolchain setup ---"
setup_toolchain

# Build targets
for t in $TARGETS; do
    build_target "$t" "FreeRTOS"
done

# Generate HSS payload (only if master was built)
if echo "$TARGETS" | grep -q "master"; then
    generate_hss_payload
fi

# Done
echo ""
echo "=============================================="
echo -e "\033[1;32m Build SUCCESSFUL! \033[0m"
echo "=============================================="
echo ""
if echo "$TARGETS" | grep -q "master"; then
    echo "  Master-Default/mpfs-rpmsg-master.elf"
fi
if echo "$TARGETS" | grep -q "remote"; then
    echo "  Remote-Default/mpfs-rpmsg-remote.elf"
fi
echo ""