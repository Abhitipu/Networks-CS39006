#include <stdio.h>
#include <time.h>

int main() {
    clock_t start_time = clock();
    for(int i = 0; i < 900000000; i++);
    clock_t end_time = clock();
    printf("%ld %ld %ld\n", start_time / CLOCKS_PER_SEC, end_time / CLOCKS_PER_SEC, (end_time - start_time) / CLOCKS_PER_SEC);
    printf("%ld %ld %ld\n", start_time, end_time, (end_time - start_time));
}
