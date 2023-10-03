#ifndef ELK_HEADER
#define ELK_HEADER

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       Error Handling
 *-------------------------------------------------------------------------------------------------------------------------*/

// Crash immediately, useful with a debugger!
#define HARD_EXIT (*(int volatile*)0) 

#define PanicIf(assertion) StopIf((assertion), HARD_EXIT)
#define Panic() HARD_EXIT
#define StopIf(assertion, error_action) if (assertion) { error_action; }

#ifndef NDEBUG
#define Assert(assertion) if(!(assertion)) { HARD_EXIT; }
#else
#define Assert(assertion)
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Date and Time Handling
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * The standard C library interface for time isn't threadsafe in general, so I'm reimplementing parts of it here.
 *
 * To do that is pretty difficult! So I'm specializing based on my needs.
 *  - I work entirely in UTC, so I won't bother with timezones. Timezones require interfacing with the OS, and so it belongs
 *    in a platform library anyway.
 *  - I mainly handle meteorological data, including observations and forecasts. I'm not concerned with time and dates
 *    before the 19th century or after the 21st century. Covering more is fine, but not necessary.
 *  - Given the restrictions above, I'm going to make the code as simple and fast as I can.
 *  - The end result covers the time ranging from midnight January 1st, 1 ADE to the last second of the day on 
 *    December 31st, 32767.
 */

extern int64_t const SECONDS_PER_MINUTE;
extern int64_t const MINUTES_PER_HOUR;
extern int64_t const HOURS_PER_DAY;
extern int64_t const DAYS_PER_YEAR;
extern int64_t const SECONDS_PER_HOUR;
extern int64_t const SECONDS_PER_DAY;
extern int64_t const SECONDS_PER_YEAR;

typedef int64_t ElkTime;

typedef struct ElkStructTime 
{
    int16_t year;
    int8_t month;
    int8_t day;
    int8_t hour;
    int8_t minute;
    int8_t second;
} ElkStructTime;

typedef enum
{
    ElkSecond = 1,
    ElkMinute = 60,
    ElkHour = 60 * 60,
    ElkDay = 60 * 60 * 24,
    ElkWeek = 60 * 60 * 24 * 7,
} ElkTimeUnit;

extern ElkTime const elk_unix_epoch_timestamp;

inline int64_t elk_time_to_unix_epoch(ElkTime time);
inline ElkTime elk_time_from_unix_timestamp(int64_t unixtime);
inline bool elk_is_leap_year(int year);
inline ElkTime elk_make_time(ElkStructTime tm);
inline ElkTime elk_time_truncate_to_hour(ElkTime time);
inline ElkTime elk_time_truncate_to_specific_hour(ElkTime time, int hour);
inline ElkTime elk_time_add(ElkTime time, int change_in_time);

extern ElkTime elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds);
extern ElkTime elk_time_from_yd_and_hms(int year, int day_of_year, int hour, int minutes, int seconds);
extern ElkStructTime elk_make_struct_time(ElkTime time);
/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Hashes
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Non-cryptographically secure hash functions.
 *
 * If you're passing user supplied data to this, it's possible that a nefarious actor could send
 * data specifically designed to jam up one of these functions. So don't use them to write a web
 * browser or server, or banking software. They should be good and fast though for data munging.
 *
 * To build a custom "fnv1a" hash function just follow the example of the implementation below of elk_fnv1a_hash below. For
 * the first member of a struct, call the accumulate function with the fnv_offset_bias, then call the accumulate function
 * for each remaining member of the struct. This will leave the padding out of the calculation, which is good because we
 * cannot guarantee what is in the padding.
 */

typedef uint64_t (*ElkHashFunction)(size_t const size_bytes, void const *value);

extern uint64_t const fnv_offset_bias;
extern uint64_t const fnv_prime;

