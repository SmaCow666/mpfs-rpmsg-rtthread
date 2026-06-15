# PolarFire SoC FreeRTOS 移植架构分析

## 文档概述

本文档系统梳理了 PolarFire SoC Icicle Kit 平台上现有 FreeRTOS + RPMsg 项目的移植架构，
涵盖目录组织、构建系统集成、CPU 移植层接口、配置管理策略以及应用层启动流程。
分析结果直接作为 RT-Thread Nano 移植骨架的设计蓝本。

---

## 1. 项目上下文

| 项目属性 | 描述 |
|---|---|
| 硬件平台 | Microchip PolarFire SoC Icicle Kit（RISC-V 多核 AMP） |
| 核心构成 | 1x E51 管理核 + 4x U54 应用核（rv64gc） |
| RTOS 版本 | FreeRTOS V8.2.3 |
| 通信框架 | RPMsg-Lite（OpenAMP）/ MIV IHC 核间中断 |
| 构建工具 | riscv64-unknown-elf-gcc，裸机 Makefile 系统 |
| 兼容拓扑 | FreeRTOS↔FreeRTOS / FreeRTOS↔BM / Linux↔FreeRTOS |

---

## 2. 源代码目录架构

```
/                                       # 项目根
├── Makefile                            # 顶层构建入口，多级 include 链
├── README.md
├── src/
│   ├── application/                    # 用户应用层
│   │   ├── Makefile                    # 应用 SRCS/INCLUDES 声明
│   │   ├── rules.mk                    # 编译器、标志、构建规则
│   │   ├── targets.mk                  # 链接与后处理规则
│   │   ├── hart0/e51.c                 # E51 应用入口（管理核）
│   │   ├── hart[1-4]/u54_[1-4].c       # U54 应用入口（计算核）
│   │   └── inc/                        # 演示代码与 RPMsg 例程
│   ├── boards/icicle-kit-es/           # 板级支持
│   │   ├── Makefile                    # BINDIR / LINKER_SCRIPT / SOC 配置
│   │   ├── fpga_design_config/         # Libero 设计导出（时钟/DDR/PMP）
│   │   └── platform_config/linker/     # 链接脚本 *.ld
│   ├── middleware/                     # 中间件层（核心关注区域）
│   │   ├── Makefile                    # 聚合 FreeRTOS / misc / rpmsg / remoteproc
│   │   ├── config/
│   │   │   ├── FreeRTOSConfig.h        # 操作系统配置
│   │   │   └── rpmsg_config.h          # RPMsg 配置
│   │   ├── FreeRTOS/                   # FreeRTOS 内核 + 移植层
│   │   │   ├── Makefile                # 变量追加式集成
│   │   │   ├── include/                # 内核 API 头文件
│   │   │   ├── croutine.c / list.c / queue.c / tasks.c / timers.c / event_groups.c
│   │   │   └── portable/
│   │   │       ├── GCC/RISCV/          # CPU 架构移植层（三层分离）
│   │   │       │   ├── portmacro.h     #  → 类型定义 + 架构宏
│   │   │       │   ├── port.c          #  → 栈初始化 + 定时器 + 临界区
│   │   │       │   └── portasm.S       #  → 上下文切换汇编
│   │   │       └── MemMang/            # heap_1 ~ heap_4
│   │   ├── misc/                       # 串口、vsprintf、c_stubs
│   │   ├── rpmsg/                      # RPMsg-Lite 完整协议栈
│   │   └── remoteproc/                 # Remote Proc 生命周期管理
│   └── platform/                       # MPFS HAL + 外设驱动
│       ├── mpfs_hal/                   # CLINT / PLIC / PMP / 多核启动
│       └── drivers/                    # UART / GPIO / IHC / SPI 等
```

**架构特点**: 所有中间件和平台代码通过 Makefile 的 `SRCS +=` / `ASM_SRCS +=` / `INCLUDES +=` 变量追加方式集成到统一构建流中，不存在独立的库编译步骤。

---

## 3. 构建系统集成模式

### 3.1 多级包含链

顶层 Makefile 依次引入应用、板级、平台、中间件的 Makefile。中间件层进一步聚合各子模块：

