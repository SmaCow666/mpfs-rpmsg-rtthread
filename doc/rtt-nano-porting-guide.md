# RT-Thread Nano 移植指南 — PolarFire SoC Icicle Kit（单核版）

## 文档概述

本文档基于当前项目已完成的基础移植骨架，提供从骨架到可运行系统的完整移植指引。
所有描述均基于 **PolarFire SoC 单 U54 核运行**前提，不涉及 AMP 多核场景。

### 文档约定

- **"当前项目"**: 指 `mpfs-rpmsg-rtthread` 仓库
- **"移植者"**: 指执行本移植工作的开发人员
- **关键缩写**: BSP（板级支持包）、CLINT（核间本地中断控制器）、PLIC（平台级中断控制器）

---

## 第一部分：移植基础 — 适配方法总论

### 1.1 什么是 RTOS 移植？为什么需要它？

**本质**: RTOS 移植是将一个 RTOS 内核从"通用 C 代码"变为"能在特定硬件上运行的可执行固件"的过程。

**核心矛盾**: RT-Thread Nano 的核心代码（调度器、IPC、内存管理）是 **纯 C、与硬件无关的**。
但 OS 需要回答三个硬件相关的问题才能运行：

| 问题 | 涉及的硬件 | 不解决的后果 |
|---|---|---|
| **何时切换任务？** | 定时器（CLINT MTimer） | 系统无时间片，无法抢占，只能协作式运行 |
| **如何切换任务？** | CPU 寄存器（RISC-V GPR） | 上下文损坏，任务跑飞 |
| **如何互斥访问？** | 中断控制器（mstatus.MIE） | 临界区失效，竞态条件导致数据错乱 |

**移植的核心工作**：
```
RT-Thread 内核（纯 C，无硬件依赖）
  ↕ 通过 "移植层接口" 适配
具体硬件（PolarFire SoC → CLINT / RISC-V ISA / PLIC / UART）
```

### 1.2 移植适配的三层接口

RT-Thread Nano 向移植层暴露三个接口组，**全部必须实现**：

```
┌───────────────────────────────────────────────────────┐
│                   应用层 (e51.c / u54_1.c)             │
│     调用 RT-Thread API 创建线程、使用 IPC、延时        │
├───────────────────────────────────────────────────────┤
│               RT-Thread Nano 内核                      │
│       调度器 / IPC / 定时器 / 内存管理 / 对象管理       │
├──────────┬──────────────┬──────────────────────────────┤
│  接口组A  │  接口组B     │  接口组C                     │
│  上下文   │  中断控制    │  板级初始化                   │
│  切换     │   & 定时器   │  & 控制台                    │
├──────────┴──────────────┴──────────────────────────────┤
│              硬件层 (PolarFire SoC)                    │
│  RISC-V rv64gc / CLINT / PLIC / UART / DDR             │
└───────────────────────────────────────────────────────┘
```

**接口组 A — 上下文切换**（必须，否则无法多任务）
- `rt_hw_context_switch(from, to)` — 从当前线程切换到另一个线程
- `rt_hw_context_switch_to(to)` — 启动第一个线程时调用
- `rt_hw_context_switch_interrupt(from, to)` — 中断中调用
- `rt_hw_stack_init(entry, param, addr, exit)` — 初始化新线程的栈帧

**接口组 B — 中断控制与系统 Tick**（必须，否则无时间片）
- `rt_hw_interrupt_disable()` / `rt_hw_interrupt_enable(level)` — 临界区保护
- 定时器 ISR → 调用 `rt_tick_increase()` — 提供系统心跳

**接口组 C — 板级初始化**（必须，否则无法启动）
- `rt_hw_board_init()` — 配置定时器、初始化堆、注册控制台

### 1.3 最小启动 vs 完整版本

| 维度 | 最小启动 | 完整版本 |
|---|---|---|
| 目标 | 验证内核能否在单一 U54 核上跑起来 | 可运行 RPMsg 核间通信演示 |
| 验证方式 | 串口看到两个线程交替打印 | 运行 ping-pong / echo 样例 |
| 需移植的接口 | 接口组 A + B + C（共 5 个函数） | 接口组 A + B + C + RPMsg 环境适配层 |
| 移植周期估计 | 1~2 天 | 1~2 周 |
| 当前状态 | **骨架已完成，待完善细节** | **未开始** |

---

## 第二部分：调用层级细化

### 2.1 完整调用链（应用层 → 硬件）

以"RT-Thread 线程 A 延时 1 秒后打印"为例，完整调用链如下：

```
应用层: rt_thread_mdelay(1000)
  │
  ▼
RT-Thread 内核: 线程 A 调用 rt_thread_mdelay()
  ├── 设置线程状态为 RT_THREAD_SUSPEND
  ├── 将线程插入 tick_sleep 链表
  └── 调用 rt_schedule()
      ├── 找到优先级最高的就绪线程（线程 B）
      └── 调用 rt_hw_context_switch(&A_sp, &B_sp)
          │
          ▼
        移植层: context_gcc.S (rt_hw_context_switch)
          ├── 保存 A 的寄存器到 A 的栈：32 个通用寄存器 + mepc + mstatus
          ├── 更新 A.sp = 当前 sp
          ├── 加载 B.sp = 目标栈指针
          ├── 从 B 的栈恢复寄存器
          └── mret → 切换到线程 B 运行
              │
              ▼
         硬件: RISC-V CPU 执行 mret，PC 跳转到线程 B 的断点
```

