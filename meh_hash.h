#ifndef __MEH_HASH
#define __MEH_HASH

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

namespace meh {
    inline uint64_t next_pow_2(uint64_t num);

    template <typename TKey, typename TValue, uint64_t (*func_hash)(const TKey &), bool (*func_compare)(const TKey &, const TKey &)>
    class Table {
        static constexpr uint32_t LOAD_FACTOR = 70;

        /* Table data structure */
        struct Bucket {
            bool   is_occupied;
            TKey   key;
            TValue value;
        };
        
        /* Calculate hash and mod to fit in the bucket array */
        uint64_t calc_hash(const TKey &key) const {
            uint64_t hash = func_hash(key) % (m_buckets_allocated - 17); // @todo
            return hash;
        }

        /* Check if the table should be expanded due to load factor */
        bool should_expand_table(void) {
            double occupied_perc = (double)(m_buckets_occupied + 1) / (double)m_buckets_allocated;
            return (occupied_perc * 100.0) >= LOAD_FACTOR;
        }

        /* Rehash values from old table data */
        void rehash_table(Bucket *buckets, uint64_t num_of_buckets) {
            for(uint64_t bucket_index = 0; bucket_index < num_of_buckets; ++bucket_index) {
                Bucket *bucket = buckets + bucket_index;

                if(bucket->is_occupied) {
                    this->insert(bucket->key, bucket->value);
                }
            }
        }

public:
        /* Intial size rounds to next power of two */
        Table(uint64_t init_size = 128) {
            init_size = next_pow_2(init_size);
            init_size = init_size < 128 ? 128 : init_size;

            uint64_t size_in_bytes = init_size * sizeof(Bucket);

            m_buckets_occupied  = 0;
            m_buckets_allocated = init_size;
            m_buckets = (Bucket *)malloc(size_in_bytes);
            memset(m_buckets, 0, size_in_bytes);
        }

        /* Release allocated memory */
        ~Table(void) {
            if(m_buckets_allocated) {
                free(m_buckets);
                m_buckets           = NULL;
                m_buckets_occupied  = 0;
                m_buckets_allocated = 0;
            }
        }

        /* Insert value at key */
        TValue *const insert(const TKey &key, const TValue &value) {
            if(this->should_expand_table()) {
                this->expand_table();
            }

            uint64_t hash = this->calc_hash(key);
            uint64_t hash_start = hash;

            while(m_buckets[hash].is_occupied && !func_compare(key, m_buckets[hash].key)) {
                hash += 1;

                _insert_collisions += 1;

                /* Wrap */
                if(hash >= m_buckets_allocated) {
                    hash = 0;
                }

                /* Really shouldn't happen... */
                if(hash == hash_start) {
                    assert(NULL);
                }
            }

            Bucket &bucket = m_buckets[hash];
            if(!bucket.is_occupied) {
                /* The *key* has not been inserted yet */
                bucket.is_occupied = true;
                bucket.key = key;
                bucket.value = value;
                m_buckets_occupied += 1;
            } else {
                /* The *key* has been insterted already, so update the contents */
                bucket.value = value;
            }

            return &bucket.value;
        }

        /* Get value at key */
        TValue *const find(const TKey &key) {
            uint64_t hash = this->calc_hash(key);
            uint64_t hash_start = hash;

            _find_collisions = 0;

            while(true) {
                if(!m_buckets[hash].is_occupied) {
                    return NULL;
                }

                if(func_compare(key, m_buckets[hash].key)) {
                    return &m_buckets[hash].value;
                }

                hash += 1;

                _find_collisions += 1;

                /* Wrap */
                if(hash >= m_buckets_allocated) {
                    hash = 0;
                }

                /* Went through whole table and did not find key */
                if(hash == hash_start) {
                    return NULL;
                }
            }

            return NULL;
        }

        const TValue *const find(const TKey &key) const {
            uint64_t hash = this->calc_hash(key);
            uint64_t hash_start = hash;

            while(true) {
                if(!m_buckets[hash].is_occupied) {
                    return NULL;
                }

                if(func_compare(key, m_buckets[hash].key)) {
                    return &m_buckets[hash].value;
                }

                hash += 1;

                /* Wrap */
                if(hash >= m_buckets_allocated) {
                    hash = 0;
                }

                /* Went through whole table and did not find key */
                if(hash == hash_start) {
                    return NULL;
                }
            }

            return NULL;
        }

        /* Resize table to the next power of 2 and rehash entries */
        void expand_table(void) {
            Bucket  *old_buckets = m_buckets;
            uint64_t old_buckets_allocated = m_buckets_allocated;

            uint64_t buckets_to_allocate = next_pow_2(old_buckets_allocated + 1);
            uint64_t size_in_bytes = buckets_to_allocate * sizeof(Bucket);

            // fprintf(stdout, "Expanding table from %llu buckets to %llu. At this time num of insert collisions: %u\n", old_buckets_allocated, buckets_to_allocate, _insert_collisions);

            m_buckets_allocated = buckets_to_allocate;
            m_buckets = (Bucket *)malloc(size_in_bytes);
            m_buckets_occupied = 0;
            memset(m_buckets, 0, size_in_bytes);

            this->rehash_table(old_buckets, old_buckets_allocated);

            free(old_buckets);
        }

        /* Check if value at key has been inserted */
        bool contains(const TKey &key) const {
            return !!this->find(key);
        }

        /* Size in bytes of allocated memory */
        uint64_t get_size_of_allocated_memory(void) const {
            return m_buckets_allocated * sizeof(Bucket);
        }

        /* Number of buckets allocated */
        uint64_t get_size_of_allocated_buckets(void) const {
            return m_buckets_allocated;
        }

        /* Size in bytes of buckets occupied */
        uint64_t get_size_of_occupied_memory(void) const {
            return m_buckets_occupied * sizeof(Bucket);
        }

        /* Number of occupied buckets */ 
        uint64_t get_size_of_occupied_buckets(void) const {
            return m_buckets_occupied;
        }

        /* Internal size of bucket */
        uint64_t get_size_of_bucket(void) const {
            return sizeof(Bucket);
        }

        /* Temporary */
        const void *const get_buckets_memory_ptr(void) const {
            return m_buckets;
        }
    
        /* When iterate_all returns false, the *key* and *value* may contain garbage value */
        struct Iterator {
            uint64_t iter_id = 0;
            TKey     key;
            TValue  *value;
        };

        /* *iter_id* must start at 0 and must not be modified */
        bool iterate_all(Iterator &iter) {
            while(true) {
                if(iter.iter_id >= m_buckets_allocated) {
                    return false;
                }

                if(m_buckets[iter.iter_id].is_occupied) {
                    Bucket *bucket = m_buckets + iter.iter_id;
                    iter.key = bucket->key;
                    iter.value = &bucket->value;
                    iter.iter_id += 1;
                    return true;                
                }

                iter.iter_id++;
            }
            return false;
        }

        void call_on_every(void (*func)(TKey key, TValue *value)) {
            Iterator iter;
            while(this->iterate_all(iter)) {
                func(iter.key, iter.value);
            }
        }

        uint32_t _insert_collisions = 0;
        uint32_t _find_collisions   = 0; // Reseted on start of find() function
private:
        uint64_t  m_buckets_allocated;
        uint64_t  m_buckets_occupied;
        Bucket   *m_buckets;
    };

    /* Next power of 2 if isn't already power of 2 */
    inline uint64_t next_pow_2(uint64_t num) {
        uint64_t res = 1;
        while(res < num) {
            res <<= 1;
        }
        return res;
    }
}

#endif /* __MEH_HASH */
