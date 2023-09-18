#include "elk.h"

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
int64_t const SECONDS_PER_MINUTE = INT64_C(60);
int64_t const MINUTES_PER_HOUR = INT64_C(60);
int64_t const HOURS_PER_DAY = INT64_C(24);
int64_t const DAYS_PER_YEAR = INT64_C(365);
int64_t const SECONDS_PER_HOUR = INT64_C(60) * INT64_C(60);
int64_t const SECONDS_PER_DAY = INT64_C(60) * INT64_C(60) * INT64_C(24);
int64_t const SECONDS_PER_YEAR = INT64_C(60) * INT64_C(60) * INT64_C(24) * INT64_C(365);

// Days in a year up to beginning of month
int64_t const sum_days_to_month[2][13] = {
    {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

ElkTime const elk_unix_epoch_timestamp = INT64_C(62135596800);

extern int64_t elk_num_leap_years_since_epoch(int64_t year);
extern int64_t elk_days_since_epoch(int year);

extern int64_t elk_time_to_unix_epoch(ElkTime time);
extern ElkTime elk_time_from_unix_timestamp(int64_t unixtime);
extern bool elk_is_leap_year(int year);
extern ElkTime elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes,
                                         int seconds);
extern ElkTime elk_make_time(ElkStructTime tm);
extern ElkStructTime elk_make_struct_time(ElkTime time);
extern ElkTime elk_time_truncate_to_specific_hour(ElkTime time, int hour);
extern ElkTime elk_time_truncate_to_hour(ElkTime time);
extern ElkTime elk_time_add(ElkTime time, int change_in_time);

/*-------------------------------------------------------------------------------------------------
 *                                         Hashes
 *-----------------------------------------------------------------------------------------------*/

uint64_t elk_fnv1a_hash_accumulate(size_t const n, void const *value, uint64_t const hash_so_far);
uint64_t elk_fnv1a_hash(size_t const n, void const *value);

/*-------------------------------------------------------------------------------------------------
 *                                       String Slice
 *-----------------------------------------------------------------------------------------------*/

_Static_assert(sizeof(size_t) == sizeof(uintptr_t), "size_t and uintptr_t aren't the same size?!");
_Static_assert(UINTPTR_MAX == SIZE_MAX, "sizt_t and uintptr_t dont' have same max?!");

extern ElkStr elk_str_from_cstring(char *src);
extern ElkStr elk_str_copy(size_t dst_len, char *restrict dest, ElkStr src);
extern int elk_str_cmp(ElkStr left, ElkStr right);
extern bool elk_str_eq(ElkStr left, ElkStr right);
extern ElkStr elk_str_strip(ElkStr input);

/*-------------------------------------------------------------------------------------------------
 *                                       String Interner
 *-----------------------------------------------------------------------------------------------*/
typedef struct ElkStringInternerHandle {
    uint64_t hash;
    ElkStr str;
} ElkStringInternerHandle;

struct ElkStringInterner {
    ElkArenaAllocator storage; // This is where to store the strings

    ElkStringInternerHandle *handles; // The hash table - handles index into storage
    uint32_t num_handles;             // The number of handles
    int8_t size_exp;                  // Used to keep track of the length of *handles
};

ElkStringInterner *
elk_string_interner_create(int8_t size_exp, int avg_string_size)
{
    assert(size_exp > 0 && size_exp <= 31);                 // Come on, 31 is HUGE
    assert(avg_string_size > 0 && avg_string_size <= 1000); // Come on, 1000 is a really big string.

    size_t const handles_len = 1 << size_exp;
    ElkStringInternerHandle *handles = calloc(handles_len, sizeof(*handles));
    assert(handles);

    size_t const storage_len = avg_string_size * (handles_len / 4);
    ElkArenaAllocator storage = {0};
    elk_arena_init(&storage, storage_len);

    ElkStringInterner *interner = malloc(sizeof(*interner));
    assert(interner);

    *interner = (ElkStringInterner){
        .storage = storage, .handles = handles, .num_handles = 0, .size_exp = size_exp};

    return interner;
}

void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    if (interner) {
        elk_allocator_destroy(&interner->storage);
        free(interner->handles);
        free(interner);
    }

    return;
}

static bool
elk_string_interner_table_large_enough(ElkStringInterner const *interner)
{
    // Shoot for no more than 50% of slots filled.
    return interner->num_handles <= (1 << (interner->size_exp - 1));
}

