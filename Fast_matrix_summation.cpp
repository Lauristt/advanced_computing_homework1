#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <random>
//for x86 windows
#if defined(__AVX2__)
#include <immintrin.h>
//for apple silicon or linux arm64
#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#endif

const int SIZE = 4096;

// Baseline: 2D layout, direct indexing (no pointer tricks)
long long sumMatrixBasic(const std::vector<std::vector<int>>& matrix) {
    long long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            sum += matrix[i][j];
        }
    }
    return sum;
}

// Optimized: per-row contiguous scan via raw pointers + 8x unroll (no extra allocation; 2D storage unchanged).
// Whole-matrix single-pointer layout would need a flat buffer owned by the caller.
// Flattened version of the matrix will not be faster than this version, because the cache is not fully utilized.
long long sumMatrixOptimized(const std::vector<std::vector<int>>& matrix) {
    long long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
        const int* p = matrix[i].data();
        const int* const row_end = p + SIZE;
        while (p + 8 <= row_end) {
            sum += p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7];
            p += 8;
        }
        while (p != row_end) {
            sum += *p++;
        }
    }
    return sum;
}

//Some test using AVX2, just wanted to see if it would be faster than the optimized version
#if defined(__AVX2__)
long long sumMatrixSIMD2D(const std::vector<std::vector<int>>& matrix) {
    long long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
        const std::vector<int>& row = matrix[i];
        const int remainder = SIZE % 8;
        const int simd_end = SIZE - remainder;
        __m256i vsum = _mm256_setzero_si256();
        for (int j = 0; j < simd_end; j += 8) {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&row[j]));
            vsum = _mm256_add_epi32(vsum, v);
        }
        alignas(32) int temp[8];
        _mm256_store_si256(reinterpret_cast<__m256i*>(temp), vsum);
        for (int k = 0; k < 8; ++k) {
            sum += temp[k];
        }
        for (int j = simd_end; j < SIZE; ++j) {
            sum += row[j];
        }
    }
    return sum;
}

#elif defined(__ARM_NEON) || defined(__aarch64__)
long long sumMatrixSIMD2D(const std::vector<std::vector<int>>& matrix) {
    long long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
        const std::vector<int>& row = matrix[i];
        const int remainder = SIZE % 4;
        const int simd_end = SIZE - remainder;
        int32x4_t vsum = vdupq_n_s32(0);
        for (int j = 0; j < simd_end; j += 4) {
            int32x4_t v = vld1q_s32(&row[j]);
            vsum = vaddq_s32(vsum, v);
        }
        sum += static_cast<long long>(vaddvq_s32(vsum));
        for (int j = simd_end; j < SIZE; ++j) {
            sum += row[j];
        }
    }
    return sum;
}
#else
long long sumMatrixSIMD2D(const std::vector<std::vector<int>>& matrix) {
    return sumMatrixOptimized(matrix);
}
#endif


int main() {
    std::vector<std::vector<int>> matrix(SIZE, std::vector<int>(SIZE));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(-100, 100);
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            matrix[i][j] = distrib(gen);
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    long long sum = sumMatrixBasic(matrix);
    auto end = std::chrono::high_resolution_clock::now();
    double basic_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "Basic Sum: " << sum << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Basic Time: " << basic_ms << " milliseconds" << std::endl;

    auto start_optimized = std::chrono::high_resolution_clock::now();
    long long optimized_sum = sumMatrixOptimized(matrix);
    auto end_optimized = std::chrono::high_resolution_clock::now();
    double optimized_ms =
        std::chrono::duration<double, std::milli>(end_optimized - start_optimized).count();

    std::cout << "Optimized Sum: " << optimized_sum << std::endl;
    std::cout << "Optimized Time: " << optimized_ms << " milliseconds" << std::endl;


    auto start_extreme = std::chrono::high_resolution_clock::now();
    long long optimized_sum_extreme = sumMatrixSIMD2D(matrix);
    auto end_extreme = std::chrono::high_resolution_clock::now();
    double optimized_ms_extreme =
        std::chrono::duration<double, std::milli>(end_extreme - start_extreme).count();

#if defined(__AVX2__)
    std::cout << "SIMD (AVX2) Sum: " << optimized_sum_extreme << std::endl;
    std::cout << "SIMD (AVX2) Time: " << optimized_ms_extreme << " milliseconds" << std::endl;
#elif defined(__ARM_NEON) || defined(__aarch64__)
    std::cout << "SIMD (NEON) Sum: " << optimized_sum_extreme << std::endl;
    std::cout << "SIMD (NEON) Time: " << optimized_ms_extreme << " milliseconds" << std::endl;
#else
    std::cout << "SIMD (scalar fallback) Sum: " << optimized_sum_extreme << std::endl;
    std::cout << "SIMD (scalar fallback) Time: " << optimized_ms_extreme << " milliseconds" << std::endl;
#endif


    return 0;
}