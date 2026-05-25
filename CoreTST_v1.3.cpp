#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <immintrin.h>
#include <intrin.h>
#include <windows.h>

std::atomic<bool> keep_burning{ true };

static void configure_thread(unsigned int core_id) {
    // Прив'язка до конкретного ядра
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << (core_id % 64));
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    // Вимикаємо denormals — запобігає різкому падінню продуктивності
    _mm_setcsr(_mm_getcsr() | 0x8040); // DAZ + FTZ
}

// 4 незалежні ланцюги — ховають латентність multiply
void burn_scalar(unsigned int core_id) {
    configure_thread(core_id);
    double a = 1.1, b = 1.2, c = 1.3, d = 1.4;
    const double mul = 1.00002, add = 0.00001;
    uint32_t iter = 0;
    while (keep_burning.load(std::memory_order_relaxed)) {
        a = a * mul + add;
        b = b * mul + add;
        c = c * mul + add;
        d = d * mul + add;
        if (++iter == 100'000) { // ФІКС: скидання значень
            a = 1.1; b = 1.2; c = 1.3; d = 1.4;
            iter = 0;
        }
    }
}

// 8 незалежних YMM-ланцюгів = 64 float за такт на 2 FMA-юнітах
void burn_avx2(unsigned int core_id) {
    configure_thread(core_id);

    __m256 a = _mm256_set1_ps(1.0f), b = _mm256_set1_ps(1.1f);
    __m256 c = _mm256_set1_ps(1.2f), d = _mm256_set1_ps(1.3f);
    __m256 e = _mm256_set1_ps(1.4f), f = _mm256_set1_ps(1.5f);
    __m256 g = _mm256_set1_ps(1.6f), h = _mm256_set1_ps(1.7f);
    const __m256 mul = _mm256_set1_ps(1.00002f);
    const __m256 add = _mm256_set1_ps(0.00001f);

    uint32_t iter = 0;
    while (keep_burning.load(std::memory_order_relaxed)) {
        // ФІКС: всі 8 ланцюгів незалежні один від одного
        a = _mm256_fmadd_ps(a, mul, add);
        b = _mm256_fmadd_ps(b, mul, add);
        c = _mm256_fmadd_ps(c, mul, add);
        d = _mm256_fmadd_ps(d, mul, add);
        e = _mm256_fmadd_ps(e, mul, add);
        f = _mm256_fmadd_ps(f, mul, add);
        g = _mm256_fmadd_ps(g, mul, add);
        h = _mm256_fmadd_ps(h, mul, add);

        if (++iter == 100'000) { // ФІКС: скидання перед overflow
            a = _mm256_set1_ps(1.0f); b = _mm256_set1_ps(1.1f);
            c = _mm256_set1_ps(1.2f); d = _mm256_set1_ps(1.3f);
            e = _mm256_set1_ps(1.4f); f = _mm256_set1_ps(1.5f);
            g = _mm256_set1_ps(1.6f); h = _mm256_set1_ps(1.7f);
            iter = 0;
        }
    }
}

bool check_avx2_support() {
    int info[4];

    // Крок 1: перевірка OSXSAVE + AVX у leaf 1
    __cpuid(info, 1);
    if (!(info[2] & (1 << 27))) return false; // OSXSAVE відсутній
    if (!(info[2] & (1 << 28))) return false; // AVX відсутній

    // Крок 2: ОС зберігає YMM-стан (XCR0 біти 1 і 2)
    // ФІКС: без цієї перевірки — Illegal Instruction на деяких системах
    if ((_xgetbv(0) & 0x6) != 0x6) return false;

    // Крок 3: ФІКС: __cpuidex із явним subleaf=0
    __cpuidex(info, 7, 0);
    return (info[1] & (1 << 5)) != 0; // AVX2
}

int main() {
    std::system("chcp 65001 > nul");

    unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) {
        SYSTEM_INFO si; GetSystemInfo(&si);
        num_cores = si.dwNumberOfProcessors;
    }
    if (num_cores == 0) num_cores = 4;

    bool has_avx2 = check_avx2_support();

    std::cout << "==================================================\n";
    std::cout << "  CoreTST ULTRA BURNER v1.3 (Industrial)\n";
    std::cout << "  Виявлено логічних потоків: " << num_cores << "\n";
    std::cout << "  Режим: " << (has_avx2 ? "AVX2 x8 FMA (МАКСИМАЛЬНИЙ ПРОГРІВ)"
        : "Scalar x4 (БЕЗПЕЧНИЙ РЕЖИМ)") << "\n";
    std::cout << "==================================================\n";
    std::cout << "Натисніть ENTER для зупинки...\n\n";

    std::vector<std::jthread> burners;
    burners.reserve(num_cores);
    for (unsigned int i = 0; i < num_cores; ++i) {
        if (has_avx2) burners.emplace_back(burn_avx2, i);
        else          burners.emplace_back(burn_scalar, i);
    }

    std::cin.get();
    std::cout << "Сигнал зупинки. Охолодження...\n";
    keep_burning = false;
    // std::jthread join відбувається автоматично при деструкції вектора
    return 0;
}