inline uint64_t elk_fnv1a_hash(size_t const n, void const *value);
inline uint64_t elk_fnv1a_hash_accumulate(size_t const size_bytes, void const *value, uint64_t const hash_so_far);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      String Slice
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * An altenrate implementation of strings with "fat pointers" that are pointers to the start and the length of the string.
 * When strings are copied or moved, every effort is made to keep them null terminated so they play nice with the standard
 * C string implementation.
 *
 * WARNING: Slices are only meant to alias into larger strings and so have no built in memory management functions. It's
 * unadvised to have an ElkStr that is the only thing that contains a pointer from malloc(). A seperate pointer to any 
 * buffer should be kept around for memory management purposes.
 *
 * WARNING: Comparisions are NOT utf-8 safe. They look 1 byte at a time, so if you're using fancy utf-8 stuff, no promises.
 */

typedef struct ElkStr 
{
    char *start;      // points at first character in the string.
    size_t len;       // the length of the string (not including a null terminator if it's there).
} ElkStr;

inline ElkStr elk_str_from_cstring(char *src);
inline ElkStr elk_str_copy(size_t dst_len, char *restrict dest, ElkStr src);
inline ElkStr elk_str_strip(ElkStr input);                        // Strips leading and trailing whitespace.
inline ElkStr elk_str_substr(ElkStr str, int start, int len);     // Create a substring from a longer string.
inline int elk_str_case_sensitive_cmp(ElkStr left, ElkStr right); // returns 0 if equal, -1 if left is first, 1 otherwise.
inline bool elk_str_eq(ElkStr left, ElkStr right);                // Faster than elk_str_cmp because it checks length first.

/* Parsing values from strings.
 *
 * In all cases the input string is assumed to be stripped of leading and trailing whitespace. Any suffixes that are non-
 * numeric will cause a parse error for the number parsing cases.
 *
 * Parsing datetimes assumes a format YYYY-MM-DD HH:MM:SS, YYYY-MM-DDTHH:MM:SS, YYYYDDDHHMMSS. The latter format is the 
 * year, day of the year, hours, minutes, and seconds.
 *
 * In general, these functions return true on success and false on failure. On falure the out argument is left untouched.
 */
extern bool elk_str_parse_int_64(ElkStr str, int64_t *result);
extern bool elk_str_parse_float_64(ElkStr str, double *out);
extern bool elk_str_parse_datetime(ElkStr str, ElkTime *out);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     String Interner
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * size_exp - The interner is backed by a hash table with a capacity that is a power of 2. The size_exp is that power of two.
 *     This value is only used initially, if the table needs to expand, it will, so it's OK to start with small values here.
 *     However, if you know it will grow larger, it's better to start larger! For most reasonable use cases, it really
 *     probably shouldn't be smaller than 5, but no checks are done for this.
 *
 * avg_string_size - Is the expected average size of the strings. This isn't really important, because we can allocate more 
 *     storage if needed. But if you know you'll always be using small strings, make this a small number like 5 or 6 to
 *     prevent overallocating memory. If you aren't sure, still use a small number and the interner will grow the storage as
 *     necessary.
 *
 * NOTE: A cstring is a null terminated string of unknown length.
 */
typedef struct ElkStringInterner ElkStringInterner;

extern ElkStringInterner *elk_string_interner_create(int8_t size_exp, int avg_string_size);
extern void elk_string_interner_destroy(ElkStringInterner *interner);
ElkStr elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string);
ElkStr elk_string_interner_intern(ElkStringInterner *interner, ElkStr str);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                         Memory
 *
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Utilities for managing memory. The recommended approach is to create an allocator by declaring it and using its init
 * function, and then from there on out use the generic_alloc functions for allocating and freeing memory on the allocator,
 * and for destroying the allocator. That means if you ever want to change your allocation strategy for a section of code,
 * you can just swap out the code that sets up the allocator and the rest should just work.
 *
 *---------------------------------------------------------------------------------------------------------------------------
 *                                                 Static Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A statically sized, non-growable arena allocator that works on top of a user supplied buffer.
 */

typedef struct ElkStaticArena 
{
    size_t buf_size;
    size_t buf_offset;
    unsigned char *buffer;
} ElkStaticArena;

