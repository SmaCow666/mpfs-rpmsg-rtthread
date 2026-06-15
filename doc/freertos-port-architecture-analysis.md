# FreeRTOS 移植架构分析文档

> 本文档针对 PolarFire SoC Icicle Kit 平台上现有的 FreeRTOS + RPMsg 项目进行移植架构分析，
> 梳理其目录组织、构建系统集成、CPU 移植层、配置管理以及应用层接口细节，
> 为后续构建 RT-Thread Nano 移植骨架提供参考蓝本。

---

## 1. 项目概述

本项目基于 Microchip PolarFire SoC（RISC-V 多核架构），
提供 FreeRTOS V8.2.3 + RPMsg-Lite 框架的 AMP（非对称多处理）示例。
支持以下通信拓扑：

- FreeRTOS ↔ FreeRTOS
- FreeRTOS ↔ Bare Metal
- Linux ↔ FreeRTOS

硬件平台为 **Icicle Kit**，包含 1 个 E51 管理核 + 4 个 U54 应用核。

---

## 2. 整体目录结构

```
/
├── Makefile                          # 顶层构建入口
├── README.md
├── src/
│   ├── application/                  # 用户应用代码
│   │   ├── Makefile                  # 应用 SRCS/INCLUDES 声明
│   │   ├── rules.mk                  # 编译器选型、编译标志、构建规则
│   │   ├── targets.mk                # 目标规则
│   │   ├── hart0/e51.c               # E51 应用入口
│   │   ├── hart1/u54_1.c ~ hart4/u54_4.c  # U54 应用入口
│   │   └── inc/
│   │       ├── common.h
│   │       ├── demo_main.c / .h      # 主演示逻辑（含 FreeRTOS 启动）
│   │       ├── pingpong_demo.c
│   │       ├── console_demo.c
│   │       ├── sample_echo_demo.c
│   │       └── rsc_table.c / .h
│   ├── boards/
│   │   └── icicle-kit-es/
│   │       ├── Makefile              # 板级配置（链接脚本、SOC 配置）
│   │       ├── fpga_design_config/   # 时钟/DDR/IOMUX/PMP 配置
│   │       └── platform_config/
│   │           └── linker/           # 链接脚本 *.ld
│   ├── middleware/
│   │   ├── Makefile                  # 中间件聚合（包含 FreeRTOS/misc/rpmsg/remoteproc）
│   │   ├── config/
│   │   │   ├── FreeRTOSConfig.h      # FreeRTOS 配置
│   │   │   └── rpmsg_config.h        # RPMsg 配置
│   │   ├── FreeRTOS/                 # FreeRTOS 内核 + 移植层
│   │   │   ├── Makefile
│   │   │   ├── include/              # 内核头文件
│   │   │   ├── croutine.c / event_groups.c / list.c /
│   │   │   │   queue.c / tasks.c / timers.c
│   │   │   └── portable/
│   │   │       ├── GCC/RISCV/        # CPU 架构移植层
│   │   │       │   ├── port.c
│   │   │       │   ├── portasm.S
│   │   │       │   └── portmacro.h
│   │   │       └── MemMang/          # 堆管理策略
│   │   │           ├── heap_1.c ~ heap_4.c
│   │   ├── misc/                     # 辅助模块（串口、vsprintf、assert）
│   │   ├── rpmsg/                    # RPMsg-Lite 协议栈
│   │   └── remoteproc/               # 远程处理器管理
│   └── platform/
│       ├── Makefile                  # 平台驱动聚合
│       ├── hal/                      # 硬件抽象层
│       ├── mpfs_hal/                 # MPFS HAL（启动、CLINT、PLIC、PMP 等）
│       └── drivers/                  # 外设驱动（UART、GPIO、IHC 等）
```

---

## 3. 构建系统集成深度分析

### 3.1 顶层 Makefile 包含链

```
顶层 Makefile
├── src/application/Makefile          # SRCS += 应用源文件, INCLUDES
├── src/boards/${BOARD}/Makefile      # BINDIR, LINKER_SCRIPT, SOC 配置生成
├── src/application/rules.mk          # 编译器、编译标志、.c->.o / .S->.o 规则
├── src/application/targets.mk        # 最终链接规则
├── src/platform/Makefile             # SRCS/ASM_SRCS/INCLUDES += 平台层
└── src/middleware/Makefile           # 聚合中间件
    ├── src/middleware/FreeRTOS/Makefile
    ├── src/middleware/misc/Makefile
    ├── src/middleware/rpmsg/Makefile
    └── (条件) src/middleware/remoteproc/Makefile
```

### 3.2 编译器与标志（rules.mk）

```
CROSS_COMPILE = riscv64-unknown-elf-
PLATFORM_RISCV_ABI = lp64d
PLATFORM_RISCV_ISA  = rv64gc
CORE_CFLAGS += -DUSING_FREERTOS
               -mcmodel=medany -mabi=lp64d -march=rv64gc
               -msmall-data-limit=8 -mstrict-align -mno-save-restore
               -O0 -ffunction-sections -fdata-sections -g3
```

