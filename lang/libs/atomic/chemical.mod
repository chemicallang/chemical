module atomic
import std

source "src"
source "arch/x86" if x86_64 or i386
source "arch/aarch64" if aarch64
source "arch/riscv" if riscv
source "arch/arm" if arm
source "arch/powerpc" if powerpc or powerpc64
