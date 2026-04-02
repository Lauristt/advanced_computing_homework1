#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <random>

const int SIZE = 4096;

// Basic function to access matrix elements
int getElement(const std::vector<std::vector<int>>& matrix, int row, int col) {
    return matrix[row][col];
}

// Basic function to add two integers
int add(int a, int b) {
    return a + b;
}

// Unoptimized summation function
long long sumMatrixBasic(const std::vector<std::vector<int>>& matrix) {
    long long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            sum = add(sum, getElement(matrix, i, j));
        }
    }
    return sum;
}

int main() {
    // Generate a large random matrix
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

    // Students will implement their optimized version here
    auto start_optimized = std::chrono::high_resolution_clock::now();
    long long optimized_sum = 0; // Students will calculate this
    auto end_optimized = std::chrono::high_resolution_clock::now();
    // the original cast only saves the milliseconds part of the duration
    // auto duration_optimized = std::chrono::duration_cast<std::chrono::milliseconds>(end_optimized - start_optimized);
    double optimized_ms =
        std::chrono::duration<double, std::milli>(end_optimized - start_optimized).count();

    std::cout << "Optimized Sum: " << optimized_sum << std::endl;
    std::cout << "Optimized Time: " << optimized_ms << " milliseconds" << std::endl;

    return 0;
}