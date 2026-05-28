# ParticleSIMD — Benchmark

## Environment

| Parameter | Value |
|---|---|
| Device | Motorola Moto G84 |
| SoC | Qualcomm Snapdragon 695 (6 nm) |
| CPU | 2x Kryo 660 Gold (Cortex-A78) @ 2.2 GHz + 6x Kryo 660 Silver (Cortex-A55) @ 1.8 GHz |
| ISA | ARMv8.2-A (AArch64) |
| SIMD | ARM NEON (128-bit registers, 32 vector registers) |
| Particles | 7168 |
| Threads | 8 |
| Compiler | Clang 21.1.8 |
| NDK | r29 |
| Flags | `-O3 -march=armv8-a+simd` |
| Renderer | Active in all tests |
| Temperature | No perceptible throttling (short runs) |
| Core affinity | Not pinned — known source of variance |

> **Note on core affinity:** The Snapdragon 695 uses a big.LITTLE topology.
> Without CPU affinity pinning, the Android scheduler may migrate threads between
> Cortex-A78 (P-cores) and Cortex-A55 (E-cores), causing occasional FPS drops.
> This is reflected in the standard deviation of the SIMD results.

---

## Explicit SIMD (ARM NEON Intrinsics)

**Raw samples (FPS):**
```
31 47 48 43 48 47 48 47 48 48 46 47 48 48 47 47 48 47 47
```

| Statistic | Full | Without warmup (frame 1 discarded) |
|---|---|---|
| Samples | 19 | 18 |
| Warmup (frame 1) | 31 | — |
| Min | 31 | 43 |
| Max | 48 | 48 |
| Mean | 46.32 | 47.17 |
| Median | — | 47.0 |
| Std. deviation | — | 1.20 |

> Frame 1 (31 FPS) is a warmup outlier — cold cache and threads waking up.

---

## Scalar (Compiler auto-vectorized)

**Raw samples (FPS):**
```
24 38 40 39 40 40 40 40 40 39 40 40 40 41 40 40 40 39 39 39 40
```

| Statistic | Full | Without warmup (frame 1 discarded) |
|---|---|---|
| Samples | 21 | 20 |
| Warmup (frame 1) | 24 | — |
| Min | 24 | 38 |
| Max | 41 | 41 |
| Mean | 38.95 | 39.70 |
| Median | — | 40.0 |
| Std. deviation | — | 0.66 |

> Lower standard deviation than explicit SIMD — less sensitive to scheduling variance,
> likely due to lower register pressure.

---

## Comparison

| Implementation | Mean (no warmup) | Std. deviation |
|---|---|---|
| Scalar (`-O3`, auto-vectorized) | 39.70 FPS | 0.66 |
| Explicit SIMD (intrinsics) | 47.17 FPS | 1.20 |
| **Relative gain** | **+18.8%** | — |

---

## Register Allocator Analysis (`-Rpass-missed=regalloc`)

Compiled with Clang 21.1.8 under release flags. Relevant output:

```
_update_simd (line 12):
  0 spills — 4 virtual register copies

i loop (line 20):
  0 spills — 3 virtual register copies

Simulation::update (line 172):
  1 spill, 1 reload — outside hot path

Simulation::Simulation (line 137):
  8 spills, 5 reloads — constructor, outside hot path
```

**Conclusion:** The critical loop (`_update_instrinsics`) produces no spills with `j+=8` unrolling.
Clang successfully allocated all intermediate variables within the 32 available NEON physical registers.
Reported spills occur exclusively outside the hot path and have no runtime performance impact.

> Register pressure exists in the loop but stays within bounds.
> Further unrolling (`j+=16`) would likely yield neutral or negative returns —
> the current bottleneck is memory throughput, not pipeline dependencies.

**Background — virtual vs physical registers:**
The compiler internally assigns one virtual register per intermediate value, treating them as unlimited.
The register allocator then maps virtual registers onto the 32 physical NEON registers (`v0`–`v31`).
When mapping is not possible:
- **Copy** — a value is moved to another free physical register. Resolved entirely within registers, nearly free due to hardware register renaming.
- **Spill** — a value is evicted to the stack (store + reload). Involves memory access and costs real cycles.

---

## Methodological Notes

- Each implementation was compiled as a separate build.
- Multiple runs were performed; samples above are representative.
- The renderer was active under identical conditions across all tests.
- Core affinity was not pinned — occasional FPS drops are expected and documented.
- The gain of explicit SIMD over auto-vectorized code does not come from vectorization itself
  (the compiler already vectorizes with `-O3`), but from hardware-specific latency knowledge:
  `vdivq_f32` runs in parallel with the Newton-Raphson chain via out-of-order execution,
  a tradeoff the compiler cannot infer statically from its instruction cost tables.