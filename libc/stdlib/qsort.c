


#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

void swap(void *a, void *b, size_t size) {
    if(a == b) return;
	unsigned char* x = (unsigned char*) a;
	unsigned char* y = (unsigned char*) b;
    for(size_t i = 0; i < size; i++) {
        x[i] ^= y[i];
        y[i] ^= x[i];
        x[i] ^= y[i];
    }
}

int rand(void);

void qsort(void* ptr, size_t n, size_t size, int (*compare)(const void *, const void *)) {
    if(n <= 1) return;
    // size_t pivot = rand() % n;
    size_t pivot = 0;
    swap(ptr, ptr + size * pivot, size);

    size_t l = 1, r = n-1;
    while(l <= r) {
        if(compare(ptr + size * l, ptr) > 0) {
            swap(ptr + size * l, ptr + size * r, size);
            r--;
        } else {
            l++;
        }
    }
    swap(ptr, ptr + size * r, size);
    qsort(ptr, l-1, size, compare);
    qsort(ptr + size * l, n-l, size, compare);
}