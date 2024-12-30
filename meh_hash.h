#ifndef __MEH_HASH
#define __MEH_HASH

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

namespace meh {
    inline uint64_t next_pow_2(uint64_t number);

    enum : uint8_t {
        BUCKET_FREE     = 0,
        BUCKET_OCCUPIED = 1,
        BUCKET_REMOVED  = 2,
    };

    template <typename TKey, typename TValue, uint64_t (*func_hash)(const TKey &), bool (*func_compare)(const TKey &, const TKey &), uint32_t load_factor = 70>
    class Table {
        /* */
        struct Bucket {
            uint8_t state;
            TKey    key;
            TValue  value;
        };

        /* Calculate hash */
        uint64_t calc_hash(const TKey &key) const {
            uint64_t hash = func_hash(key) % (m_buckets_allocated - 1);
            return hash;
        }

        /* Check if the table should be expanded */
        bool should_expand_table(void) {
            return m_buckets_occupied * 100 >= m_buckets_allocated * load_factor;
        }

        /* Reallocate memory and rehash values from old table data */
        void rehash_table(uint64_t buckets_to_allocate) {
            assert(buckets_to_allocate >= m_buckets_allocated && "Trying to rehash to table of lower size");

            Bucket  *old_buckets = m_buckets;
            uint64_t old_buckets_allocated = m_buckets_allocated;
            uint64_t new_size_in_bytes = buckets_to_allocate * sizeof(Bucket);

            /* Clear allocated table */
            m_buckets_removed = 0;
            m_buckets_occupied = 0;
            m_buckets_allocated = buckets_to_allocate;
            m_buckets = (Bucket *)malloc(new_size_in_bytes);
            assert(m_buckets && "Failed to allocate new table memory");
            memset(m_buckets, 0, new_size_in_bytes);

            for(uint64_t bucket_index = 0; bucket_index < old_buckets_allocated; ++bucket_index) {
                Bucket *bucket = old_buckets + bucket_index;
                if(bucket->state == BUCKET_OCCUPIED) {
                    this->insert(bucket->key, bucket->value);
                }
            }

            free(old_buckets);
        }

        Bucket *find_occupied_bucket_by_key(const TKey &key) {
            uint64_t hash = this->calc_hash(key);
            uint64_t hash_start = hash;

            while(true) {
                if(m_buckets[hash].state == BUCKET_FREE) {
                    break;
                }

                if(func_compare(key, m_buckets[hash].key) && m_buckets[hash].state == BUCKET_OCCUPIED) {
                    return &m_buckets[hash];
                }

                hash += 1;
                if(hash >= m_buckets_allocated) {
                    hash = 0; /* Wrap */
                }

                /* Went through whole table and did not find key */
                if(hash == hash_start) {
                    break;
                }
            }

            return NULL;
        }

        TValue *find_by_key(const TKey &key) {
            Bucket *bucket = this->find_occupied_bucket_by_key(key);
            TValue *value = NULL;
            if(bucket) {
                value = &bucket->value;
            }
            return value;
        }

public:
        /* When iterate_all returns false, the *key* and *value* may contain garbage value */
        struct Iterator {
            TKey     key;
            TValue   value;
            uint64_t iter_id = 0;
        };
    
        Table(void) = default;
        ~Table(void) = default;

        /* @TODO : NOT IMPLEMENTED */
        Table(const Table &) = delete;
        Table &operator=(const Table &) = delete;
        Table(Table &&) = default;
        Table &operator=(Table &&) = default;

        /* Needs to be called only once when initializing or after destroying */
        void initialize_table(uint64_t init_size = 128) {
            init_size = next_pow_2(init_size);

            /* Clamp size to be atleast 128 buckets */
            init_size = init_size < 128 ? 128 : init_size;

            uint64_t size_in_bytes = init_size * sizeof(Bucket);

            m_buckets_occupied  = 0;
            m_buckets_allocated = init_size;
            m_buckets = (Bucket *)malloc(size_in_bytes);
            assert(m_buckets && "Failed to allocate table memory");
            memset(m_buckets, 0, size_in_bytes);
        }

        /* Release allocated memory */
        void delete_table(void) {
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

            while(m_buckets[hash].state == BUCKET_OCCUPIED && !func_compare(key, m_buckets[hash].key)) {
                hash += 1;
                if(hash >= m_buckets_allocated) {
                    /* Wrap */
                    hash = 0;
                }

                /* Shouldn't happen... */
                if(hash == hash_start) {
                    assert(false && "Invalid code path, no space left in table, should've already expand...");
                }
            }

            Bucket &bucket = m_buckets[hash];
            if(bucket.state != BUCKET_OCCUPIED) {
                /* The *key* has not been inserted yet */
                bucket.state = BUCKET_OCCUPIED;
                bucket.key = key;
                bucket.value = value;
                m_buckets_occupied += 1;
            } else {
                /* The *key* has been insterted already, so update the contents */
                bucket.value = value;
            }

            return &bucket.value;
        }