inline void elk_static_arena_init(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[]);
inline void elk_static_arena_destroy(ElkStaticArena *arena);
inline void elk_static_arena_reset(ElkStaticArena *arena);  // Set the offset to zero, all previous allocations now invalid.
inline void * elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment); // returns NULL if not enough room.
inline void elk_static_arena_free(ElkStaticArena *arena, void *ptr); // No-op for now

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Growable Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Reset is useful if you don't want to return the memory to the OS because you will reuse it soon, e.g. in a loop, but 
 * you're done with the objects allocated off it so far.
 *
 * WARNING: When using the reset function If there are multiple blocks allocated in the arena, they will be freed and a new
 * block the same size as the sum of all the previous blocks will be allocated in their place. If you want the memory to
 * shrink, then use elk_arena_destroy() followed by a call to elk_arena_init() to ensure that it doesn't remain larger than
 * needed.
 *
 * The alloc function returns NULL when it's out of space, but that should never happen unless the computer runs out of 
 * space.
 */
typedef struct ElkArenaAllocator
{
    ElkStaticArena head;
} ElkArenaAllocator;

inline void elk_arena_init(ElkArenaAllocator *arena, size_t starting_block_size);
inline void elk_arena_reset(ElkArenaAllocator *arena);
inline void elk_arena_destroy(ElkArenaAllocator *arena);
inline void elk_arena_free(ElkStaticArena *arena, void *ptr);  // No-op for now.
inline void * elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment); // returns NULL when out of space.

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Static Pool Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * This is an error prone and brittle type. If you get it all working, a refactor or code edit later is likely to break it.
 *
 * WARNING: It is the user's responsibility to make sure that there is at least object_size * num_objects bytes in the
 * backing buffer. If that isn't true, you'll probably get a seg-fault during initialization.
 *
 * WARNING: Object_size must be a multiple of sizeof(void*) to ensure the buffer is aligned to hold pointers also. That also
 * means object_size must be at least sizeof(void*).
 *
 * WARNING: It is the user's responsibility to make sure the buffer is correctly aligned for the type of objects they will
 * be storing in it. This isn't a concern if the memory came from malloc() et al as they return memory with the most
 * pessimistic alignment. However, if using a stack allocated or static memory section, you should use an _Alignas to force
 * the alignment.
 */
typedef struct ElkStaticPool
{
    size_t object_size;    // The size of each object.
    size_t num_objects;    // The capacity, or number of objects storable in the pool.
    void *free;            // The head of a free list of available slots for objects.
    unsigned char *buffer; // The buffer we actually store the data in.
} ElkStaticPool;

inline void elk_static_pool_reset(ElkStaticPool *pool);
inline void elk_static_pool_init(ElkStaticPool *pool, size_t object_size, size_t num_objects, unsigned char buffer[]);
inline void elk_static_pool_destroy(ElkStaticPool *pool);
inline void elk_static_pool_free(ElkStaticPool *pool, void *ptr);
inline void * elk_static_pool_alloc(ElkStaticPool *pool); // returns NULL if there's no more space available.

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Generic Allocator API
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A C11 _Generic based API for swapping out allocators.
 *
 * The API doesn't include any initialization functions, those are unique enough to each allocator and they are tied closely
 * to the type.
 */

#define elk_allocator_malloc(alloc, type) (type *)_Generic((alloc),                                                         \
        ElkStaticArena*: elk_static_arena_alloc,                                                                            \
        ElkArenaAllocator*: elk_arena_alloc,                                                                                \
        ElkStaticPool*: elk_static_pool_alloc_aligned,                                                                      \
        default: elk_panic_allocator_alloc_aligned)(alloc, sizeof(type), _Alignof(type))

#define elk_allocator_nmalloc(alloc, count, type) (type *)_Generic((alloc),                                                 \
        ElkStaticArena*: elk_static_arena_alloc,                                                                            \
        ElkArenaAllocator*: elk_arena_alloc,                                                                                \
        ElkStaticPool*: elk_static_pool_alloc_aligned,                                                                      \
        default: elk_panic_allocator_alloc_aligned)(alloc, count * sizeof(type), _Alignof(type))

#define elk_allocator_free(alloc, ptr) _Generic((alloc),                                                                    \
        ElkStaticArena*: elk_static_arena_free,                                                                             \
        ElkArenaAllocator*: elk_arena_free,                                                                                 \
        ElkStaticPool*: elk_static_pool_free,                                                                               \
        default: elk_panic_allocator_free)(alloc, ptr)

