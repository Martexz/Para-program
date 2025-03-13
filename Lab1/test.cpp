#include <iostream>
#include <windows.h>

using namespace std;

const int N = 5000;
int a[N][N], b[N];
int res[N]; 

inline void init()
{
    for (int i = 0; i < N; i ++)
        for (int j = 0; j < N; j ++)
            a[i][j] = i + j;
    for (int i = 0; i < N; i ++)
        b[i] = i + i; 
}

inline void normal_algo() // 平凡算法
{
    for (int i = 0; i < N; i ++)
        for (int j = 0; j < N; j ++)
            res[i] += a[j][i] * b[j];
}

inline void cache_algo() // cache优化算法
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            res[j] += a[i][j] * b[i];
}

int main()
{   
    LARGE_INTEGER st, ed, freq;
    init();
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&st);

    normal_algo();

    QueryPerformanceCounter(&ed);
    double cost1 = static_cast<double>(ed.QuadPart - st.QuadPart) / freq.QuadPart;
    printf("Normal Algorithm Cost: %lf s\n", cost1);
// ----------------------------------
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&st);

    cache_algo();

    QueryPerformanceCounter(&ed);
    double cost2 = static_cast<double>(ed.QuadPart - st.QuadPart) / freq.QuadPart;
    printf("Cache Algorithm Cost: %lf s\n", cost2);

    return 0;
}