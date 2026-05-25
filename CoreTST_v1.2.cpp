#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <immintrin.h>
#include <windows.h> 

std::atomic<bool> keep_burning{ true };

void burn_scalar() {
    double x = 1.00001;
    while (keep_burning.load(std::memory_order_relaxed)) {
        x = x * 1.00002 + x;
        if (x > 1000.0) x = 1.00001;
    }
}

void burn_avx2() {
    __m256 reg1 = _mm256_set1_ps(1.00001f);
    __m256 reg2 = _mm256_set1_ps(1.00002f);
    while (keep_burning.load(std::memory_order_relaxed)) {
        reg1 = _mm256_fmadd_ps(reg1, reg2, reg1);
        reg2 = _mm256_fmadd_ps(reg2, reg1, reg2);
    }
}

bool check_avx2_support() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 7);
    return (cpuInfo[1] & (1 << 5)) != 0;
}

int main() {
    std::system("chcp 65001 > nul");

    unsigned int num_cores = std::thread::hardware_concurrency();

    if (num_cores == 0) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        num_cores = sysinfo.dwNumberOfProcessors;
    }

    if (num_cores == 0) {
        num_cores = 4;
    }

    bool has_avx2 = check_avx2_support();

    std::cout << "==================================================\n";
    std::cout << "  CoreTST ULTRA BURNER v1.2 (Industrial)\n";
    std::cout << "  Виявлено логічних потоків у системі: " << num_cores << "\n";
    std::cout << "  Режим обчислень: " << (has_avx2 ? "AVX2 (МАКСИМАЛЬНИЙ ПРОГРІВ)" : "Scalar (БЕЗПЕЧНИЙ РЕЖИМ)") << "\n";
    std::cout << "==================================================\n";
    std::cout << "Натисніть ENTER для зупинки тесту та охолодження...\n\n";

    std::vector<std::jthread> burners;
    burners.reserve(num_cores);

    for (unsigned int i = 0; i < num_cores; ++i) {
        if (has_avx2) {
            burners.emplace_back(burn_avx2);
        }
        else {
            burners.emplace_back(burn_scalar);
        }
    }

    std::cin.get();

    std::cout << "Сигнал зупинки отримано. Охолодження процесора..." << std::endl;
    keep_burning = false;

    return 0;
}