```
顶层 Makefile
├── src/application/Makefile
├── src/boards/${BOARD}/Makefile
├── src/application/rules.mk            # 编译器、编译标志、构建规则
├── src/application/targets.mk          # 最终链接规则
├── src/platform/Makefile
└── src/middleware/Makefile             # 聚合中间件
    ├── src/middleware/FreeRTOS/Makefile
    ├── src/middleware/misc/Makefile
    ├── src/middleware/rpmsg/Makefile
    └── (条件) src/middleware/remoteproc/Makefile
```

### 3.2 编译器与关键标志

工具链采用 `riscv64-unknown-elf-gcc`，架构参数匹配 PolarFire SoC U54 核心：

- 架构：`rv64gc`，ABI：`lp64d`
- 寻址模型：`medany`（PIC 寻址，适合 DDR 运行）
- 内存对齐：`mstrict-align`（防止非对齐访问异常）
- 调试等级：`-O0 -g3`（可优化为 `-Os` 用于生产）
- RTOS 选择宏：`-DUSING_FREERTOS`（为多 RTOS 共存提供条件编译基础）

### 3.3 FreeRTOS 集成的具体模式

FreeRTOS 通过独立的 Makefile 将自身源文件追加到全局编译变量中。内核源文件、移植层源文件、配置路径均以变量追加方式加入，不产生独立的归档库：

```
# src/middleware/FreeRTOS/Makefile（集成模式示意）
SRCS    += kernel/*.c  port.c  heap_2.c
ASM_SRCS += portasm.S
INCLUDES += -Iinclude  -Iconfig  -Iportable/GCC/RISCV
```

---

## 4. CPU 移植层三层分离模型

FreeRTOS 的 RISC-V 移植遵循严格的层次分离：

| 层次 | 文件 | 职责 | 核心接口 |
|---|---|---|---|
| 类型与宏层 | portmacro.h | 数据类型映射、架构特性宏、临界区/任务切换宏 | `portSTACK_TYPE`、`portDISABLE_INTERRUPTS()`、`portYIELD()` |
| C 实现层 | port.c | 栈帧初始化、系统滴答配置、临界区进入/退出 | `pxPortInitialiseStack()`、`vPortSetupTimer()`、`vPortSysTickHandler()` |
| 汇编层 | portasm.S | 上下文保存/恢复、调度器启动、任务切换、ISR 内切换 | `portSAVE_CONTEXT`/`portRESTORE_CONTEXT`、`xPortStartScheduler()` |

### 4.1 移植层关键关注点

**portmacro.h** 定义了 RISC-V 移植的核心抽象：

- 自适应 rv32/rv64 的类型体系（`portSTACK_TYPE` = uint32_t 或 uint64_t）
- 栈向下增长（`portSTACK_GROWTH = -1`）
- 临界区通过操作 `mstatus.MIE` 位实现中断开关
- 任务切换宏映射到汇编例程 `vPortYield()`、`vPortYieldISR()`

**port.c** 负责硬件相关的 C 代码：

- 栈帧初始化支持 rv32/rv64，按 RISC-V ABI 布局 34 个寄存器
- 系统滴答通过 CLINT MTimer 驱动，寄存器地址硬编码为 PolarFire SoC 映射
- 临界区嵌套计数存储在 TCB 中

**portasm.S** 实现性能敏感的上下文切换：

- 保存/恢复所有通用寄存器（x1~x31）到任务栈
- 通过 `mepc` 恢复程序计数器，通过 `mstatus` 恢复机器模式
- 支持两种返回路径：`mret`（直接任务切换）和 `ret`（ISR 尾部切换）

### 4.2 定时器中断处理流程

```
CLINT MTimer 触发机器级定时器中断
  → TIMER_CMP_INT（汇编入口，portasm.S）
    → portSAVE_CONTEXT（保存当前任务上下文）
    → portSAVE_EPC（保存故障 PC）
    → vPortSysTickHandler（C 处理函数，port.c）
      → 更新 timecmp = mtime + tick_interval
      → xTaskIncrementTick() → 如有必要 vTaskSwitchContext()
    → portRESTORE_CONTEXT（恢复目标任务上下文）
    → mret（返回任务模式）
```

---

## 5. 配置管理策略

### 5.1 FreeRTOS 配置参数

`FreeRTOSConfig.h` 定义了以下关键参数：