#define elk_allocator_reset(alloc) _Generic((alloc),                                                                        \
        ElkStaticArena*: elk_static_arena_reset,                                                                            \
        ElkArenaAllocator*: elk_arena_reset,                                                                                \
        ElkStaticPool*: elk_static_pool_reset,                                                                              \
        default: elk_panic_allocator_reset)(alloc)

#define elk_allocator_destroy(alloc) _Generic((alloc),                                                                      \
        ElkStaticArena*: elk_static_arena_destroy,                                                                          \
        ElkArenaAllocator*: elk_arena_destroy,                                                                              \
        ElkStaticPool*: elk_static_pool_destroy,                                                                            \
        default: elk_panic_allocator_destroy)(alloc)

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
 *---------------------------------------------------------------------------------------------------------------------------
 *
 *  Ordered collections are things like arrays (sometimes called vectors), queues, dequeues, and heaps. I'm taking a 
 *  different approach to how I implement them. They will be implemented in two parts, the second of which might not be
 *  necessary. 
 *
 *  The first part is the ledger, which just does all the bookkeeping about how full the collection is, which elements are
 *  valid, and the order of elements. A user can use these with their own supplied buffer for storing objects. The ledger
 *  types only track indexes.
 *
 *  One advantage of the ledger approach is that the user can manage their own memory, allowing them to use the most
 *  efficient allocation strategy. 
 *
 *  Another advantage of this is that if you have several parallel collections (e.g. parallel arrays), you can use a single
 *  instance of a bookkeeping type (e.g. \ref ElkQueueLedger) to track the state of all the arrays that back it. Further,
 *  different collections in the parallel collections can have different sized objects.
 *
 *  Complicated memory management can be a disadvantage of the ledger approach. For instance, implementing growable
 *  collections can require shuffling objects around in the backing buffer during the resize operation. A queue, for 
 *  instance, is non-trivial to expand because you have to account for wrap-around (assuming it is backed by a circular
 *  buffer). So not all the ledger types APIs will directly support resizing, and even those that do will further require 
 *  the user to make sure as they allocate more memory in the backing buffer, then also update the ledger. Finally, if you
 *  want to pass the collection as a whole to a function, you'll have to pass the ledger and buffer separately or create 
 *  your own composite type to package them together in.
 *
 *  The second part is an implementation that manages memory for the user. This gives the user less control over how memory
 *  is allocated / freed, but it also burdens them less with the responsibility of doing so. These types can reliably be
 *  expanded on demand. These types will internally use the ledger types.
 *
 *  The more full featured types that manage memory also require the use of void pointers for adding and retrieving data 
 *  from the collections. So they are less type safe, whereas when using just the ledger types they only deal in indexes, so
 *  you can make the backing buffer have any type you want.
 *
 *  Adjacent just means that the objects are stored adjacently in memory. All of the collection ledger types must be backed
 *  by an array or a block of contiguous memory.
 *
 *  NOTE: The resizable collections may not be implemented unless I need them. I've found that I need dynamically expandable
 *  collections much less frequently.
 */

extern int64_t const ELK_COLLECTION_EMPTY;
extern int64_t const ELK_COLLECTION_FULL;

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Queue Ledger
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Mind the ELK_COLLECTION_FULL and ELK_COLLECTION_EMPTY return values.
 */

typedef struct ElkQueueLedger
{
    size_t capacity;
    size_t length;
    size_t front;
    size_t back;
} ElkQueueLedger;

inline ElkQueueLedger elk_queue_ledger_create(size_t capacity);
inline bool elk_queue_ledger_full(ElkQueueLedger *queue);
inline bool elk_queue_ledger_empty(ElkQueueLedger *queue);
inline int64_t elk_queue_ledger_push_back_index(ElkQueueLedger *queue); // returns index of next location to put an object.
inline int64_t elk_queue_ledger_pop_front_index(ElkQueueLedger *queue); // returns index of next location to take object.
inline int64_t elk_queue_ledger_peek_front_index(ElkQueueLedger *queue); // index of next object, but not incremented
inline size_t elk_queue_ledger_len(ElkQueueLedger const *queue);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Array Ledger
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Mind the ELK_COLLECTION_FULL and ELK_COLLECTION_EMPTY return values.
 */
