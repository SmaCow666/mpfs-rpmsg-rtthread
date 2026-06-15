# PolarFire SoC MPFS250T ICE-KIT 外设清单与驱动适配进度

> **文档说明**
> 1. 本清单基于 Microchip 官方技术手册整理，分为 MSS 内置硬化外设、板级外围器件与扩展接口两大部分。
> 2. MSS 内置外设新增「适配状态」列，用于跟踪驱动开发进度：`- [ ]` 代表未启动适配，`- [x]` 代表已完成适配。
> 3. 所有参数均来自官方手册，引用来源标注于对应条目。
> 4. 适用板卡：PolarFire SoC Icicle Kit（MPFS250T）

---

## 一、MSS 内置硬化外设

### 1.1 处理器核心

| 外设名称 | 参数/说明 | 原文引用 | 适配状态 |
| :--- | :--- | :--- | :--- |
| RISC-V 多核子系统 | 1×E51 监控核（RV64IMAC）+ 4×U54 应用核（RV64GC，支持 Sv39 MMU） | 1. 《PolarFire_SoC_Icicle_Kit_QuickStart_Guide_50003930.pdf》Introduction：combines a hardened RISC-V multi-core processor subsystem with low-power FPGA fabric.<br>2. 《PolarFire_SoC_FPGA_MSS_Technical_Reference_Manual_VC.pdf》第10章 CPU Core Complex Address Space：Hart Accessibility 标注为 Hart0–4，对应1个监控核+4个应用核 | - [ ] 未适配 |

### 1.2 存储控制器

| 外设名称 | 参数/说明 | 原文引用 | 适配状态 |
| :--- | :--- | :--- | :--- |
| DDR 控制器 | 支持 DDR3/DDR3L/DDR4/LPDDR3/LPDDR4，板载 LPDDR4 连接至此 | 《PolarFire_SoC_FPGA_MSS_Technical_Reference_Manual_VC.pdf》Table 1-1 | - [ ] 未适配 |
| QSPI-XIP 控制器 | 1×，支持串行 Flash 就地执行，板载 1Gb SPI Flash 连接至此 | 同上 Table 1-1：QSPI-XIP (1x) | - [ ] 未适配 |
| eMMC/SD/SDIO 控制器 | 1×，支持 eMMC 5.1、SD、SDIO，板载 8GB eMMC 和 SD 卡座连接至此 | 同上 Table 1-1：eMMC 5.1 (1x), SD (1x), and SDIO (1x) | - [ ] 未适配 |
| eNVM Controller | 1×，嵌入式非易失性存储器控制器，用于存储启动代码 | 《PolarFire_SoC_FPGA_MSS_Technical_Reference_Manual_VC.pdf》3.12节：The MSS includes the following peripherals: ... eNVM Controller ... | - [ ] 未适配 |

### 1.3 通信外设

| 外设名称 | 参数/说明 | 原文引用 | 适配状态 |
| :--- | :--- | :--- | :--- |
| MMUART | 5×，引脚路由至 MSS Bank2/Bank4，对应 MMUART0~4 | 1. 同上 Table 1-1：MMUART (5x)<br>2. 《mpfs-icicle-kit-es-schematics.pdf》PolarFire SoC Bank Assignment：明确列出 MMUART0~4 的引脚定义 | - [ ] 未适配 |
| SPI 控制器 | 2×，仅 Master 模式，最高速率 HCLK/2 | 1. 同上 Table 1-1：SPI (2x)<br>2. 《roadmap-zh.md》：SPI (×2) | - [ ] 未适配 |
| I2C 控制器 | 2×，支持 Master/Slave、100Kbps/400Kbps | 1. 同上 Table 1-1：I2C (2x)<br>2. 《roadmap-zh.md》：I2C (×2) | - [ ] 未适配 |
| CAN 控制器 | 2×，32 发 / 32 收缓冲区 | 同上 Table 1-1：CAN (2x) | - [ ] 未适配 |
| 千兆以太网 MAC（GEM） | 2×，支持 SGMII、10/100/1000Mbps | 同上 Table 1-1：Gigabit Ethernet MAC (GEM 2x) | - [ ] 未适配 |
| USB 2.0 OTG 控制器 | 1×，支持 ULPI 接口 | 同上 Table 1-1：USB OTG 2.0 controller (1x) | - [ ] 未适配 |
| GPIO 块 | 3×，共 96 个 GPIO（GPIO0/GPIO1/GPIO2），用户 LED/按键连接至此 | 1. 同上 Table 1-1：GPIO (3x)<br>2. 《roadmap-zh.md》：GPIO (×96, 3 blocks) | - [ ] 未适配 |

