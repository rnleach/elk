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

bool
elk_str_parse_int_64(ElkStr str, int64_t *result)
{
    // Empty string is an error
    if (str.len == 0) {
        return false;
    }

    uint64_t parsed = 0;
    bool neg_flag = false;
    bool in_digits = false;
    char const *c = str.start;
    while (true) {
        // We've reached the end of the string.
        if (!*c || c >= (str.start + (uintptr_t)str.len)) {
            break;
        }

        // Is a digit?
        if (*c + 0U - '0' <= 9U) {
            in_digits = true; // Signal we've passed +/- signs

            // Accumulate digits
            parsed = parsed * 10 + (*c - '0');

        } else if (*c == '-' && !in_digits) {
            neg_flag = true;
        } else if (*c == '+' && !in_digits) {
            neg_flag = false;
        } else {
            return false;
        }

        // Move to the next character
        ++c;
    }

    *result = neg_flag ? -parsed : parsed;
    return true;
}

bool
elk_str_parse_float_64(ElkStr str, double *out)
{
    StopIf(str.len == 0, goto ERR_RETURN);

    // clang-format off

    // check for nan/NAN/NaN/Nan inf/INF/Inf
    if (str.len == 3) {
        char *c = str.start;

        if (c[0] == 'n' && c[1] == 'a' && c[2] == 'n') { *out = NAN; return true; }
        if (c[0] == 'N' && c[1] == 'A' && c[2] == 'N') { *out = NAN; return true; }
        if (c[0] == 'N' && c[1] == 'a' && c[2] == 'N') { *out = NAN; return true; }
        if (c[0] == 'N' && c[1] == 'a' && c[2] == 'n') { *out = NAN; return true; }

        if (c[0] == 'i' && c[1] == 'n' && c[2] == 'f') { *out = INFINITY; return true; }
        if (c[0] == 'I' && c[1] == 'n' && c[2] == 'f') { *out = INFINITY; return true; }
        if (c[0] == 'I' && c[1] == 'N' && c[2] == 'F') { *out = INFINITY; return true; }
    }

    // check for -inf/-INF/-Inf
    if (str.len == 4 && str.start[0] == '-') {
        char *c = str.start;

        if (c[1] == 'i' && c[2] == 'n' && c[3] == 'f') { *out = -INFINITY; return true; }
        if (c[1] == 'I' && c[2] == 'n' && c[3] == 'f') { *out = -INFINITY; return true; }
        if (c[1] == 'I' && c[2] == 'N' && c[3] == 'F') { *out = -INFINITY; return true; }
    }

    // clang-format on

    int8_t sign = 0;     // 0 is positive, 1 is negative
    int8_t exp_sign = 0; // 0 is positive, 1 is negative
    int16_t exponent = 0;
    int64_t mantissa = 0;
    int64_t extra_exp = 0; // decimal places after the point

    char const *c = str.start;
    char const *end = str.start + str.len;

    // Check parse a sign
    if (*c == '-') {
        sign = 1;
        ++c;
    } else if (*c == '+') {
        sign = 0;
        ++c;
    }

    // Parse the mantissa up to the decimal point or exponent part
    while (c < end && (*c - '0') < 10 && (*c - '0') >= 0) {
        mantissa = mantissa * 10 + (*c - '0');
        ++c;
    }

    // Check for the decimal point
    if (c < end && *c == '.') {
        ++c;

        // Parse the mantissa up to the decimal point or exponent part
        while (c < end && (*c - '0') < 10 && (*c - '0') >= 0) {
            char val = *c - '0';

            // overflow check
            if ((INT64_MAX - val) / 10 < mantissa) {
                goto ERR_RETURN;
            }

            mantissa = mantissa * 10 + val;
            extra_exp += 1;
            ++c;
        }
    }

    // Account for negative signs
    mantissa = sign == 1 ? -mantissa : mantissa;

    // Start the exponent
    if (c < end && (*c == 'e' || *c == 'E')) {
        ++c;

        if (*c == '-') {
            exp_sign = 1;
            ++c;
        } else if (*c == '+') {
            exp_sign = 0;
            ++c;
        }

        // Parse the mantissa up to the decimal point or exponent part
        while (c < end && (*c - '0') < 10 && (*c - '0') >= 0) {
            char val = *c - '0';

            // Overflow check
            if ((INT16_MAX - val) / 10 < exponent) {
                goto ERR_RETURN;
            }

            exponent = exponent * 10 + val;
            ++c;
        }

        // Account for negative signs
        exponent = exp_sign == 1 ? -exponent : exponent;
    }

    // Once we get here we're done. Should be end of string.
    if (c != end) {
        goto ERR_RETURN;
    }

    // Account for decimal point location.
    exponent -= extra_exp;

    // Check for overflow
    StopIf(exponent < -307 || exponent > 308, goto ERR_RETURN);

    double exp_part = 1.0;
    for (int i = 0; i < exponent; ++i) {
        exp_part *= 10.0;
    }
    for (int i = 0; i > exponent; --i) {
        exp_part /= 10.0;
    }

    double value = (double)mantissa * exp_part;
    if (isinf(value)) {
        goto ERR_RETURN;
    }
    *out = value;
    return true;

ERR_RETURN:
    *out = NAN;
    return false;
}

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
