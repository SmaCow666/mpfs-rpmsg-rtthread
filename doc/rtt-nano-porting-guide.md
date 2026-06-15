# RT-Thread Nano 移植指南 — PolarFire SoC Icicle Kit

## 文档概述

本文档基于当前项目（v0.1 + v0.2）中已完成的 RT-Thread Nano 移植骨架，
提供从骨架到可运行系统的详细移植逻辑和实施路径。

**当前状态**：
- RT-Thread Nano 源码已通过 `git subtree` 从 `https://github.com/RT-Thread/rtthread-nano.git` 引入
- 移植骨架（board、portable、Makefile）已完成搭建
- 构建系统已集成 `-DUSING_RTTHREAD` 编译器标志

**文档目标**：
- 每步描述移植的核心需求与目标
- 定义**最小启动移植目标**（minimal boot target）和**完整版本移植目标**（full version target）
- 明确从当前骨架到可运行系统的完整路径

---

## 第一部分：移植目标与里程碑

### 1.1 最小启动移植目标（Minimal Boot Target）

| 条件 | 描述 |
|---|---|
| 目标 | 在单一 U54 核上启动 RT-Thread Nano 内核，创建两个线程交替打印 |
| 依赖硬件 | CLINT MTimer（Tick）、PLIC（可选）、UART（控制台输出） |
| 内核功能 | 线程创建 / 销毁、调度器、信号量、定时器、内存管理 |
| 排除功能 | AMP 核间通信（RPMsg）、Remoteproc、多核协同 |
| 验证方式 | 串口终端观察到两个线程交替输出 "Hello from thread 1/2" |
| 预期代码量 | ~200 行新增板级适配代码 |

### 1.2 完整版本移植目标（Full Version Target）

| 条件 | 描述 |
|---|---|
| 目标 | 在 AMP 配置中运行 RT-Thread Nano，支持 RPMsg-Lite 核间通信 |
| 依赖硬件 | CLINT + PLIC + IHC + UART（全部） |
| 内核功能 | 完整 Nano 功能 + 设备框架 + FinSH 控制台（可选） |
| 通信能力 | RPMsg-Lite 在 RT-Thread 环境下的 master/remote 双模式 |
| 多核协同 | E51 管理 + U54 计算核的 AMP 启动序列 |
| 验证方式 | 运行 ping-pong / console / echo 示例（同现有 FreeRTOS 演示） |
| 预期代码量 | ~1000 行新增代码（含 rpmsg_env 适配） |

### 1.3 移植路线图

```
阶段 1：最小启动（当前骨架 → 可运行内核）
  ├─ Step 1: 确认 rt-thread nano 源码结构           ← 已完成
  ├─ Step 2: 确认构建系统集成                       ← 已完成
  ├─ Step 3: 补齐 cpuport.c 和 context_gcc.S        ← 已完成（common 目录）
  ├─ Step 4: 实现 board.c（Tick + Heap + Console）    → 待完善
  ├─ Step 5: 搭建最小测试应用                        → 待实现
  └─ Step 6: 编译并调试                              → 待实现

阶段 2：AMP 多核适配
  ├─ Step 7: 链接脚本适配（.RTThread.heap 段）
  ├─ Step 8: 多核核间同步协议
  └─ Step 9: RPMsg-Lite 通信验证

阶段 3：功能完善
  ├─ Step 10: rpmsg_env_rtthread.c 实现（信号量/队列/互斥体）
  ├─ Step 11: FinSH 控制台（可选）
  └─ Step 12: 性能调优（Tick 精度、上下文切换开销）
```

---

## 第二部分：详细移植步骤

### Step 1：确认 RT-Thread Nano 源码结构

**核心需求**：验证 `rt-thread/` 目录包含正确的 Nano 内核源码，且路径与 Makefile 引用一致。

**当前状态**：已通过 git subtree 引入，目录结构如下：

