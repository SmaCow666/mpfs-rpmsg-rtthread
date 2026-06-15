#
# RT-Thread Nano support
#
# Mimics the FreeRTOS Makefile integration pattern:
#   kernel src -> SRCS, asm port -> ASM_SRCS, includes -> INCLUDES

SRCS += \
	src/middleware/RTThread/rt-thread/src/clock.c \
	src/middleware/RTThread/rt-thread/src/components.c \
	src/middleware/RTThread/rt-thread/src/device.c \
	src/middleware/RTThread/rt-thread/src/idle.c \
	src/middleware/RTThread/rt-thread/src/ipc.c \
	src/middleware/RTThread/rt-thread/src/irq.c \
	src/middleware/RTThread/rt-thread/src/kservice.c \
	src/middleware/RTThread/rt-thread/src/mem.c \
	src/middleware/RTThread/rt-thread/src/object.c \
	src/middleware/RTThread/rt-thread/src/scheduler.c \
	src/middleware/RTThread/rt-thread/src/thread.c \
	src/middleware/RTThread/rt-thread/src/timer.c \
	src/middleware/RTThread/board/board.c \
	src/middleware/RTThread/portable/cpuport.c \

ASM_SRCS += \
	src/middleware/RTThread/rt-thread/libcpu/risc-v/rv64/context_switch.S \

INCLUDES += \
	-Isrc/middleware/RTThread/rt-thread/include \
	-Isrc/middleware/config \
	-Isrc/middleware/RTThread/portable \
	-Isrc/middleware/RTThread/board \
