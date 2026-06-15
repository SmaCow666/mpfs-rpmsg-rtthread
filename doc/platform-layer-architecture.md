# PolarFire SoC 平台层架构与驱动接口文档

## 文档概述

本文档系统梳理 `src/platform/` 层的目录结构、层次划分、驱动 API 接口以及构建集成方式。
平台层是 PolarFire SoC Icicle Kit 硬件与上层软件（RTOS / 中间件 / 应用程序）之间的硬件抽象和驱动接口层。

---

## 第一部分：平台层整体架构

### 1.1 层次定位

```
┌─────────────────────────────────────────────────┐
│           Application / Middleware               │
│   (hart0~4/e51.c, u54_1~4.c, demo_main, ...)   │
├──────────────┬──────────────────────────────────┤
│  FreeRTOS    │  RPMsg-Lite / Remoteproc          │
├──────────────┴──────────────────────────────────┤
│              Platform Layer                      │  ← 本文档范围
│                                                  │
│  ┌────────────┬─────────────┬─────────────────┐  │
│  │  Drivers   │   HAL       │   MPFS HAL      │  │
│  │ (MSS/FPGA) │ (抽象层)     │ (SoC 固件 HAL)   │  │
│  └────────────┴─────────────┴─────────────────┘  │
├──────────────────────────────────────────────────┤
│           Hardware (PolarFire SoC)                │
│    E51 rv64imac + U54_1~4 rv64gc + DDR + ENVM   │
└──────────────────────────────────────────────────┘
```

### 1.2 完整目录结构

```
src/platform/
├── Makefile                    # 构建集成：SRCS/ASM_SRCS/INCLUDES 变量追加
│
├── hal/                        # 硬件抽象层（极简、跨平台抽象）
│   ├── hal.h                   #   HAL 主头文件
│   ├── hal_assert.h            #   断言宏
│   ├── hal_irq.c               #   中断使能/禁用抽象
│   ├── hal_version.h           #   HAL 版本定义
│   ├── cpu_types.h             #   CPU 类型定义
│   ├── hw_macros.h             #   硬件操作通用宏
│   ├── hw_reg_access.h         #   寄存器读写访问宏
│   ├── hw_reg_access.S         #   寄存器访问汇编实现
│   └── readme.md
│
├── mpfs_hal/                   # PolarFire SoC 硬件抽象层（SoC 特定实现）
│   ├── mss_hal.h               #   MPFS HAL 主头文件（包含全部子模块）
│   ├── mpfs_hal_version.h      #   版本定义
│   ├── readme.md
│   │
│   ├── common/                 # HAL 核心功能实现
│   │   ├── mss_clint.c/h       #   CLINT（核间本地中断 + 定时器）驱动
│   │   ├── mss_plic.c/h        #   PLIC（平台级中断控制器）驱动
│   │   ├── mss_pmp.c/h         #   PMP（物理内存保护）配置
│   │   ├── mss_mpu.c/h         #   MPU（内存保护单元）配置
│   │   ├── mss_mtrap.c/h       #   机器模式陷阱处理
│   │   ├── mss_l2_cache.c/h    #   L2 缓存初始化
│   │   ├── mss_axiswitch.c/h   #   交叉开关矩阵配置
│   │   ├── mss_h2f.c/h         #   H2F（主机到 FPGA）桥接
│   │   ├── mss_peripherals.c/h #   外设时钟与复位管理
│   │   ├── mss_util.c/h        #   工具函数（延时、ID 获取）
│   │   ├── mss_hart_ints.h     #   核中断号定义
│   │   ├── mss_seg.h           #   段寄存定义
│   │   ├── mss_sysreg.h        #   系统寄存器定义
│   │   ├── mss_irq_handler_stubs.c  # 中断处理存根
│   │   ├── mss_legacy_defines.h      # 遗留宏定义
│   │   ├── encoding.h / bits.h / atomic.h  # 辅助定义
│   │   │
│   │   └── nwc/                # 非易失性 / 时钟 / DDR / IOMUX 管理
│   │       ├── mss_cfm.c/h     #   时钟分频配置
│   │       ├── mss_pll.c/h     #   PLL 配置
│   │       ├── mss_ddr.c/h     #   DDR 初始化与配置
│   │       ├── mss_ddr_debug.c/h    # DDR 调试
│   │       ├── mss_ddr_test_pattern.c # DDR 测试
│   │       ├── mss_sgmii.c/h   #   SGMII 接口配置
│   │       ├── mss_io.c/h      #   IOMUX 配置
│   │       ├── mss_nwc_init.c/h     #  NWC（非易失性 + 时钟）初始化
│   │       └── simulation.h    #   仿真辅助
│   │
│   └── startup_gcc/            # GCC 启动代码
│       ├── mss_entry.S         #   汇编启动入口（_start）
│       ├── mss_utils.S         #   汇编辅助函数
│       ├── system_startup.c    #   C 启动初始化
│       ├── system_startup.h    #   启动头文件
│       ├── system_startup_defs.h  # 启动常量定义
│       └── newlib_stubs.c      #   Newlib 系统调用存根
│
├── drivers/                    # 外设驱动层
│   ├── mss/                    # MSS（微处理器子系统）片上外设
│   │   ├── mss_mmuart/         #   UART 串口
│   │   ├── mss_gpio/           #   GPIO 通用 IO
│   │   ├── mss_timer/          #   定时器
│   │   ├── mss_i2c/            #   I²C 接口
│   │   ├── mss_spi/            #   SPI 接口
│   │   ├── mss_can/            #   CAN 总线
│   │   ├── mss_watchdog/       #   看门狗（WDT）
│   │   ├── mss_rtc/            #   实时时钟（RTC）
│   │   ├── mss_qspi/           #   QSPI Flash 控制器
│   │   ├── mss_pdma/           #   PDMA（外设 DMA）
│   │   ├── mss_mmc/            #   MMC/SD/eMMC 存储
│   │   ├── mss_ethernet_mac/   #   千兆以太网 MAC
│   │   ├── mss_usb/            #   USB 2.0 OTG
│   │   ├── mss_sys_services/   #   系统服务（复位、NVM、自检）
│   │   └── pf_pcie/            #   PCIe 控制器
│   │
│   ├── fpga_ip/                # FPGA 构造 IP 驱动（Libero 设计集成）
│   │   ├── miv_ihc/            #   IHC（核间通信）IP
│   │   ├── CoreGPIO/           #   CoreGPIO（FPGA GPIO）
│   │   ├── CorePWM/            #   CorePWM（PWM 发生器）
│   │   └── CoreSysServices_PF/ #   CoreSysServices（系统服务 IP）
│   │
│   └── off_chip/               # 板级外设驱动
│       └── README.md
│
└── soc_config_generator/       # SOC 配置生成器（Python）
    └── mpfs_configuration_generator.py
```

