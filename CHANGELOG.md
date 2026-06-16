# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.8] - 2026-06-16

### Added

- **RT-Thread Nano 启动测试 Demo** (oard/rtt_demo.c):
  - 实现 start_rtt_demo() 启动测试函数
  - Step 1: 初始化 MMUART0 控制台 (115200-8N1) + PLIC
  - Step 2: 调 t_hw_board_init() 初始化 Tick 和堆
  - Step 3: 关 MIE（折衷 SysTick_Config 过早开 MIE）
  - Step 4: 初始化调度器和定时器子系统
  - Step 5: 创建两个测试线程（1 秒和 0.5 秒间隔打印）
  - Step 6: 启动调度器（不返回）
- **src/middleware/RTThread/Makefile**: 新增 rtt_demo.c 编译

---

## [v0.7] - 2026-06-15

### Added

- **一键编译脚本** (`build.sh`):
  - 支持 Master/Remote/All/Clean 四种命令（`./build.sh master`、`./build.sh remote`、`./build.sh all`、`./build.sh clean`）
  - 自动检测 RISC-V 工具链路径（SoftConsole / 系统 PATH）
  - 并行编译（`-j$(nproc)`）
  - HSS payload 生成步骤可选（YAML 文件存在时才执行）
  - 编译成功校验：自动解析 ELF 文件的 text/data/bss 段大小

### Fixed

- **`build.sh`**: 修复 HSS payload generator 路径和 YAML 引用失效问题
- **`CHANGELOG.md`**: 移除 v0.1 中关于 git subtree "未完成"的过时 Notes

---


---

## [v0.5] - 2026-06-15

### Added

- **平台层存量驱动复用方案**（`doc/rtt-nano-porting-guide.md` 补充章节）:
  - 新增「第二点五部分：平台层存量驱动复用方案」（7 个子节，~300 行）
  - Tick 定时器完全复用 HAL 链路: `SysTick_Config()` → `handle_m_timer_interrupt()` → `U54_1_sysTick_IRQHandler()`
  - Trap/异常处理完全由平台层 `mtvec` → `trap_vector` → `trap_from_machine_mode()` 链路承担
  - UART 控制台直接调用已编译的 MSS UART 裸机驱动
  - 可复用性总表涵盖 10 个功能模块的复用方式
  - 总结：真正需要编写的板级代码仅 ~25 行

### Changed

- **`board/board.c`** — 重写为 HAL 复用方案:
  - 直接使用 `mpfs_hal/mss_hal.h` 头文件访问 CLINT 结构体
  - Tick 初始化通过 `SysTick_Config()` 完成，不再手动操作寄存器
  - Tick 中断通过 weak 函数 `U54_1_sysTick_IRQHandler()` 钩入 HAL 链路
  - 控制台输出通过 `rt_hw_console_output()` 绑定到 MMS UART 裸机驱动

- **`board/board.h`** — 简化，移除与 HAL 重复的 CLINT 地址宏定义

### Removed

- **`portable/cpuport.c`** 和 `portable/rtt_port.h` — 手工存根，rt-thread 官方 port 文件已覆盖其功能
- **Makefile 中引用上述存根的行**

---

## [v0.4] - 2026-06-15

### Added

- **平台层架构文档**（`doc/platform-layer-architecture.md`）:
  - HAL 层 API 接口（CLINT/PLIC/PMP/MPU/L2 Cache）
  - MSS 外设驱动 API 表（15 个 MSS 外设 + 4 个 FPGA IP 驱动）
  - 启动代码流程（mss_entry.S → system_startup.c → 用户入口）
  - 驱动启用指南（当前编译状态 + 新增驱动 Makefile 示例）
  - 关键硬件地址映射附录

### Fixed

