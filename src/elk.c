#include "elk.h"

#include <string.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
static int64_t const SECONDS_PER_MINUTE = INT64_C(60);
static int64_t const MINUTES_PER_HOUR = INT64_C(60);
static int64_t const HOURS_PER_DAY = INT64_C(24);
static int64_t const DAYS_PER_YEAR = INT64_C(365);
static int64_t const SECONDS_PER_HOUR = INT64_C(60) * INT64_C(60);
static int64_t const SECONDS_PER_DAY = INT64_C(60) * INT64_C(60) * INT64_C(24);
static int64_t const SECONDS_PER_YEAR = INT64_C(60) * INT64_C(60) * INT64_C(24) * INT64_C(365);

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
 *                                       String Slice
 *-----------------------------------------------------------------------------------------------*/

_Static_assert(sizeof(size_t) == sizeof(uintptr_t), "size_t and uintptr_t aren't the same size?!");
_Static_assert(UINTPTR_MAX == SIZE_MAX, "sizt_t and uintptr_t dont' have same max?!");

ElkStr
elk_str_from_cstring(char *src)
{
    assert(src);

    size_t len;
    for (len = 0; *(src + len) != '\0'; ++len)
        ; // intentionally left blank.
    return (ElkStr){.start = src, .len = len};
}

ElkStr
elk_str_copy(size_t dst_len, char *restrict dest, ElkStr src)
{
    assert(dest);

    size_t const src_len = src.len;
    size_t const copy_len = src_len < dst_len ? src_len : dst_len;
    memcpy(dest, src.start, copy_len);

    size_t end = copy_len < dst_len ? copy_len : dst_len - 1;
    dest[end] = '\0';

    return (ElkStr){.start = dest, .len = end};
}

int
elk_str_cmp(ElkStr left, ElkStr right)
{
    size_t len = left.len > right.len ? right.len : left.len;

    for (size_t i = 0; i < len; ++i) {
        if (left.start[i] < right.start[i])
            return -1;
        else if (left.start[i] > right.start[i])
            return 1;
    }

    if (left.len == right.len)
        return 0;
    if (left.len > right.len)
        return 1;
    return -1;
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
 *                                    Static Arena Allocator
 *-----------------------------------------------------------------------------------------------*/
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
elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment)
{
    assert(arena);
    assert(size > 0);

    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->buf_offset;
    uintptr_t offset = elk_align_pointer(curr_ptr, alignment);
    offset -= (uintptr_t)arena->buffer; // change to relative offset

    // Check to see if there is enough space left
    if (offset + size <= arena->buf_size) {
        void *ptr = &arena->buffer[offset];
        arena->buf_offset = offset + size;

        return ptr;
    } else {
        return NULL;
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                       Arena Allocator
 *-----------------------------------------------------------------------------------------------*/

static void
elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size)
{
    uint32_t max_block_size = arena->head.buf_size > block_size ? arena->head.buf_size : block_size;
    assert(max_block_size > sizeof(ElkStaticArena));

    unsigned char *buffer = calloc(max_block_size, 1);
    PanicIf(!buffer, "out of memory");

    ElkStaticArena next = arena->head;

    elk_static_arena_init(&arena->head, max_block_size, buffer);
    ElkStaticArena *next_ptr = elk_static_arena_malloc(&arena->head, ElkStaticArena);
    assert(next_ptr);
    *next_ptr = next;
}

void
elk_arena_init(ElkArenaAllocator *arena, size_t starting_block_size)
{
    assert(arena);

    size_t const min_size = sizeof(arena->head) + 8;
    starting_block_size = starting_block_size > min_size ? starting_block_size : min_size;

    // Zero everything out - important for the intrusive linked list used to keep track of how
    // many blocks have been added. The NULL buffer signals the end of the list.
    *arena = (ElkArenaAllocator){
        .head = (ElkStaticArena){.buffer = NULL, .buf_size = 0, .buf_offset = 0}};

    elk_arena_add_block(arena, starting_block_size);
}

static void
elk_arena_free_blocks(ElkArenaAllocator *arena)
{
    unsigned char *curr_buffer = arena->head.buffer;

    // Relies on the first block's buffer being initialized to NULL
    while (curr_buffer) {
        // Copy next into head.
        arena->head = *(ElkStaticArena *)&curr_buffer[0];

        // Free the buffer
        free(curr_buffer);

        // Update to point to the next buffer
        curr_buffer = arena->head.buffer;
    }
}

void
elk_arena_reset(ElkArenaAllocator *arena)
{
    assert(arena);

    // Get the total size in all the blocks
    size_t sum_block_sizes = arena->head.buf_size;

    // It's always placed at the very beginning of the buffer.
    ElkStaticArena *next = (ElkStaticArena *)&arena->head.buffer[0];

    // Relies on the first block's buffer being initialized to NULL in the arena initialization.
    while (next->buffer) {
        sum_block_sizes += next->buf_size;
        next = (ElkStaticArena *)&next->buffer[0];
    }

    if (sum_block_sizes > arena->head.buf_size) {
        // Free the blocks
        elk_arena_free_blocks(arena);

        // Re-initialize with a larger block size.
        elk_arena_init(arena, sum_block_sizes);
    } else {
        // We only have one block, no reaseon to free and reallocate, but we need to maintain
        // the header describing the "next" block.
        arena->head.buf_offset = sizeof(ElkStaticArena);
    }

    return;
}

void
elk_arena_destroy(ElkArenaAllocator *arena)
{
    assert(arena);
    elk_arena_free_blocks(arena);
    arena->head.buf_size = 0;
    arena->head.buf_offset = 0;
}

void *
elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment)
{

    assert(arena && arena->head.buffer);

    void *ptr = elk_static_arena_alloc(&arena->head, bytes, alignment);

    if (!ptr) {
        // add a new block of at least the required size
        elk_arena_add_block(arena, bytes + sizeof(ElkStaticArena) + alignment);
        ptr = elk_static_arena_alloc(&arena->head, bytes, alignment);
    }

    return ptr;
}

