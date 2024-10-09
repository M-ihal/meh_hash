#define MEH_DEBUG
#include "meh_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct vec2i {
    int32_t x;
    int32_t y;
};

uint32_t func_hash(const vec2i &v) {
    uint32_t x = (*(uint32_t *)&v.x) * 31;
    uint32_t y = (*(uint32_t *)&v.y) * 17;
    return x ^ y;
}

bool func_compare(const vec2i &a, const vec2i &b) {
    return a.x == b.x && a.y == b.y;
}

typedef meh::Table<vec2i, int32_t, func_hash, func_compare> MehTable;

int32_t pow(int32_t b, int32_t p) {
    int32_t res = 1;
    for(int32_t i = 0; i < p; ++i) {
        res *= b;
    }
    return res;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // TEST

    for(int32_t i = 0; i < 10; ++i) {
        MehTable table;
        fprintf(stdout, "--- TABLE SIZE %llu, max: %d\n", table.get_size_of_allocated_buckets(), pow(10, i));

        int32_t p = pow(10, i);
        for(int32_t j = 0; j < p; ++j) {
            table.insert({ rand() | (rand() << 16), rand() | (rand() << 16) }, j);
            // table.insert(j, j);
        }

        int32_t cnt = 0;
        MehTable::Iterator iter;
        while(table.iterate_all(iter)) {
            fprintf(stdout, "VALUE FOUND AT: {%d,%d}: %d\n", iter.key.x, iter.key.y, *iter.value);
            cnt++;
            if(cnt >= 10) {
                break;
            }
        }

        fprintf(stdout, "INSERTED VALUES:     %lld\n", table.get_size_of_occupied_buckets());
        fprintf(stdout, "INSERT COLLISIONS:   %u\nSIZE AFTER: %llu\n", table._insert_collisions, table.get_size_of_allocated_buckets());
        fprintf(stdout, "COLLISION/INSERT:    %f\n\n", (double)table._insert_collisions / (double)table.get_size_of_occupied_buckets());
    }


    return 0;
}
