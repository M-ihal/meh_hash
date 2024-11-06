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
        meh::TableSimple<int64_t, int64_t> table;
        table.initialize_table();


        for(int64_t i = 0; i < 100; ++i) {
            int64_t random_key = i; // (int64_t)(rand() % RAND_MAX) | ((int64_t)(rand() % RAND_MAX) << 16);
            table.insert(random_key, rand());
        }

        printf("INSERTED b4 removes: %llu\n", table.get_count_all());
        for(int64_t i = 0; i < 80; ++i) {
            int64_t random_key = 10 + i; // (int64_t)(rand() % RAND_MAX) | ((int64_t)(rand() % RAND_MAX) << 16);
            if(table.contains(random_key)) {
                int64_t removed;
                if(table.remove(random_key, &removed)) {
                    fprintf(stdout, "key:%lld | removed:%lld | occupied:%lld\n", random_key, removed, table.get_count());
                }
            }
        }
        printf("INSERTED af removes: %llu\n", table.get_count_all());
        table.rehash_table();
        printf("INSERTED af rehash:  %llu\n", table.get_count_all());

        meh::TableSimple<int64_t, int64_t>::Iterator iter;
        while(table.iterate_all(iter)) {
            fprintf(stdout, "key:%lld | value:%lld\n", iter.key, iter.value);
        }

        table.delete_table();
    }


    {
        meh::TableSimpleInt32 _tab;
        _tab.initialize_table(1000);
      
        _tab.insert(12333, 10);
        _tab.insert(12334, 13);
        _tab.insert(12335, 12);
        fprintf(stdout, "FOUND: %s\n", _tab.find(12333) ? "true" : "false");
        _tab.remove(12333);
        fprintf(stdout, "FOUND: %s\n", _tab.find(12333) ? "true" : "false");


        meh::TableSimpleInt32::Iterator _iter;
        while(_tab.iterate_all(_iter)) {
            fprintf(stdout, "Conatins: key:%d, val:%d\n", _iter.key, _iter.value);
        }

        _tab.delete_table();
    }


    {
        meh::TableSimple<uint32_t, float> table;
        table.initialize_table();

        for(int32_t i = 0; i < 20110; ++i) {
            table.insert(i, (float)(rand() % 20000) / 20000.0f);
        }

        float sum = 0.0f;
        table.call_on_every(&sum, [] (uint32_t key, float value, void *user_ptr) {
            float *_sum = (float *)user_ptr;
            *_sum += value;
        });

        fprintf(stdout, "SUM: %.3f, AVG: %.3f\n", sum, sum / (float)table.get_count());
    }

    return 0;
}