### 1.4 定时与中断外设

| 外设名称 | 参数/说明 | 原文引用 | 适配状态 |
| :--- | :--- | :--- | :--- |
| 32 位定时器 | 2× | 同上 Table 1-1：Timer (2x32 bit) | - [ ] 未适配 |
| 看门狗 | 5× | 同上 Table 1-1：Watchdogs (5x) | - [ ] 未适配 |
| RTC 实时时钟 | 1× | 同上 Table 1-1：RTC (1x) | - [ ] 未适配 |
| FRQ Meter（频率计） | 1×，频率测量外设 | 1. 同上 Table 1-1：FRQMeter<br>2. 同上 3.12节：The MSS includes ... FRQ Meter ... | - [ ] 未适配 |
| CLINT（核本地中断控制器） | 1×，提供 mtime 定时器、软件中断 | 同上 Functional Blocks 章节：CLINT provides M Mode Timer/Software Interrupt. | - [ ] 未适配 |
| PLIC（平台级中断控制器） | 1×，管理 169 个全局中断 | 同上：PLIC manages global interrupts (169). | - [ ] 未适配 |

### 1.5 其他内置外设

| 外设名称 | 参数/说明 | 原文引用 | 适配状态 |
| :--- | :--- | :--- | :--- |
| 用户加密处理器 | 1×，Athena F5200 TeraFire | 同上 Figure 3-19 Peripherals Block Diagram：MSS 包含 Crypto Processor | - [ ] 未适配 |
| FIC（Fabric 接口控制器） | 5×：FIC0/FIC1（64位 AXI4）、FIC2（64位 AXI4）、FIC3（32位 APB）、FIC4（32位 AHB-Lite） | 同上 6.1节：FICs in PolarFire SoC FPGA are referred as FIC0, FIC1, FIC2, FIC3, and FIC4 ... 3个64位AXI4, 1个32位APB, 1个32位AHB-Lite. | - [ ] 未适配 |
| M2F 中断控制器 | 1×，管理 MSS 与 FPGA Fabric 的中断路由 | 同上 3.12节：M2F Interrupt Controller | - [ ] 未适配 |

---

## 二、板级外围器件与扩展接口

### 2.1 存储类

| 器件名称 | 型号/参数 | 连接接口 | 原文引用 |
| :--- | :--- | :--- | :--- |
| LPDDR4 内存 | Micron MT53D512M32D2DS-053，16Gb，800MHz | MSS Bank6 | 《microchip_polarfire_soc_fpga_icicle_kit_user_guide_vb.pdf》3.1节：LPDDR4 is connected to the MSS BANK 6. Part number: MT53D512M32D2DS-053 WT:D TR, 16 Gb, 800 MHz. |
| SPI Flash | Micron MT25QL01GBBB8ESF-0SIT，1Gb | MSS Bank3 | 同上 3.2节：1 Gb SPI flash, Part number: MT25QL01GBBB8ESF-0SIT, connected to BANK3 SC-SPI pins. |
| eMMC 存储 | SanDisk SDINBDG4-8G，8GB，eMMC 5.1 | MSS eMMC 控制器 | 同上 Table 1-3：eMMC U45, SDINBDG4-8G, Memory size: 8 GB. |
| Micro SD 卡座 | Amphenol 10067847-001RLF，支持 UHS-I | MSS SD 控制器 | 同上 3.3.2节：SD card connector Part number: 10067847-001RLF, supports UHS-I. |

### 2.2 通信类

