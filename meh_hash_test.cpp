#define MEH_DEBUG
#include "meh_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct vec2i {
    int32_t x;
    int32_t y;
};

uint64_t func_hash(const vec2i &v) {
    vec2i x = v;
    x.x += 0;
    x.y += 0;
    x.x *= 11;
    x.y *= 31;
    uint64_t hash = (*(uint64_t *)&v.x) | ((*(uint64_t *)&v.y) >> 32);
    // fprintf(stdout, "%llu hash: \n", hash);
    return hash;
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

    for(int32_t i = 2; i < 5; ++i) {
        MehTable table;

        int32_t p = pow(10, i) / 2;
        fprintf(stdout, "--- TABLE SIZE %llu, adding: %d\n", table.get_size_of_allocated_buckets(), (p/2)*(p/2));
        for(int32_t j = 0; j < p / 2; ++j) {
            for(int32_t k = 0; k < p / 2; k++) {
               //   table.insert({ rand() | (rand() << 16), rand() | (rand() << 16) }, j);
                table.insert({ j, k }, j * k);
            }
        }

#if 1
        uint32_t highest = 0;
        for(int32_t j = 0; j < p / 2; ++j) {
            for(int32_t k = 0; k < p / 2; k++) {
                vec2i key = { rand() | (rand() << 16), rand() | (rand() << 16) };
                table.find(key);
                // fprintf(stdout, "Collisions while searching for key: %d %d : %u\n", key.x, key.y, table._find_collisions);
                // table.insert({ j, k }, j * k);
                if(table._find_collisions > highest) {
                    highest = table._find_collisions;
                }
            }
        }
        fprintf(stdout, "HIGHEST FIND COLLISIONS: %u!\n", highest);
#endif

            
        fprintf(stdout, "INSERTED VALUES:     %lld\n", table.get_size_of_occupied_buckets());
        fprintf(stdout, "INSERT COLLISIONS:   %u\nSIZE AFTER: %llu\n", table._insert_collisions, table.get_size_of_allocated_buckets());
        fprintf(stdout, "COLLISION/INSERT:    %f\n\n", (double)table._insert_collisions / (double)table.get_size_of_occupied_buckets());
    }


    return 0;
}
