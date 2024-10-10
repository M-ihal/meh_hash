#include "meh_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int32_t pow(int32_t b, int32_t p) {
    int32_t res = 1;
    for(int32_t i = 0; i < p; ++i) {
        res *= b;
    }
    return res;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    return 0;
}