```
rt-thread/                          # git subtree 根目录
├── src/                            # 内核源码（12+ 个 C 文件）
│   ├── clock.c                     # 时钟管理
│   ├── components.c                # 组件初始化
│   ├── cpu.c                       # CPU 核心管理
│   ├── device.c                    # 设备框架
│   ├── idle.c                      # 空闲线程
│   ├── ipc.c                       # 进程间通信（信号量/互斥体/事件/邮箱/队列）
│   ├── irq.c                       # 中断管理
│   ├── kservice.c                  # 内核服务（字符串/打印）
│   ├── mem.c                       # 内存管理（小内存管理算法）
│   ├── memheap.c                   # 多堆管理
│   ├── mempool.c                   # 内存池
│   ├── object.c                    # 对象管理
│   ├── scheduler.c                 # 调度器
│   ├── signal.c                    # 信号（预留）
│   ├── slab.c                      # SLAB 内存管理
│   ├── thread.c                    # 线程管理
│   └── timer.c                     # 软件定时器
├── include/                        # 内核头文件
│   ├── rtdef.h                     # 核心类型与宏定义
│   ├── rtthread.h                  # 主 API 头文件
│   ├── rthw.h                      # 硬件抽象层接口
│   └── rtdbg.h / rtservice.h      # 调试与工具
├── libcpu/risc-v/common/           # ◄── RISC-V 通用移植层（核心关注）
│   ├── context_gcc.S               # 上下文切换汇编（7824 bytes）
│   ├── cpuport.c                   # CPU 端口 C 实现（6522 bytes）
│   ├── cpuport.h                   # 端口辅助头文件
│   ├── riscv-ops.h                 # RISC-V 操作宏
│   └── riscv-plic.h                # PLIC 中断控制器支持（4819 bytes）
├── libcpu/risc-v/bumblebee/        # BumbleBee 核心专用中断处理
│   └── interrupt_gcc.S
├── bsp/                            # 板级支持包示例
│   ├── _template/                  # 通用模板（board.c / rtconfig.h）
│   └── stm32f407-msh/              # STM32 参考实现
└── components/                     # 可选组件
    └── finsh/                      # FinSH 控制台
```

**关键确认点**：
- [x] `libcpu/risc-v/common/context_gcc.S` 包含 `rt_hw_context_switch()`、`rt_hw_context_switch_to()`、`rt_hw_context_switch_interrupt()` 的汇编实现
- [x] `libcpu/risc-v/common/cpuport.c` 包含 `rt_hw_stack_init()` 实现
- [x] `include/` 目录提供完整的 API 声明

**目标**：确认当前 Makefile 中所有 `SRCS +=` 和 `INCLUDES +=` 路径均指向存在的文件。

---

### Step 2：确认构建系统集成

**核心需求**：验证 Makefile 包含链完整，编译器标志正确，RT-Thread 源文件被正确加入编译。

**当前状态**：已在 `rules.mk` 中添加 `-DUSING_RTTHREAD` 宏；中间件 Makefile 已包含 `src/middleware/RTThread/Makefile`。

**Makefile 集成关键路径**：

```
顶层 Makefile
├── src/application/rules.mk          # -DUSING_RTTHREAD 标志
├── src/application/targets.mk        # 链接规则
└── src/middleware/Makefile
    ├── src/middleware/FreeRTOS/Makefile   # FreeRTOS 源文件
    ├── src/middleware/RTThread/Makefile   # RT-Thread 源文件   ← NEW
    │   ├── rt-thread/src/*.c            # 内核源文件
    │   ├── rt-thread/libcpu/risc-v/common/*.c / *.S  # 移植层
    │   ├── board/board.c                # 板级初始化
    │   └── portable/cpuport.c           # 项目定制扩展
    ├── src/middleware/misc/Makefile
    └── src/middleware/rpmsg/Makefile
```

**编译器标志验证**：

| 标志 | 位置 | 用途 |
|---|---|---|
| -DUSING_RTTHREAD | rules.mk | 标识当前启用 RT-Thread |
| -DUSING_FREERTOS | rules.mk | 保留（FreeRTOS 仍可编译） |
| -march=rv64gc | rules.mk | RISC-V 64 位通用架构 |
| -mabi=lp64d | rules.mk | LP64 数据模型 + 双精度浮点 |
| -mcmodel=medany | rules.mk | 中地址模型（支持 >2GB 寻址） |