typedef struct ElkArrayLedger 
{
    size_t capacity;
    size_t length;
} ElkArrayLedger;

inline ElkArrayLedger elk_array_ledger_create(size_t capacity);
inline bool elk_array_ledger_full(ElkArrayLedger *array);
inline bool elk_array_ledger_empty(ElkArrayLedger *array);
inline int64_t elk_array_ledger_push_back_index(ElkArrayLedger *array);
inline size_t elk_array_ledger_len(ElkArrayLedger const *array);
inline void elk_array_ledger_reset(ElkArrayLedger *array);
inline void elk_array_ledger_set_capacity(ElkArrayLedger *array, size_t capacity);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *                                          Implementation of `inline` functions.
 *
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
inline int64_t
elk_num_leap_years_since_epoch(int64_t year)
{
    Assert(year >= 1);

    year -= 1;
    return year / 4 - year / 100 + year / 400;
}

inline int64_t
elk_days_since_epoch(int year)
{
    // Days in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    int64_t ts = (year - 1) * DAYS_PER_YEAR + num_leap_years_since_epoch;

    return ts;
}

inline int64_t
elk_time_to_unix_epoch(ElkTime time)
{
    return time - elk_unix_epoch_timestamp;
}

inline ElkTime
elk_time_from_unix_timestamp(int64_t unixtime)
{
    return unixtime + elk_unix_epoch_timestamp;
}

inline bool
elk_is_leap_year(int year)
{
    if (year % 4 != 0) { return false; }
    if (year % 100 == 0 && year % 400 != 0) { return false; }
    return true;
}