- **`rules.mk`** — 从初始提交 `v0.0` 恢复，修正 v0.1 中因管道编码问题导致的文件清空
- **`doc/rtt-nano-porting-guide.md`** — 全文重写:
  - 删除所有 `-DUSING_RTTHREAD` 引用（该宏未在 `rules.mk` 中定义）
  - 将定位从 AMP 多核改为单 U54 核运行
  - 新增完整调用链（应用层 → RT-Thread 内核 → 移植层 → 硬件）
  - 新增适配原因分析：每步包含「为什么要做？」+「后果」
  - 修正定时器 ISR 入口描述

---

## [v0.3] - 2026-06-15

### Added

- **RT-Thread Nano 详细移植指南**（`doc/rtt-nano-porting-guide.md`）:
  - Step 1~8 逐步移植逻辑，每步含核心需求、目标、当前状态
  - 最小启动移植目标 + 完整版本移植目标（Minimal/Full Version Target）
  - FreeRTOS ↔ RT-Thread API 对照表
  - 硬件地址附录、调试检查清单、配置建议

### Fixed

- **`rt-thread/` 目录嵌套修复** — 将 `rt-thread/rt-thread/*` 内容上移至 `rt-thread/`，`context_gcc.S`
  和 `cpuport.c` 路径从 `rv64/` 修正为 `risc-v/common/`
- **`doc/freertos-port-architecture-analysis.md`** — 规范化标题和表述
  - 删除所有原始 C/ASM/Makefile 代码引用
  - 用表格/流程图/关系图替代原始代码块

### Changed

- **`src/middleware/RTThread/Makefile`** — 源文件路径更新为 `libcpu/risc-v/common/`

---

## [v0.2] - 2026-06-15

### Added

- **git subtree 引入 `rtthread-nano` 源码**:
  - 用户通过 `git subtree add --prefix=src/middleware/RTThread/rt-thread`
    `https://github.com/RT-Thread/rtthread-nano.git master --squash` 完成
  - 包含 `src/`（内核源码）、`include/`（内核头文件）、`libcpu/`（RISC-V 移植层）
  - 另含 `bsp/`（板级参考）、`components/finsh`（FinSH 控制台源码）

### Fixed

- **清理 v0.1 手工存根**: 删除旧 `rt-thread/` 目录中的手动创建 stub 文件，为 git subtree 清理路径

---

## [v0.1] - 2026-06-15

### Added

- **RT-Thread Nano 移植骨架**: 在 `src/middleware/RTThread/` 下创建了完整的 RT-Thread Nano 移植骨架。

  - **构建系统集成** (`src/middleware/RTThread/Makefile`):
    - 通过 `SRCS +=` / `ASM_SRCS +=` / `INCLUDES +=` 模式集成
    - 在 `src/middleware/Makefile` 中新增 `include`

  - **配置文件** (`src/middleware/config/rtconfig.h`):
    - 优先级数 31，Tick 频率 1000Hz，堆大小 512KB
    - 启用信号量/互斥量/事件/邮箱/消息队列等 IPC 机制

  - **板级支持** (`board/board.c` / `board.h`):
    - OS Tick 定时器初始化（CLINT MTimer）
    - 堆内存初始化
    - 板级硬件初始化框架

  - **CPU 移植层**:
    - `rtt_port.h`: 移植宏定义（临界区管理、类型定义）
    - `context_switch.S`: 汇编上下文切换
    - `cpuport.c`: 线程栈初始化（rt_hw_stack_init）

  - **RPMsg-Lite 环境适配** (`rpmsg_env_rtthread.c`): 占位文件

- **架构分析文档** (`doc/freertos-port-architecture-analysis.md`):
  - 梳理 FreeRTOS 移植架构（目录组织、构建系统、CPU 移植层、配置管理、应用层启动流程）

### Changed

- `src/middleware/Makefile`: 新增 `include src/middleware/RTThread/Makefile`



- 后续需在有网络环境时运行 `git subtree add` 获取完整 Nano 源码

---

## [v0.0] - Baseline

- 初始导入 mpfs-rpmsg-freertos 基项目（PolarFire SoC FreeRTOS + RPMsg AMP 示例）

