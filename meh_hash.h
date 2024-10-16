#ifndef __MEH_HASH
#define __MEH_HASH

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

namespace meh {
    inline uint64_t next_pow_2(uint64_t num);

    template <typename TKey, typename TValue, uint64_t (*func_hash)(const TKey &) = NULL, bool (*func_compare)(const TKey &, const TKey &) = NULL>
    class Table {

        // @todo Make a template parameter
        static constexpr uint32_t LOAD_FACTOR = 70;

        /* Table data structure */
        struct Bucket {
            bool   is_occupied;
            TKey   key;
            TValue value;
        };

        /* Calculate hash and mod to fit in the bucket array */
        uint64_t calc_hash(const TKey &key) const {
            uint64_t hash = func_hash(key) % (m_buckets_allocated - 1); // @todo can i -17 here?
            return hash;
        }

        /* Check if the table should be expanded due to load factor */
        bool should_expand_table(void) {
            return m_buckets_occupied * 100 >= m_buckets_allocated * LOAD_FACTOR;
        }

        /* Reallocate memory and rehash values from old table data */
        void rehash_table(uint64_t buckets_to_allocate) {
            assert(buckets_to_allocate >= m_buckets_allocated);

            Bucket  *old_buckets = m_buckets;
            uint64_t old_buckets_allocated = m_buckets_allocated;
            uint64_t new_size_in_bytes = buckets_to_allocate * sizeof(Bucket);

            /* Clear newly allocated table */
            m_buckets_occupied = 0;
            m_buckets_allocated = buckets_to_allocate;
            m_buckets = (Bucket *)malloc(new_size_in_bytes);
            assert(m_buckets); // @todo Handle fail case
            memset(m_buckets, 0, new_size_in_bytes);

            for(uint64_t bucket_index = 0; bucket_index < old_buckets_allocated; ++bucket_index) {
                Bucket *bucket = old_buckets + bucket_index;

                if(bucket->is_occupied) {
                    this->insert(bucket->key, bucket->value);
                }
            }

            free(old_buckets);
        }

        TValue *find_by_key(const TKey &key) {
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

public:
        /* When iterate_all returns false, the *key* and *value* may contain garbage value */
        struct Iterator {
            uint64_t iter_id = 0;
            TKey     key;
            TValue  *value;
        };

        /* Intial size rounds to next power of two */
        Table(uint64_t init_size = 128) {
            init_size = next_pow_2(init_size);

            /* Clamp size to be atleast 128 buckets */
            init_size = init_size < 128 ? 128 : init_size;

            uint64_t size_in_bytes = init_size * sizeof(Bucket);

            m_buckets_occupied  = 0;
            m_buckets_allocated = init_size;
            m_buckets = (Bucket *)malloc(size_in_bytes);
            assert(m_buckets); // @todo Handle fail case
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

        /* Get pointer to the value at key */
        TValue *find(const TKey &key) {
            TValue *value = this->find_by_key(key);
            return value;
        }

        /* Resize table to the next power of 2 and rehash entries */
        void expand_table(void) {
            uint64_t new_number_of_buckets = next_pow_2(m_buckets_allocated + 1);
            fprintf(stdout, "EXPANDING %llu -> %llu (perc: %f)\n", m_buckets_allocated, new_number_of_buckets, (double)m_buckets_occupied / (double)m_buckets_allocated);
            this->rehash_table(new_number_of_buckets);
        }

        /* Rehashes the table with the same size @todo, when added removing elements */
        void rehash_table(void) {
            this->rehash_table(m_buckets_allocated);
        }

        /* Clears the table but does not reallocate memory */
        void clear_table(void) {
            m_buckets_occupied = 0;
            memset(m_buckets, 0, m_buckets_allocated * sizeof(Bucket));

            _insert_collisions = 0;
            _find_collisions = 0;
        }

        /* Check if value at key has been inserted */
        bool contains(const TKey &key) const {
            return !!this->find(key);
        }

        /* Number of occupied buckets */ 
        uint64_t get_count(void) const {
            return m_buckets_occupied;
        }

        /* Number of buckets allocated */
        uint64_t get_size(void) const {
            return m_buckets_allocated;
        }
    
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

        /* Calls *func* on every found bucket */
        void call_on_every(void (*func)(TKey key, TValue *value)) {
            Iterator iter;
            while(this->iterate_all(iter)) {
                func(iter.key, iter.value);
            }
        }

        uint32_t _insert_collisions = 0;
        uint32_t _find_collisions   = 0; // Reset at the beginning of find() function
                                         
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

    /* @todo Can figure hash function based on template type in compile time??? */

    inline uint64_t _func_hash_def_int32_t(const int32_t &key) {
        int32_t _key = key * 31;
        return *(uint64_t *)&_key; 
    }

    inline bool _func_compare_def_int32_t(const int32_t &key_a, const int32_t &key_b) {
        return key_a == key_b;
    }

    inline uint64_t _func_hash_def_int64_t(const int64_t &key) {
        int64_t _key = key * 31;
        return *(uint64_t *)&_key; 
    }

    inline bool _func_compare_def_int64_t(const int64_t &key_a, const int64_t &key_b) {
        return key_a == key_b;
    }

    typedef Table<int32_t, int32_t, _func_hash_def_int32_t, _func_compare_def_int32_t> TableInt32;
    typedef Table<int64_t, int64_t, _func_hash_def_int64_t, _func_compare_def_int64_t> TableInt64;

    template<typename TKey>
    inline uint64_t _func_hash_def_simple_32_or_64(const TKey &key) {
        // Is this compiletime???
        if(sizeof(TKey) == 4) {
            uint32_t _key32 = *(uint32_t *)&key;
            uint64_t _key64 = _key32 * 31;
            return _key64;
        } else if(sizeof(TKey) == 8) {
            uint64_t _key = *(uint64_t *)&key;
            _key *= 31;
            return _key; 
        } else {
            assert(NULL);
        }
    }

    template<typename TKey>
    inline bool _func_compare_def_simple_32_or_64(const TKey &key_a, const TKey &key_b) {
        assert(sizeof(TKey) == 4 || sizeof(TKey) == 8);
        return key_a == key_b;
    }

    template <typename TKey, typename TValue>
    using TableSimple = Table<TKey, TValue, _func_hash_def_simple_32_or_64, _func_compare_def_simple_32_or_64>;
}

#endif /* __MEH_HASH */