**目标**：执行 `make BOARD=icicle-kit-es CONFIG_WITH_ARCH=1 2>&1` 应无编译错误。

---

### Step 3：补齐 CPU 移植层

**核心需求**：RISC-V 通用移植层（`libcpu/risc-v/common/`）已提供完整的上下文切换和栈初始化实现。当前骨架的 `portable/cpuport.c` 和 `board/board.c` 需要与通用层协同工作。

**移植层调用关系**：

```
RT-Thread 调度器
  → rt_hw_context_switch(from, to)          # context_gcc.S
  → rt_hw_context_switch_to(to)             # context_gcc.S
  → rt_hw_stack_init(entry, param, addr, exit) # cpuport.c (common)
     ↓
  ⟳ portSAVE_CONTEXT / portRESTORE_CONTEXT  # context_gcc.S

定时器中断
  → 机器模式定时器中断入口                   # 需自行实现（参考 bumblebee/interrupt_gcc.S）
  → rt_hw_tick_handler()                    # board.c
    → rt_tick_increase()                    # kernel/src/clock.c

临界区
  → rt_hw_interrupt_disable()               # context_gcc.S
  → rt_hw_interrupt_enable(level)           # context_gcc.S
```

**关键差异分析（vs FreeRTOS 移植）**：

| 特性 | FreeRTOS (portasm.S) | RT-Thread (context_gcc.S) |
|---|---|---|
| 上下文保存 | 手动宏（portSAVE_CONTEXT） | 汇编函数（rt_hw_context_switch） |
| 临界区 | 内联汇编宏 | 汇编函数（rt_hw_interrupt_disable/enable） |
| 栈帧布局 | 34 寄存器 + mepc + mscratch | 标准 RISC-V 调用帧（与 cpuport.c 协商） |
| 调度器启动 | xPortStartScheduler → portRESTORE_CONTEXT | rt_system_scheduler_start() → 软件异常或直接切换 |
| 定时器 ISR | TIMER_CMP_INT → portSAVE_EPC | 需自定义机器模式定时器 ISR（参考 bumblebee） |

**需要自行实现的部分**：

```
需编写：
  src/middleware/RTThread/rt-thread/libcpu/risc-v/polarfire/interrupt_gcc.S
  或直接在 board.c 中编写内联汇编定时器 ISR

参考：bumblebee/interrupt_gcc.S 中的 mtimer_handler 模式（3447 bytes）
```

**目标**：
1. `context_gcc.S` 和 `cpuport.c` 的默认实现可在 PolarFire SoC 上运行
2. 定时器 ISR 入口需与 CLINT 中断向量关联
3. 临界区实现通过 mstatus.MIE 位控制（与 FreeRTOS 端口一致）

---

### Step 4：实现板级初始化（board.c）

**核心需求**：`board/board.c` 包含 OS 启动所需的最小硬件初始化：系统 Tick、堆内存、控制台。

**当前 board.c 已有实现**：

| 功能 | 状态 | 说明 |
|---|---|---|
| CLINT MTimer 初始化 | 骨架已实现 | `rt_hw_tick_init()` → `*timecmp = *mtime + interval` |
| 定时器中断使能 | 骨架已实现 | `csrs mie, 0x80`（MTIE 位） |
| 堆初始化 | 骨架已实现 | `rt_system_heap_init(&__heap_start, &__heap_end)` |
| 控制台初始化 | 骨架占位 | 调用者需自行初始化 UART |

**需要完善的部分**：