        /* Removes bucket && return inserted value by value (slot gets freed on next or manual rehash) */
        bool remove(const TKey &key, TValue *out_removed_value = NULL) {
            Bucket *bucket = this->find_occupied_bucket_by_key(key);
            if(bucket) {
                bucket->state = BUCKET_REMOVED;
                if(out_removed_value) {
                    *out_removed_value = bucket->value;
                }
                m_buckets_removed += 1;
                return true;
            }
            return false;
        }

        /* Get pointer to the value at key */
        TValue *find(const TKey &key) {
            TValue *value = this->find_by_key(key);
            return value;
        }

        /* Resize table to the next power of 2 and rehash entries */
        void expand_table(void) {
            uint64_t new_number_of_buckets = next_pow_2(m_buckets_allocated + 1);
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
        }

        /* Check if value at key has been inserted */
        bool contains(const TKey &key) {
            return !!this->find(key);
        }

        /* Number of occupied buckets (not counting removed that still occupy buckets) */ 
        uint64_t get_count(void) const {
            return m_buckets_occupied - m_buckets_removed;
        }

        /* Number of occupied buckets including buckets that were removed but still occupy memory */
        uint64_t get_count_all(void) const {
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

                if(m_buckets[iter.iter_id].state == BUCKET_OCCUPIED) {
                    Bucket *bucket = m_buckets + iter.iter_id;
                    iter.key = bucket->key;
                    iter.value = bucket->value;
                    iter.iter_id += 1;
                    return true;                
                }

                iter.iter_id++;
            }
            return false;
        }

        /* Calls *func* on every found bucket */
        void call_on_every(void *user_ptr, void (*func)(TKey, TValue, void *)) {
            Iterator iter;
            while(this->iterate_all(iter)) {
                func(iter.key, iter.value, user_ptr);
            }
        }

private:
        uint64_t  m_buckets_allocated = 0;
        uint64_t  m_buckets_occupied = 0;
        uint64_t  m_buckets_removed = 0;
        Bucket   *m_buckets = NULL;
    };

    /* Next power of 2 if isn't already power of 2 */
    inline uint64_t next_pow_2(uint64_t number) {
        if(number == 0) {
            return 1;
        }

        int64_t value = number -= 1;
        for(uint32_t shift = 1; shift < 64; shift <<= 1) {
            value |= value >> shift; 
        }
        return value + 1;
    }

    template<typename TKey>
    inline uint64_t _func_hash_def_simple(const TKey &key) {
        static_assert(sizeof(TKey) == 1 || sizeof(TKey) == 2 || sizeof(TKey) == 4 || sizeof(TKey) == 8);

        uint64_t hash;
        if constexpr (sizeof(TKey) == 1) {
            uint64_t _key8 = (uint64_t)*(uint8_t *)&key;
            uint64_t _key64 = _key8 * 3312;
            hash = _key64;
        } else if constexpr (sizeof(TKey) == 4) {
            uint64_t _key16 = (uint64_t)*(uint8_t *)&key;
            uint64_t _key64 = _key16 * 331;
            hash = _key64;
        } else if constexpr (sizeof(TKey) == 4) {
            uint64_t _key32 = (uint64_t)*(uint32_t *)&key;
            uint64_t _key64 = _key32 * 31;
            hash = _key64;
        } else if constexpr (sizeof(TKey) == 8) {
            uint64_t _key = *(uint64_t *)&key;
            _key *= 17;
            hash = _key; 
        } else {
            assert(false && "Invalid code path");
        }
        return hash;
    }

    template<typename TKey>
    inline bool _func_compare_def_simple(const TKey &key_a, const TKey &key_b) {
        static_assert(sizeof(TKey) == 1 || sizeof(TKey) == 2 || sizeof(TKey) == 4 || sizeof(TKey) == 8);
        return !memcmp(&key_a, &key_b, sizeof(TKey));
    }

    template <typename TKey, typename TValue>
    using TableSimple = Table<TKey, TValue, _func_hash_def_simple, _func_compare_def_simple>;
    using TableSimpleInt64 = TableSimple<int64_t, int64_t>;
    using TableSimpleInt32 = TableSimple<int32_t, int32_t>;
}

#endif /* __MEH_HASH */
