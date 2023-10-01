#include "elk.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-------------------------------------------------------------------------------------------------------------------------*/
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
extern ElkTime elk_make_time(ElkStructTime tm);

ElkTime
elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds)
{
    Assert(year >= 1 && year <= INT16_MAX);
    Assert(day >= 1 && day <= 31);
    Assert(month >= 1 && month <= 12);
    Assert(hour >= 0 && hour <= 23);
    Assert(minutes >= 0 && minutes <= 59);
    Assert(seconds >= 0 && seconds <= 59);

    // Seconds in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    ElkTime ts = (year - 1) * SECONDS_PER_YEAR + num_leap_years_since_epoch * SECONDS_PER_DAY;

    // Seconds in the months up to the start of this month
    int64_t const days_until_start_of_month =
        elk_is_leap_year(year) ? sum_days_to_month[1][month] : sum_days_to_month[0][month];
    ts += days_until_start_of_month * SECONDS_PER_DAY;

    // Seconds in the days of the month up to this one.
    ts += (day - 1) * SECONDS_PER_DAY;

    // Seconds in the hours, minutes, & seconds so far this day.
    ts += hour * SECONDS_PER_HOUR;
    ts += minutes * SECONDS_PER_MINUTE;
    ts += seconds;

    Assert(ts >= 0);

    return ts;
}

ElkTime 
elk_time_from_yd_and_hms(int year, int day_of_year, int hour, int minutes, int seconds)
{
    Assert(year >= 1 && year <= INT16_MAX);
    Assert(day_of_year >= 1 && day_of_year <= 366);
    Assert(hour >= 0 && hour <= 23);
    Assert(minutes >= 0 && minutes <= 59);
    Assert(seconds >= 0 && seconds <= 59);

    // Seconds in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    ElkTime ts = (year - 1) * SECONDS_PER_YEAR + num_leap_years_since_epoch * SECONDS_PER_DAY;

    // Seconds in the days up to now.
    ts += (day_of_year - 1) * SECONDS_PER_DAY;

    // Seconds in the hours, minutes, & seconds so far this day.
    ts += hour * SECONDS_PER_HOUR;
    ts += minutes * SECONDS_PER_MINUTE;
    ts += seconds;

    Assert(ts >= 0);

    return ts;
}

ElkStructTime 
elk_make_struct_time(ElkTime time)
{
    Assert(time >= 0);

    // Get the seconds part and then trim it off and convert to minutes
    int const second = time % SECONDS_PER_MINUTE;
    time = (time - second) / SECONDS_PER_MINUTE;
    Assert(time >= 0 && second >= 0 && second <= 59);

    // Get the minutes part, trim it off and convert to hours.
    int const minute = time % MINUTES_PER_HOUR;
    time = (time - minute) / MINUTES_PER_HOUR;
    Assert(time >= 0 && minute >= 0 && minute <= 59);

    // Get the hours part, trim it off and convert to days.
    int const hour = time % HOURS_PER_DAY;
    time = (time - hour) / HOURS_PER_DAY;
    Assert(time >= 0 && hour >= 0 && hour <= 23);

    // Rename variable for clarity
    int64_t const days_since_epoch = time;

    // Calculate the year
    int year = days_since_epoch / (DAYS_PER_YEAR) + 1; // High estimate, but good starting point.
    int64_t test_time = elk_days_since_epoch(year);
    while (test_time > days_since_epoch) 
    {
        int step = (test_time - days_since_epoch) / (DAYS_PER_YEAR + 1);
        step = step == 0 ? 1 : step;
        year -= step;
        test_time = elk_days_since_epoch(year);
    }
    Assert(test_time <= elk_days_since_epoch(year));
    time -= elk_days_since_epoch(year); // Now it's days since start of the year.
    Assert(time >= 0);

    // Calculate the month
    int month = 0;
    int leap_year_idx = elk_is_leap_year(year) ? 1 : 0;
    for (month = 1; month <= 11; month++)
    {
        if (sum_days_to_month[leap_year_idx][month + 1] > time) { break; }
    }
    Assert(time >= 0 && month > 0 && month <= 12);
    time -= sum_days_to_month[leap_year_idx][month]; // Now in days since start of month

    // Calculate the day
    int const day = time + 1;
    Assert(day > 0 && day <= 31);

    return (ElkStructTime)
    {
        .year = year,
        .month = month, 
        .day = day, 
        .hour = hour, 
        .minute = minute, 
        .second = second
    };
}