### 1.3 构建集成分析

`src/platform/Makefile` 通过变量追加模式将平台层文件加入编译。当前项目中实际编译的模块分为四个层级：

| 层级 | 编译模块 | 文件数 |
|---|---|---|
| 汇编启动 | mss_entry.S / mss_utils.S / hw_reg_access.S | 3 |
| C 启动 & Newlib | system_startup.c / newlib_stubs.c | 2 |
| MPFS HAL 核心 | clint, plic, pmp, mpu, l2_cache, mtrap, axiswitch, h2f, peripherals, util, irq_handler_stubs | 13 |
| NWC 子系统 | mss_cfm, mss_pll, mss_ddr, mss_ddr_debug, mss_ddr_test_pattern, mss_io, mss_nwc_init, mss_sgmii | 8 |
| MSS 外设驱动 | mss_uart, mss_gpio (主动编译) | 2 |
| FPGA IP 驱动 | miv_ihc (主动编译) | 1 |

**注意**：`mss_timer/`、`mss_i2c/`、`mss_spi/`、`mss_can/`、`mss_mmc/` 等驱动存在于源码树中，但未在当前项目的 Makefile 中启用编译。需要时在 `platform/Makefile` 中追加 `SRCS +=` 和 `INCLUDES +=`。

---

## 第二部分：HAL 层 API 接口

### 2.1 HAL 抽象层（`platform/hal/`）

HAL 是最底层的硬件抽象，提供与具体 SoC 无关的极简接口。

#### hal.h — HAL 主接口

| 宏/函数 | 类型 | 描述 |
|---|---|---|
| `HAL_IRQ_LOCAL_DISABLE(irq)` | 宏 | 禁止本地中断 |
| `HAL_IRQ_LOCAL_ENABLE(irq)` | 宏 | 使能本地中断 |
| `HAL_IRQ_GLOBAL_DISABLE()` | 宏 | 禁止全局中断 |
| `HAL_IRQ_GLOBAL_ENABLE()` | 宏 | 使能全局中断 |
| `HAL_REG_WRITE(addr, val)` | 宏 | 写 32 位寄存器 |
| `HAL_REG_READ(addr)` | 宏 | 读 32 位寄存器 |
| `HAL_REG_SET_BIT(addr, bit)` | 宏 | 置位寄存器位 |
| `HAL_REG_CLR_BIT(addr, bit)` | 宏 | 清零寄存器位 |

#### hal_irq.c — 中断管理

| 函数 | 返回值 | 描述 |
|---|---|---|
| `__enable_irq(void)` | void | 使能全局中断（写 mstatus.MIE） |
| `__disable_irq(void)` | void | 禁止全局中断（清 mstatus.MIE） |
| `__enable_local_irq(uint32_t irq)` | void | 使能指定本地中断（写 mie） |
| `__disable_local_irq(uint32_t irq)` | void | 禁止指定本地中断（清 mie） |

#### hw_reg_access.h / hw_reg_access.S — 寄存器访问

