#!/usr/bin/env python3
"""Builds bench_render.c + the whole game for the Cortex-M7 and runs it in Unicorn,
counting instructions per rendered frame (1 instruction ~ 1 cycle at 216 MHz, the same
convention as tools/emu.py). Also reports deepest stack use and a checksum per frame,
so two builds can be checked for pixel-identical output.

Usage: bench_render.py [--src DIR] [OUTDIR]
  --src DIR  game sources (default: the tree's games/numblocks/src); pass a copy
             with reverted optimizations for a before/after comparison.
"""
import os
import subprocess
import sys

from elftools.elf.elffile import ELFFile
from unicorn import UC_ARCH_ARM, UC_MODE_MCLASS, UC_MODE_THUMB, Uc
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_SP

HERE = os.path.dirname(os.path.abspath(__file__))
TREE_SRC = os.path.normpath(os.path.join(HERE, "..", "..", "..", "src"))
args = [a for a in sys.argv[1:] if not a.startswith("--")]
SRC = TREE_SRC
if "--src" in sys.argv:
    SRC = sys.argv[sys.argv.index("--src") + 1]
    args = [a for a in args if a != SRC]
OUT = args[0] if args else "/tmp"
os.makedirs(OUT, exist_ok=True)
SEED = 12345
if "--seed" in sys.argv:
    SEED = int(sys.argv[sys.argv.index("--seed") + 1])
FANCY = 1 if "--fancy" in sys.argv else 0
RAIN = 1 if "--rain" in sys.argv else 0
NMOBS = 1 if "--mobs" in sys.argv else 0
NTICKS = 1 if "--ticks" in sys.argv else 0

ELF = os.path.join(OUT, "render_bench.elf")
GAME = ["world.c", "edits.c", "portal.c", "render.c", "player.c", "phys.c", "inv.c", "entity.c", "mob.c",
        "tick.c", "gui.c", "hand.c", "save.c", "data.c", "panorama.c", "names.c", "command.c", "gen.c"]
# the device build's flags (games/numblocks/Makefile), so the code matches the calculator's
CFLAGS = ["-std=gnu11", "-mcpu=cortex-m7", "-mthumb", "-mfloat-abi=hard", "-mfpu=fpv5-sp-d16", "-O2",
          "-fno-math-errno", "-fsingle-precision-constant", "-ffast-math", "-fno-tree-loop-distribute-patterns",
          "-flto", "-ffunction-sections", "-fdata-sections", "-g", "-Wall", "-Wextra", "-Wno-unused-parameter",
          f"-I{SRC}"]
subprocess.check_call(["arm-none-eabi-gcc", *CFLAGS, "-nostartfiles", "--specs=nano.specs", "--specs=nosys.specs",
                       "-T", os.path.join(HERE, "bench.ld"), "-Wl,--gc-sections",
                       os.path.join(HERE, "bench_render.c"), *[os.path.join(SRC, f) for f in GAME],
                       "-lm", "-o", ELF, f"-DSEED={SEED}LL", f"-DFANCY={FANCY}", f"-DRAIN={RAIN}",
                       f"-DNMOBS={NMOBS}", f"-DNTICKS={NTICKS}"])
print("built", ELF)
subprocess.call(["arm-none-eabi-size", ELF])

syms = {}
with open(ELF, "rb") as f:
    elf = ELFFile(f)
    for s in elf.get_section_by_name(".symtab").iter_symbols():
        syms[s.name] = s["st_value"]
    segs = [(p["p_paddr"], p.data()) for p in elf.iter_segments() if p["p_type"] == "PT_LOAD"]

uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
uc.mem_map(0x08000000, 1 << 20)
uc.mem_map(0x20000000, 512 << 10)
for addr, data in segs:
    uc.mem_write(addr, data)
STACK_TOP = 0x20000000 + (512 << 10)
PAINT = 64 << 10
uc.mem_write(STACK_TOP - PAINT, b"\xa5" * PAINT)
vec = uc.mem_read(0x08000000, 8)
sp = int.from_bytes(vec[0:4], "little")
pc = int.from_bytes(vec[4:8], "little")
uc.reg_write(UC_ARM_REG_SP, sp)