```c
// board.c 必须补充的完整逻辑

void rt_hw_board_init(void)
{
    /* Step 1: 初始化硬件 Tick 定时器（CLINT MTimer） */
    rt_hw_tick_init();

    /* Step 2: 初始化系统堆 */
    /* 堆边界来自链接脚本符号 __heap_start / __heap_end */
    /* 当前链接脚本中尚无 .RTThread.heap 段，暂用 .bss 后区域 */
#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)&__heap_start, (void *)&__heap_end);
#endif

    /* Step 3: 注册控制台设备 */
#ifdef RT_USING_CONSOLE
    /* 暂用 MPFS HAL 的 UART 直接输出，后续注册为 rt_device */
    /* rt_console_set_device("uart1"); */
#endif

    /* Step 4: 调用板级组件初始化（空函数，用于用户扩展） */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
```

**链接脚本适配（链接脚本段声明）**：

当前链接脚本（`mpfs-ddr-loaded-by-boot-loader-remote.ld`）使用 DDR 区域的 `ddr_cached_32bit`(0x91C00000)。需要新增 RT-Thread 堆段：

```
/* 在链接脚本中新增段声明 */
.rtthread_heap : ALIGN(0x10)
{
    __rtthread_heap_start = .;
    . += RTTHREAD_HEAP_SIZE;    /* 512 KB */
    __rtthread_heap_end = .;
} > ddr_cached_32bit
```

**简化方案**（最小启动阶段）：暂时使用 `.bss` 段之后的内存作为堆，通过 `__bss_end` 符号计算堆边界：

```c
extern unsigned char __bss_end;
extern unsigned char _end;

#define RT_HW_HEAP_BEGIN    ((void *)&__bss_end)
#define RT_HW_HEAP_END      ((void *)((unsigned long)&_end + 512 * 1024))
```

**目标**：`board.c` 完成三板斧——Tick 配置、堆初始化、控制台输出——即可启动调度器。

---

### Step 5：最小测试应用

**核心需求**：创建最小化的 RT-Thread 测试应用，验证内核能否正确启动和调度。

**应用设计**（参考 FreeRTOS 的 `demo_main.c`）：在某个 U54 核（例如 `u54_1.c`）的入口函数中调用 RT-Thread 启动流程：

```c
/* src/application/hart1/u54_1.c - RT-Thread 启动入口示例 */

#include <rtthread.h>
#include <rthw.h>

/* 线程 1：打印信息 */
static void thread1_entry(void *parameter)
{
    while (1) {
        rt_kprintf("Thread 1: Hello from RT-Thread Nano!\n");
        rt_thread_mdelay(1000);
    }
}

/* 线程 2：打印信息 */
static void thread2_entry(void *parameter)
{
    while (1) {
        rt_kprintf("Thread 2: Running... tick=%d\n", rt_tick_get());
        rt_thread_mdelay(500);
    }
}

/* 主启动函数（由 u54_1.c 调用） */
void start_rtt_demo(void)
{
    rt_thread_t tid1, tid2;

    /* 初始化硬件定时器（需在第一个线程前完成） */
    /* 不同于 FreeRTOS 在第一个任务中调 vPortSetupTimer()
     * RT-Thread 要求 tick 在 board_init 阶段完成 */

    /* 创建线程 1 */
    tid1 = rt_thread_create("t1", thread1_entry, RT_NULL,
                             2048, 10, 20);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    /* 创建线程 2 */
    tid2 = rt_thread_create("t2", thread2_entry, RT_NULL,
                             2048, 15, 10);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    /* 启动调度器（不会返回） */
    rt_system_scheduler_start();
}
```

**差异说明（vs FreeRTOS）**：

| 行为 | FreeRTOS | RT-Thread Nano |
|---|---|---|
| 调度器启动 | `vTaskStartScheduler()` | `rt_system_scheduler_start()` |
| 任务创建 | `xTaskCreate(func, "name", stack, param, prio, &handle)` | `rt_thread_create(name, func, param, stack_size, priority, tick)` |
| 任务启动 | 创建即自动就绪 | 需单独调用 `rt_thread_startup()` |
| 定时器配置 | 第一个任务中手动调 `vPortSetupTimer()` | board.c 的 `rt_hw_board_init()` 中完成 |
| 系统 Tick | `xTaskGetTickCount()` | `rt_tick_get()` |
| 毫秒延时 | `vTaskDelay(pdMS_TO_TICKS(ms))` | `rt_thread_mdelay(ms)` |