```
CLINT MTimer 中断触发（1ms 后）
  │
  ▼
硬件: CLINT 将 mtimecmp 匹配，mie.MTIE 置位
  → CPU 跳转到机器模式定时器中断向量
  │
  ▼
移植层: 需自定义的定时器 ISR
  ├── 更新 timecmp = mtime + tick_interval（设置下一次中断）
  ├── 调用 rt_tick_increase()
  │     │
  │     ▼
  │   RT-Thread 内核:
  │     ├── 全局 tick 计数 +1
  │     └── 检查 tick_sleep 链表中的线程
  │         └── 如果线程 A 的延时已到 → 唤醒线程 A（状态置为 READY）
  │
  └── 检查是否需调度
      └── rt_schedule() → 如果 A 优先级更高，切换回 A
```

### 2.2 各层次职责与依赖

```
应用层      职责：创建线程、使用 IPC、处理数据
           依赖：RT-Thread API + UART 驱动 API
──────────  边界：应用只调 RT-Thread API，不直接操作移植层函数

RT-Thread    职责：任务调度、时间管理、同步互斥、内存分配
内核层       依赖：移植层的 5 个接口
           边界：内核不感知具体硬件细节

RISC-V       职责：实现上下文切换的汇编代码、临界区控制
移植层       依赖：RISC-V 指令集（mstatus、mepc、mret）
(context_   边界：不涉及具体 SoC 外设，只操作 CPU CSR
gcc.S)

板级支持    职责：初始化 CLINT 定时器、堆空间、控制台
(board.c)   依赖：MPFS HAL (CLINT 寄存器地址、堆边界符号)
           边界：板级代码只初始化，不参与运行时调度

MPFS HAL    职责：封装 PolarFire SoC 硬件寄存器操作
(mss_hal.h) 依赖：SoC 内存映射（CLINT @ 0x02000000, PLIC @ 0x0C000000）
           边界：HAL 是平台层，与 RTOS 无关

硬件层      职责：执行指令、触发中断、存取外设
(RISC-V CPU  依赖：时钟、电源、DDR 初始化已由启动代码完成
 + CLINT)   边界：无
```

---

---

## 第二点五部分：平台层存量驱动复用方案

### 概述

PolarFire SoC 的 MPFS HAL 层和启动代码已经提供了 RT-Thread 所需的大部分硬件基础设施。
移植者**不需要**自己设置 `mtvec`、编写 trap 处理器或操作 CLINT 寄存器，
平台层已经内置了这些功能的完整实现。RT-Thread 只需要"钩入"已有的链路即可。

**核心原则：优先复用 HAL，避免造轮子。**

```
RT-Thread 需提供的              平台层已提供的（可直接复用）
───────────────────────────      ───────────────────────────────────────
✔ rt_tick_increase() 调用点     ✔ SysTick_Config() → MTIMECMP + MTIE + MIE
✔ 堆初始化（边界地址）          ✔ mss_entry.S → 启动向量 + mtvec
✔ rt_kprintf 底层输出绑定       ✔ trap_vector → 所有机器模式 trap 分发 + 保存/恢复上下文
✔ 弱函数覆盖                    ✔ handle_m_timer_interrupt → 定时器中断自动重编程
                                ✔ UART / GPIO 裸机驱动（已编译）
                                ✔ LD 链接脚本（内存布局 + 堆段）
                                ✔ PLIC 中断控制器初始化
```

### 2.3 Tick 定时器：直接复用 HAL 链路

#### 平台已有的完整 Tick 链路

```
┌───────────────────────────────────────────────────────────────────┐
│ SysTick_Config()            (mss_clint.c:48)                       │
│  读取 HART1_TICK_RATE_MS = 5 → 5ms per tick = 200Hz              │
│  计算 g_systick_increment[hart_id]                                  │
│  写 CLINT->MTIMECMP[hart_id] 首次超时值                             │
│  开 mie.MTIE                                                       │
│  开 mstatus.MIE (__enable_irq)                                     │
└───────────────────────┬───────────────────────────────────────────┘
                        │ 定时器到期
                        ▼
┌───────────────────────────────────────────────────────────────────┐
│ handle_m_timer_interrupt()   (mss_clint.c:92)                     │
│                                                                    │
│  STAGE 1: clear_csr(mie, MIP_MTIP)     ← 关 MTIE，防重入          │
│  STAGE 2: U54_1_sysTick_IRQHandler()   ← ⚠️ 弱函数，RT-Thread 需填入 │
│  STAGE 3: MTIMECMP = MTIME + g_systick_increment  ← 自动重编程     │
│  STAGE 4: set_csr(mie, MIP_MTIP)       ← 开 MTIE                 │
└───────────────────────────────────────────────────────────────────┘
```

**关键发现**：`handle_m_timer_interrupt()` 的 STAGE 3 和 STAGE 4 已经自动完成了
MTIMECMP 重编程和 MTIE 开关。移植者**只需要在 STAGE 2 的弱函数中填入
`rt_tick_increase()`**，不需要自行操作任何寄存器。

#### Tick 间隔配置与硬件时钟源

