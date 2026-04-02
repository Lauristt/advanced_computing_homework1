# Homework 1: Fast Matrix Summation

## Problem

Sum all elements of a 4096×4096 integer matrix as fast as possible, applying:
- **Inlining** — eliminate function call overhead
- **Cache locality** — exploit row-major memory layout
- **Pointer manipulation** — reduce indexing overhead
- **SIMD** (bonus) — use ARM NEON / x86 AVX2 intrinsics

---

## Files

| File | Description |
|------|-------------|
| `Fast_matrix_summation_base.cpp` | Starter code with `getElement` / `add` helper functions |
| `Fast_matrix_summation.cpp` | Optimized implementation (pointer unroll + SIMD) |

---

## Build & Run

```bash
# With compiler optimization (-O2)
clang++ -std=c++17 -O2 -o fast_matrix_summation_base Fast_matrix_summation_base.cpp
clang++ -std=c++17 -O2 -o fast_matrix_summation      Fast_matrix_summation.cpp

./fast_matrix_summation_base
./fast_matrix_summation

# Without compiler optimization (-O0) — reveals manual optimization impact most clearly
clang++ -std=c++17 -O0 -o fast_matrix_summation_base_O0 Fast_matrix_summation_base.cpp
clang++ -std=c++17 -O0 -o fast_matrix_summation_O0      Fast_matrix_summation.cpp

./fast_matrix_summation_base_O0
./fast_matrix_summation_O0
```

---

## Benchmark Results (Apple M-series, SIZE=4096)

### Without Compiler Optimization (`-O0`) — Manual Optimizations Matter Most

| Method | Time (ms) | Speedup vs Basic |
|--------|-----------|-----------------|
| Basic (getElement + add calls) | ~65 ms | 1× |
| Optimized (pointer + 8× unroll) | ~5 ms | **~13×** |
| SIMD NEON | ~31 ms | ~2× |

> At `-O0`, the compiler performs no auto-vectorization, so every optimization is purely from hand-written code. The unrolled pointer version achieves a **~13× speedup** over the baseline.

### With Compiler Optimization (`-O2`) — Compiler Closes the Gap

| Method | Time (ms) | Speedup vs Basic |
|--------|-----------|-----------------|
| Basic (getElement + add calls) | ~1.5–2.4 ms | 1× |
| Optimized (pointer + 8× unroll) | ~1.5–2.4 ms | ~1× |
| SIMD NEON | ~2.4–3.1 ms | slightly slower |

> At `-O2`, the compiler inlines functions and auto-vectorizes loops automatically, so the manual optimizations show little additional gain. The SIMD version is slightly slower here because the compiler's auto-vectorizer already handles the scalar loop well and the manual NEON intrinsic path incurs extra overhead.

**Target Execution Time:** Under **10 ms** at `-O0` (the optimized version consistently achieves ~5 ms).

---

## Optimizations Explained

### 1. Inlining (`getElement` and `add`)

The baseline calls `getElement(matrix, i, j)` and `add(sum, elem)` for every single element — that is **2 × 4096 × 4096 = 33,554,432 function calls** per summation. Each call incurs:
- Stack frame setup/teardown
- Register save/restore
- Branch prediction disruption

The optimized version eliminates both helper functions entirely. Accessing `matrix[i][j]` directly (or via raw pointer) costs no extra frames.

### 2. Cache Locality (Row-Major Access)

`std::vector<std::vector<int>>` stores each row as a separate heap allocation. Within a single row, elements **are** contiguous in memory (guaranteed by the C++ standard for `std::vector`). Iterating row by row (`i` outer, `j` inner) accesses each row's data sequentially, maximizing cache-line utilization.

A column-major traversal (`j` outer, `i` inner) would jump between separate heap allocations on every inner step, causing frequent cache misses.

### 3. Pointer Arithmetic

Instead of `matrix[i][j]` (which involves two pointer dereferences and bounds-check overhead in debug mode), the optimized version uses:

```cpp
const int* p = matrix[i].data();
```

`data()` returns a raw pointer to the first element of the row. Incrementing `p` steps through contiguous integers with zero indexing overhead.

### 4. Loop Unrolling (8×)

The inner loop processes **8 elements per iteration**:

```cpp
while (p + 8 <= row_end) {
    sum += p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7];
    p += 8;
}
```

This reduces loop-control overhead (branch checks, counter increments) by 8×. It also exposes instruction-level parallelism — modern out-of-order processors can execute multiple additions simultaneously when they see no dependency between them.

A scalar cleanup loop handles any remainder elements (SIZE=4096 is divisible by 8, so the remainder is always 0 here).

### 5. SIMD (ARM NEON / x86 AVX2) — Bonus

The `sumMatrixSIMD2D` function uses platform-specific SIMD intrinsics:

- **ARM NEON** (`__aarch64__`): processes 4 × `int32` per `vld1q_s32` / `vaddq_s32` instruction
- **x86 AVX2**: processes 8 × `int32` per `_mm256_loadu_si256` / `_mm256_add_epi32` instruction

At `-O0`, the NEON version runs at ~31 ms — slower than the 8× unrolled scalar version (~5 ms) because the SIMD path still has loop overhead that the compiler cannot optimize away at `-O0`. At `-O2`, the compiler's own auto-vectorizer outperforms both.

---

## Key Takeaways

| Principle | Impact (at -O0) |
|-----------|----------------|
| Eliminate helper function calls | Removes 33M+ call/return pairs |
| Row-major pointer traversal | Keeps all accesses cache-line sequential |
| 8× manual unrolling | Reduces loop overhead by 8×, enables ILP |
| SIMD intrinsics | Useful when compiler cannot auto-vectorize; at -O2 the compiler often does better |

The most important lesson: **function call overhead and memory access patterns dominate performance at low optimization levels**. At `-O2`, the compiler handles inlining and vectorization automatically, which is why production code is almost always compiled with `-O2` or `-O3`.