**目标**：在串口终端观察到两个线程交替打印，确认调度器正常运转。

---

### Step 6：编译与调试（最小启动验证）

**核心需求**：确认构建系统可以正确编译 RT-Thread 内核，生成可执行的 `.elf` 文件。

**编译验证步骤**：

```bash
# 清理之前构建产物
make clean

# 编译 RT-Thread 版本（使用 -DUSING_RTTHREAD）
make BOARD=icicle-kit-es CONFIG_WITH_ARCH=1

# 检查生成的符号
riscv64-unknown-elf-nm -n Remote-Default/mpfs-rpmsg-remote.elf | grep rt_
```

**常见编译错误排除**：

| 问题 | 可能原因 | 解决方案 |
|---|---|---|
| `undefined reference to rt_hw_interrupt_disable` | context_gcc.S 未汇编 | 确认 ASM_SRCS 包含正确的路径 |
| `undefined reference to __heap_start` | 链接脚本未定义堆符号 | 使用 `_end` 或 `__bss_end` 替代 |
| `multiple definition of rt_hw_stack_init` | portable/cpuport.c 与 common/cpuport.c 冲突 | 移除 portable/cpuport.c 中的重复定义 |
| `csr mstatus not implemented` | 编译器架构参数错误 | 确认 `-march=rv64gc` 标志 |
| `relocation truncated` | medany 模型但代码段过大 | 检查链接脚本内存布局 |

**目标**：成功生成 `.elf` 文件，通过 OpenOCD 或 HSS 加载到 Icicle Kit 上运行。

---

### Step 7：链接脚本适配

**核心需求**：为 RT-Thread Nano 添加专用的堆段和处理栈布局。

**当前链接脚本分析**：

现有链接脚本 `mpfs-ddr-loaded-by-boot-loader-remote.ld` 包含：
- 一个 `.FreeRTOSheap` 段（FreeRTOS 专用）
- 各核的栈分配（`STACK_SIZE_E51_APPLICATION` 等）
- DDR 内存映射（`ddr_cached_32bit: 0x91C00000, 1M`）

**需要的修改**：

```ld
/* 在 SECTIONS 中添加 RT-Thread 堆段 */
.rtthread_heap : ALIGN(0x10)
{
    __rtthread_heap_start = .;
    *(.rtthread.heap)
    . = ALIGN(0x10);
    __rtthread_heap_end = .;
} > ddr_cached_32bit

/* 或者在已有 .heap 段中增加 RT-Thread 区域 */
.heap : ALIGN(0x10)
{
    __heap_start = .;
    *(.FreeRTOSheap)
    *(.rtthread.heap)         /* ← 新增 */
    __heap_end = .;
    _heap_end = __heap_end;
} > ddr_cached_32bit
```

**简化方案**（最小启动阶段）：暂时使用 FreeRTOS 的堆区域，RT-Thread 从同一区域分配内存。

---

### Step 8：多核 AMP 适配

**核心需求**：RT-Thread Nano 运行在单个 U54 核上，多核协同需遵循 PolarFire SoC 的 AMP 启动协议。

**E51 管理核调整**（参考 `e51.c` 的 FreeRTOS 版本）：

```
E51（管理核）
  1. 初始化 DDR、CLINT、PLIC、UART
  2. 将 RT-Thread 入口地址写入目标核的启动地址
  3. 发送软件中断（MSIP）唤醒目标 U54 核
  4. 进入空闲循环或运行裸机管理程序

U54_X（应用核，运行 RT-Thread）
  1. 从 WFI 中被 MSIP 唤醒
  2. 清除软件中断标志
  3. 调用 board.c 的 rt_hw_board_init()
  4. 创建应用线程
  5. 调用 rt_system_scheduler_start()
```

**核间同步参数**：