| 参数域 | 典型值 | 含义 |
|---|---|---|
| CPU 时钟 | 100 MHz | configCPU_CLOCK_HZ |
| CLINT 时钟 | 1 MHz | configTICK_CLOCK_HZ |
| 系统 Tick | 1000 Hz | configTICK_RATE_HZ |
| 最大优先级 | 31 | configMAX_PRIORITIES |
| 堆大小 | 512 KB | configTOTAL_HEAP_SIZE |
| 堆分配策略 | 应用层分配 | configAPPLICATION_ALLOCATED_HEAP = 1 |

### 5.2 堆管理机制

采用 heap_2.c（最佳适配算法），堆内存显式声明并放置在特定链接段：

```
uint8_t __attribute__((section(".FreeRTOSheap"))) ucHeap[512 * 1024];
```

链接脚本将 `.FreeRTOSheap` 段定位到 `ddr_cached_32bit` 区域（0x91C00000+）。

---

## 6. 应用层启动与集成

### 6.1 多核 AMP 启动流程

```
上电复位
  → mss_entry.S（汇编初始化，设置栈指针）
  → system_startup.c
    → init_memory()（复制 .text/.data，清零 .bss）
    → main_first_hart()（E51 依次唤醒 U54 核）
      → 对于每个从核：检查 wfi → 发送 MSIP 软件中断 → 确认唤醒
    → main_other_hart()
      → switch(hartid) { case 0: e51(); case 1: u54_1(); ... }
```

### 6.2 FreeRTOS 启动模式

应用核中的启动示例（U54_1）：

1. AMP 核间同步：从核在 WFI 中等待 E51 发送软件中断（MSIP）
2. 外设初始化：配置 PLIC、UART
3. 创建 FreeRTOS 任务：`xTaskCreate(task_one, ...)`
4. 启动调度器：`vTaskStartScheduler()`
5. 第一个任务中配置系统定时器：`vPortSetupTimer()`
6. RPMsg 初始化：`rpmsg_lite_master_init()` 或 `rpmsg_lite_remote_init()`
7. 进入任务主循环

### 6.3 关键耦合点

| 耦合点 | 详情 | 影响范围 |
|---|---|---|
| CLINT 寄存器地址 | mtime(0x0200bff8) / timecmp(0x02004000) | 定时器驱动的硬件依赖 |
| 堆段名 | `.FreeRTOSheap` | 需在链接脚本中显式声明 |
| 核间同步协议 | MSIP 软件中断 | AMP 多核必要机制 |
| PLIC 中断路由 | RPMsg IHC 通道 8(主) / 21(从) | 核间通信 |
| UART 外设 | MMUART1(主) / MMUART3(从) | 调试与控制台 |

---

## 7. 软件栈层次耦合图

```
┌─────────────────────────────────────────────────┐
│              Application Layer                   │
│  e51.c / u54_*.c → start_demo → freertos_task   │
│         ↕ RTOS API + MPFS HAL + RPMsg API        │
├────────────┬──────────┬──────────┬───────────────┤
│  FreeRTOS  │ RPMsg    │ Remoteproc │   misc      │
│  Kernel    │ -Lite    │ (IHC)     │   (UART)     │
├────────────┴──────────┴──────────┴───────────────┤
│          MPFS HAL + Drivers Layer                 │
│  CLINT / PLIC / PMP / UART / IHC / GPIO / Cache  │
├──────────────────────────────────────────────────┤
│            PolarFire SoC (Icicle Kit)             │
│     E51(rv64imac) + U54_1~4(rv64gc) + DDR        │
└──────────────────────────────────────────────────┘
```

---

## 8. 为 RT-Thread Nano 移植提供的设计模式

以下设计模式可直接迁移到 RT-Thread Nano 的移植实现中：

1. **目录着色**：在 `src/middleware/RTThread/` 下镜像 FreeRTOS 的目录结构
2. **变量追加式构建**：通过 `SRCS +=` / `ASM_SRCS +=` / `INCLUDES +=` 模式集成
3. **CPU 移植三层分离**：rtt_port.h（类型宏层） / cpuport.c（C 实现层） / context_gcc.S（汇编层）
4. **定时器模型复用**：同样使用 CLINT MTimer → mtime/timecmp 寄存器驱动 OS Tick
5. **配置分离**：rtconfig.h 位于 `src/middleware/config/`，与 FreeRTOSConfig.h 同级
6. **堆管理**：参照 `.FreeRTOSheap` 段模式，使用链接脚本自定义段
7. **AMP 协同**：通过 RPMsg-Lite 的 rpmsg_env 抽象层支持多 RTOS 环境