/*-------------------------------------------------------------------------------------------------
 *                                       Pool Allocator
 *-----------------------------------------------------------------------------------------------*/
static void
elk_static_pool_initialize_linked_list(unsigned char *buffer, size_t object_size,
                                       size_t num_objects)
{

    // Initialize the free list to a linked list.

    // start by pointing to last element and assigning it NULL
    size_t offset = object_size * (num_objects - 1);
    uintptr_t *ptr = (uintptr_t *)&buffer[offset];
    *ptr = (uintptr_t)NULL;

    // Then work backwards to the front of the list.
    while (offset) {
        size_t next_offset = offset;
        offset -= object_size;
        ptr = (uintptr_t *)&buffer[offset];
        uintptr_t next = (uintptr_t)&buffer[next_offset];
        *ptr = next;
    }
}

void
elk_static_pool_init(ElkStaticPool *pool, size_t object_size, size_t num_objects,
                     unsigned char *buffer)
{
    assert(pool);
    assert(object_size >= sizeof(void *));       // Need to be able to fit at least a pointer!
    assert(object_size % _Alignof(void *) == 0); // Need for alignment of pointers.
    assert(num_objects > 0);

    pool->buffer = buffer;
    pool->object_size = object_size;
    pool->num_objects = num_objects;

    elk_static_pool_reset(pool);
}

void
elk_static_pool_reset(ElkStaticPool *pool)
{
    assert(pool && pool->buffer && pool->num_objects && pool->object_size);

    // Initialize the free list to a linked list.
    elk_static_pool_initialize_linked_list(pool->buffer, pool->object_size, pool->num_objects);
    pool->free = &pool->buffer[0];
}

void
elk_static_pool_destroy(ElkStaticPool *pool)
{
    assert(pool);
    memset(pool, 0, sizeof(*pool));
}

void *
elk_static_pool_alloc(ElkStaticPool *pool)
{
    assert(pool);

    void *ptr = pool->free;
    uintptr_t *next = pool->free;
    if (ptr) {
        pool->free = (void *)*next;
    }

    return ptr;
}

void
elk_static_pool_free(ElkStaticPool *pool, void *ptr)
{
    assert(pool && ptr);

    uintptr_t *next = ptr;
    *next = (uintptr_t)pool->free;
    pool->free = ptr;
}