```
时钟链：
  MSS_RTC_TOGGLE_CLK (通常是 1MHz) ──→ CLINT MTimer 递增计数器
       │
       └── SysTick_Config() 中的计算逻辑：
           g_systick_increment = (MSS_RTC_TOGGLE_CLK / 1000) × tick_rate_ms

           · MSS_RTC_TOGGLE_CLK = 1,000,000 (1 MHz)
           · HART1_TICK_RATE_MS = 5 (来自 mss_sw_config.h)
           → g_systick_increment = (1,000,000 / 1000) × 5 = 5,000

           · 实际 Tick 频率 = 1,000,000 / 5,000 = 200 Hz = 5ms per tick
```

如果需要把 RT-Thread 的 Tick 从 200Hz(5ms) 改为 1000Hz(1ms)，
只需要在 `mss_sw_config.h` 中将 `HART1_TICK_RATE_MS` 从 `5` 改为 `1`：

```c
// src/boards/icicle-kit-es/platform_config/mpfs_hal_config/mss_sw_config.h
#define HART1_TICK_RATE_MS  1UL    // 5 → 1：从 200Hz 变为 1000Hz
```

#### 移植者需要做的事

```c
// board.c — 完全复用 HAL Tick 链路的正确实现

#include <rtthread.h>
#include "mpfs_hal/mss_hal.h"       // ← 获取 CLINT 寄存器定义 + mie 宏

void rt_hw_board_init(void)
{
    /* ── Tick 初始化：用 HAL 的 SysTick_Config ── */
    // 注意：SysTick_Config 内含 __enable_irq()，如果在 board_init 中调用
    // 会在调度器就绪前打开全局中断。解决方案：调完立即关中断。
    SysTick_Config();                // 设 MTIMECMP + 开 MTIE + 开 MIE
    __disable_irq();                 // 立即关 MIE，mret 再开
    
    /* ── 堆初始化 ── */
    extern unsigned char __bss_end;
    extern unsigned char _end;
    rt_system_heap_init(&__bss_end, (void*)((unsigned long)&_end + 512*1024));
    
    /* ── 组件初始化 ── */
    rt_components_board_init();
}

// ⭐ 这是整个 RT-Thread 移植中"含金量"最高的几行代码
//    它利用了 HAL 从 mtvec → trap_vector → handle_m_timer_interrupt
//    → U54_1_sysTick_IRQHandler 的完整链路
void U54_1_sysTick_IRQHandler(void)
{
    rt_tick_increase();
    // handle_m_timer_interrupt 自动后续：
    //   MTIMECMP = MTIME + g_systick_increment  ✓
    //   set_csr(mie, MIP_MTIP)                   ✓
}
```

#### 对比方案

| 方案 | 代码量 | 复用度 | 风险 |
|---|---|---|---|
| **复用 HAL**（推荐） | board.c ~10 行 | 完全复用 SysTick_Config + handle_m_timer_interrupt | 最低 |
| **手动操作寄存器**（当前 board.c） | 自己写 timecmp + 开 MTIE | 不复用，且 rt_hw_tick_handler 无人调用 | 与 HAL 冲突 |
| **全部接管**（修改 mtvec） | 自己实现全套 trap 链 | 零复用 | 最高，不推荐 |

---

### 2.4 Trap/异常处理：完全由平台层承担

#### 现有链路

```
机器模式异常/中断触发
  ↓
mtvec（mss_entry.S:37 设置）→ trap_vector（mss_utils.S）
  ↓ 保存全部 32 个寄存器上下文
trap_from_machine_mode()（mss_mtrap.c）
  ↓ 解码 mcause
分支：
  ├─ IRQ_M_TIMER（定时器中断） → handle_m_timer_interrupt()
  ├─ IRQ_M_SOFT（软件中断）   → handle_m_soft_interrupt()
  ├─ IRQ_M_EXT（外部中断）    → handle_m_ext_interrupt()
  ├─ 非法指令陷阱             → illegal_insn_trap()
  ├─ 地址未对齐               → misaligned_store/load_trap()
  ├─ PMP 违规                 → pmp_trap()
  └─ 其他                     → bad_trap() → while(1)
```

**移植者不需要做任何事**。`mtvec` 已在 `mss_entry.S` 中设置指向 `trap_vector`，
`trap_vector` 会在汇编层保存/恢复所有寄存器上下文，
然后 C 层的 `trap_from_machine_mode()` 按 `mcause` 分发。
定时器中断会正确路由到 `handle_m_timer_interrupt()` → `U54_1_sysTick_IRQHandler()`。

---

### 2.5 UART 控制台：直接调用 MSS UART 裸机驱动

#### 现有状态

MSS UART 驱动已经在此项目的 Makefile 中**编译启用**：

```makefile
# src/platform/Makefile（已存在）
SRCS += src/platform/drivers/mss/mss_mmuart/mss_uart.c
INCLUDES += -Isrc/platform/drivers/mss/mss_mmuart
```

#### 移植者需要做的事

RT-Thread 的 `rt_kprintf()` 底层通过两种方式输出字符：

1. **方案 A（推荐）**：修改 `rt_hw_console_output()` — RT-Thread 预留的底层输出钩子
2. **方案 B**：将 `_write()` newlib 存根与 UART 绑定

```c
// board.c — 方案 A：绑定 RT-Thread 控制台到 MSS UART

#include "drivers/mss/mss_mmuart/mss_uart.h"  // ← 直接引用 HAL 的头文件

// RT-Thread 的 rt_kprintf 底层会调用此函数
void rt_hw_console_output(const char *str)
{
    // 直接使用平台已编译的 MSS UART 驱动
    // 选择一个可用的 UART 实例（通常用 MMUART1 或 MMUART3）
    MSS_UART_polled_tx_string(&g_mss_uart1_lo, str);
}

// 或者方案 A2：注册为设备
void rt_hw_board_init(void)
{
    // ...
    // 使用 RT-Thread 的设备注册机制
    // rt_console_set_device("uart1");
}
```