static uint32_t
elk_string_interner_lookup(uint64_t hash, int8_t exp, uint32_t idx)
{
    // Copied from https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.
    uint32_t mask = (UINT32_C(1) << exp) - 1;
    uint32_t step = (hash >> (64 - exp)) | 1; // the | 1 forces an odd number
    return (idx + step) & mask;
}

static void
elk_string_interner_expand_table(ElkStringInterner *interner)
{
    int8_t const size_exp = interner->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    size_t const handles_len = 1 << size_exp;
    size_t const new_handles_len = 1 << new_size_exp;

    ElkStringInternerHandle *new_handles = calloc(new_handles_len, sizeof(*new_handles));
    assert(new_handles);

    for (uint32_t i = 0; i < handles_len; i++) {
        ElkStringInternerHandle *handle = &interner->handles[i];

        // Check if it's empty - and if so skip it!
        if (handle->str.start == NULL)
            continue;

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash; // This truncates, but it's OK, the *_lookup function takes care of it.
        while (true) {
            j = elk_string_interner_lookup(hash, new_size_exp, j);
            ElkStringInternerHandle *new_handle = &new_handles[j];

            if (!new_handle->str.start) {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    free(interner->handles);
    interner->handles = new_handles;
    interner->size_exp = new_size_exp;

    return;
}

ElkStr
elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string)
{
    assert(interner && string);

    ElkStr str = elk_str_from_cstring(string);
    return elk_string_interner_intern(interner, str);
}

ElkStr
elk_string_interner_intern(ElkStringInterner *interner, ElkStr str)
{
    assert(interner);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash(str.len, str.start);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true) {
        i = elk_string_interner_lookup(hash, interner->size_exp, i);
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (!handle->str.start) {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_string_interner_table_large_enough(interner)) {

                char *dest = elk_allocator_nmalloc(&interner->storage, str.len + 1, char);
                ElkStr interned_str = elk_str_copy(str.len + 1, dest, str);

                *handle = (ElkStringInternerHandle){.hash = hash, .str = interned_str};
                interner->num_handles += 1;

                return handle->str;
            } else {
                // Grow the table so we have room
                elk_string_interner_expand_table(interner);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_string_interner_intern(interner, str);
            }
        } else if (handle->hash == hash && !elk_str_cmp(str, handle->str)) {
            // found it!
            return handle->str;
        }
    }
}

/*-------------------------------------------------------------------------------------------------
 *
 *                                          Memory
 *
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *                                    Static Arena Allocator
 *-----------------------------------------------------------------------------------------------*/
#ifndef NDEBUG
extern bool elk_is_power_of_2(uintptr_t p);
#endif

extern void elk_static_arena_init(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[]);
extern uintptr_t elk_align_pointer(uintptr_t ptr, size_t align);
extern void *elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment);

/*-------------------------------------------------------------------------------------------------
 *                                     Growable Arena Allocator
 *-----------------------------------------------------------------------------------------------*/

extern void elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size);
extern void elk_arena_free_blocks(ElkArenaAllocator *arena);
extern void elk_arena_init(ElkArenaAllocator *arena, size_t starting_block_size);
extern void elk_arena_reset(ElkArenaAllocator *arena);
extern void elk_arena_destroy(ElkArenaAllocator *arena);
extern void *elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment);

/*-------------------------------------------------------------------------------------------------
 *                                       Pool Allocator
 *-----------------------------------------------------------------------------------------------*/
extern void elk_static_pool_initialize_linked_list(unsigned char *buffer, size_t object_size,
                                                   size_t num_objects);
extern void elk_static_pool_init(ElkStaticPool *pool, size_t object_size, size_t num_objects,
                                 unsigned char buffer[]);
extern void elk_static_pool_reset(ElkStaticPool *pool);
extern void elk_static_pool_destroy(ElkStaticPool *pool);
extern void *elk_static_pool_alloc(ElkStaticPool *pool);
extern void elk_static_pool_free(ElkStaticPool *pool, void *ptr);

/*-------------------------------------------------------------------------------------------------
 *
 *
 *                                         Collections
 *
 *
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *
 *                                     Ordered Collections
 *
 *-----------------------------------------------------------------------------------------------*/

ptrdiff_t const ELK_COLLECTION_LEDGER_EMPTY = -1;
ptrdiff_t const ELK_COLLECTION_LEDGER_FULL = -2;

/*-------------------------------------------------------------------------------------------------
 *                                         Queue Ledger
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *                                            Array Ledger
 *-----------------------------------------------------------------------------------------------*/