inline ElkTime
elk_make_time(ElkStructTime tm)
{
    return elk_time_from_ymd_and_hms(tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
}

inline ElkTime
elk_time_truncate_to_hour(ElkTime time)
{
    ElkTime adjusted = time;

    int64_t const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    int64_t const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    Assert(adjusted >= 0);

    return adjusted;
}

inline ElkTime
elk_time_truncate_to_specific_hour(ElkTime time, int hour)
{
    Assert(hour >= 0 && hour <= 23 && time >= 0);

    ElkTime adjusted = time;

    int64_t const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    int64_t const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    int64_t actual_hour = (adjusted / SECONDS_PER_HOUR) % HOURS_PER_DAY;
    if (actual_hour < hour) { actual_hour += 24; }

    int64_t change_hours = actual_hour - hour;
    Assert(change_hours >= 0);
    adjusted -= change_hours * SECONDS_PER_HOUR;

    Assert(adjusted >= 0);

    return adjusted;
}

inline ElkTime
elk_time_add(ElkTime time, int change_in_time)
{
    ElkTime result = time + change_in_time;
    Assert(result >= 0);
    return result;
}


inline uint64_t
elk_fnv1a_hash_accumulate(size_t const size_bytes, void const *value, uint64_t const hash_so_far)
{
    uint8_t const *data = value;

    uint64_t hash = hash_so_far;
    for (size_t i = 0; i < size_bytes; ++i)
    {
        hash ^= data[i];
        hash *= fnv_prime;
    }

    return hash;
}

inline uint64_t
elk_fnv1a_hash(size_t const n, void const *value)
{
    return elk_fnv1a_hash_accumulate(n, value, fnv_offset_bias);
}

inline ElkStr
elk_str_from_cstring(char *src)
{
    Assert(src);

    size_t len;
    for (len = 0; *(src + len) != '\0'; ++len) ; // intentionally left blank.
    return (ElkStr){.start = src, .len = len};
}

inline ElkStr
elk_str_copy(size_t dst_len, char *restrict dest, ElkStr src)
{
    Assert(dest);

    size_t const src_len = src.len;
    size_t const copy_len = src_len < dst_len ? src_len : dst_len;
    memcpy(dest, src.start, copy_len);

    size_t end = copy_len < dst_len ? copy_len : dst_len - 1;
    dest[end] = '\0';

    return (ElkStr){.start = dest, .len = end};
}

inline ElkStr
elk_str_strip(ElkStr input)
{
    char *const start = input.start;
    int start_offset = 0;
    for (start_offset = 0; start_offset < input.len; ++start_offset)
    {
        if (start[start_offset] > 0x20) { break; }
    }

    int end_offset = 0;
    for (end_offset = input.len - 1; end_offset > start_offset; --end_offset)
    {
        if (start[end_offset] > 0x20) { break; }
    }

    return (ElkStr)
    {
        .start = &start[start_offset],
        .len = end_offset - start_offset + 1
    };
}

inline 
ElkStr elk_str_substr(ElkStr str, int start, int len)
{
    Assert(start >= 0 && len > 0 && start + len <= str.len);
    char *ptr_start = (char *)((uintptr_t)str.start + start);
    return (ElkStr) {.start = ptr_start, .len = len};
}

inline int
elk_str_cmp(ElkStr left, ElkStr right)
{
    Assert(left.start && right.start);

    size_t len = left.len > right.len ? right.len : left.len;

    for (size_t i = 0; i < len; ++i) 
    {
        if (left.start[i] < right.start[i]) { return -1; }
        else if (left.start[i] > right.start[i]) { return 1; }
    }

    if (left.len == right.len) { return 0; }
    if (left.len > right.len) { return 1; }
    return -1;
}

inline bool
elk_str_eq(ElkStr left, ElkStr right)
{
    Assert(left.start && right.start);

    if (left.len != right.len) { return false; }

    size_t len = left.len > right.len ? right.len : left.len;

    for (size_t i = 0; i < len; ++i)
    {
        if (left.start[i] != right.start[i]) { return false; }
    }

    return true;
}

#ifndef NDEBUG
inline bool
elk_is_power_of_2(uintptr_t p)
{
    return (p & (p - 1)) == 0;
}
#endif

inline uintptr_t
elk_align_pointer(uintptr_t ptr, size_t align)
{

    Assert(elk_is_power_of_2(align));

    uintptr_t a = (uintptr_t)align;
    uintptr_t mod = ptr & (a - 1); // Same as (ptr % a) but faster as 'a' is a power of 2

    if (mod != 0)
    {
        // push the address forward to the next value which is aligned
        ptr += a - mod;
    }

    return ptr;
}

inline void
elk_static_arena_init(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[])
{
    Assert(arena && buffer);

    *arena = (ElkStaticArena)
    {
        .buf_size = buf_size,
        .buf_offset = 0,
        .buffer = buffer
    };

    return;
}

inline void
elk_static_arena_destroy(ElkStaticArena *arena)
{
    // no-op
    return;
}

inline void
elk_static_arena_reset(ElkStaticArena *arena)
{
    Assert(arena && arena->buffer);
    arena->buf_offset = 0;
    return;
}

inline void *
elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment)
{
    Assert(arena);
    Assert(size > 0);

    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->buf_offset;
    uintptr_t offset = elk_align_pointer(curr_ptr, alignment);
    offset -= (uintptr_t)arena->buffer; // change to relative offset

    // Check to see if there is enough space left
    if (offset + size <= arena->buf_size)
    {
        void *ptr = &arena->buffer[offset];
        arena->buf_offset = offset + size;

        return ptr;
    }
    else { return NULL; }
}

inline void
elk_static_arena_free(ElkStaticArena *arena, void *ptr)
{
    // no-op - we don't own the buffer!
    return;
}

inline void
elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size)
{
    uint32_t max_block_size = arena->head.buf_size > block_size ? arena->head.buf_size : block_size;
    Assert(max_block_size > sizeof(ElkStaticArena));

    unsigned char *buffer = calloc(max_block_size, 1);
    PanicIf(!buffer);

    ElkStaticArena next = arena->head;

    elk_static_arena_init(&arena->head, max_block_size, buffer);
    ElkStaticArena *next_ptr = elk_static_arena_alloc(&arena->head, sizeof(ElkStaticArena), _Alignof(ElkStaticArena));

    *next_ptr = next;
    return;
}