**注意**：此方案不涉及任何 RTOS 适配——MSS UART 驱动是纯裸机驱动，不依赖 FreeRTOS 或 RT-Thread，
可以直接在任何上下文中调用。

---

### 2.6 内存与堆：复用链接脚本

#### 现有链接脚本

链接脚本（`mpfs-ddr-loaded-by-boot-loader-remote.ld`）已在 `boards/icicle-kit-es/` 中，
定义了完整的 DDR 区域和 `.FreeRTOSheap` 段。

#### 移植者需要做的事

**最小启动阶段**：不需要修改链接脚本。RT-Thread 的堆可以直接指向 `.FreeRTOSheap` 段
的起始地址（或更简单地使用 `__bss_end` 之后的内存）：

```c
// 方案：使用 _end 符号之后的可用 DDR 空间
extern unsigned char _end;
#define RT_HW_HEAP_BEGIN    ((void*)&__bss_end)
#define RT_HW_HEAP_END      ((void*)((unsigned long)&_end + 512 * 1024))
```

**长期维护**：在链接脚本中新增 `.rtthread.heap` 段，与 `.FreeRTOSheap` 隔离：

```ld
/* 链接脚本新增段 */
.rtthread_heap : ALIGN(0x10)
{
    __rtthread_heap_start = .;
    *(.rtthread.heap)
    . = ALIGN(0x10);
    __rtthread_heap_end = .;
} > ddr_cached_32bit
```

---

### 2.7 系统启动序列：完全由平台层承担

#### 现有链路

```
mss_entry.S（汇编）
  → 设置 mtvec = trap_vector
  → 关 mstatus.MIE + mie（所有中断关闭）
  → 根据 hart_id 设栈指针
  → 清零 BSS

system_startup.c（C）
  → init_memory()：复制 .text/.data
  → main_first_hart() / main_other_hart()
    → e51() / u54_1() / u54_2() / u54_3() / u54_4()

用户应用入口（例如 u54_1.c）
  → 清除软件中断 + 等待 E51 唤醒（非单核场景）
  → start_demo()
    → rt_hw_board_init()    ← RT-Thread 板级初始化（Tick + 堆 + 控制台）
    → 创建线程
    → rt_system_scheduler_start()  ← 启动调度器
```

**移植者不需要改启动代码**。`mss_entry.S` 和 `system_startup.c` 由 Microchip 官方提供，
已经为多核做好了通用初始化。RT-Thread 的入口是在**用户应用层**（`u54_1.c`）插入启动调用。

---

### 2.8 可复用性总表

| 功能模块 | 平台层文件 | RTOS 依赖 | 复用方式 | 是否需要改 |
|---|---|---|---|---|
| **Tick 定时器** | `mss_clint.c/h` | 无 | `U54_1_sysTick_IRQHandler` 弱函数 | 需覆盖弱函数 |
| **mtvec / Trap 链** | `mss_entry.S` + `mss_utils.S` + `mss_mtrap.c` | 无 | 已自动指向 trap_vector | 不需要 |
| **UART 控制台** | `mss_uart.c/h` | 无 | 直接调 MSS_UART_polled_tx_string | 不需要 |
| **GPIO** | `mss_gpio.c/h` | 无 | 直接调 MSS_GPIO_* | 不需要 |
| **PLIC** | `mss_plic.c/h` | 无 | PLIC_init 已在启动中调用 | 不需要 |
| **PMP/MPU** | `mss_pmp.c` / `mss_mpu.c` | 无 | 已自动配置 | 不需要 |
| **堆内存** | 链接脚本 `*.ld` | 无 | 提供 `__bss_end` / `_end` 符号 | 建议加 .rtthread.heap 段 |
| **系统启动** | `system_startup.c` | 无 | 自动调用到用户入口 | 不需要 |
| **Newlib 存根** | `newlib_stubs.c` | 无 | `_write()` / `_sbrk()` 已实现 | 若 _write 输出到 UART 需绑定 |

---

### 2.9 总结：移植者真正需要写的代码

将上述所有复用方案汇总后，RT-Thread 单核移植的最小代码量：

```c
// board.c — 真正的全部代码量 ~25 行

#include <rtthread.h>
#include "mpfs_hal/mss_hal.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"

void rt_hw_board_init(void)
{
    SysTick_Config();                 // 复用 HAL Tick 配置
    __disable_irq();                  // 折衷 SysTick_Config 的 __enable_irq()
    
    extern unsigned char __bss_end;
    extern unsigned char _end;
    rt_system_heap_init(&__bss_end, (void*)((unsigned long)&_end + 512*1024));
    
    rt_components_board_init();
}

void U54_1_sysTick_IRQHandler(void)
{
    rt_tick_increase();               // 利用 HAL 的定时器中断链路
}

void rt_hw_console_output(const char *str)
{
    MSS_UART_polled_tx_string(&g_mss_uart1_lo, str);  // 复用 MSS UART 驱动
}
```

**移植的核心不是"实现硬件接口"，而是"找到正确的钩子点，把 RT-Thread 挂上已有的 HAL 链路"。**