| 函数 | 描述 |
|---|---|
| `HW_REG_READ(addr)` | 读取 32 位内存映射寄存器 |
| `HW_REG_WRITE(addr, val)` | 写入 32 位内存映射寄存器 |
| `HW_REG_SET_BIT(addr, bit)` | 对指定寄存器位执行读-改-写置位 |
| `HW_REG_CLR_BIT(addr, bit)` | 对指定寄存器位执行读-改-写清零 |

---

### 2.2 MPFS HAL 核心（`platform/mpfs_hal/common/`）

MPFS HAL 是 PolarFire SoC 特定实现，封装了 SoC 核间架构特性。

#### mss_clint.c/h — CLINT（Core Local Interrupter）

CLINT 负责核间软件中断和定时器中断管理。

| 函数 | 返回值 | 描述 |
|---|---|---|
| `CLINT_init(void)` | void | 初始化 CLINT |
| `CLINT_clear_soft_interrupt(hartid)` | void | 清除指定核的软件中断 |
| `CLINT_raise_soft_interrupt(hartid)` | void | 触发指定核的软件中断 |
| `CLINT_set_mtimecmp(hartid, value)` | void | 设置指定核的定时器比较值 |
| `CLINT_get_mtime(void)` | uint64_t | 获取全局单调计数值 |

**关键寄存器地址**：

| 寄存器 | 基地址偏移 | 描述 |
|---|---|---|
| MSIP | 0x0000 + hartid * 4 | 核间软件中断挂起 |
| MTIMECMP | 0x4000 + hartid * 8 | 定时器比较值 |
| MTIME | 0xBFF8 | 全局 64 位定时器 |

#### mss_plic.c/h — PLIC（Platform-Level Interrupt Controller）

PLIC 负责管理所有平台级外设中断。

| 函数 | 返回值 | 描述 |
|---|---|---|
| `PLIC_init(void)` | void | 初始化 PLIC（设置优先级阈值） |
| `PLIC_init_on_reset(void)` | void | 复位时初始化 PLIC |
| `PLIC_SetPriority(source, priority)` | void | 设置中断源优先级 |
| `PLIC_SetThreshold(hart_id, threshold)` | void | 设置指定核的中断阈值 |
| `PLIC_EnableIRQ(source)` | void | 使能指定中断源 |
| `PLIC_DisableIRQ(source)` | void | 禁止指定中断源 |
| `PLIC_ClearPending(source)` | void | 清除中断挂起状态 |
| `PLIC_ClaimIRQ(hart_id)` | uint32_t | 申请当前最高优先级中断 ID |
| `PLIC_CompleteIRQ(hart_id, source)` | void | 完成中断处理 |

#### mss_pmp.c/h — PMP（Physical Memory Protection）

| 函数 | 返回值 | 描述 |
|---|---|---|
| `pmp_configure(hart_id)` | void | 为指定核配置 PMP 区域 |

#### mss_mpu.c/h — MPU（Memory Protection Unit）

| 函数 | 返回值 | 描述 |
|---|---|---|
| `mpu_configure(void)` | void | 配置总线错误单元的 MPU 区域 |

#### mss_mtrap.c/h — 机器模式陷阱处理

| 函数 | 返回值 | 描述 |
|---|---|---|
| `mss_mtrap_init(void)` | void | 初始化异常/陷阱处理 |
| `mss_mtrap_handler(regs)` | void | 机器模式异常/中断处理入口 |

#### mss_l2_cache.c/h — L2 缓存管理

| 函数 | 返回值 | 描述 |
|---|---|---|
| `config_l2_cache(void)` | void | 配置 L2 零缓存（Zero Cache）模式 |

#### mss_peripherals.c/h — 外设时钟与复位

| 函数 | 返回值 | 描述 |
|---|---|---|
| `mss_config_clk_rst(periph, hart, op)` | uint8_t | 配置指定外设的时钟与复位 |
| `mss_enable_fabric(void)` | void | 使能 FPGA 构造时钟 |

**支持的外设枚举**：`MSS_PERIPH_MMUART0` ~ `MSS_PERIPH_MMUART4`、`MSS_PERIPH_GPIO0`/`1`、`MSS_PERIPH_SPI0`~`3`、`MSS_PERIPH_I2C0`~`3`、`MSS_PERIPH_TIMER`、`MSS_PERIPH_WDT`、`MSS_PERIPH_FIC0`~`3` 等。

#### mss_axiswitch.c/h — 交叉开关

| 函数 | 返回值 | 描述 |
|---|---|---|
| `AXI_switch_init(void)` | void | 初始化 AXI 交叉开关 |
| `AXI_switch_set_master(port, master_id)` | void | 设置主端口路由 |

#### mss_h2f.c/h — H2F 桥接

| 函数 | 返回值 | 描述 |
|---|---|---|
| `H2F_init(void)` | void | 初始化主机到 FPGA 桥接 |

#### mss_util.c/h — 工具函数

| 函数 | 返回值 | 描述 |
|---|---|---|
| `read_csr(csr)` | uint64_t | 读 RISC-V CSR 寄存器（宏） |
| `write_csr(csr, value)` | void | 写 RISC-V CSR 寄存器（宏） |
| `delay_1ms(hart_id)` | void | 延时 1ms（基于 CLINT 定时器） |
| `get_hart_id(void)` | uint64_t | 获取当前 Hart ID |