| 参数 | 值 | 说明 |
|---|---|---|
| 软件中断寄存器 | MSIP（mip.MSIP 位） | 每个核独立 MSIP 寄存器 |
| CLINT 基址 | 0x02000000 | 包含 MSIP（0x02000000）和 MTimer（0x02004000） |
| 目标核编号 | 0=E51, 1~4=U54_1~U54_4 | HLS（Hart Local Storage）中标识 |

---

### Step 9~12：完整版本移植

**（概要说明，具体实现在后续版本中展开）**

#### Step 9: RPMsg-Lite 适配

实现 `rpmsg_env_rtthread.c`，完成 RT-Thread 环境下的 RPMsg-Lite 环境抽象层：

| RPMsg 环境接口 | RT-Thread 实现 | 对应 FreeRTOS 实现 |
|---|---|---|
| `env_create_mutex` | `rt_mutex_create` | `xSemaphoreCreateCounting` |
| `env_create_sync_lock` | `rt_sem_create` | `xSemaphoreCreateCounting` |
| `env_create_queue` | `rt_mq_create` | `xQueueCreate` |
| `env_sleep_msec` | `rt_thread_mdelay` | `vTaskDelay` |
| `env_allocate_memory` | `rt_malloc` | `pvPortMalloc` |

#### Step 10: FinSH 控制台调试

启用 RT-Thread 的 FinSH 组件：
- 在 `rtconfig.h` 中定义 `RT_USING_FINSH`
- 参考 `components/finsh/` 的源文件集成
- 通过 UART 实现命令行交互

#### Step 11: 性能调优

| 调优目标 | 方法 | 参考值 |
|---|---|---|
| 上下文切换开销 | 测量 `rt_hw_context_switch` 执行周期 | 预期 < 100 cycles |
| Tick 精度 | 验证 CLINT 定时器中断调度抖动 | 预期 < 50 us |
| 内存占用 | 优化线程栈大小、heap 利用率 | 最小内核 ~8KB RAM |

---

## 第三部分：文件清单与对照表

### 3.1 FreeRTOS ↔ RT-Thread Nano 文件映射

| FreeRTOS 文件 | RT-Thread 文件 | 当前位置 | 状态 |
|---|---|---|---|
| `FreeRTOS/include/*` | `rt-thread/include/*` | rt-thread/include/ | 已就绪 |
| `portable/GCC/RISCV/portmacro.h` | `portable/rtt_port.h` | RTThread/portable/ | 骨架已创建 |
| `portable/GCC/RISCV/port.c` | `rt-thread/libcpu/common/cpuport.c` | rt-thread/libcpu/ | 已就绪 |
| `portable/GCC/RISCV/portasm.S` | `rt-thread/libcpu/common/context_gcc.S` | rt-thread/libcpu/ | 已就绪 |
| `portable/MemMang/heap_2.c` | `rt-thread/src/mem.c` | rt-thread/src/ | 已就绪 |
| `config/FreeRTOSConfig.h` | `config/rtconfig.h` | middleware/config/ | 骨架已创建 |
| `FreeRTOS/Makefile` | `RTThread/Makefile` | RTThread/ | 已就绪 |
| `FreeRTOS/croutine.c` | `rt-thread/src/clock.c` | rt-thread/src/ | 已就绪 |
| `FreeRTOS/tasks.c` | `rt-thread/src/thread.c` | rt-thread/src/ | 已就绪 |
| `FreeRTOS/queue.c` | `rt-thread/src/ipc.c` | rt-thread/src/ | 已就绪 |
| `FreeRTOS/timers.c` | `rt-thread/src/timer.c` | rt-thread/src/ | 已就绪 |

### 3.2 需要自行实现的文件

| 文件 | 优先级 | 说明 |
|---|---|---|
| `board/board.c` | **紧急** | 最小启动必须，已完成骨架需完善 |
| `board/board.h` | **紧急** | 板级定义，已完成 |
| `portable/rtt_port.h` | **紧急** | 移植宏定义，已完成 |
| `portable/cpuport.c` | **一般** | 项目定制扩展（当前为空） |
| `libcpu/risc-v/polarfire/interrupt_gcc.S` | **高** | 机器模式中断入口（可选在 board.c 中内联实现） |
| `rpmsg_env_rtthread.c` | **低** | 完整版本才需要 |

