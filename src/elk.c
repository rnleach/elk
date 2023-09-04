#include "elk.h"

#include <assert.h>
#include <string.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
static int64_t const SECONDS_PER_MINUTE = INT64_C(60);
static int64_t const MINUTES_PER_HOUR = INT64_C(60);
static int64_t const HOURS_PER_DAY = INT64_C(24);
static int64_t const DAYS_PER_YEAR = INT64_C(365);
static int64_t const SECONDS_PER_HOUR = MINUTES_PER_HOUR * SECONDS_PER_MINUTE;
static int64_t const SECONDS_PER_DAY = SECONDS_PER_HOUR * HOURS_PER_DAY;
static int64_t const SECONDS_PER_YEAR = SECONDS_PER_DAY * DAYS_PER_YEAR;

// Days in a year up to beginning of month
static int64_t const sum_days_to_month[2][13] = {
    {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

static int64_t
elk_num_leap_years_since_epoch(int64_t year)
{
    assert(year >= 1);

    year -= 1;
    return year / 4 - year / 100 + year / 400;
}

ElkTime const elk_unix_epoch_timestamp = INT64_C(62135596800);

bool
elk_is_leap_year(int year)
{
    if (year % 4 != 0)
        return false;
    if (year % 100 == 0 && year % 400 != 0)
        return false;
    return true;
}

ElkTime
elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds)
{
    assert(year >= 1 && year <= INT16_MAX);
    assert(day >= 1 && day <= 31);
    assert(month >= 1 && month <= 12);
    assert(hour >= 0 && hour <= 23);
    assert(minutes >= 0 && minutes <= 59);
    assert(seconds >= 0 && seconds <= 59);

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

    assert(ts >= 0);

    return ts;
}

ElkTime
elk_make_time(ElkStructTime tm)
{
    return elk_time_from_ymd_and_hms(tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
}

static int64_t
elk_days_since_epoch(int year)
{
    // Days in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    int64_t ts = (year - 1) * DAYS_PER_YEAR + num_leap_years_since_epoch;

    return ts;
}

ElkStructTime
elk_make_struct_time(ElkTime time)
{
    assert(time >= 0);

    // Get the seconds part and then trim it off and convert to minutes
    int const second = time % SECONDS_PER_MINUTE;
    time = (time - second) / SECONDS_PER_MINUTE;
    assert(time >= 0 && second >= 0 && second <= 59);

    // Get the minutes part, trim it off and convert to hours.
    int const minute = time % MINUTES_PER_HOUR;
    time = (time - minute) / MINUTES_PER_HOUR;
    assert(time >= 0 && minute >= 0 && minute <= 59);

    // Get the hours part, trim it off and convert to days.
    int const hour = time % HOURS_PER_DAY;
    time = (time - hour) / HOURS_PER_DAY;
    assert(time >= 0 && hour >= 0 && hour <= 23);

    // Rename variable for clarity
    int64_t const days_since_epoch = time;

    // Calculate the year
    int year = days_since_epoch / (DAYS_PER_YEAR) + 1; // High estimate, but good starting point.
    int64_t test_time = elk_days_since_epoch(year);
    while (test_time > days_since_epoch) {
        int step = (test_time - days_since_epoch) / (DAYS_PER_YEAR + 1);
        step = step == 0 ? 1 : step;
        year -= step;
        test_time = elk_days_since_epoch(year);
    }
    assert(test_time <= elk_days_since_epoch(year));
    time -= elk_days_since_epoch(year); // Now it's days since start of the year.
    assert(time >= 0);

    // Calculate the month
    int month = 0;
    int leap_year_idx = elk_is_leap_year(year) ? 1 : 0;
    for (month = 1; month <= 11; month++) {
        if (sum_days_to_month[leap_year_idx][month + 1] > time) {
            break;
        }
    }
    assert(time >= 0 && month > 0 && month <= 12);
    time -= sum_days_to_month[leap_year_idx][month]; // Now in days since start of month

    // Calculate the day
    int const day = time + 1;
    assert(day > 0 && day <= 31);

    return (ElkStructTime){
        .year = year, .month = month, .day = day, .hour = hour, .minute = minute, .second = second};
}

ElkTime
elk_time_truncate_to_specific_hour(ElkTime time, int hour)
{
    assert(hour >= 0 && hour <= 23 && time >= 0);

    ElkTime adjusted = time;

    int64_t const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    int64_t const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    int64_t actual_hour = (adjusted / SECONDS_PER_HOUR) % HOURS_PER_DAY;
    if (actual_hour < hour) {
        actual_hour += 24;
    }

    int64_t change_hours = actual_hour - hour;
    assert(change_hours >= 0);
    adjusted -= change_hours * SECONDS_PER_HOUR;

    assert(adjusted >= 0);

    return adjusted;
}

ElkTime
elk_time_truncate_to_hour(ElkTime time)
{
    ElkTime adjusted = time;

    int64_t const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    int64_t const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    assert(adjusted >= 0);

    return adjusted;
}

ElkTime
elk_time_add(ElkTime time, int change_in_time)
{
    ElkTime result = time + change_in_time;
    assert(result >= 0);
    return result;
}

/*-------------------------------------------------------------------------------------------------
 *                                       String Interner
 *-----------------------------------------------------------------------------------------------*/
typedef struct ElkStringInternerHandle {
    uint64_t hash;
    uint32_t position;
} ElkStringInternerHandle;

struct ElkStringInterner {
    char *storage;                  // This is where to store the strings
    uint32_t storage_len;           // The length of the storage in bytes
    uint32_t next_storage_location; // The next available place to store a string

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
    char *storage = calloc(storage_len, sizeof(*storage));
    assert(storage);

    ElkStringInterner *interner = malloc(sizeof(*interner));
    assert(interner);

    *interner = (ElkStringInterner){
        .handles = handles,
        .storage = storage,
        .storage_len = storage_len,
        .next_storage_location = 1, // Don't start at 0, that is an empty flag in the handles table.
        .size_exp = size_exp};

    return interner;
}

void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    if (interner) {
        free(interner->storage);
        free(interner->handles);
        free(interner);
    }

    return;
}

static bool
elk_string_interner_has_enough_storage(ElkStringInterner const *interner, size_t strlen)
{
    return (interner->storage_len - interner->next_storage_location) > (strlen + 1);
}

static bool
elk_string_interner_table_large_enough(ElkStringInterner const *interner)
{
    // Shoot for no more than 50% of slots filled.
    return interner->num_handles <= (1 << (interner->size_exp - 1));
}

static void
elk_string_interner_expand_storage(ElkStringInterner *interner)
{
    assert(interner && interner->storage);

    size_t new_storage_len = 3 * interner->storage_len / 2;
    char *new_storage = realloc(interner->storage, new_storage_len);
    assert(new_storage);

    interner->storage = new_storage;
    interner->storage_len = new_storage_len;

    return;
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
        if (handle->position == 0)
            continue;

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash; // This truncates, but it's OK, the *_lookup function takes care of it.
        while (true) {
            j = elk_string_interner_lookup(hash, new_size_exp, j);
            ElkStringInternerHandle *new_handle = &new_handles[j];

            if (!new_handle->position) {
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

ElkInternedString
elk_string_interner_intern(ElkStringInterner *interner, char const *string)
{
    assert(interner);

    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.
    size_t str_len = strlen(string);
    uint64_t const hash = elk_fnv1a_hash(str_len, string);
    uint32_t i =
        hash; // I know this truncates, but it's OK, the *_lookup function takes care of it.
    while (true) {
        i = elk_string_interner_lookup(hash, interner->size_exp, i);
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (!handle->position) {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_string_interner_table_large_enough(interner)) {

                // Check if we have enough storage for the string.
                while (!elk_string_interner_has_enough_storage(interner, str_len)) {
                    elk_string_interner_expand_storage(interner);
                }

                *handle = (ElkStringInternerHandle){.hash = hash,
                                                    .position = interner->next_storage_location};

                strcpy(&interner->storage[interner->next_storage_location], string);
                interner->next_storage_location += (str_len + 1);
                interner->num_handles += 1;

                return handle->position;
            } else {
                // Grow the table so we have room
                elk_string_interner_expand_table(interner);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_string_interner_intern(interner, string);
            }
        } else if (handle->hash == hash && !strcmp(&interner->storage[handle->position], string)) {
            // found it!
            return handle->position;
        }
    }
}

char const *
elk_string_interner_retrieve(ElkStringInterner const *interner, ElkInternedString const position)
{
    assert(interner);

    return &interner->storage[position];
}

/*-------------------------------------------------------------------------------------------------
 *                                       Arena Allocator
 *-----------------------------------------------------------------------------------------------*/

typedef struct ElkArenaBlockHeader {
    unsigned char *next_block;
    size_t next_block_size;
} ElkArenaBlockHeader;

static void
elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size)
{
    uint32_t max_block_size = arena->buf_size > block_size ? arena->buf_size : block_size;

    unsigned char *buffer = calloc(max_block_size, 1);
    PanicIf(!buffer, "out of memory");

    // Put the header in the front of the new block
    ElkArenaBlockHeader *header = (ElkArenaBlockHeader *)&buffer[0];
    *header =
        (ElkArenaBlockHeader){.next_block = arena->buffer, .next_block_size = arena->buf_size};

    arena->buffer = buffer;
    arena->buf_size = max_block_size;
    arena->buf_offset = sizeof(*header);
}

void
elk_arena_initialize(ElkArenaAllocator *arena, size_t starting_block_size)
{
    assert(arena);
    assert(starting_block_size > 8);

    // Zero everything out.
    *arena = (ElkArenaAllocator){.buffer = NULL, .buf_size = 0, .buf_offset = 0};

    elk_arena_add_block(arena, starting_block_size);
}

static void
elk_arena_free_blocks(ElkArenaAllocator *arena)
{
    assert(arena);
    ElkArenaBlockHeader *header = (ElkArenaBlockHeader *)&arena->buffer[0];
    unsigned char *curr_block = arena->buffer;
    while (curr_block) {
        // Move the buffer pointer to the next block
        arena->buffer = header->next_block;

        // Free the current block
        free(curr_block);

        // Set up for the next iteration
        header = (ElkArenaBlockHeader *)&arena->buffer[0];
        curr_block = arena->buffer;
    }
}

void
elk_arena_reset(ElkArenaAllocator *arena)
{
    assert(arena);

    // Get the total size in all the blocks
    size_t sum_block_sizes = arena->buf_size;
    ElkArenaBlockHeader *header = (ElkArenaBlockHeader *)&arena->buffer[0];
    while (header->next_block) {
        sum_block_sizes += header->next_block_size;
        header = (ElkArenaBlockHeader *)&header->next_block[0];
    }

    if (sum_block_sizes > arena->buf_size) {
        // Free the blocks
        elk_arena_free_blocks(arena);

        // Re-initialize
        elk_arena_initialize(arena, sum_block_sizes);
    } else {
        // We only have one block, no reaseon to free and reallocate
        arena->buf_offset = sizeof(ElkArenaBlockHeader);
    }
}

void
elk_arena_destroy(ElkArenaAllocator *arena)
{
    assert(arena);
    elk_arena_free_blocks(arena);
    arena->buf_size = 0;
    arena->buf_offset = 0;
}

#ifndef NDEBUG
static bool
elk_is_power_of_2(uintptr_t p)
{
    return (p & (p - 1)) == 0;
}
#endif

static uintptr_t
elk_align_pointer(uintptr_t ptr, size_t align)
{

    assert(elk_is_power_of_2(align));

    uintptr_t a = (uintptr_t)align;
    uintptr_t mod = ptr & (a - 1); // Same as (ptr % a) but faster as 'a' is a power of 2

    if (mod != 0) {
        // push the address forward to the next value which is aligned
        ptr += a - mod;
    }

    return ptr;
}

void *
elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment)
{
    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->buf_offset;
    uintptr_t offset = elk_align_pointer(curr_ptr, alignment);
    offset -= (uintptr_t)arena->buffer; // change to relative offset

    // Check to see if the current block has space left
    if (offset + bytes <= arena->buf_size) {
        void *ptr = &arena->buffer[offset];
        arena->buf_offset = offset + bytes;

        return ptr;
    } else {
        // add a new block of at least the required size and recurse to try again
        elk_arena_add_block(arena, bytes + sizeof(ElkArenaBlockHeader));
        return elk_arena_alloc(arena, bytes, alignment);
    }

    // should never be able to get here
    return NULL;
}