#### mss_hart_ints.h — 核中断映射

定义各核的中断向量映射常量：

| 宏 | 值 | 描述 |
|---|---|---|
| `MPFS_HAL_FIRST_HART` | 0 或 1 | 系统最低编号 Hart |
| `MPFS_HAL_LAST_HART` | 4 | 系统最高编号 Hart |
| `M_MACHINE_TIMER_IRQ` | 7 | 机器模式定时器中断编号 |
| `M_MACHINE_SOFTWARE_IRQ` | 3 | 机器模式软件中断编号 |
| `M_MACHINE_EXTERNAL_IRQ` | 11 | 机器模式外部（PLIC）中断编号 |

---

### 2.3 NWC 子系统（`platform/mpfs_hal/common/nwc/`）

NWC 子系统负责 SoC 上电时的一次性初始化序列。

| 模块 | 函数 | 描述 |
|---|---|---|
| **初始化顺序** | `mss_nwc_init()` | 按序初始化：PLL → CFM → IOMUX → SGMII → DDR |
| **PLL** | `mss_pll_config()` | 配置 MSS PLL 锁相环 |
| **CFM** | `mss_cfm_init()` | 配置时钟分频因子 |
| **IOMUX** | `mss_io_config()` | 配置 IO 多路复用 |
| **SGMII** | `mss_sgmii_init()` | 初始化 SGMII 接口 |
| **DDR** | `mss_ddr_init()` | DDR 初始化/训练 |
| **DDR 调试** | `mss_ddr_debug()` | DDR 调试信息输出 |

---

## 第三部分：MSS 外设驱动 API 接口

### 3.1 MMUART 串口（mss_mmuart）— 当前已编译

**驱动文件**：`mss_uart.c` / `mss_uart.h` / `mss_uart_regs.h`

**依赖关系**：仅 `<stddef.h>`、`<stdint.h>`，无 RTOS 依赖，纯裸机驱动。

**核心 API 函数**：

| 函数 | 返回值 | 描述 |
|---|---|---|
| `MSS_UART_init(inst, baud, config)` | void | 初始化 UART 实例（波特率、数据位/校验/停止位） |
| `MSS_UART_polled_tx(inst, buf, size)` | void | 轮询发送数据 |
| `MSS_UART_polled_tx_string(inst, str)` | void | 轮询发送字符串 |
| `MSS_UART_get_rx(inst, buf, size)` | uint8_t | 读取接收数据（返回接收字节数） |
| `MSS_UART_irq_tx(inst, buf, size)` | void | 中断方式发送 |
| `MSS_UART_irq_rx(inst, buf, size)` | void | 中断方式接收 |
| `MSS_UART_set_baud(inst, baud)` | void | 运行时更改波特率 |
| `MSS_UART_enable_irq(inst, irqs)` | void | 使能 UART 中断 |
| `MSS_UART_disable_irq(inst, irqs)` | void | 禁止 UART 中断 |
| `MSS_UART_set_irq_handler(inst, handler)` | void | 设置中断回调 |
| `MSS_UART_set_loopback(inst, mode)` | void | 设置回环模式 |
| `MSS_UART_set_break(inst)` | void | 发送 BREAK 信号 |
| `MSS_UART_clear_break(inst)` | void | 清除 BREAK 信号 |

**UART 实例**：

| 实例变量 | 说明 |
|---|---|
| `g_mss_uart0_lo` / `g_mss_uart0_hi` | MMUART0（低/高寄存器接口） |
| `g_mss_uart1_lo` / `g_mss_uart1_hi` | MMUART1 |
| `g_mss_uart2_lo` / `g_mss_uart2_hi` | MMUART2 |
| `g_mss_uart3_lo` / `g_mss_uart3_hi` | MMUART3 |
| `g_mss_uart4_lo` / `g_mss_uart4_hi` | MMUART4 |

**波特率配置宏**：`MSS_UART_115200_BAUD`、`MSS_UART_57600_BAUD`、`MSS_UART_38400_BAUD`、`MSS_UART_19200_BAUD`、`MSS_UART_9600_BAUD`

**数据格式宏**：`MSS_UART_DATA_8_BITS`、`MSS_UART_DATA_7_BITS`、`MSS_UART_NO_PARITY`、`MSS_UART_EVEN_PARITY`、`MSS_UART_ONE_STOP_BIT`、`MSS_UART_TWO_STOP_BITS`

**中断源宏**：`MSS_UART_RBF_IRQ`（接收缓冲满）、`MSS_UART_TBE_IRQ`（发送缓冲空）、`MSS_UART_TO_IRQ`（超时）、`MSS_UART_PARITY_ERR_IRQ`、`MSS_UART_FRAMING_ERR_IRQ`、`MSS_UART_OVERFLOW_ERR_IRQ`

**当前项目使用示例**（`demo_main.c`）：

