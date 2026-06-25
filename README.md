# RT-Thread Nano BSP for PolarFire SoC Icicle Kit

RT-Thread Nano real-time operating system port for the Microchip PolarFire SoC (MPFS250T) Icicle Kit, running on a single U54 core in machine mode.

This project is derived from [mpfs-rpmsg-freertos](https://github.com/polarfire-soc/mpfs-rpmsg-freertos) and replaces FreeRTOS with RT-Thread Nano while reusing the existing MPFS HAL and RPMsg-Lite framework.

## Features

- **RT-Thread Nano v4.1.0** kernel running on U54_1 (single core, machine mode)
- **Hardware timer** via CLINT MTimer (HAL `SysTick_Config()` chain)
- **UART console** (MMUART1, 115200-8N1) with interrupt-driven RX and background-thread TX
- **FinSH/MSH shell** for interactive command-line debugging
- **SPSC ring buffer** for lock-free ISR-safe UART I/O
- **RPMsg-Lite** framework integration (master/remote AMP communication)
- **Build system**: GNU Make, compatible with SoftConsole and command-line builds

## Directory Structure

```
src/
  application/          User application code (start_rtt_demo, test threads)
  middleware/
    FreeRTOS/           Original FreeRTOS port (retained for reference)
    RTThread/           RT-Thread Nano port
      rt-thread/        Kernel source (git subtree from rtthread-nano repo)
      board/            Board-level support (board.c/h, ringbuf.h/c)
      components/       Shell, commands
    config/             Kernel configuration (rtconfig.h)
    rpmsg/              RPMsg-Lite framework
  platform/
    mpfs_hal/           Microchip MPFS HAL (bare-metal drivers)
    drivers/            MSS peripheral drivers (UART, GPIO, etc.)
  boards/               Board configuration, linker scripts
```

## Quick Start

### Prerequisites

- RISC-V toolchain (riscv64-unknown-elf-gcc, included with SoftConsole)
- GNU Make
- Hart Software Services (HSS) for Icicle Kit
- Microchip SoftConsole (optional, for IDE debugging)

### Building

```bash
# Build the RT-Thread master application (U54_1)
./build.sh
```

The build script auto-detects the RISC-V toolchain (SoftConsole path or system PATH).

### Running

1. Generate an HSS payload containing the built ELF file
2. Flash to SD card or Use Flash Pro
3. Boot the Icicle Kit - the RT-Thread application starts automatically on U54_1
4. Connect to MMUART1 (115200-8N1) for the FinSH console

## Console Output

On successful boot, the serial console shows:

```
"RT - Thread  version"
msh />
```

The `msh />` prompt indicates the FinSH shell is ready. Type `help` for available commands.

## UART Configuration

| Parameter | Value |
|-----------|-------|
| UART Instance | MMUART1 |
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| TX | Background thread + ring buffer (non-blocking) |
| RX | Interrupt-driven via PLIC/local interrupt |

## License

This project inherits the MIT license from the original mpfs-rpmsg-freertos base.
See the [LICENSE](LICENSE) file for details.