inline void
elk_arena_free_blocks(ElkArenaAllocator *arena)
{
    unsigned char *curr_buffer = arena->head.buffer;

    // Relies on the first block's buffer being initialized to NULL
    while (curr_buffer) 
    {
        // Copy next into head.
        arena->head = *(ElkStaticArena *)&curr_buffer[0];

        // Free the buffer
        free(curr_buffer);

        // Update to point to the next buffer
        curr_buffer = arena->head.buffer;
    }

    return;
}

inline void
elk_arena_init(ElkArenaAllocator *arena, size_t starting_block_size)
{
    Assert(arena);

    size_t const min_size = sizeof(arena->head) + 8;
    starting_block_size = starting_block_size > min_size ? starting_block_size : min_size;

    // Zero everything out - important for the intrusive linked list used to keep track of how
    // many blocks have been added. The NULL buffer signals the end of the list.
    *arena = (ElkArenaAllocator)
    {
        .head = (ElkStaticArena)
        {
            .buffer = NULL, 
            .buf_size = 0, .buf_offset = 0
        }
    };

    elk_arena_add_block(arena, starting_block_size);

    return;
}

inline void
elk_arena_reset(ElkArenaAllocator *arena)
{
    Assert(arena);

    // Get the total size in all the blocks
    size_t sum_block_sizes = arena->head.buf_size;

    // It's always placed at the very beginning of the buffer.
    ElkStaticArena *next = (ElkStaticArena *)&arena->head.buffer[0];

    // Relies on the first block's buffer being initialized to NULL in the arena initialization.
    while (next->buffer) 
    {
        sum_block_sizes += next->buf_size;
        next = (ElkStaticArena *)&next->buffer[0];
    }

    if (sum_block_sizes > arena->head.buf_size)
    {
        // Free the blocks
        elk_arena_free_blocks(arena);

        // Re-initialize with a larger block size.
        elk_arena_init(arena, sum_block_sizes);
    }
    else
    {
        // We only have one block, no reaseon to free and reallocate, but we need to maintain
        // the header describing the "next" block.
        arena->head.buf_offset = sizeof(ElkStaticArena);
    }

    return;
}

inline void
elk_arena_destroy(ElkArenaAllocator *arena)
{
    Assert(arena);
    elk_arena_free_blocks(arena);
    arena->head.buf_size = 0;
    arena->head.buf_offset = 0;
}

inline void
elk_arena_free(ElkStaticArena *arena, void *ptr)
{
    // no-op
    return;
}

inline void *
elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment)
{
    Assert(arena && arena->head.buffer);

    void *ptr = elk_static_arena_alloc(&arena->head, bytes, alignment);

    if (!ptr)
    {
        // add a new block of at least the required size
        elk_arena_add_block(arena, bytes + sizeof(ElkStaticArena) + alignment);
        ptr = elk_static_arena_alloc(&arena->head, bytes, alignment);
    }

    return ptr;
}

inline void
elk_static_pool_initialize_linked_list(unsigned char *buffer, size_t object_size,
                                       size_t num_objects)
{

    // Initialize the free list to a linked list.

    // start by pointing to last element and assigning it NULL
    size_t offset = object_size * (num_objects - 1);
    uintptr_t *ptr = (uintptr_t *)&buffer[offset];
    *ptr = (uintptr_t)NULL;

    // Then work backwards to the front of the list.
    while (offset) 
    {
        size_t next_offset = offset;
        offset -= object_size;
        ptr = (uintptr_t *)&buffer[offset];
        uintptr_t next = (uintptr_t)&buffer[next_offset];
        *ptr = next;
    }

    return;
}

inline void
elk_static_pool_reset(ElkStaticPool *pool)
{
    Assert(pool && pool->buffer && pool->num_objects && pool->object_size);

    // Initialize the free list to a linked list.
    elk_static_pool_initialize_linked_list(pool->buffer, pool->object_size, pool->num_objects);
    pool->free = &pool->buffer[0];
}

inline void
elk_static_pool_init(ElkStaticPool *pool, size_t object_size, size_t num_objects, unsigned char buffer[])
{
    Assert(pool);
    Assert(object_size >= sizeof(void *));       // Need to be able to fit at least a pointer!
    Assert(object_size % _Alignof(void *) == 0); // Need for alignment of pointers.
    Assert(num_objects > 0);

    pool->buffer = buffer;
    pool->object_size = object_size;
    pool->num_objects = num_objects;

    elk_static_pool_reset(pool);
}