```c
MSS_UART_init(&g_mss_uart0_lo, MSS_UART_115200_BAUD,
              MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
MSS_UART_polled_tx_string(&g_mss_uart0_lo, g_info_string);
rx_size = MSS_UART_get_rx(UART_APP, rx_buff, sizeof(rx_buff));
```

---

### 3.2 GPIO（mss_gpio）— 当前已编译

**驱动文件**：`mss_gpio.c` / `mss_gpio.h`

**依赖关系**：仅 `<stdint.h>`，纯裸机驱动。

**核心 API 函数**：

| 函数 | 返回值 | 描述 |
|---|---|---|
| `MSS_GPIO_init()` | void | 初始化 GPIO 模块 |
| `MSS_GPIO_config(port, pin, config)` | void | 配置指定引脚（输入/输出/中断） |
| `MSS_GPIO_set_output(port, pin, value)` | void | 设置引脚输出值 |
| `MSS_GPIO_get_input(port, pin)` | uint8_t | 读取引脚输入值 |
| `MSS_GPIO_enable_irq(port, pin)` | void | 使能引脚中断 |
| `MSS_GPIO_disable_irq(port, pin)` | void | 禁止引脚中断 |
| `MSS_GPIO_clear_irq(port, pin)` | void | 清除引脚中断标志 |
| `MSS_GPIO_set_irq_handler(port, handler)` | void | 设置 GPIO 中断回调 |

**配置宏**：

| 宏 | 描述 |
|---|---|
| `MSS_GPIO_INPUT_MODE` | 输入模式 |
| `MSS_GPIO_OUTPUT_MODE` | 输出模式 |
| `MSS_GPIO_IRQ_MODE_LEVEL_HIGH` | 高电平触发中断 |
| `MSS_GPIO_IRQ_MODE_LEVEL_LOW` | 低电平触发中断 |
| `MSS_GPIO_IRQ_MODE_EDGE_POSITIVE` | 上升沿触发中断 |
| `MSS_GPIO_IRQ_MODE_EDGE_NEGATIVE` | 下降沿触发中断 |

---

### 3.3 定时器（mss_timer）— 树中存在，当前未编译

**驱动文件**：`mss_timer.c`（空的，仅声明） / `mss_timer.h` / `mss_timer_regs.h`

**依赖关系**：`mpfs_hal/mss_hal.h`、`mss_timer_regs.h`

**核心 API 函数**：

| 函数 | 返回值 | 描述 |
|---|---|---|
| `MSS_TIMER_init(timer, mode)` | void | 初始化定时器实例 |
| `MSS_TIMER_start(timer)` | void | 启动定时器 |
| `MSS_TIMER_stop(timer)` | void | 停止定时器 |
| `MSS_TIMER_get_count(timer)` | uint32_t | 获取当前计数值 |
| `MSS_TIMER_set_count(timer, value)` | void | 设置计数值 |
| `MSS_TIMER_enable_irq(timer)` | void | 使能定时器中断 |
| `MSS_TIMER_disable_irq(timer)` | void | 禁止定时器中断 |
| `MSS_TIMER_set_irq_handler(timer, handler)` | void | 设置中断回调 |

**注意**：当前项目的 OS Tick（FreeRTOS 和 RT-Thread）均使用 CLINT MTimer，而非 MSS 定时器。

---

### 3.4 I²C（mss_i2c）— 树中存在，当前未编译

**驱动文件**：`mss_i2c.c` / `mss_i2c.h` / `mss_i2c_regs.h`

**依赖关系**：仅 `<stdint.h>`，纯裸机驱动。

**核心 API 函数**：

| 函数 | 返回值 | 描述 |
|---|---|---|
| `MSS_I2C_init(inst, speed)` | void | 初始化 I²C 实例 |
| `MSS_I2C_write(inst, addr, buf, size)` | uint8_t | 向设备写入数据 |
| `MSS_I2C_read(inst, addr, buf, size)` | uint8_t | 从设备读取数据 |
| `MSS_I2C_write_byte(inst, addr, byte)` | uint8_t | 写入单字节 |
| `MSS_I2C_read_byte(inst, addr, byte)` | uint8_t | 读取单字节 |
| `MSS_I2C_enable_irq(inst)` | void | 使能 I²C 中断 |
| `MSS_I2C_disable_irq(inst)` | void | 禁止 I²C 中断 |
| `MSS_I2C_set_irq_handler(inst, handler)` | void | 设置中断回调 |

**速度宏**：`MSS_I2C_STANDARD_MODE` (100kHz)、`MSS_I2C_FAST_MODE` (400kHz)

---

### 3.5 SPI（mss_spi）— 树中存在，当前未编译

**驱动文件**：`mss_spi.c` / `mss_spi.h`

**核心 API 函数**：