| 器件名称 | 型号/参数 | 连接接口 | 原文引用 |
| :--- | :--- | :--- | :--- |
| 千兆以太网接口 | VSC8662 PHY + 2×RJ45（J1/J2），支持 10/100/1000Mbps | MSS GEM 控制器 | 同上 3.5.1节：VSC8662 device is a low-power, dual Gigabit Ethernet transceiver, 2个RJ45 connectors J1/J2. |
| USB 2.0 OTG 接口 | USB3340 PHY + J16 Micro USB AB | MSS USB OTG 控制器 | 同上 3.5.3节：USB3340 is a Hi-Speed USB 2.0 Transceiver, J16 Micro USB connector. |
| 4 路 USB-UART 桥 | Silicon Labs CP2108-B02-GM + J11 Micro USB AB | FPGA Bank1 | 同上 3.9.1节：CP2108 is a USB to quad UART bridge controller, J11 connector, UART IOs are connected to the Fabric IOs (Bank1). |
| CAN 接口 | MCP2562FDT-E/SN + J25/J27 3pin 接头 | MSS CAN 控制器 | 同上 3.5.2节：Two CAN interfaces, Part number: MCP2562FDT-E/SN, J25/J27. |
| PCIe x16 Root Port 连接器 | Amphenol 10025026-10103TLF，4 路 SerDes 通道 | MSS SerDes | 同上 3.4.2节：PCIx16 Connector J6, 4-TX/RX pairs are connected to SERDES block. |

### 2.3 扩展接口

| 接口名称 | 规格参数 | 连接接口 | 原文引用 |
| :--- | :--- | :--- | :--- |
| mikroBUS 扩展接口 | 2 个 16pin 接口（J44/J8），支持 UART/SPI/I2C | FPGA IO | 同上 3.6.2节：Two mikroBUS interface connectors J44/J8, support UART/SPI/I2C. |
| 40pin RPi 4 接口 | Wurth 61204021621，兼容树莓派 4 引脚定义 | GPIO Bank1 / Bank9 | 同上 3.6.1节：40 pin Raspberry Pi 4 interface connector J26, signals use GPIO BANK1 and BANK9. |

### 2.4 调试与编程

| 器件/接口名称 | 规格参数 | 原文引用 |
| :--- | :--- | :--- |
| 板载 FlashPro6 编程器 | U26，通过 J33 USB 连接 PC | 同上 Getting Started：The on-board FlashPro6 programmer is used to develop and debug embedded applications. |
| 外部 JTAG 编程头 | J23，10pin，支持外接 FlashPro 4/5/6 | 同上 Table 1-3：JTAG programming header J23, 10-pin, for external FlashPro. |

### 2.5 时钟源

| 时钟源 | 规格参数 | 连接对象 | 原文引用 |
| :--- | :--- | :--- | :--- |
| 125MHz MSS 参考时钟 | DSC1123BL5-125.0000，LVDS 输出 | MSS 子系统 | 同上 Table 1-3：On-board 125 MHz oscillator X2, LVDS output, MSS Reference clk input. |
| 50MHz FPGA 参考时钟 | DSC1001DL5-050.0000，单端输出 | FPGA 逻辑资源 | 同上 3.13节：On-board 50 MHz oscillator X5, single-ended output, connected to FPGA fabric. |

### 2.6 用户交互

| 器件名称 | 规格参数 | 连接接口 | 原文引用 |
| :--- | :--- | :--- | :--- |
| 用户 LED | LED1-LED4，共 4 个，低电平点亮 | HSIO Bank0 | 同上 3.8.2节：Four active-high LEDs LED1-LED4, connected to HSIO BANK0. |
| 用户按键 | SW1-SW4，共 4 个，按下为低电平 | HSIO Bank0 | 同上 3.8.1节：Four tact switches SW1-SW4, connected to HSIO BANK0. |

### 2.7 电源与复位

| 器件/接口名称 | 规格参数 | 原文引用 |
| :--- | :--- | :--- |
| 电源监控芯片 | PAC1934T-I/JQ，I2C 接口，监控 4 路电源电流 | 同上 3.7节：Current sensing is done by PAC1934T-I/JQ, I2C interface, monitor VDD/VDD25/VDDA25/VDDA. |
| 电源状态 LED | 共 9 个：12P0/5P0/2P5V/VDDAUX4/3P3V/VDD/1P8/1P1V_LPDDR4/VDDA | 同上 Table 2-2：Power Supply LEDs list. |
| 电源输入与开关 | 12V/5A DC 输入接口 J29 + SW6 电源开关 | 同上 Table 1-3：12V power supply input J29, ON/OFF switch SW6. |
| 系统复位芯片 | MCP121T-315E，生成 DEVRST_N 复位信号 | 《mpfs-icicle-kit-es-schematics.pdf》Programming Section：MCP121T-315E/OT, generate PF SoC DEVRST N. |
| 多路电源调节器 | MIC26950（5V/3.3V）、MIC22705（1V VDD）等 | 同上 Table 2-5：Power Regulators list. |