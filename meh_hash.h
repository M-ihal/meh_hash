#ifndef __MEH_HASH
#define __MEH_HASH

#include <stdint.h>
#include <malloc.h>
#include <string.h>

#if defined(MEH_DEBUG) 
#include <assert.h>
#include <stdio.h>
#endif

namespace meh {
    inline size_t next_pow_2(size_t num);

    template <typename TKey, typename TValue, uint32_t (*func_hash)(const TKey &), bool (*func_compare)(const TKey &, const TKey &)>
    class Table {
        static constexpr double LOAD_FACTOR = 70.0;

        /* Table data structure */
        struct Bucket {
            bool     is_occupied;
            TKey     key;
            TValue   value;
        };
        
        /* Calculate hash and mod to fit in the bucket array */
        uint32_t calc_hash(const TKey &key) const {
            uint32_t hash = func_hash(key) % m_buckets_allocated;
            return hash;
        }

        /* Check if the table should be expanded due to load factor */
        bool should_expand_table(void) {
            double occupied_perc = (double)(m_buckets_occupied + 1) / (double)m_buckets_allocated;
            return (occupied_perc * 100.0) >= LOAD_FACTOR;
        }

    public:
        /* Intial size rounds to next power of two */
        Table(size_t init_size = 64) {
            init_size = next_pow_2(init_size);
            init_size = init_size < 32 ? 32 : init_size;

            size_t size_in_bytes = init_size * sizeof(Bucket);

            m_buckets_occupied  = 0;
            m_buckets_allocated = init_size;
            m_buckets = (Bucket *)malloc(size_in_bytes);
            memset(m_buckets, 0, size_in_bytes);
            
#if defined(MEH_DEBUG)
            fprintf(stdout, "Table created!\n");
            fprintf(stdout, "- memory:      %lld\n", this->get_size_of_allocated_memory());
            fprintf(stdout, "- buckets:     %lld\n", this->get_size_of_allocated_buckets());
            fprintf(stdout, "- bucket size: %lld\n", this->get_size_of_bucket());
            fprintf(stdout, "- memory ptr:  %p\n",   this->get_buckets_memory_ptr());
#endif
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

            uint32_t hash = this->calc_hash(key);
            uint32_t hash_start = hash;

            while(m_buckets[hash].is_occupied && !func_compare(key, m_buckets[hash].key)) {
                hash += 1;

#if defined(MEH_DEBUG)
                _insert_collisions += 1;
#endif
             
                /* Wrap */
                if(hash >= m_buckets_allocated) {
                    hash = 0;
                }

                /* Went through whole table and did not free spot somehow */
                if(hash == hash_start) {
                    assert(NULL);
                    return NULL;
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
        TValue *const find(const TKey &key) const {
            uint32_t hash = this->calc_hash(key);
            uint32_t hash_start = hash;

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
            Bucket *old_buckets = m_buckets;
            size_t  old_buckets_allocated = m_buckets_allocated;

            size_t buckets_to_allocate = next_pow_2(old_buckets_allocated + 1);
            size_t size_in_bytes = buckets_to_allocate * sizeof(Bucket);

            m_buckets_allocated = buckets_to_allocate;
            m_buckets = (Bucket *)malloc(size_in_bytes);
            m_buckets_occupied = 0;
            memset(m_buckets, 0, size_in_bytes);

            this->rehash_table(old_buckets, old_buckets_allocated);

            free(old_buckets);

#if defined(MEH_DEBUG)
            fprintf(stdout, "Table expanded from %lld to %lld | buckets_occupied: %d | allocates now %lld bytes (%.04f MB).\n", 
                    old_buckets_allocated, 
                    buckets_to_allocate, 
                    m_buckets_occupied, 
                    buckets_to_allocate * sizeof(Bucket), 
                    double(buckets_to_allocate * sizeof(Bucket)) / 1024.0 / 1024.0);
#endif
        }

        /* Check if value at key has been inserted */
        bool contains(const TKey &key) const {
            return !!this->find(key);
        }

        /* Size in bytes of allocated memory */
        size_t get_size_of_allocated_memory(void) const {
            return m_buckets_allocated * sizeof(Bucket);
        }

        /* Number of buckets allocated */
        size_t get_size_of_allocated_buckets(void) const {
            return m_buckets_allocated;
        }

        /* Size in bytes of buckets occupied */
        size_t get_size_of_occupied_memory(void) const {
            return m_buckets_occupied * sizeof(Bucket);
        }

        /* Number of occupied buckets */ 
        size_t get_size_of_occupied_buckets(void) const {
            return m_buckets_occupied;
        }

        /* Internal size of bucket */
        size_t get_size_of_bucket(void) const {
            return sizeof(Bucket);
        }

        /* Temporary */
        const void *const get_buckets_memory_ptr(void) const {
            return m_buckets;
        }
    
        /* When iterate_all returns false, the *key* and *value* may contain garbage value */
        struct Iterator {
            uint32_t iter_id = 0;
            TKey     key     = 0;
            TValue  *value   = NULL;
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

#if defined(MEH_DEBUG)
        uint32_t _insert_collisions = 0;
#endif

private:
        /* Rehash values from old table data */
        void rehash_table(Bucket *buckets, size_t num_of_buckets) {
            for(size_t bucket_index = 0; bucket_index < num_of_buckets; ++bucket_index) {
                Bucket *bucket = buckets + bucket_index;

                if(bucket->is_occupied) {
                    this->insert(bucket->key, bucket->value);
                }
            }
        }

        size_t  m_buckets_allocated;
        size_t  m_buckets_occupied;
        Bucket *m_buckets;
    };

    /* Next power of 2 if isn't already power of 2 */
    inline size_t next_pow_2(size_t num) {
        size_t res = 1;
        while(res < num) {
            res <<= 1;
        }
        return res;
    }
}

#endif /* __MEH_HASH */