| 函数 | 返回值 | 描述 |
|---|---|---|
| `MSS_SPI_init(inst, mode, speed)` | void | 初始化 SPI 实例 |
| `MSS_SPI_transfer(inst, tx_buf, rx_buf, size)` | void | 同步双工传输 |
| `MSS_SPI_write(inst, buf, size)` | void | 写入数据 |
| `MSS_SPI_read(inst, buf, size)` | void | 读取数据 |
| `MSS_SPI_set_speed(inst, speed)` | void | 设置传输速率 |
| `MSS_SPI_enable_irq(inst)` | void | 使能 SPI 中断 |

**模式宏**：`MSS_SPI_MODE_0` ~ `MSS_SPI_MODE_3`（CPOL/CPHA 组合）

---

### 3.6 CAN（mss_can）— 树中存在，当前未编译

**驱动文件**：`mss_can.c` / `mss_can.h`

| 函数 | 描述 |
|---|---|
| `MSS_CAN_init(inst, baud)` | 初始化 CAN 控制器 |
| `MSS_CAN_send_msg(inst, msg)` | 发送 CAN 报文 |
| `MSS_CAN_receive_msg(inst, msg)` | 接收 CAN 报文 |
| `MSS_CAN_config_filter(inst, filter)` | 配置接收过滤器 |

---

### 3.7 看门狗（mss_watchdog）— 树中存在，当前未编译

**驱动文件**：`mss_watchdog.c` / `mss_watchdog.h`

| 函数 | 描述 |
|---|---|
| `MSS_WDT_init(timeout)` | 初始化看门狗（超时值） |
| `MSS_WDT_start(void)` | 启动看门狗 |
| `MSS_WDT_stop(void)` | 停止看门狗 |
| `MSS_WDT_clear(void)` | 喂狗（清除计数器） |

---

### 3.8 RTC 实时时钟（mss_rtc）— 树中存在，当前未编译

**驱动文件**：`mss_rtc.c` / `mss_rtc.h`

| 函数 | 描述 |
|---|---|
| `MSS_RTC_init(config)` | 初始化 RTC |
| `MSS_RTC_set_time(tm)` | 设置时间 |
| `MSS_RTC_get_time(tm)` | 获取时间 |
| `MSS_RTC_set_alarm(alarm)` | 设置闹钟 |

---

### 3.9 QSPI（mss_qspi）— 树中存在，当前未编译

**驱动文件**：`mss_qspi.c` / `mss_qspi.h` / `mss_qspi_regs.h`

| 函数 | 描述 |
|---|---|
| `MSS_QSPI_init(inst, config)` | 初始化 QSPI 控制器 |
| `MSS_QSPI_read(addr, buf, size)` | 从 QSPI Flash 读取 |
| `MSS_QSPI_write(addr, buf, size)` | 写入 QSPI Flash |
| `MSS_QSPI_erase(addr, len)` | 擦除 QSPI Flash 区域 |

---

### 3.10 PDMA（mss_pdma）— 树中存在，当前未编译

**驱动文件**：`mss_pdma.c` / `mss_pdma.h` / `mss_pdma_regs.h`

| 函数 | 描述 |
|---|---|
| `MSS_PDMA_init(ch)` | 初始化 PDMA 通道 |
| `MSS_PDMA_transfer(ch, src, dst, size)` | 启动 DMA 传输 |
| `MSS_PDMA_wait_complete(ch)` | 等待传输完成 |
| `MSS_PDMA_set_irq_handler(ch, handler)` | 设置 DMA 中断回调 |

---

### 3.11 MMC/SD（mss_mmc）— 树中存在，当前未编译

**驱动文件**：`mss_mmc.c` / `mss_mmc.h` / `mss_mmc_if.c` / `mss_mmc_if.h`

| 函数 | 描述 |
|---|---|
| `MSS_MMC_init(config)` | 初始化 MMC 控制器 |
| `MSS_MMC_read(blk_addr, buf, blk_cnt)` | 读块数据 |
| `MSS_MMC_write(blk_addr, buf, blk_cnt)` | 写块数据 |
| `MSS_MMC_get_card_status(void)` | 获取卡状态 |

---

### 3.12 以太网 MAC（mss_ethernet_mac）— 树中存在，当前未编译

**驱动文件**：`mss_ethernet_mac.c` / `mss_ethernet_mac.h` + PHY 驱动（多文件）

| 函数 | 描述 |
|---|---|
| `MSS_MAC_init(mac, config)` | 初始化以太网 MAC |
| `MSS_MAC_send_frame(mac, frame, len)` | 发送以太网帧 |
| `MSS_MAC_receive_frame(mac, buf, len)` | 接收以太网帧 |
| `MSS_MAC_set_irq_handler(mac, handler)` | 设置 MAC 中断回调 |

**PHY 驱动支持**：`vsc8541_phy.c`、`vsc8575_phy.c`、`ti_dp83867_phy.c`、`vsc8662_phy.c`

---

### 3.13 USB（mss_usb）— 树中存在，当前未编译

**驱动文件**：多文件结构（`mss_usb_host.c` / `mss_usb_device.c` + 接口层）

