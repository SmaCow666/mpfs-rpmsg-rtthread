# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [v0.1] - 2026-06-15

### Added

- **RT-Thread Nano 移植骨架**: 在 `src/middleware/RTThread/` 下创建了完整的 RT-Thread Nano 移植骨架，仿照现有 FreeRTOS 移植架构设计。

  - **git subtree 引入**: `rt-thread/` 目录为 RT-Thread Nano 源码位置，通过 `git subtree` 从 `https://github.com/RT-Thread/rtthread-nano.git` 引入。
    > 当前为骨架 stubs（提供类型声明和 API 接口），完整的 subtree 拉取需网络访问 GitHub。
    > 完成命令: `git subtree add --prefix=src/middleware/RTThread/rt-thread https://github.com/RT-Thread/rtthread-nano.git master --squash`

  - **构建系统集成** (`src/middleware/RTThread/Makefile`):
    - 新增 `RTThread/Makefile`，通过 `SRCS +=` / `ASM_SRCS +=` / `INCLUDES +=` 模式集成
    - 在 `src/middleware/Makefile` 中新增 `include src/middleware/RTThread/Makefile`

  - **配置文件** (`src/middleware/config/rtconfig.h`):
    - 优先级数 31，Tick 频率 1000Hz，堆大小 512KB
    - 启用信号量/互斥量/事件/邮箱/消息队列等 IPC 机制
    - 控制台输出支持，libc 支持

  - **板级支持** (`board/board.c` / `board.h`):
    - OS Tick 定时器初始化（CLINT MTimer，复用 FreeRTOS 相同的寄存器地址）
    - 堆内存初始化
    - 板级硬件初始化框架

  - **CPU 移植层** (`portable/rtt_port.h` + `libcpu/risc-v/rv64/`):
    - `rtt_port.h`: 移植宏定义（临界区管理、类型定义、栈增长方向）
    - `context_switch.S`: 汇编上下文切换（rt_hw_context_switch, rt_hw_context_switch_to, rt_hw_interrupt_disable/enable）
    - `cpuport.c`: 栈初始化（rt_hw_stack_init）

  - **RPMsg-Lite 环境适配** (`rpmsg_env_rtthread.c`): 占位文件，待后续实现

- **架构分析文档** (`doc/freertos-port-architecture-analysis.md`):
  - 详细梳理了 FreeRTOS 移植架构（目录组织、构建系统、CPU 移植层、配置管理、应用层启动流程）
  - 提供了 RT-Thread Nano 移植的 7 条设计模式总结

### Changed

- `src/middleware/Makefile`: 新增 `include src/middleware/RTThread/Makefile`

### Notes

- 网络限制导致 `git subtree add` 未能完成，`rt-thread/` 目录当前是手动创建的骨架文件
- 后续需在有 GitHub 网络访问的环境下运行 `git subtree add` 以获取完整 Nano 源码
- 链接脚本中需添加 `.RTThread.heap` 段（当前沿用 FreeRTOS 的分配方式）

***

## [v0.0] - Baseline

- 初始导入 mpfs-rpmsg-freertos 基项目（PolarFire SoC FreeRTOS + RPMsg AMP 示例）