inline void
elk_static_pool_destroy(ElkStaticPool *pool)
{
    Assert(pool);
    memset(pool, 0, sizeof(*pool));
}

inline void
elk_static_pool_free(ElkStaticPool *pool, void *ptr)
{
    Assert(pool && ptr);

    uintptr_t *next = ptr;
    *next = (uintptr_t)pool->free;
    pool->free = ptr;
}

inline void *
elk_static_pool_alloc(ElkStaticPool *pool)
{
    Assert(pool);

    void *ptr = pool->free;
    uintptr_t *next = pool->free;

    if (ptr) 
    {
        pool->free = (void *)*next;
    }

    return ptr;
}

// Just a stub so that it will work in the generic macros.
static inline void *
elk_static_pool_alloc_aligned(ElkStaticPool *pool, size_t size, size_t alignment)
{
    Assert(pool && pool->object_size == size);
    return elk_static_pool_alloc(pool);
}

// These are stubs for the default case in the generic macros below that have the same API, these
// are there so that it compiles, but it will abort the program. FAIL FAST.

static inline void
elk_panic_allocator_reset(void *alloc)
{
    Panic();
}

static inline void
elk_panic_allocator_destroy(void *arena)
{
    Panic();
}

static inline void
elk_panic_allocator_free(void *arena, void *ptr)
{
    Panic();
}

static inline void
elk_panic_allocator_alloc_aligned(void *arena, size_t size, size_t alignment)
{
    Panic();
}

inline ElkQueueLedger
elk_queue_ledger_create(size_t capacity)
{
    return (ElkQueueLedger)
    {
        .capacity = capacity, 
        .length = 0,
        .front = 0, 
        .back = 0
    };
}

inline bool 
elk_queue_ledger_full(ElkQueueLedger *queue)
{ 
    return queue->length == queue->capacity;
}

inline bool
elk_queue_ledger_empty(ElkQueueLedger *queue)
{ 
    return queue->length == 0;
}

inline int64_t
elk_queue_ledger_push_back_index(ElkQueueLedger *queue)
{
    Assert(queue);
    if(elk_queue_ledger_full(queue)) { return ELK_COLLECTION_FULL; }

    int64_t idx = queue->back % queue->capacity;
    queue->back += 1;
    queue->length += 1;
    return idx;
}

inline int64_t
elk_queue_ledger_pop_front_index(ElkQueueLedger *queue)
{
    if(elk_queue_ledger_empty(queue)) { return ELK_COLLECTION_EMPTY; }

    int64_t idx = queue->front % queue->capacity;
    queue->front += 1;
    queue->length -= 1;
    return idx;
}

inline int64_t
elk_queue_ledger_peek_front_index(ElkQueueLedger *queue)
{
    if(queue->length == 0) { return ELK_COLLECTION_EMPTY; }
    return queue->front % queue->capacity;
}

inline size_t
elk_queue_ledger_len(ElkQueueLedger const *queue)
{
    return queue->length;
}

inline ElkArrayLedger
elk_array_ledger_create(size_t capacity)
{
    return (ElkArrayLedger)
    {
        .capacity = capacity,
        .length = 0
    };
}

inline bool 
elk_array_ledger_full(ElkArrayLedger *array)
{ 
    return array->length == array->capacity;
}

inline bool
elk_array_ledger_empty(ElkArrayLedger *array)
{ 
    return array->length == 0;
}

inline int64_t
elk_array_ledger_push_back_index(ElkArrayLedger *array)
{
    Assert(array);
    if(elk_array_ledger_full(array)) { return ELK_COLLECTION_FULL; }

    int64_t idx = array->length;
    array->length += 1;
    return idx;
}

inline size_t
elk_array_ledger_len(ElkArrayLedger const *array)
{
    Assert(array);
    return array->length;
}

inline void
elk_array_ledger_reset(ElkArrayLedger *array)
{
    Assert(array);
    array->length = 0;
}

inline void
elk_array_ledger_set_capacity(ElkArrayLedger *array, size_t capacity)
{
    Assert(array);
    array->capacity = capacity;
}

#endif