## 第三部分：逐模块移植细节与适配原因

### Step 1：确认 RT-Thread Nano 源码结构

**为什么要做？**
RT-Thread Nano 的源码包含了内核实现（src/）、API 定义（include/）、CPU 移植预备代码（libcpu/）。
必须确认这些文件存在且路径正确，否则编译器找不到头文件或源文件。

**适配方法**：
```
rt-thread/                         # git subtree 引入的 Nano 源码根
├── src/clock.c                    # 系统时钟管理：rt_tick_increase()
├── src/ipc.c                      # 进程间通信：信号量/互斥体/事件/邮箱/队列
├── src/scheduler.c                # 调度器：rt_schedule()
├── src/thread.c                   # 线程管理：rt_thread_create/startup
├── src/timer.c                    # 软件定时器
├── src/mem.c                      # 小内存管理：rt_malloc/rt_free
├── src/kservice.c                 # 内核服务：rt_kprintf
├── src/irq.c                      # 中断管理：rt_interrupt_enter/leave
├── src/idle.c                     # 空闲线程
├── src/object.c                   # 对象管理
├── src/components.c               # 组件初始化
├── src/cpu.c                      # CPU 空闲指令
├── include/rtthread.h             # 主头文件（应用 include 此文件）
├── include/rtdef.h                # 核心类型定义（#include 树基础）
├── include/rthw.h                 # 硬件接口声明
├── libcpu/risc-v/common/
│   ├── context_gcc.S              # 上下文切换汇编（接口组 A 的实现）
│   ├── cpuport.c                  # CPU 端口 C 实现（rt_hw_stack_init）
│   └── riscv-plic.h               # PLIC 操作宏
```

**当前状态**：✅ 已完成（git subtree 已验证）

### Step 2：实现上下文切换（接口组 A 核心）

**为什么要做？**
没有上下文切换，RT-Thread 只能运行一个线程。调度器没有"交换正在执行的任务"的能力，
任何多任务场景（哪怕只是两个线程交替打印）都无法实现。

**后果**：
- `rt_schedule()` 调用 `rt_hw_context_switch()` 时，函数不存在 → 链接报错
- 即使绕过报错，强行执行 → 寄存器未保存/恢复 → 栈损坏 → 系统崩溃

**适配方法**：
当前项目已通过 git subtree 引入 `libcpu/risc-v/common/context_gcc.S`，其中已实现三个关键函数：

| 函数 | 作用 | 被谁调用 | 调用时机 |
|---|---|---|---|
| `rt_hw_context_switch(from, to)` | 保存当前线程 CPU 寄存器，加载目标线程寄存器 | `rt_schedule()` | 每次调度 |
| `rt_hw_context_switch_to(to)` | 直接加载目标线程寄存器（当前无上下文要保存） | `rt_system_scheduler_start()` | 启动第一个线程 |
| `rt_hw_context_switch_interrupt(from, to)` | 中断中的轻量切换 | 中断退出路径 | ISR 产生更高优先级线程时 |

**实现细节**：
```
context_gcc.S 中的上下文保存布局（34 × 8 = 272 字节栈空间）：
偏移    寄存器    说明
0       ra(x1)    返回地址
8×1     sp(x2)    栈指针
8×2     gp(x3)    全局指针（保留，不切换）
8×3     tp(x4)    线程指针
8×4~30  t0~t6,    a0~a7, s0~s11  通用寄存器
8×31    mepc      PC（mret 目标）
8×32    mstatus   机器状态（含 MIE 使能位）
8×33    （未使用，保留对齐）
```

**当前状态**：✅ 文件已引入，路径已在 Makefile 中配置

### Step 3：实现栈初始化（接口组 A 的第四个函数）

**为什么要做？**
当一个新线程被创建时（`rt_thread_create` 或 `rt_thread_init`），内核需要初始化它的栈，
使栈帧看起来像是"刚从上下文切换中保存的"状态，这样调度器第一次切换到它时就能正常运行。

**后果**：
- `rt_hw_stack_init` 未实现 → 新线程无法创建，`rt_thread_create` 失败
- 实现错误（如未正确设置 mepc）→ 第一次切换时 mret 到错误地址

**适配方法**：
已通过 `libcpu/risc-v/common/cpuport.c` 实现：

```c
rt_uint8_t *rt_hw_stack_init(void *tentry,      // 线程入口函数
                              void *parameter,   // 传入参数
                              rt_uint8_t *stack_addr, // 栈顶地址
                              void *texit)       // 线程退出函数
{
    // 1. 对齐到 8 字节边界
    // 2. 预留 34 × 8 = 272 字节栈帧
    // 3. 设置 ra = texit（线程返回时跳转到退出函数）
    // 4. 设置 a0(x10) = parameter（第一个参数寄存器）
    // 5. 设置 mepc = tentry（mret 后从此处开始执行）
    // 6. 设置 mstatus = 0x1880（机器模式 + 中断使能）
    // 7. 返回新的栈指针
}
```

**当前状态**：✅ 文件已引入

### Step 4：实现中断控制（接口组 B）

**为什么要做？**
临界区是 RTOS 正确运行的基础。`rt_hw_interrupt_disable()` 和 `rt_hw_interrupt_enable()`
被内核中几乎所有操作使用：插入队列、移除节点、分配内存、IPC 操作……

**后果**：
- 未实现 → 几乎所有的内核操作都在无保护状态下进行
- 具体表现：如果在定时器中断中调度器修改了就绪链表，而任务也在修改同一链表 → **链表损坏** → 死循环或非法地址访问