extern ElkTime elk_time_truncate_to_specific_hour(ElkTime time, int hour);
extern ElkTime elk_time_truncate_to_hour(ElkTime time);
extern ElkTime elk_time_add(ElkTime time, int change_in_time);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Hashes
 *-------------------------------------------------------------------------------------------------------------------------*/

uint64_t const fnv_offset_bias = 0xcbf29ce484222325;
uint64_t const fnv_prime = 0x00000100000001b3;

extern uint64_t elk_fnv1a_hash(size_t const n, void const *value);
extern uint64_t elk_fnv1a_hash_accumulate(size_t const n, void const *value, uint64_t const hash_so_far);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      String Slice
 *-------------------------------------------------------------------------------------------------------------------------*/

_Static_assert(sizeof(size_t) == sizeof(uintptr_t), "size_t and uintptr_t aren't the same size?!");
_Static_assert(UINTPTR_MAX == SIZE_MAX, "sizt_t and uintptr_t dont' have same max?!");

extern ElkStr elk_str_from_cstring(char *src);
extern ElkStr elk_str_copy(size_t dst_len, char *restrict dest, ElkStr src);
extern ElkStr elk_str_substr(ElkStr str, int start, int len);
extern int elk_str_cmp(ElkStr left, ElkStr right);
extern bool elk_str_eq(ElkStr left, ElkStr right);
extern ElkStr elk_str_strip(ElkStr input);

bool
elk_str_parse_int_64(ElkStr str, int64_t *result)
{
    // Empty string is an error
    StopIf(str.len == 0, return false);

    uint64_t parsed = 0;
    bool neg_flag = false;
    bool in_digits = false;
    char const *c = str.start;
    while (true)
    {
        // We've reached the end of the string.
        if (!*c || c >= (str.start + (uintptr_t)str.len)) { break; }

        // Is a digit?
        if (*c + 0U - '0' <= 9U) 
        {
            in_digits = true;                    // Signal we've passed +/- signs
            parsed = parsed * 10 + (*c - '0');   // Accumulate digits

        }
        else if (*c == '-' && !in_digits) { neg_flag = true; }
        else if (*c == '+' && !in_digits) { neg_flag = false; }
        else { return false; }

        // Move to the next character
        ++c;
    }

    *result = neg_flag ? -parsed : parsed;
    return true;
}

