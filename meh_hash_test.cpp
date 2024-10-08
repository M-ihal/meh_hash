#define MEH_DEBUG
#include "meh_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

uint32_t func_hash(const int &v) {
    return v;
}

bool func_compare(const int &a, const int &b) {
    return a == b;
}

typedef meh::Table<int32_t, int, func_hash, func_compare> MehTable;

int main(int argc, char *argv[]) {
    srand(time(NULL));

    MehTable table;

    const int to_add = 20000000;

    for(int32_t index = 0; index < to_add; ++index) {
        table.insert(index % 414244, index);
    }

    fprintf(stdout, "INSERTED VALUES:   %lld\n", table.get_size_of_occupied_buckets());
    fprintf(stdout, "INSERT COLLISIONS: %d\n", table._insert_collisions);

    MehTable::Iterator iter;
    while(table.iterate_all(iter)) {
        // fprintf(stdout, "FOUND: %d <- %d\n", iter.key, *iter.value);
    }

    return 0;
}