**适配方法**：
RISC-V 上通过操作 `mstatus` CSR 的 `MIE`（Machine Interrupt Enable，bit 3）位实现：

| 操作 | 汇编 | 说明 |
|---|---|---|
| 关中断 | `csrci mstatus, 8` | 清除 mstatus 的 bit 3（MIE） |
| 开中断 | `csrs mstatus, 8` | 设置 mstatus 的 bit 3（MIE） |
| 读当前状态 | `csrr rd, mstatus` | 读取完整的 mstatus 用于恢复 |

关键逻辑：关中断时必须返回**关中断之前**的 mstatus 值，而不仅仅是"存个数"，
因为中断可能是嵌套调用的。

```c
rt_base_t rt_hw_interrupt_disable(void) {
    rt_base_t level;
    __asm volatile("csrr %0, mstatus" : "=r"(level));
    __asm volatile("csrc mstatus, 8");  // 关中断
    return level;  // 返回旧的中断状态
}

void rt_hw_interrupt_enable(rt_base_t level) {
    __asm volatile("csrw mstatus, %0" : : "r"(level));  // 恢复旧状态
}
```

**当前状态**：✅ `context_gcc.S` 已包含这两个函数的实现

### Step 5：实现系统 Tick 定时器（接口组 B 核心）

**为什么要做？**
RT-Thread 需要周期性中断来驱动时间相关功能：
- **时间片轮转**：每个线程只能运行一个时间片（默认 10 个 Tick），超时后强制切换到下一个同级优先级线程
- **线程延时**：`rt_thread_mdelay(ms)` / `rt_thread_sleep(tick)` 依赖 Tick 计数
- **软件定时器**：`RT_USING_TIMER` 使能时，定时器回调依赖 Tick 中断

**后果**：
- 无 Tick → 线程调度变为**协作式**（只有线程主动调用 `rt_schedule()` 才切换）
- `rt_thread_mdelay()` 无法实现 — 线程永不超时
- 软件定时器永不触发

**适配方法**：
在 PolarFire SoC 上使用 **CLINT MTimer** 作为系统 Tick 源：

| 寄存器 | 地址 | 类型 | 作用 |
|---|---|---|---|
| mtime | 0x0200BFF8 | 只读 | 64 位单调计数器，1MHz 递增 |
| mtimecmp | 0x02004000 + hartid × 8 | 读写 | 当 mtime 达到此值时触发中断 |

**时序关系**：
```
configTICK_CLOCK_HZ = 1 MHz（CLINT 时钟）
configTICK_RATE_HZ  = 1000 Hz（期望的系统 Tick 频率）
中断间隔 = 1 MHz / 1000 Hz = 1000 个 CLINT 计数

实现逻辑：
  timecmp = mtime + 1000     → 1ms 后触发中断
  中断处理程序中：
    1. timecmp = mtime + 1000  （设置下一次中断）
    2. rt_tick_increase()      （通知内核一个 Tick 已过）
```

**定时器 ISR 入口问题**：
RISC-V 中 MTimer 中断属于**机器模式异常**，中断向量地址由 `mtvec` CSR 指定。
在 FreeRTOS 移植中，定时器中断入口在 `portasm.S` 中直接定义为 `TIMER_CMP_INT:`
然后调用 `vPortSysTickHandler()`。

RT-Thread 的 `libcpu/risc-v/common/context_gcc.S` **不包含**定时器中断入口！
这是因为不同的 RISC-V SoC 的中断向量管理方式不同（有些使用 mtvec 直接分发，
有些使用 PLIC + 外部中断，有些使用 CLINT + 自定义处理）。
因此**移植者需要自己实现定时器中断入口**。

**适配方法**：
方案一（推荐）：在 `board.c` 内联实现
```c
// 在 board.c 中直接定义机器模式定时器中断处理
void __attribute__((interrupt("machine"))) rt_hw_timer_isr(void)
{
    // 1. 清除中断源（更新 timecmp 到下一次）
    *timecmp = *mtime + (CLINT_FREQ / RT_TICK_PER_SECOND);
    
    // 2. 通知内核 Tick 增加
    rt_tick_increase();
    
    // 3. 检查是否需要调度
    rt_schedule();
}
```

方案二：在 `mss_entry.S` 的 trap 向量表中注册 `rt_hw_timer_isr`。

**当前状态**：⚠️ 定时器 ISR 入口需要自行编写（已占位）

### Step 6：实现板级初始化（接口组 C）

**为什么要做？**
板级初始化是 RT-Thread 启动流程的第一个 C 函数调用。它必须完成三件事：
1. **让 Tick 定时器开始工作** → OS 才有时间概念
2. **告诉内核堆内存的范围** → `rt_malloc()` / `rt_free()` 才能工作
3. **注册控制台输出** → `rt_kprintf()` 才能输出到 UART

**后果**：
- 不初始化 Tick → 系统无时间片（如 Step 5 所述）
- 不初始化堆 → `rt_malloc()` 返回 NULL → 线程创建/消息队列/邮箱分配失败
- 不初始化控制台 → `rt_kprintf()` 无输出