bool
elk_str_parse_float_64(ElkStr str, double *out)
{
#define ELK_NAN (0.0 / 0.0)
#define ELK_INF (1.0 / 0.0 )
#define ELK_NEG_INF (-1.0 / 0.0)

    StopIf(str.len == 0, goto ERR_RETURN);

    char const *c = str.start;
    char const *end = str.start + str.len;
    size_t len_remaining = str.len;

    int8_t sign = 0;        // 0 is positive, 1 is negative
    int8_t exp_sign = 0;    // 0 is positive, 1 is negative
    int16_t exponent = 0;
    int64_t mantissa = 0;
    int64_t extra_exp = 0;  // decimal places after the point

    // Check & parse a sign
    if (*c == '-')      { sign =  1; --len_remaining; ++c; }
    else if (*c == '+') { sign =  0; --len_remaining; ++c; }

    // check for nan/NAN/NaN/Nan inf/INF/Inf
    if (len_remaining == 3) 
    {
        if(memcmp(c, "nan", 3) == 0 || memcmp(c, "NAN", 3) == 0 ||
                memcmp(c, "NaN", 3) == 0 || memcmp(c, "Nan", 3) == 0) 
        { *out = ELK_NAN; return true; }

        if(memcmp(c, "inf", 3) == 0 || memcmp(c, "Inf", 3) == 0 || memcmp(c, "INF", 3) == 0)
        {
            if(sign == 0) *out = ELK_INF;
            else if(sign == 1) *out = ELK_NEG_INF;
            return true; 
        }
    }

    // check for infinity/INFINITY/Infinity
    if (len_remaining == 8) 
    {
        if(memcmp(c, "infinity", 8) == 0 ||
                memcmp(c, "Infinity", 8) == 0 || memcmp(c, "INFINITY", 8) == 0)
        {
            if(sign == 0) *out = ELK_INF;
            else if(sign == 1) *out = ELK_NEG_INF;
            return true; 
        }
    }

    // Parse the mantissa up to the decimal point or exponent part
    char digit = *c - '0';
    while (c < end && digit  < 10 && digit  >= 0)
    {
        mantissa = mantissa * 10 + digit;
        ++c;
        digit = *c - '0';
    }

    // Check for the decimal point
    if (c < end && *c == '.')
    {
        ++c;

        // Parse the mantissa up to the decimal point or exponent part
        digit = *c - '0';
        while (c < end && digit < 10 && digit >= 0)
        {
            // overflow check
            StopIf((INT64_MAX - digit) / 10 < mantissa, goto ERR_RETURN); 

            mantissa = mantissa * 10 + digit;
            extra_exp += 1;

            ++c;
            digit = *c - '0';
        }
    }

    // Account for negative signs
    mantissa = sign == 1 ? -mantissa : mantissa;

    // Start the exponent
    if (c < end && (*c == 'e' || *c == 'E')) 
    {
        ++c;

        if (*c == '-') { exp_sign = 1; ++c; }
        else if (*c == '+') { exp_sign = 0; ++c; }

        // Parse the mantissa up to the decimal point or exponent part
        digit = *c - '0';
        while (c < end && digit < 10 && digit >= 0)
        {
            // Overflow check
            StopIf((INT16_MAX - digit) / 10 < exponent, goto ERR_RETURN); 

            exponent = exponent * 10 + digit;

            ++c;
            digit = *c - '0';
        }

        // Account for negative signs
        exponent = exp_sign == 1 ? -exponent : exponent;
    }

    // Once we get here we're done. Should be end of string.
    StopIf( c!= end, goto ERR_RETURN);

    // Account for decimal point location.
    exponent -= extra_exp;

    // Check for overflow
    StopIf(exponent < -307 || exponent > 308, goto ERR_RETURN);

    double exp_part = 1.0;
    for (int i = 0; i < exponent; ++i) { exp_part *= 10.0; }
    for (int i = 0; i > exponent; --i) { exp_part /= 10.0; }

    double value = (double)mantissa * exp_part;
    StopIf(value == ELK_INF || value == ELK_NEG_INF, goto ERR_RETURN);

    *out = value;
    return true;

ERR_RETURN:
    *out = ELK_NAN;
    return false;

#undef ELK_NAN
#undef ELK_INF
#undef ELK_NEG_INF
}

