# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [v1.0] - 2026-06-24

### Added

- **SPSC ring buffer module** (board/ringbuf.h/c):
  - Lock-free single-producer/single-consumer, ISR-safe
  - TX instance (512B): rt_hw_console_output -> con print thread -> UART
  - RX instance (64B): UART ISR _uart_rx_handler -> FinSH input

- **Background print thread** (board/board.c):
  - console_print_entry() at lowest priority, blocks on semaphore
  - rt_hw_console_output() writes to TX ring buffer, releases semaphore

- **UART RX interrupt-driven input** (board/board.c):
  - _uart_rx_handler() registered via MSS_UART_set_rx_handler()
  - MSS_UART_get_rx() drains FIFO into RX ring buffer
  - Local interrupt path: MSS_UART_enable_local_irq()

- **RTT context format conversion** (mss_entry.S):
  - #ifdef USING_RTTHREAD: in-place HAL frame to RTT frame conversion
  - Flag check after jal trap_from_machine_mode
  - Restored missing restore_regs: label

- **FinSH/MSH console**:
  - rtconfig.h: added FINSH_USING_SYMTAB, FINSH_USING_DESCRIPTION
  - rt_hw_console_getchar() reads from RX ring buffer
  - finsh_system_init() called before rt_system_scheduler_start()

- **Linker script extensions** (master + remote):
  - .rti_fn sections: __rt_init_rti_start/end + KEEP(SORT(.rti_fn*))
  - FSymTab sections: __fsymtab_start/end + KEEP(FSymTab*)

- **Documents** (doc/):
  - v0.11 scheduler hang analysis
  - v0.12 context format fix guide

### Fixed

- **Scheduler hang**: U54_1_sysTick_IRQHandler added rt_interrupt_enter/leave
- **UART ISR ASSERT**: Removed dual-path conflict (MSS_UART_enable_local_irq + MIP_MEIP)
- **Empty FinSH command table**: FSymTab section added KEEP() in linker script
- **INIT_APP_EXPORT broken**: finsh_system_init called directly before scheduler start
- **Dead code**: finsh_system_init moved before scheduler start
- **cmd.c compile error**: Removed extra quotes from MSH_CMD_EXPORT descriptions

### Changed

- Build system: Makefile added ringbuf.c, shell.c, msh.c etc.
- Configuration: rtconfig.h FinSH config; mss_sw_config.h user config

### Removed

- Empty placeholder: libcpu/risc-v/polarfire/interrupt_gcc.s (0 bytes)

---

## [v0.10] - 2026-06-16

### Fixed

- **rtt_demo.c fix**: Changed UART from MMUART0 to MMUART1; added rthw.h for rt_hw_interrupt_disable() visibility; thread stack 1024 -> 2048
- **u54_1.c**: Added include for rtt_demo.h to make start_rtt_demo() declaration visible

### Changed

- **board.c**: rt_hw_console_output MMUART0 -> MMUART1 (user config)
- **board.h**: UART instance type uintptr_t -> mss_uart_instance_t (user config)
- **build.sh, rpmsg_platform.c**: User configuration adjustments
- **hss-payload-rtthread.yaml**: Added HSS payload configuration file
- **Makefile**: Added rtt_demo.c to app; removed orphan RTT references

---

## [v0.9] - 2026-06-16

### Fixed

- **`src/middleware/Makefile` BOM fix**: Removed UTF-8 BOM (0xEF BB BF) to fix make "missing separator" error
- **Full Makefile encoding audit**: All Makefiles verified clean (no BOM, no NUL, pure ASCII)

---

## [v0.8] - 2026-06-16

### Added

- **RT-Thread Nano startup test demo** (board/rtt_demo.c):
  - Implemented start_rtt_demo() startup test function
  - Step 1: Init MMUART0 console (115200-8N1) + PLIC
  - Step 2: Call rt_hw_board_init() for Tick and heap init
  - Step 3: Disable MIE (workaround for SysTick_Config early MIE enable)
  - Step 4: Init scheduler and timer subsystems
  - Step 5: Create two test threads (1s and 0.5s print intervals)
  - Step 6: Start scheduler (never returns)
- **src/middleware/RTThread/Makefile**: Added rtt_demo.c compilation

---

## [v0.7] - 2026-06-15

### Added

- **One-click build script** (`build.sh`):
  - Supports Master/Remote/All/Clean commands (`./build.sh master`, `./build.sh remote`, `./build.sh all`, `./build.sh clean`)
  - Auto-detects RISC-V toolchain path (SoftConsole / system PATH)
  - Parallel build (`-j$(nproc)`)
  - Optional HSS payload generation (only if YAML file exists)
  - Build validation: auto-parses ELF text/data/bss section sizes

### Fixed

- **`build.sh`**: Fixed HSS payload generator path and YAML reference issues
- **`CHANGELOG.md`**: Removed outdated v0.1 git subtree notes

---

## [v0.6] - 2026-06-15

### Added

- **board.c reuse MPFS HAL**: Tick via SysTick_Config(); override U54_1_sysTick_IRQHandler(); rt_hw_console_output() mapped to MSS UART; heap from __bss_end to _end + 512KB
- **CHANGELOG completion**: Added v0.2 ~ v0.5 entries

### Changed

- `board.h`: Simplified, removed CLINT address macros duplicated with HAL
- `Makefile`: Removed portable/cpuport.c stub reference

