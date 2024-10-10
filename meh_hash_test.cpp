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
 
    {
        meh::TableInt64 table;

        for(int64_t i = 0; i < 8000000; ++i) {
            table.insert((int64_t)(rand() % RAND_MAX) | ((int64_t)(rand() % RAND_MAX) << 16) | ((int64_t)(rand() % RAND_MAX) << 32) | ((int64_t)(rand() % RAND_MAX) << 48), rand());
        }

        table.rehash_table();

        fprintf(stdout, "size now: %llu\ninserted: %llu\ncollisions: %u\ncol/insert: %f\n\n", table.get_size(), table.get_count(), table._insert_collisions, (double)table._insert_collisions / (double)table.get_count());
    }

    {
        meh::TableInt32 table;

        for(int32_t i = 0; i < 8000000; ++i) {
            table.insert((int32_t)(rand() % RAND_MAX) | ((int32_t)(rand() % RAND_MAX) << 16), rand());
        }

        table.rehash_table();

        fprintf(stdout, "size now: %llu\ninserted: %llu\ncollisions: %u\ncol/insert: %f\n\n", table.get_size(), table.get_count(), table._insert_collisions, (double)table._insert_collisions / (double)table.get_count());
    }

    {
        meh::TableSimple<uint32_t, int *> table;

        for(int32_t i = 0; i < 8000000; ++i) {
            table.insert((int32_t)(rand() % RAND_MAX) | ((int32_t)(rand() % RAND_MAX) << 16), (int *)rand());
        }

        table.rehash_table();

        fprintf(stdout, "size now: %llu\ninserted: %llu\ncollisions: %u\ncol/insert: %f\n\n", table.get_size(), table.get_count(), table._insert_collisions, (double)table._insert_collisions / (double)table.get_count());

        meh::TableSimple<uint32_t, int *>::Iterator iter;
        while(table.iterate_all(iter)) {
            // fprintf(stdout, "Found %p at %u !\n", *iter.value, iter.key);
        }
    }



    return 0;
}