CHUNK = 21600
PROFILE = "--profile" in sys.argv
phase_addr = syms["phase"]
funcs = []
for s in ELFFile(open(ELF, "rb")).get_section_by_name(".symtab").iter_symbols():
    if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
        funcs.append((s["st_value"] & ~1, s["st_size"], s.name))
per = {}
prof = {}
insns = 0
while True:
    try:
        uc.emu_start(pc | 1, 0xFFFFFFFF, count=CHUNK)
        insns += CHUNK
        pc = uc.reg_read(UC_ARM_REG_PC)
        ph = int.from_bytes(uc.mem_read(phase_addr, 4), "little")
        per[ph] = per.get(ph, 0) + CHUNK
        if PROFILE and 1 <= ph < 1000:
            key = (ph, pc & ~1)
            prof[key] = prof.get(key, 0) + 1
    except Exception as e:
        p = uc.reg_read(UC_ARM_REG_PC)
        if uc.mem_read(p & ~1, 2) == b"\x00\xbe":  # bkpt: done
            break
        print(f"fault {e} at pc {p:#x} after ~{insns} insns", file=sys.stderr)
        raise
    if uc.mem_read(pc & ~1, 2) == b"\x00\xbe":  # bkpt
        break
    if insns > 20_000_000_000:
        break

NAMES = ([f"eye yaw {k * 45}" for k in range(8)] + ["high down 0", "high down 90"]
         + [f"high -45 yaw {k * 90}" for k in range(4)] + ["high flat 0", "high flat 180"]
         + ["eye -45", "eye -60"])
total = 0
nframes = 0
for ph in sorted(per):
    d = per[ph]
    if ph == 0 or ph >= 1000:
        print(f"settle    {d / 1e6:8.2f} M insns  ~{d / 216e3:7.1f} ms at 216 MHz")
        continue
    name = NAMES[ph - 1] if 1 <= ph <= len(NAMES) else (f"tick {ph - 100}" if ph >= 100 else f"phase {ph}")
    total += d
    nframes += 1
    print(f"frame {ph:2d} {name:14s} {d / 1e6:8.2f} M insns  ~{d / 216e3:7.1f} ms at 216 MHz")
print(f"all {nframes} frames: {total / 1e6:.2f} M insns = {total / 216e3:.1f} ms")
st = uc.mem_read(STACK_TOP - PAINT, PAINT)
used = PAINT - next(i for i in range(PAINT) if st[i] != 0xA5)
print(f"deepest stack: {used} bytes")
n = int.from_bytes(uc.mem_read(syms["phase"], 4), "little")
sums = uc.mem_read(syms["sums"], 4 * 64)
px = uc.mem_read(syms["npixels"], 4 * 64)
import struct
print("checksums:", " ".join(f"{struct.unpack('<I', sums[4 * i:4 * i + 4])[0]:08x}" for i in range(1, n + 1)))
print("pixels:   ", " ".join(f"{struct.unpack('<I', px[4 * i:4 * i + 4])[0]}" for i in range(1, n + 1)))

if PROFILE:
    import bisect
    funcs.sort()
    starts = [f[0] for f in funcs]

    def fname(addr):
        i = bisect.bisect_right(starts, addr) - 1
        if 0 <= i < len(funcs) and addr < funcs[i][0] + funcs[i][1]:
            return funcs[i][2]
        return "???"

    # heavy frames only (over 40 ms): top functions each
    for ph in sorted({k[0] for k in prof}):
        agg = {}
        for (p, addr), c in prof.items():
            if p != ph:
                continue
            name = fname(addr)
            agg[name] = agg.get(name, 0) + c
        tot = sum(agg.values()) * CHUNK
        if tot / 216e3 < 40:
            continue
        print(f"--- frame {ph} ({tot / 216e3:.1f} ms) top ---")
        for name, c in sorted(agg.items(), key=lambda kv: -kv[1])[:12]:
            print(f"  {c * CHUNK / tot * 100:5.1f}% {name}")
