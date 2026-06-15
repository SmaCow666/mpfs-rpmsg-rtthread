/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * portable/cpuport.c - Project-specific CPU port extensions.
 *
 * This file is for PolarFire SoC-specific port hooks only.
 * The standard RISC-V rv64 port (rt_hw_stack_init, context switch helpers)
 * lives in rt-thread/libcpu/risc-v/rv64/cpuport.c (from git subtree).
 *
 * Responsibilities (when populated):
 *   - PLIC interrupt controller interface for RT-Thread
 *   - CLINT timer abstraction override if needed
 *   - Performance counter access
 */
#include <rtthread.h>
#include <rthw.h>

/* Placeholder for PolarFire SoC-specific port extensions.
 * Add PLIC integration, cache management, etc. here in future versions. */