**适配方法**：
```c
void rt_hw_board_init(void)
{
    /* === 1. 初始化系统 Tick 定时器 === */
    *timecmp = *mtime + (1000000 / 1000);  // CLINT 1MHz / 1000Hz = 1000
    __asm volatile("csrs mie, %0" :: "r"(0x80));  // 使能 MTIE
    
    /* === 2. 初始化系统堆 === */
    // 堆空间：从 __bss_end 到 _end 之后 512KB
    extern unsigned char __bss_end;
    extern unsigned char _end;
    rt_system_heap_init(&__bss_end, (void*)((unsigned long)&_end + 512*1024));
    
    /* === 3. 初始化控制台 === */
    // rt_kprintf 底层通过 _write() 输出
    // _write() 需要关联到 UART（在 newlib_stubs.c 或 board.c 中映射）
    
    /* === 4. 组件初始化 === */
    rt_components_board_init();
}
```

**堆边界的选择**（重要）：
当前链接脚本中已有 `.heap : { *(.FreeRTOSheap) }` 段。
RT-Thread 可以：
- **方案 A（推荐）**：复用 `.FreeRTOSheap` 段，让 RT-Thread 从同一区域分配
- **方案 B**：新增 `.rtthread.heap` 段（需修改链接脚本）

最小启动阶段推荐方案 A，避免修改链接脚本。

**当前状态**：⚠️ `board.c` 文件已创建，但实际连接新lib的 `_write()` 和定时器 ISR 待完善

### Step 7：实现控制台输出

**为什么要做？**
控制台是 RT-Thread 调试的生命线。没有它，既看不到内核启动信息，也无法用 `rt_kprintf()` 调试。

**后果**：
- `rt_kprintf()` 写入字符后无输出 → 无法确认系统是否启动成功
- 无法输出调试信息 → 遇到崩溃（如异常地址）时无法定位

**适配方法**：
RT-Thread 的 `rt_kprintf()` 最终调用 `rt_hw_console_output(str)`。
该函数需要在 `board.c` 中实现，或通过 `rt_console_set_device()` 注册设备。

在最小启动中，最简单的方式是直接在 `kservice.c` 或 `board.c` 中重定向：

```c
// 方案：直接在 board.c 中实现控制台输出
void rt_hw_console_output(const char *str)
{
    extern void MSS_UART_polled_tx_string(void* inst, const char* str);
    extern uintptr_t g_mss_uart1_lo;
    
    while (*str) {
        // 等待 UART 发送完成
        // 发送字符
        str++;
    }
}
```

**当前状态**：⚠️ 需要移植者根据使用的 UART 实例（MMUART0/1/3）绑定输出

### Step 8：最小测试应用 — 让两个线程交替打印

**为什么要做？**
这是验证整个移植是否成功的**唯一标准**。能创建两个线程并让它们交替输出，
说明调度器、上下文切换、定时器中断、控制台全部正常工作。

**测试方法**：

```c
#include <rtthread.h>

static void thread1_entry(void *parameter)
{
    int count = 0;
    while (1) {
        rt_kprintf("Thread 1 running... count=%d\n", count++);
        rt_thread_mdelay(1000);   // 延时 1 秒
    }
}

static void thread2_entry(void *parameter)
{
    while (1) {
        rt_kprintf("Thread 2: tick=%d\n", rt_tick_get());
        rt_thread_mdelay(500);    // 延时 0.5 秒
    }
}

void start_rtt_demo(void)
{
    // 第一步：初始化板级硬件（Tick、堆、控制台）
    // 注意：不同于 FreeRTOS 在第一个任务中调 vPortSetupTimer()
    // RT-Thread 要求在 board_init 阶段完成 Tick 配置
    rt_hw_board_init();
    
    // 第二步：初始化调度器
    rt_system_scheduler_init();
    
    // 第三步：初始化定时器
    rt_system_timer_init();
    
    // 第四步：创建线程
    rt_thread_t tid1 = rt_thread_create("t1", thread1_entry, RT_NULL,
                                        2048, 10, 10);
    if (tid1) rt_thread_startup(tid1);
    
    rt_thread_t tid2 = rt_thread_create("t2", thread2_entry, RT_NULL,
                                        2048, 15, 10);
    if (tid2) rt_thread_startup(tid2);
    
    // 第五步：启动调度器（不会返回）
    rt_system_scheduler_start();
}
```

**FreeRTOS 启动 vs RT-Thread 启动对照**：

| 时序 | FreeRTOS | RT-Thread Nano |
|---|---|---|
| 定时器配置点 | 第一个任务中手动调 `vPortSetupTimer()` | `rt_hw_board_init()` 中完成 |
| 调度器初始化的位置 | `vTaskStartScheduler()` 内部 | **需显式调用** `rt_system_scheduler_init()` |
| 创建任务即加入调度 | 是，`xTaskCreate()` 后任务即就绪 | 否，还需调 `rt_thread_startup()` |
| 启动调度器函数 | `vTaskStartScheduler()` | `rt_system_scheduler_start()` |
| 毫秒延时 | `vTaskDelay(pdMS_TO_TICKS(ms))` | `rt_thread_mdelay(ms)` |
| 获取 Tick | `xTaskGetTickCount()` | `rt_tick_get()` |

**关键差异**：
- FreeRTOS 的 `vTaskStartScheduler()` 在内部初始化定时器和调度器，一步到位
- RT-Thread 的启动流程是**分步的**：`board_init` → `scheduler_init` → `timer_init` → `thread_create` → `scheduler_start`
- 移植者必须按顺序手动调用这些初始化函数

**当前状态**：⚠️ 需要移植者编写并部署测试应用