---

## 第四部分：调试与验证

### 4.1 逐步验证检查清单

```
□ Step 1: rt-thread/src/clock.c 等源文件存在且非空
□ Step 2: make BOARD=icicle-kit-es CONFIG_WITH_ARCH=1 编译通过
□ Step 3: objdump 可看到 rt_hw_context_switch 等符号
□ Step 4: 串口输出 RT-Thread 启动信息
□ Step 5: 两个线程交替执行，rt_tick_get() 递增
□ Step 6: 信号量同步正常工作
□ Step 7: 软件定时器触发回调
□ Step 8: FinSH 控制台可用
□ Step 9: RPMsg 通信正常（ping-pong / echo 示例）
```

### 4.2 调试提示

1. **定时器不触发**：检查 `mie.MTIE` 位是否设置，`mtime` 寄存器地址是否正确
2. **上下文切换崩溃**：确认栈帧布局与 `cpuport.c` 中的 `rt_hw_stack_init` 一致
3. **串口无输出**：确认 `rt_kprintf` 映射到 UART 输出函数（在 `kservice.c` 或 `board.c` 中绑定）
4. **链接错误**：检查 MEMORY 区域的地址范围和大小是否匹配硬件配置
5. **GP 寄存器错误**：确认链接脚本中的 `__global_pointer$` 设置在 `.sdata` 段中间

---

## 附录 A：关键硬件地址

| 外设 | 基地址 | 说明 |
|---|---|---|
| CLINT | 0x02000000 | 核间中断 + 定时器 |
| CLINT MSIP | 0x02000000 + hart_id * 4 | 每个核的软件中断寄存器 |
| CLINT MTIMECMP | 0x02004000 + hart_id * 8 | 每个核的定时器比较值 |
| CLINT MTIME | 0x0200BFF8 | 全局单调递增定时器 |
| PLIC | 0x0C000000 | 平台级中断控制器 |
| MIV IHC | (FPGA fabric) | 核间通信 IP |
| MSS UART0 | 0x10000000 | 控制台 UART |
| MSS UART1 | 0x10010000 | RPMsg 主核 UART |
| MSS UART3 | 0x10030000 | RPMsg 从核 UART |

## 附录 B：rtconfig.h 配置建议

最小启动阶段的 rtconfig.h 配置建议：

```c
/* === 最小启动配置 === */
#define RT_THREAD_PRIORITY_MAX  31     /* 优先级数 */
#define RT_TICK_PER_SECOND      1000   /* Tick 频率 */

/* === 必须使能 === */
#define RT_USING_HEAP                   /* 动态内存分配 */
#define RT_USING_SEMAPHORE              /* 信号量 */
#define RT_USING_MUTEX                  /* 互斥体 */
#define RT_USING_EVENT                  /* 事件 */
#define RT_USING_MAILBOX                /* 邮箱 */
#define RT_USING_MESSAGEQUEUE           /* 消息队列 */
#define RT_USING_TIMER                  /* 软件定时器 */
#define RT_USING_CONSOLE                /* 控制台输出 */
#define RT_USING_HOOK                   /* 钩子函数 */

/* === 可选（按需开启） === */
/* #define RT_USING_FINSH */            /* FinSH 命令行 */
/* #define RT_USING_DEVICE */           /* 设备框架 */
/* #define RT_USING_LIBC */             /* C 库接口 */

/* === 架构 === */
#define ARCH_RISCV
#define ARCH_RISCV64
#define RT_ALIGN_SIZE          8
#define RT_NAME_MAX            8
```

---

## 文档版本

| 版本 | 日期 | 说明 |
|---|---|---|
| v0.1 | 2026-06-15 | 初始版，基于移植骨架 v0.1 + v0.2 |
