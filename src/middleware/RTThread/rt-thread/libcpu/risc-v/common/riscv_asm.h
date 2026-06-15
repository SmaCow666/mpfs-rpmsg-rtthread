/* riscv_asm.h - RISC-V assembly macros for RT-Thread Nano port */
#ifndef __RISCV_ASM_H__
#define __RISCV_ASM_H__

#if __riscv_xlen == 64
# define STORE    sd
# define LOAD     ld
# define REGBYTES 8
#else
# define STORE    sw
# define LOAD     lw
# define REGBYTES 4
#endif

#endif /* __RISCV_ASM_H__ */
