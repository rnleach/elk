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
 *                                                      String Slice
 *-------------------------------------------------------------------------------------------------------------------------*/

_Static_assert(sizeof(size_t) == sizeof(uintptr_t), "size_t and uintptr_t aren't the same size?!");
_Static_assert(UINTPTR_MAX == SIZE_MAX, "size_t and uintptr_t dont' have same max?!");

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
	// The following block is required to create NAN/INF witnout using math.h on MSVC Using
	// #define NAN (0.0/0.0) doesn't work either on MSVC, which gives C2124 divide by zero error.
	static double const ELK_ZERO = 0.0;
	double const ELK_INF = 1.0 / ELK_ZERO;
	double const ELK_NEG_INF = -1.0 / ELK_ZERO;
	double const ELK_NAN = 0.0 / ELK_ZERO;

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
        if(memcmp(c, "nan", 3) == 0 || memcmp(c, "NAN", 3) == 0 || memcmp(c, "NaN", 3) == 0 || memcmp(c, "Nan", 3) == 0) 
        { 
            *out = ELK_NAN; return true;
        }

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
        if(memcmp(c, "infinity", 8) == 0 || memcmp(c, "Infinity", 8) == 0 || memcmp(c, "INFINITY", 8) == 0)
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
 *                                                      Hashes
 *-------------------------------------------------------------------------------------------------------------------------*/

uint64_t const fnv_offset_bias = 0xcbf29ce484222325;
uint64_t const fnv_prime = 0x00000100000001b3;

extern uint64_t elk_fnv1a_hash(size_t const n, void const *value);
extern uint64_t elk_fnv1a_hash_accumulate(size_t const n, void const *value, uint64_t const hash_so_far);
extern uint64_t elk_fnv1a_hash_str(ElkStr str);

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
    elk_arena_create(&storage, storage_len);

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
elk_hash_table_large_enough(size_t num_handles, int8_t size_exp)
{
    // Shoot for no more than 50% of slots filled.
    return num_handles <= (1 << (size_exp - 1));
}

static uint32_t
elk_hash_lookup(uint64_t hash, int8_t exp, uint32_t idx)
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
            j = elk_hash_lookup(hash, new_size_exp, j);
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

    uint64_t const hash = elk_fnv1a_hash_str(str);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true)
    {
        i = elk_hash_lookup(hash, interner->size_exp, i);
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (!handle->str.start)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(interner->num_handles, interner->size_exp))
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

extern void elk_static_arena_create(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[]);
extern void elk_static_arena_destroy(ElkStaticArena *arena);
extern void elk_static_arena_reset(ElkStaticArena *arena);
extern uintptr_t elk_align_pointer(uintptr_t ptr, size_t align);
extern void *elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment);
extern void * elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, size_t size);
extern void elk_static_arena_free(ElkStaticArena *arena, void *ptr);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Growable Arena Allocator
 *-------------------------------------------------------------------------------------------------------------------------*/

extern void elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size);
extern void elk_arena_free_blocks(ElkArenaAllocator *arena);
extern void elk_arena_create(ElkArenaAllocator *arena, size_t starting_block_size);
extern void elk_arena_reset(ElkArenaAllocator *arena);
extern void elk_arena_destroy(ElkArenaAllocator *arena);
inline void elk_arena_free(ElkArenaAllocator *arena, void *ptr);
extern void *elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Static Pool Allocator
 *-------------------------------------------------------------------------------------------------------------------------*/
extern void elk_static_pool_initialize_linked_list(unsigned char *buffer, size_t object_size, size_t num_objects);
extern void elk_static_pool_create(ElkStaticPool *pool, size_t object_size, size_t num_objects, unsigned char buffer[]);
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

