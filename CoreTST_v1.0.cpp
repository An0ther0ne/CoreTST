#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <immintrin.h> // Бібліотека для прямого керування інструкціями процесора

// Прапорець для безпечного контролю роботи потоків
std::atomic<bool> keep_burning{ true };

// Важка функція для тотального завантаження кожного окремого потоку
void burn_core() {
    // Створюємо 256-бітні векторні регістри. Кожен містить по 8 float-чисел.
    __m256 reg1 = _mm256_set1_ps(1.00001f);
    __m256 reg2 = _mm256_set1_ps(1.00002f);

    while (keep_burning.load(std::memory_order_relaxed)) {
        // FMA (Fused Multiply-Add): reg1 = reg1 * reg2 + reg1
        // Найважча арифметика, яка змушує працювати SIMD-блоки ядра на 100%
        reg1 = _mm256_fmadd_ps(reg1, reg2, reg1);
        reg2 = _mm256_fmadd_ps(reg2, reg1, reg2);
    }
}

int main() {
    std::system("chcp 65001 > nul"); // Вмикаємо UTF-8 в консолі Windows
    // Визначаємо кількість логічних потоків твого Ryzen (має повернути 12)

    unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 6;

    std::cout << "==================================================\n";
    std::cout << "  C++ CoreTST ULTRA BURNER v1.0\n";
    std::cout << "  Виявлено логічних потоків: " << num_cores << "\n";
    std::cout << "  Запускаємо потоки обчислень AVX2...\n";
    std::cout << "==================================================\n";
    std::cout << "Натисніть ENTER для зупинки тесту...\n\n";

    // Створюємо вектор сучасних потоків jthread (C++20)
    std::vector<std::jthread> burners;
    burners.reserve(num_cores);

    // Запускаємо по одному важкому обчислювальному потоку на кожне ядро
    for (unsigned int i = 0; i < num_cores; ++i) {
        burners.emplace_back(burn_core);
    }

    // Програма застигає і чекає натискання клавіші Enter користувачем
    std::cin.get();

    std::cout << "Зупинка обчислень, охолодження процесора..." << std::endl;
    keep_burning = false; // Потоки миттєво побачать цей прапорець і завершаться

    return 0;
}
