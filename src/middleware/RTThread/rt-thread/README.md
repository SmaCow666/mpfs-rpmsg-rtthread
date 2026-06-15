#
# RT-Thread Nano - Git Subtree Management
#
# This directory (rt-thread/) is intended to be managed via git subtree.
# The authoritative upstream repository is:
#
#   https://github.com/RT-Thread/rtthread-nano.git
#
# Current status: The rt-thread/ directory currently contains scaffold
# stubs created during initial port skeleton setup (v0.1).
# These stubs provide the type/API declarations and context switch
# assembly needed for compilation, but the production implementation
# should come from the upstream Nano repository.
#
# To complete the subtree setup, run:
#
#   git subtree add --prefix=src/middleware/RTThread/rt-thread \
#       https://github.com/RT-Thread/rtthread-nano.git master --squash
#
# To update later (once subtree is registered):
#
#   git subtree pull --prefix=src/middleware/RTThread/rt-thread \
#       https://github.com/RT-Thread/rtthread-nano.git master --squash
#
# NOTE: The initial subtree add requires network access to GitHub.
# If network is unavailable (as during initial scaffold creation),
# manually place the Nano source files into this directory following
# the structure:
#
#   rt-thread/
#   ├── src/                    # Kernel source (clock.c, thread.c, ...)
#   ├── include/                # Kernel headers (rtdef.h, rtthread.h, rthw.h)
#   └── libcpu/risc-v/         # CPU port files (context_switch.S, cpuport.c)
#       ├── rv64/
#       └── common/