| 函数 | 描述 |
|---|---|
| `MSS_USB_init(mode)` | 初始化 USB（主机/设备模式） |
| `MSS_USB_host_init()` | USB 主机模式初始化 |
| `MSS_USB_device_init(config)` | USB 设备模式初始化 |
| `MSS_USB_host_msc_read(lba, buf, count)` | 主机 MSC 读块 |
| `MSS_USB_host_msc_write(lba, buf, count)` | 主机 MSC 写块 |
| `MSS_USB_device_hid_send(buf, len)` | 设备 HID 发送 |
| `MSS_USB_device_msd_process()` | 设备 MSC 处理 |

**子模块**：主机模式（hid/msc）、设备模式（hid/msc），约 20+ 源文件。

---

### 3.14 系统服务（mss_sys_services）— 树中存在，当前未编译

**驱动文件**：`mss_sys_services.c` / `mss_sys_services.h`

| 函数 | 描述 |
|---|---|
| `MSS_SYS_services_init()` | 初始化系统服务 |
| `MSS_SYS_soft_reset()` | 软件复位 |
| `MSS_SYS_read_serial_number()` | 读取芯片序列号 |
| `MSS_SYS_read_digest()` | 读取摘要 |
| `MSS_SYS_self_test(mode)` | 自测试 |

---

### 3.15 PCIe（pf_pcie）— 树中存在，当前未编译

**驱动文件**：`pf_pcie.c` / `pf_pcie.h` / `pf_pcie_regs.h` / `pf_pcie_types.h`

| 函数 | 描述 |
|---|---|
| `PF_PCIE_init(config)` | 初始化 PCIe 控制器 |
| `PF_PCIE_enum_bus()` | 枚举 PCIe 总线 |
| `PF_PCIE_config_read(bus, dev, func, offset)` | 读配置空间 |
| `PF_PCIE_config_write(bus, dev, func, offset, val)` | 写配置空间 |
| `PF_PCIE_dma_transfer(src, dst, size)` | PCIe DMA 传输 |

---

## 第四部分：FPGA IP 驱动 API 接口

### 4.1 MIV IHC（核间通信）— 当前已编译

**驱动文件**：`miv_ihc.c` / `miv_ihc.h` + regs/types/version

**依赖关系**：HAL 寄存器访问宏，无 OS 依赖。

IHC（Inter-Hart Communication）是 RPMsg-Lite 核间通信的底层传输层，使用 FPGA 构造中例化的 MIV IHC IP 核。

| 函数 | 描述 |
|---|---|
| `MIV_IHC_init(channel)` | 初始化 IHC 通道 |
| `MIV_IHC_send(channel, data, size)` | 通过 IHC 发送消息 |
| `MIV_IHC_receive(channel, buf, size)` | 从 IHC 接收消息 |
| `MIV_IHC_enable_irq(channel)` | 使能 IHC 通道中断 |
| `MIV_IHC_disable_irq(channel)` | 禁止 IHC 通道中断 |
| `MIV_IHC_set_irq_handler(channel, handler)` | 设置 IHC 中断回调 |
| `MIV_IHC_get_status(channel)` | 获取 IHC 通道状态 |

**中断号（示例）**：

| 宏 | 值 |
|---|---|
| `RL_PLATFORM_MIV_IHC_CH8_LINK_ID` | IHC 通道 8（主端） |
| `RL_PLATFORM_MIV_IHC_CH21_LINK_ID` | IHC 通道 21（从端） |

**当前项目使用**：`src/middleware/rpmsg/rpmsg_lite/porting/platform/microchip/miv/rpmsg_platform.c` 中封装 IHC 作为 RPMsg 的核间通信链路。

---

### 4.2 CoreGPIO（FPGA GPIO）— 树中存在，当前未编译

**驱动文件**：`core_gpio.c` / `core_gpio.h` / `coregpio_regs.h`

| 函数 | 描述 |
|---|---|
| `CoreGPIO_init(inst)` | 初始化 FPGA GPIO 实例 |
| `CoreGPIO_set_output(inst, mask)` | 设置输出值 |
| `CoreGPIO_get_input(inst)` | 读取输入值 |

### 4.3 CorePWM（PWM 发生器）— 树中存在，当前未编译

**驱动文件**：`core_pwm.c` / `core_pwm.h` / `corepwm_regs.h`

| 函数 | 描述 |
|---|---|
| `CorePWM_init(inst, config)` | 初始化 PWM 发生器 |
| `CorePWM_set_duty(inst, ch, duty)` | 设置占空比 |
| `CorePWM_set_period(inst, period)` | 设置周期 |
| `CorePWM_start(inst)` | 启动 PWM 输出 |
| `CorePWM_stop(inst)` | 停止 PWM 输出 |

### 4.4 CoreSysServices_PF — 树中存在，当前未编译

**驱动文件**：`core_sysservices_pf.c` / `core_sysservices_pf.h`

系统服务 IP 驱动，提供 SPI 总线访问 SoC 系统服务寄存器。