---

## 第四部分：完整版本移植目标

### 4.1 完整版本需新增的模块

在最小启动基础上，完整版本需要额外实现：

```
┌──────────────────────────────────────────────────┐
│              完整 RT-Thread Nano 应用              │
│    RPMsg ping-pong / echo / console 示例         │
├────────────┬─────────────────────────────────────┤
│ 最小启动    │ 额外适配                             │
│ (已验证)    │                                     │
│            │ rpmsg_env_rtthread.c                 │
│ 调度器 ✅   │   ├── env_create_mutex()            │
│ 线程管理 ✅  │   ├── env_create_queue()            │
│ 信号量 ✅   │   ├── env_sleep_msec()              │
│ 互斥体 ✅   │   └── env_allocate_memory()          │
│ 定时器 ✅   │                                     │
│ 内存管理 ✅  │ 链接脚本适配                         │
│ 控制台输出 ✅ │   ├── .rtthread.heap 段             │
│            │   └── stack 尺寸调整                  │
└────────────┴─────────────────────────────────────┘
```

### 4.2 RPMsg-Lite 环境适配层

**为什么需要？**
RPMsg-Lite 是一个**跨 RTOS** 的核间通信库。它通过 `rpmsg_env.h` 定义了一组抽象接口，
需要在不同 RTOS 上分别实现。FreeRTOS 版本是 `rpmsg_env_freertos.c`，
RT-Thread 版本需要实现 `rpmsg_env_rtthread.c`。

**关键接口映射**：

| RPMsg 接口 | FreeRTOS 实现 | RT-Thread 实现 | 不实现的后果 |
|---|---|---|---|
| `env_create_mutex` | `xSemaphoreCreateCounting` | `rt_mutex_create` | RPMsg 内部保护失败，并发发送接收数据错乱 |
| `env_create_sync_lock` | `xSemaphoreCreateBinary` | `rt_sem_create` | 无法同步中断上下文和任务上下文 |
| `env_create_queue` | `xQueueCreate` | `rt_mq_create` | 无法接收 RPMsg 数据 |
| `env_sleep_msec` | `vTaskDelay` | `rt_thread_mdelay` | RPMsg 等待超时无法实现 |
| `env_allocate_memory` | `pvPortMalloc` | `rt_malloc` | RPMsg 内存分配失败 |

**当前状态**：❌ `rpmsg_env_rtthread.c` 已创建为空占位，内容待实现

### 4.3 关于链接脚本的说明

最小启动阶段不需要修改链接脚本 — RT-Thread 可直接复用 FreeRTOS 的 `.FreeRTOSheap` 段。
但长期维护建议新增独立段以避免两个 RTOS 的堆区域冲突。

---

## 第五部分：调试与验证

### 5.1 逐步验证检查清单

```
□ Step 1: rt-thread/src/clock.c 等源文件存在且非空
□ Step 2: Makefile 中 SRCS 路径指向正确文件
□ Step 3: context_gcc.S 包含 rt_hw_context_switch / rt_hw_interrupt_disable
□ Step 4: cpuport.c 包含 rt_hw_stack_init
□ Step 5: board.c 实现了 Tick 初始化 + 堆初始化 + 控制台
□ Step 6: 定时器 ISR 与 CLINT 中断向量关联
□ Step 7: make BOARD=icicle-kit-es CONFIG_WITH_ARCH=1 编译通过
□ Step 8: objdump 可看到 rt_hw_context_switch 等符号
□ Step 9: HSS 加载后串口输出 RT-Thread 启动信息
□ Step 10: 两个线程交替执行，rt_tick_get() 每次增加 1
```

### 5.2 常见问题与排除

| 现象 | 最可能原因 | 验证方法 |
|---|---|---|
| 编译报错 `undefined reference to rt_hw_interrupt_disable` | context_gcc.S 路径错误 | 确认 Makefile 中 ASM_SRCS 正确 |
| 编译报错 `undefined reference to __heap_start` | 链接脚本未定义堆符号 | 使用 `&__bss_end` 替代 |
| 串口无输出 | UART 未初始化或控制台未绑定 | 检查 `_write()` 实现是否发送到 UART |
| 系统卡死，无任何输出 | 定时器 ISR 未正确安装 | 检查 mtvec 是否指向 ISR |
| 只有一个线程运行 | 上下文切换错误 | 检查栈帧布局与 cpuport.c 是否一致 |
| `mret` 后崩溃 | mepc 设置为无效地址 | 检查 `rt_hw_stack_init` 中 mepc 设置 |

---

## 附录：关键硬件地址

| 外设 | 基地址 | 说明 |
|---|---|---|
| CLINT | 0x02000000 | 核间中断 + 定时器 |
| CLINT MTIME | 0x0200BFF8 | 全局 64 位单调递增计时器 |
| CLINT MTIMECMP | 0x02004000 + hartid × 8 | 定时器比较值寄存器 |
| PLIC | 0x0C000000 | 平台级中断控制器 |
| MSS UART0 | 0x10000000 | 可用作控制台 |
| MSS UART1 | 0x10010000 | 可用作控制台 |
| MSS UART3 | 0x10030000 | 可用作控制台 |
| DDR (32位) | 0x91C00000 | 1MB（当前项目使用的 DDR 区域） |
| CLINT 时钟 | 1 MHz | MTimer 递增频率 |
| CPU 时钟 | 100 MHz | U54 核心频率 |