/*---------------------------------------------------------------------------------------------------------------------------
 *                                         
 *                                                  Unordered Collections
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    Hash Map (Table)
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct 
{
    uint64_t hash;
    void *key;
	void *value;
} ElkHashMapHandle;

struct ElkHashMap
{
	int8_t size_exp;
	ElkHashMapHandle *handles;
    size_t num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
};

ElkHashMap *
elk_hash_map_create(int8_t size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq)
{
    Assert(size_exp > 0 && size_exp <= 31);                 // Come on, 31 is HUGE

    size_t const handles_len = 1 << size_exp;
    ElkHashMapHandle *handles = calloc(handles_len, sizeof(*handles));
    Assert(handles);

    ElkHashMap *map = malloc(sizeof(*map));

    *map = (ElkHashMap)
    {
        .size_exp = size_exp,
        .handles = handles,
        .num_handles = 0, 
        .hasher = key_hash,
        .eq = key_eq
    };

    return map;
}

void 
elk_hash_map_destroy(ElkHashMap *map)
{
    if(map)
    {
        free(map->handles);
        free(map);
    }
}

static void
elk_hash_table_expand(ElkHashMap *map)
{
    int8_t const size_exp = map->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    size_t const handles_len = 1 << size_exp;
    size_t const new_handles_len = 1 << new_size_exp;

    ElkHashMapHandle *new_handles = calloc(new_handles_len, sizeof(*new_handles));
    Assert(new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkHashMapHandle *handle = &map->handles[i];

        if (handle->key == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash; // This truncates, but it's OK, the *_lookup function takes care of it.
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkHashMapHandle *new_handle = &new_handles[j];

            if (!new_handle->key)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    free(map->handles);
    map->handles = new_handles;
    map->size_exp = new_size_exp;

    return;
}

void *
elk_hash_map_insert(ElkHashMap *map, void *key, void *value)
{
    Assert(map);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = map->hasher(key);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkHashMapHandle *handle = &map->handles[i];

        if (!handle->key)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(map->num_handles, map->size_exp))
            {

                *handle = (ElkHashMapHandle){.hash = hash, .key=key, .value=value};
                map->num_handles += 1;

                return handle->value;
            }
            else 
            {
                // Grow the table so we have room
                elk_hash_table_expand(map);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_hash_map_insert(map, key, value);
            }
        }
        else if (handle->hash == hash && map->eq(handle->key, key)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

void *
elk_hash_map_lookup(ElkHashMap *map, void *key)
{
    Assert(map);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = map->hasher(key);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkHashMapHandle *handle = &map->handles[i];

        if (!handle->key)
        {
            return NULL;
            
        }
        else if (handle->hash == hash && map->eq(handle->key, key)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

ElkHashMapKeyIter 
elk_hash_map_key_iter(ElkHashMap *map)
{
	return 0;
}

void *
elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter)
{
	size_t const max_iter = (1 << map->size_exp);
	void *next_key = NULL;
	if(*iter >= max_iter) { return next_key; }

	do
	{
		next_key = map->handles[*iter].key;
		*iter += 1;

	} while(next_key == NULL && *iter < max_iter);

	return next_key;
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                            Hash Map (Table, ElkStr as keys)
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct
{
    uint64_t hash;
    ElkStr key;
	void *value;
} ElkStrMapHandle;

struct ElkStrMap
{
	int8_t size_exp;
	ElkStrMapHandle *handles;
    size_t num_handles;
};

ElkStrMap *
elk_str_map_create(int8_t size_exp)
{
    Assert(size_exp > 0 && size_exp <= 31);                 // Come on, 31 is HUGE

    size_t const handles_len = 1 << size_exp;
    ElkStrMapHandle *handles = calloc(handles_len, sizeof(*handles));
    Assert(handles);

    ElkStrMap *map = malloc(sizeof(*map));

    *map = (ElkStrMap)
    {
        .size_exp = size_exp,
        .handles = handles,
        .num_handles = 0, 
    };

    return map;
}

void
elk_str_map_destroy(ElkStrMap *map)
{
    if(map)
    {
        free(map->handles);
        free(map);
    }
}

static void
elk_str_table_expand(ElkStrMap *map)
{
    int8_t const size_exp = map->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    size_t const handles_len = 1 << size_exp;
    size_t const new_handles_len = 1 << new_size_exp;

    ElkStrMapHandle *new_handles = calloc(new_handles_len, sizeof(*new_handles));
    Assert(new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkStrMapHandle *handle = &map->handles[i];

        if (handle->key.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash; // This truncates, but it's OK, the *_lookup function takes care of it.
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkStrMapHandle *new_handle = &new_handles[j];

            if (!new_handle->key.start)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    free(map->handles);
    map->handles = new_handles;
    map->size_exp = new_size_exp;

    return;
}

void *
elk_str_map_insert(ElkStrMap *map, ElkStr key, void *value)
{
    Assert(map);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash_str(key);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkStrMapHandle *handle = &map->handles[i];

        if (!handle->key.start)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(map->num_handles, map->size_exp))
            {

                *handle = (ElkStrMapHandle){.hash = hash, .key=key, .value=value};
                map->num_handles += 1;

                return handle->value;
            }
            else 
            {
                // Grow the table so we have room
                elk_str_table_expand(map);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_str_map_insert(map, key, value);
            }
        }
        else if (handle->hash == hash && !elk_str_cmp(key, handle->key)) 
        {
            // found it!
            return handle->value;
        }
    }
}

void *
elk_str_map_lookup(ElkStrMap *map, ElkStr key)
{
    Assert(map);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash_str(key);
    uint32_t i = hash; // I know it truncates, but it's OK, the *_lookup function takes care of it.
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkStrMapHandle *handle = &map->handles[i];

        if (!handle->key.start) { return NULL; }
        else if (handle->hash == hash && !elk_str_cmp(key, handle->key)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

ElkHashMapKeyIter 
elk_str_map_key_iter(ElkStrMap *map)
{
	return 0;
}

ElkStr 
elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter)
{
	size_t const max_iter = (1 << map->size_exp);
	if(*iter >= max_iter) 
	{
		return (ElkStr){.start=NULL, .len=0};
	}

	ElkStr next_key = map->handles[*iter].key;
	*iter += 1;
	while(next_key.start == NULL && *iter < max_iter)
	{
		next_key = map->handles[*iter].key;
		*iter += 1;
	}

	return next_key;
}