| 函数 | 描述 |
|---|---|
| `CoreSysServices_init(inst)` | 初始化系统服务 |
| `CoreSysServices_read_reg(inst, addr)` | 读系统服务寄存器 |
| `CoreSysServices_write_reg(inst, addr, val)` | 写系统服务寄存器 |
| `CoreSysServices_spi_xfer(inst, data)` | 通过 SPI 传输 |

---

## 第五部分：启动代码

### 5.1 汇编启动（mss_entry.S）

**文件**：`src/platform/mpfs_hal/startup_gcc/mss_entry.S`

启动入口 `_start`，执行以下操作：

1. 读取 `mhartid` CSR 获取当前 Hart ID
2. 根据 Hart ID 设置各核独立的栈指针（`__stack_top_h0$` ~ `__stack_top_h4$`）
3. 设置 `tp`（线程指针）指向 HLS（Hart Local Storage）
4. 标记 `in_wfi_indicator` 为 `HLS_OTHER_HART_IN_WFI`（从核）
5. 跳转至 `main_other_hart()`（C 启动函数）

### 5.2 汇编辅助（mss_utils.S）

提供汇编级的工具函数，包括：
- 内存拷贝与块填充
- 段复制与清零例程
- 上下文切换辅助

### 5.3 C 启动（system_startup.c）

**详细流程**：

```
mss_entry.S (汇编)
  → init_memory()              # 复制 .text / .data 段，清零 .bss
  → __disable_all_irqs()       # 关闭所有中断

  → main_first_hart() 或 main_first_hart_app()  # E51 或主核运行
    → (可选) config_l2_cache()
    → init_memory()
    → init_bus_error_unit()
    → init_mem_protection_unit()
    → init_pmp(hart_id)
    → mss_nwc_init() + mss_nwc_init_ddr()  # 硬件初始化序列
    → PLIC_init_on_reset()
    → 依次唤醒其他从核（通过 MSIP 软件中断）
    → 调用 main_other_hart()

  → main_other_hart()          # 所有核最终到达此处
    → switch(hart_id):
        0: e51()
        1: u54_1()
        ...
```

### 5.4 Newlib 存根（newlib_stubs.c）

提供 Newlib C 库所需的最低限系统调用存根：

| 函数 | 描述 |
|---|---|
| `_sbrk()` | 堆内存增长 |
| `_write()` | 标准输出写入 |
| `_read()` | 标准输入读取 |
| `_close()` | 关闭文件描述符 |
| `_fstat()` | 文件状态 |
| `_isatty()` | 是否为终端判断 |
| `_lseek()` | 文件定位 |
| `_exit()` | 进程退出 |

---

## 第六部分：驱动启用指南

### 6.1 当前编译状态

| 驱动 | 编译状态 | 路径在 Makefile 中 |
|---|---|---|
| mss_uart | ✅ 已启用 | `SRCS +=` + `INCLUDES +=` |
| mss_gpio | ✅ 已启用 | `SRCS +=` + `INCLUDES +=` |
| miv_ihc | ✅ 已启用 | `SRCS +=` + `INCLUDES +=` |
| 其余 mss_* | ❌ 未启用 | 树中存在，需自行添加 |
| CoreGPIO, CorePWM | ❌ 未启用 | 树中存在，需自行添加 |

### 6.2 启用新驱动的方法

在 `src/platform/Makefile` 中添加：

```makefile
# 示例：启用 MSS SPI 驱动
SRCS += \
    src/platform/drivers/mss/mss_spi/mss_spi.c \

INCLUDES += \
    -Isrc/platform/drivers/mss/mss_spi \
```

---

## 附录：关键硬件地址映射

| 外设 | 基地址 | 大小 | 说明 |
|---|---|---|---|
| CLINT | 0x02000000 | 16 KB | 核间中断与定时器 |
| PLIC | 0x0C000000 | 4 MB | 平台级中断控制器 |
| MMUART0 | 0x10000000 | 4 KB | 控制台串口 |
| MMUART1 | 0x10010000 | 4 KB | RPMsg 主核串口 |
| MMUART2 | 0x10020000 | 4 KB | 通用串口 |
| MMUART3 | 0x10030000 | 4 KB | RPMsg 从核串口 |
| MMUART4 | 0x10040000 | 4 KB | 通用串口 |
| GPIO0 | 0x10012000 | 4 KB | MSS GPIO0 |
| GPIO1 | 0x10013000 | 4 KB | MSS GPIO1 |
| SPI0~3 | 0x10008000~ | 4 KB each | SPI 控制器 |
| I2C0~3 | 0x1000A000~ | 4 KB each | I²C 控制器 |
| TIMER | 0x10008000 | 4 KB | MSS 定时器 |
| WDT | 0x10001000 | 4 KB | 看门狗 |
| QSPI | 0x10005000 | 4 KB | QSPI 控制器 |
| ENVM | 0x20220000 | 128 KB | 嵌入式非易失存储器 |
| DDR | 0x1000000000 | 1 GB | DDR4 缓存（38位） |
| DDR | 0x91C00000 | 1 MB | DDR4 缓存（32位，当前使用） |
| IHC | FPGA 构造 | — | MIV IHC 核间通信 IP |
