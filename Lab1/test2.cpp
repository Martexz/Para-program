#include <iostream>
#include <windows.h>

using namespace std;

const int N = 1 << 20;
int a[N];

inline void init() {
    for (int i = 0; i < N; i ++) {
        a[i] = i;
    }
}

inline void normal_algo() { // 朴素线性算法
    int sum = 0;
    for (int i = 0; i < N; i ++)
        sum += a[i];
}

inline void cache_algo() { // 多链路式
    int sum1 = 0, sum2 = 0, res = 0;
    for (int i = 0; i < N; i += 2) {
        sum1 += a[i];
        sum2 += a[i + 1];
    }
    res = sum1 + sum2;
}

int main() {
    LARGE_INTEGER st, ed, freq;
    init();
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&st);

    int T = 50;
    while (T --)
        normal_algo();

    QueryPerformanceCounter(&ed);
    double cost1 = static_cast<double>(ed.QuadPart - st.QuadPart) / freq.QuadPart;
    printf("Normal Algorithm Cost: %lf s\n", cost1);
// ----------------------------------
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&st);

    T = 50;
    while (T --)
        cache_algo();

    QueryPerformanceCounter(&ed);
    double cost2 = static_cast<double>(ed.QuadPart - st.QuadPart) / freq.QuadPart;
    printf("Cache Algorithm Cost: %lf s\n", cost2);
}