bool
elk_str_parse_datetime(ElkStr str, ElkTime *out)
{
    // Check the length to find out what type of string we are parsing.
    if(str.len == 19)
    {
        // YYYY-MM-DD HH:MM:SS and YYYY-MM-DDTHH:MM:SS formats
        int64_t year = INT64_MIN;
        int64_t month = INT64_MIN;
        int64_t day = INT64_MIN;
        int64_t hour = INT64_MIN;
        int64_t minutes = INT64_MIN;
        int64_t seconds = INT64_MIN;

        if(
            elk_str_parse_int_64(elk_str_substr(str,  0, 4), &year    ) && 
            elk_str_parse_int_64(elk_str_substr(str,  5, 2), &month   ) &&
            elk_str_parse_int_64(elk_str_substr(str,  8, 2), &day     ) &&
            elk_str_parse_int_64(elk_str_substr(str, 11, 2), &hour    ) &&
            elk_str_parse_int_64(elk_str_substr(str, 14, 2), &minutes ) &&
            elk_str_parse_int_64(elk_str_substr(str, 17, 2), &seconds ))
        {
            *out = elk_time_from_ymd_and_hms(year, month, day, hour, minutes, seconds);
            return true;
        }

        return false;
    }
    else if(str.len == 13)
    {
        // YYYYDDDHHMMSS format
        int64_t year = INT64_MIN;
        int64_t day_of_year = INT64_MIN;
        int64_t hour = INT64_MIN;
        int64_t minutes = INT64_MIN;
        int64_t seconds = INT64_MIN;

        if(
            elk_str_parse_int_64(elk_str_substr(str,  0, 4), &year        ) && 
            elk_str_parse_int_64(elk_str_substr(str,  4, 3), &day_of_year ) &&
            elk_str_parse_int_64(elk_str_substr(str,  7, 2), &hour        ) &&
            elk_str_parse_int_64(elk_str_substr(str,  9, 2), &minutes     ) &&
            elk_str_parse_int_64(elk_str_substr(str, 11, 2), &seconds     ))
        {
            *out = elk_time_from_yd_and_hms(year, day_of_year, hour, minutes, seconds);
            return true;
        }

        return false;
    }
    return false;
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     String Interner
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct ElkStringInternerHandle 
{
    uint64_t hash;
    ElkStr str;
} ElkStringInternerHandle;

struct ElkStringInterner 
{
    ElkArenaAllocator storage;        // This is where to store the strings

    ElkStringInternerHandle *handles; // The hash table - handles index into storage
    uint32_t num_handles;             // The number of handles
    int8_t size_exp;                  // Used to keep track of the length of *handles
};

ElkStringInterner *
elk_string_interner_create(int8_t size_exp, int avg_string_size)
{
    Assert(size_exp > 0 && size_exp <= 31);                 // Come on, 31 is HUGE
    Assert(avg_string_size > 0 && avg_string_size <= 1000); // Come on, 1000 is a really big string.

    size_t const handles_len = 1 << size_exp;
    ElkStringInternerHandle *handles = calloc(handles_len, sizeof(*handles));
    Assert(handles);

    size_t const storage_len = avg_string_size * (handles_len / 4);
    ElkArenaAllocator storage = {0};
    elk_arena_init(&storage, storage_len);

    ElkStringInterner *interner = malloc(sizeof(*interner));

    *interner = (ElkStringInterner)
    {
        .storage = storage,
        .handles = handles,
        .num_handles = 0, 
        .size_exp = size_exp
    };

    return interner;
}

void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    if (interner)
    {
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
    uint32_t step = (hash >> (64 - exp)) | 1;    // the | 1 forces an odd number
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
    Assert(new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (handle->str.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash; // This truncates, but it's OK, the *_lookup function takes care of it.
        while (true) 
        {
            j = elk_string_interner_lookup(hash, new_size_exp, j);
            ElkStringInternerHandle *new_handle = &new_handles[j];

            if (!new_handle->str.start)
            {
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
    Assert(interner && string);

    ElkStr str = elk_str_from_cstring(string);
    return elk_string_interner_intern(interner, str);
}

ElkStr
elk_string_interner_intern(ElkStringInterner *interner, ElkStr str)
{
    Assert(interner);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash(str.len, str.start);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true)
    {
        i = elk_string_interner_lookup(hash, interner->size_exp, i);
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (!handle->str.start)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_string_interner_table_large_enough(interner))
            {

                char *dest = elk_allocator_nmalloc(&interner->storage, str.len + 1, char);
                ElkStr interned_str = elk_str_copy(str.len + 1, dest, str);

                *handle = (ElkStringInternerHandle){.hash = hash, .str = interned_str};
                interner->num_handles += 1;

                return handle->str;
            }
            else 
            {
                // Grow the table so we have room
                elk_string_interner_expand_table(interner);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_string_interner_intern(interner, str);
            }
        }
        else if (handle->hash == hash && !elk_str_cmp(str, handle->str)) 
        {
            // found it!
            return handle->str;
        }
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                         Memory
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Static Arena Allocator
 *-------------------------------------------------------------------------------------------------------------------------*/
#ifndef NDEBUG
extern bool elk_is_power_of_2(uintptr_t p);
#endif

extern void elk_static_arena_init(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[]);
extern void elk_static_arena_destroy(ElkStaticArena *arena);
extern void elk_static_arena_reset(ElkStaticArena *arena);
extern uintptr_t elk_align_pointer(uintptr_t ptr, size_t align);
extern void *elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment);
extern void elk_static_arena_free(ElkStaticArena *arena, void *ptr);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Growable Arena Allocator
 *-------------------------------------------------------------------------------------------------------------------------*/

extern void elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size);
extern void elk_arena_free_blocks(ElkArenaAllocator *arena);
extern void elk_arena_init(ElkArenaAllocator *arena, size_t starting_block_size);
extern void elk_arena_reset(ElkArenaAllocator *arena);
extern void elk_arena_destroy(ElkArenaAllocator *arena);
inline void elk_arena_free(ElkStaticArena *arena, void *ptr);
extern void *elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Static Pool Allocator
 *-------------------------------------------------------------------------------------------------------------------------*/
extern void elk_static_pool_initialize_linked_list(unsigned char *buffer, size_t object_size,
                                                   size_t num_objects);
extern void elk_static_pool_init(ElkStaticPool *pool, size_t object_size, size_t num_objects, unsigned char buffer[]);
extern void elk_static_pool_reset(ElkStaticPool *pool);
extern void elk_static_pool_destroy(ElkStaticPool *pool);
extern void *elk_static_pool_alloc(ElkStaticPool *pool);
extern void elk_static_pool_free(ElkStaticPool *pool, void *ptr);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Generic Allocator API
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                       Collections
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------------------------
 *                                         
 *                                                   Ordered Collections
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

int64_t const ELK_COLLECTION_EMPTY = -1;
int64_t const ELK_COLLECTION_FULL = -2;

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Queue Ledger
 *-------------------------------------------------------------------------------------------------------------------------*/
extern ElkQueueLedger elk_queue_ledger_create(size_t capacity);
extern bool elk_queue_ledger_full(ElkQueueLedger *queue);
extern bool elk_queue_ledger_empty(ElkQueueLedger *queue);
extern int64_t elk_queue_ledger_push_back_index(ElkQueueLedger *queue);
extern int64_t elk_queue_ledger_pop_front_index(ElkQueueLedger *queue);
extern int64_t elk_queue_ledger_peek_front_index(ElkQueueLedger *queue);
extern size_t elk_queue_ledger_len(ElkQueueLedger const *queue);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Array Ledger
 *-------------------------------------------------------------------------------------------------------------------------*/
extern ElkArrayLedger elk_array_ledger_create(size_t capacity);
extern bool elk_array_ledger_full(ElkArrayLedger *array);
extern bool elk_array_ledger_empty(ElkArrayLedger *array);
extern int64_t elk_array_ledger_push_back_index(ElkArrayLedger *array);
extern size_t elk_array_ledger_len(ElkArrayLedger const *array);
extern void elk_array_ledger_reset(ElkArrayLedger *array);
extern void elk_array_ledger_set_capacity(ElkArrayLedger *array, size_t capacity);