关键观察：通过 -DUSING_FREERTOS 宏标识当前 RTOS 类型，为多 RTOS 共存提供了条件编译基础。

### 3.3 FreeRTOS Makefile 的集成模式

```makefile
# src/middleware/FreeRTOS/Makefile
SRCS += \
    src/middleware/FreeRTOS/croutine.c \
    src/middleware/FreeRTOS/event_groups.c \
    src/middleware/FreeRTOS/list.c \
    src/middleware/FreeRTOS/queue.c \
    src/middleware/FreeRTOS/tasks.c \
    src/middleware/FreeRTOS/timers.c \
    src/middleware/FreeRTOS/portable/GCC/RISCV/port.c \
    src/middleware/FreeRTOS/portable/MemMang/heap_2.c \

ASM_SRCS += \
    src/middleware/FreeRTOS/portable/GCC/RISCV/portasm.S \

INCLUDES += \
    -Isrc/middleware/FreeRTOS/include \
    -Isrc/middleware/config \
    -Isrc/middleware/FreeRTOS/portable/GCC/RISCV \
```

集成模式 = 将 RTOS 内核源文件、移植层源文件、配置路径直接追加到全局 SRCS/ASM_SRCS/INCLUDES 变量中。这是一种轻量级无侵入的"变量追加"集成方式，不需要单独的库构建步骤。

---

## 4. FreeRTOS 移植层细节

### 4.1 移植层文件职责

| 文件 | 职责 |
|---|---|
| portmacro.h | 数据类型定义（StackType_t, TickType_t）、架构宏（栈增长方向、字节对齐）、临界区管理宏、任务切换宏 |
| port.c | 栈初始化（pxPortInitialiseStack）、系统滴答定时器（vPortSysTickHandler, vPortSetupTimer）、临界区进入/退出、中断屏蔽 |
| portasm.S | 汇编上下文保存/恢复（portSAVE_CONTEXT/portRESTORE_CONTEXT）、调度器启动（xPortStartScheduler）、任务切换（vPortYield）、ISR 内切换（vPortYieldISR）、定时器中断处理（TIMER_CMP_INT） |

### 4.2 移植层关键接口

**portmacro.h — 类型与宏定义：**

```c
// RISC-V 64/32 自适应
#if __riscv_xlen == 64
    #define portSTACK_TYPE    uint64_t
    #define portBYTE_ALIGNMENT  8
#else
    #define portSTACK_TYPE    uint32_t
    #define portBYTE_ALIGNMENT  4
#endif

// 栈增长方向（向下）
#define portSTACK_GROWTH      ( -1 )

// 临界区
#define portDISABLE_INTERRUPTS()  __asm volatile ( "csrc mstatus,8" )
#define portENABLE_INTERRUPTS()   __asm volatile ( "csrs mstatus,8" )

// 任务切换
#define portYIELD()           vPortYield()
#define portYIELDisr()        vPortYieldisr()
#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    if( xSwitchRequired ) vPortYieldISR()
```

**port.c — 硬件定时器与栈初始化：**

```c
// MTIME/MTIMECMP 寄存器地址（PolarFire SoC 特定）
volatile uint64_t* mtime   = (volatile uint64_t*)0x0200bff8;
volatile uint64_t* timecmp = ((volatile uint64_t*)0x02004000) + hart_id;

// 定时器中断设置
static void prvSetNextTimerInterrupt(void) {
    *timecmp = *mtime + (configTICK_CLOCK_HZ / configTICK_RATE_HZ);
}

// 栈帧布局（模拟上下文切换中断）
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters) {
    // PC -> mepc, 参数 -> a0, thread pointer -> tp, return -> prvTaskExitError
    // 模拟 34 个寄存器压栈布局
}
```

**portasm.S — 上下文切换汇编：**

```asm
.macro portSAVE_CONTEXT
    addi    sp, sp, -REGBYTES * 34     # 分配栈帧
    STORE   x1, 0x0(sp)                # 保存 ra
    STORE   x2, 1 * REGBYTES(sp)       # 保存 sp
    ...                                 # 保存 x3-x31
    LOAD    t0, pxCurrentTCB           # 更新 TCB->pxTopOfStack
    STORE   sp, 0x0(t0)
.endm

.macro portRESTORE_CONTEXT
    LOAD    sp, pxCurrentTCB
    LOAD    sp, 0x0(sp)                # 加载新任务的栈指针
    LOAD    t0, 31 * REGBYTES(sp)
    csrw    mepc, t0                   # 恢复 mepc
    ...                                 # 恢复 x1, x4-x31
    # 支持两种返回路径：
    #   mret - 直接切回任务
    #   ret  - 从 ISR 尾部切回（嵌套中断场景）
.endm
```

### 4.3 定时器中断处理流程

```
CLINT MTimer 触发
    -> TIMER_CMP_INT (汇编入口)
        -> portSAVE_CONTEXT
        -> portSAVE_EPC
        -> jal vPortSysTickHandler
            -> prvSetNextTimerInterrupt()
            -> xTaskIncrementTick()
            -> vTaskSwitchContext() (如果需要)
        -> portRESTORE_CONTEXT
        -> mret
```