### Removed

- `portable/cpuport.c`, `portable/rtt_port.h`: Manual stubs removed; official RTT port covers these

---

## [v0.5] - 2026-06-15

### Added

- **Platform layer driver reuse** (`doc/rtt-nano-porting-guide.md` supplement):
  - Added section 2.5: Platform driver reuse scheme (7 subsections, ~300 lines)
  - Tick timer fully reuses HAL chain: SysTick_Config() -> handle_m_timer_interrupt() -> U54_1_sysTick_IRQHandler()
  - Trap/exception handling fully by platform layer: mtvec -> trap_vector -> trap_from_machine_mode()
  - UART console directly calls compiled MSS UART bare-metal driver
  - Reuse summary table covering 10 functional modules
  - Conclusion: only ~25 lines of board code actually needed

### Changed

- **`board/board.c`** - Rewritten with HAL reuse:
  - Uses mpfs_hal/mss_hal.h directly for CLINT structure access
  - Tick init via SysTick_Config(), no manual register operations
  - Tick ISR hooks into HAL chain via weak function U54_1_sysTick_IRQHandler()
  - Console output bound to MSS UART via rt_hw_console_output()
- **`board/board.h`** - Simplified, removed CLINT address macros duplicated with HAL

### Removed

- **`portable/cpuport.c`** and **`portable/rtt_port.h`** - Manual stubs removed; covered by official RTT port files
- **Makefile**: Removed stub references

---

## [v0.4] - 2026-06-15

### Added

- **Platform architecture doc** (`doc/platform-layer-architecture.md`):
  - HAL API interfaces (CLINT/PLIC/PMP/MPU/L2 Cache)
  - MSS peripheral driver API table (15 MSS peripherals + 4 FPGA IP drivers)
  - Boot flow: mss_entry.S -> system_startup.c -> user entry
  - Driver enable guide (build status + Makefile examples)
  - Key hardware address map appendix

### Fixed

- **`rules.mk`** - Restored from v0.0 initial commit, fixed pipe encoding corruption from v0.1
- **`doc/rtt-nano-porting-guide.md`** - Full rewrite:
  - Removed all -DUSING_RTTHREAD references (macro not defined in rules.mk)
  - Changed target from AMP multi-core to single U54 core operation
  - Added complete call chain (app -> RTT kernel -> port layer -> hardware)
  - Added adaptation reasoning: each step explains why + consequences
  - Fixed timer ISR entry description

---

## [v0.3] - 2026-06-15

### Added

- **Detailed RTT Nano porting guide** (`doc/rtt-nano-porting-guide.md`):
  - Step 1-8 porting logic, each with core requirements, targets, status
  - Minimal boot target + full version target
  - FreeRTOS <-> RT-Thread API mapping table
  - Hardware address appendix, debug checklist, config suggestions

### Fixed

- **`rt-thread/` directory nesting fix** - Moved rt-thread/rt-thread/* up to rt-thread/; fixed context_gcc.S and cpuport.c paths from rv64/ to risc-v/common/
- **`doc/freertos-port-architecture-analysis.md`** - Standardized titles and wording:
  - Removed all raw C/ASM/Makefile code references
  - Replaced code blocks with tables/flowcharts/diagrams

### Changed

- **`src/middleware/RTThread/Makefile`** - Updated source paths to libcpu/risc-v/common/

---

## [v0.2] - 2026-06-15

### Added

- **git subtree import of rtthread-nano source**:
  - User ran `git subtree add --prefix=src/middleware/RTThread/rt-thread` `https://github.com/RT-Thread/rtthread-nano.git master --squash`
  - Includes src/ (kernel source), include/ (kernel headers), libcpu/ (RISC-V port layer)
  - Also includes bsp/ (board references), components/finsh (FinSH console source)

### Fixed

- **Clean v0.1 stubs**: Removed manually created stub files from old rt-thread/ directory, clearing path for git subtree

---

## [v0.1] - 2026-06-15

### Added

- **RT-Thread Nano port skeleton**: Created complete RT-Thread Nano port skeleton under src/middleware/RTThread/.

  - **Build system integration** (`src/middleware/RTThread/Makefile`):
    - Integration via SRCS += / ASM_SRCS += / INCLUDES += pattern
    - Added include in src/middleware/Makefile

  - **Configuration** (`src/middleware/config/rtconfig.h`):
    - 31 priorities, 1000Hz tick, 512KB heap
    - Enabled semaphore/mutex/event/mailbox/message queue IPC

  - **Board support** (`board/board.c` / `board.h`):
    - OS Tick timer init (CLINT MTimer)
    - Heap memory initialization
    - Board hardware init framework

  - **CPU port layer**:
    - rtt_port.h: Port macros (critical section management, type definitions)
    - context_switch.S: Assembly context switch
    - cpuport.c: Thread stack init (rt_hw_stack_init)

  - **RPMsg-Lite environment adapter** (`rpmsg_env_rtthread.c`): Placeholder

- **Architecture analysis doc** (`doc/freertos-port-architecture-analysis.md`):
  - FreeRTOS port architecture overview (directory structure, build system, CPU port, configuration, app startup flow)

### Changed

- `src/middleware/Makefile`: Added include src/middleware/RTThread/Makefile

---

## [v0.0] - Baseline

- Initial import of mpfs-rpmsg-freertos base project (PolarFire SoC FreeRTOS + RPMsg AMP example)