---

## 5. 配置管理

### 5.1 FreeRTOS 配置（FreeRTOSConfig.h）

| 配置项 | 值 | 说明 |
|---|---|---|
| configCPU_CLOCK_HZ | 100000000 | CPU 时钟频率 |
| configTICK_CLOCK_HZ | 1000000 | 定时器时钟频率（CLINT 频率） |
| configTICK_RATE_HZ | 1000 | 系统滴答频率 |
| configTOTAL_HEAP_SIZE | 512 * 1024 | 堆大小 |
| configAPPLICATION_ALLOCATED_HEAP | 1 | 堆由应用层分配 |
| configMAX_PRIORITIES | 31 | 最大优先级 |

### 5.2 堆管理策略

使用 heap_2.c（最佳适配算法），堆内存通过链接脚本放置在 .FreeRTOSheap 段：

```c
// demo_main.c
uint8_t __attribute__ ((section (".FreeRTOSheap"))) ucHeap[configTOTAL_HEAP_SIZE];
```

链接脚本中 .FreeRTOSheap 段被放置在 ddr_cached_32bit 区域。

---

## 6. 应用层集成模式

### 6.1 启动流程

```
系统上电
  -> mss_entry.S (汇编启动)
  -> system_startup.c (C 初始化)
      -> init_memory() - 复制 .text/.data，清零 .bss
      -> main_first_hart() - E51 依次唤醒其他核
      -> main_other_hart() - 调用 e51() / u54_1() / ...
```

### 6.2 FreeRTOS 启动（以 U54_1 为例）

```c
void u54_1(void) {
    // 1. AMP 核间同步（等待 E51 唤醒）
    clear_soft_interrupt();
    set_csr(mie, MIP_MSIP);

    // 2. 分派到 start_demo()
    if (MPFS_HAL_FIRST_HART == hartid) {
        start_demo();  // <- 创建 FreeRTOS 任务并启动调度器
    }
}

void start_demo() {
    // 初始化 UART、PLIC
    xTaskCreate(freertos_task_one, "task1", ...);
    vTaskStartScheduler();  // <- 启动调度器
    // 不会返回
}

void freertos_task_one(void *pvParameters) {
    vPortSetupTimer();     // <- 在第一个任务中配置定时器中断
    // RPMsg 初始化、菜单循环...
}
```

### 6.3 关键耦合点

| 耦合点 | 描述 |
|---|---|
| 定时器寄存器地址 | mtime/timecmp 硬编码为 PolarFire SoC CLINT 地址 |
| 堆区段名 | .FreeRTOSheap 在链接脚本中有明确定义 |
| 核间同步 | 软件中断（MSIP）由 HAL 管理 |
| PLIC 中断 | RPMsg 通过 PLIC + IHC 进行核间通信 |
| UART 控制台 | demo_main.c 中直接使用 MPFS HAL 的 UART 驱动 |

---

## 7. 层次耦合关系图

```
+-------------------------------------------------------+
|                    Application                          |
|  (e51.c / u54_*.c -> start_demo -> freertos_task)     |
|         <= depends on FreeRTOS API + MPFS HAL          |
+---------------+---------------+----------------------+
|  FreeRTOS     |   RPMsg-Lite |  Remoteproc            |
|  Kernel       |   (rpmsg)    |  (IHC控制)             |
|  tasks.c      |   rpmsg_lite |                        |
|  queue.c      |   virtqueue  |                        |
|  timers.c     |   env_freertos                        |
+---------------+---------------+----------------------+
|          Platform Layer (MPFS HAL + Drivers)           |
|  CLINT / PLIC / PMP / UART / IHC / GPIO / Cache       |
+-------------------------------------------------------+
|           Hardware (PolarFire SoC Icicle Kit)          |
|           E51 + U54_1~U54_4 / DDR / ENVM              |
+-------------------------------------------------------+
```

---

## 8. 为 RT-Thread Nano 移植提供的设计模式总结

以下模式可直接仿照用于 RT-Thread Nano 移植：

1. 目录着色：src/middleware/RTThread/ 镜像 src/middleware/FreeRTOS/ 结构
2. 变量追加式构建集成：通过 SRCS += / ASM_SRCS += / INCLUDES += 加入构建系统
3. CPU 移植三层分离：
   - rtt_port.h          <- 类型定义 + 架构宏（对应 portmacro.h）
   - cpuport.c           <- 栈初始化、GCC 内联汇编（对应 port.c）
   - context_switch.S    <- 汇编上下文切换（对应 portasm.S）
4. 定时器模型：复用 CLINT MTimer，通过 mtime/timecmp 寄存器驱动 OS Tick
5. 配置分离：rtconfig.h 位于 src/middleware/config/
6. 堆管理：参照 .FreeRTOSheap 段模式，使用链接脚本 .RTThread.heap 段
7. AMP 协同：通过 RPMsg-Lite 的 rpmsg_env 层支持多 RTOS 环境
