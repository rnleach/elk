#ifndef _ELK_HEADER_
#define _ELK_HEADER_

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------------------------
 * Define the few parts of the standard headers that I need.
 *-------------------------------------------------------------------------------------------------------------------------*/

// stdbool.h
#ifndef bool
  #define bool int
  #define false 0
  #define true 1
#endif

// string.h
void *memcpy(void *dst, void const *src, size_t num_bytes);
void *memset(void *buffer, int val, size_t num_bytes);
int memcmp(const void *s1, const void *s2, size_t num_bytes);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       Error Handling
 *-------------------------------------------------------------------------------------------------------------------------*/

// Crash immediately, useful with a debugger!
#ifndef HARD_EXIT
  #define HARD_EXIT (*(int volatile*)0) 
#endif

#ifndef PanicIf
  #define PanicIf(assertion) StopIf((assertion), HARD_EXIT)
#endif

#ifndef Panic
  #define Panic() HARD_EXIT
#endif

#ifndef StopIf
  #define StopIf(assertion, error_action) if (assertion) { error_action; }
#endif

#ifndef Assert
  #ifndef NDEBUG
    #define Assert(assertion) if(!(assertion)) { HARD_EXIT; }
  #else
    #define Assert(assertion)
  #endif
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

static int64_t const SECONDS_PER_MINUTE = INT64_C(60);
static int64_t const MINUTES_PER_HOUR = INT64_C(60);
static int64_t const HOURS_PER_DAY = INT64_C(24);
static int64_t const DAYS_PER_YEAR = INT64_C(365);
static int64_t const SECONDS_PER_HOUR = INT64_C(60) * INT64_C(60);
static int64_t const SECONDS_PER_DAY = INT64_C(60) * INT64_C(60) * INT64_C(24);
static int64_t const SECONDS_PER_YEAR = INT64_C(60) * INT64_C(60) * INT64_C(24) * INT64_C(365);

typedef int64_t ElkTime;

typedef struct 
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

static ElkTime const elk_unix_epoch_timestamp = INT64_C(62135596800);

static inline int64_t elk_time_to_unix_epoch(ElkTime time);
static inline ElkTime elk_time_from_unix_timestamp(int64_t unixtime);
static inline bool elk_is_leap_year(int year);
static inline ElkTime elk_make_time(ElkStructTime tm);
static inline ElkTime elk_time_truncate_to_hour(ElkTime time);
static inline ElkTime elk_time_truncate_to_specific_hour(ElkTime time, int hour);
static inline ElkTime elk_time_add(ElkTime time, int change_in_time);

static inline ElkTime elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds);
static inline ElkTime elk_time_from_yd_and_hms(int year, int day_of_year, int hour, int minutes, int seconds);
static inline ElkStructTime elk_make_struct_time(ElkTime time);
/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      String Slice
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * An altenrate implementation of strings with "fat pointers" that are pointers to the start and the length of the string.
 * When strings are copied or moved, every effort is made to keep them null terminated so they MIGHT play nice with the
 * standard C string implementation, but you can't count on this. If there isn't enough room when a string is copied for the
 * null terminator, it won't be there.
 *
 * WARNING: Slices are only meant to alias into larger strings and so have no built in memory management functions. It's
 * unadvised to have an ElkStr that is the only thing that contains a pointer from malloc(). A seperate pointer to any 
 * buffer should be kept around for memory management purposes.
 *
 * WARNING: Comparisions are NOT utf-8 safe. They look 1 byte at a time, so if you're using fancy utf-8 stuff, no promises.
 */

typedef struct 
{
    char *start;      // points at first character in the string
    intptr_t len;     // the length of the string (not including a null terminator if it's there)
} ElkStr;

static inline ElkStr elk_str_from_cstring(char *src);
static inline ElkStr elk_str_copy(intptr_t dst_len, char *restrict dest, ElkStr src);
static inline ElkStr elk_str_strip(ElkStr input);                              // Strips leading and trailing whitespace
static inline ElkStr elk_str_substr(ElkStr str, intptr_t start, intptr_t len); // Create a substring from a longer string
static inline int elk_str_case_sensitive_cmp(ElkStr left, ElkStr right);       // 0 if equal, -1 if left is first, 1 otherwise
static inline bool elk_str_eq(ElkStr const left, ElkStr const right);          // Faster than elk_str_cmp, checks length first

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
static inline bool elk_str_parse_int_64(ElkStr str, int64_t *result);
static inline bool elk_str_parse_float_64(ElkStr str, double *out);
static inline bool elk_str_parse_datetime(ElkStr str, ElkTime *out);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                         Memory
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

#define ELK_KiB(a) ((a) * INT64_C(1024))
#define ELK_MiB(a) (ELK_KiB(a) * INT64_C(1024))
#define ELK_GiB(a) (ELK_MiB(a) * INT64_C(1024))
#define ELK_TiB(a) (ELK_GiB(a) * INT64_C(1024))

#define ELK_KB(a) ((a) * INT64_C(1000))
#define ELK_MB(a) (ELK_KB(a) * INT64_C(1000))
#define ELK_GB(a) (ELK_MB(a) * INT64_C(1000))
#define ELK_TB(a) (ELK_GB(a) * INT64_C(1000))

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Static Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A statically sized, non-growable arena allocator that works on top of a user supplied buffer.
 */

typedef struct 
{
    intptr_t buf_size;
    intptr_t buf_offset;
    unsigned char *buffer;
    void *prev_ptr;
    intptr_t prev_offset;
} ElkStaticArena;

static inline void elk_static_arena_create(ElkStaticArena *arena, intptr_t buf_size, unsigned char buffer[]);
static inline void elk_static_arena_destroy(ElkStaticArena *arena);
static inline void elk_static_arena_reset(ElkStaticArena *arena);  // Set offset to 0, invalidates all previous allocations
static inline void * elk_static_arena_alloc(ElkStaticArena *arena, intptr_t size, intptr_t alignment); // ret NULL if OOM
static inline void * elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, intptr_t size); // ret NULL if ptr is not most recent allocation
static inline void elk_static_arena_free(ElkStaticArena *arena, void *ptr); // Undo if it was last allocation, otherwise no-op

#define elk_static_arena_malloc(arena, type) (type *)elk_static_arena_alloc(arena, sizeof(type), _Alignof(type))
#define elk_static_arena_nmalloc(arena, count, type) (type *)elk_static_arena_alloc(arena, count * sizeof(type), _Alignof(type))
#define elk_static_arena_nrealloc(arena, ptr, count, type) (type *) elk_static_arena_realloc(arena, (ptr), sizeof(type) * (count))

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
typedef struct 
{
    intptr_t object_size;    // The size of each object
    intptr_t num_objects;    // The capacity, or number of objects storable in the pool
    void *free;              // The head of a free list of available slots for objects
    unsigned char *buffer;   // The buffer we actually store the data in
} ElkStaticPool;

static inline void elk_static_pool_create(ElkStaticPool *pool, intptr_t object_size, intptr_t num_objects, unsigned char buffer[]);
static inline void elk_static_pool_destroy(ElkStaticPool *pool);
static inline void elk_static_pool_reset(ElkStaticPool *pool);
static inline void elk_static_pool_free(ElkStaticPool *pool, void *ptr);
static inline void * elk_static_pool_alloc(ElkStaticPool *pool); // returns NULL if there's no more space available.
// no elk_static_pool_realloc because that doesn't make sense!

#define elk_static_pool_malloc(alloc, type) (type *)elk_static_pool_alloc(alloc)

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
typedef bool (*ElkEqFunction)(void const *left, void const *right);

typedef uint64_t (*ElkHashFunction)(size_t const size_bytes, void const *value);
typedef uint64_t (*ElkSimpleHash)(void const *object); // Already knows the size of the object to be hashed!

static inline uint64_t elk_fnv1a_hash(intptr_t const n, void const *value);
static inline uint64_t elk_fnv1a_hash_accumulate(intptr_t const size_bytes, void const *value, uint64_t const hash_so_far);
static inline uint64_t elk_fnv1a_hash_str(ElkStr str);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     String Interner
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * size_exp - The interner is backed by a hash table with a capacity that is a power of 2. The size_exp is that power of two.
 *     This value is only used initially, if the table needs to expand, it will, so it's OK to start with small values here.
 *     However, if you know it will grow larger, it's better to start larger! For most reasonable use cases, it really
 *     probably shouldn't be smaller than 5, but no checks are done for this.
 *
 * NOTE: A cstring is a null terminated string of unknown length.
 *
 * NOTE: The interner copies any successfully interned string, so once it has been interned you can reclaim the memory that 
 * was holding it before.
 */
typedef struct // Internal only
{
    uint64_t hash;
    ElkStr str;
} ElkStringInternerHandle;

typedef struct 
{
    ElkStaticArena *storage;          // This is where to store the strings

    ElkStringInternerHandle *handles; // The hash table - handles index into storage
    uint32_t num_handles;             // The number of handles
    int8_t size_exp;                  // Used to keep track of the length of *handles
} ElkStringInterner;


static inline ElkStringInterner elk_string_interner_create(int8_t size_exp, ElkStaticArena *storage);
static inline void elk_string_interner_destroy(ElkStringInterner *interner);
static inline ElkStr elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string);
static inline ElkStr elk_string_interner_intern(ElkStringInterner *interner, ElkStr str);

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

static intptr_t const ELK_COLLECTION_EMPTY = -1;
static intptr_t const ELK_COLLECTION_FULL = -2;

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Queue Ledger
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Mind the ELK_COLLECTION_FULL and ELK_COLLECTION_EMPTY return values.
 */

typedef struct 
{
    intptr_t capacity;
    intptr_t length;
    intptr_t front;
    intptr_t back;
} ElkQueueLedger;

static inline ElkQueueLedger elk_queue_ledger_create(intptr_t capacity);
static inline bool elk_queue_ledger_full(ElkQueueLedger *queue);
static inline bool elk_queue_ledger_empty(ElkQueueLedger *queue);
static inline intptr_t elk_queue_ledger_push_back_index(ElkQueueLedger *queue);  // index of next location to put an object
static inline intptr_t elk_queue_ledger_pop_front_index(ElkQueueLedger *queue);  // index of next location to take object
static inline intptr_t elk_queue_ledger_peek_front_index(ElkQueueLedger *queue); // index of next object, but not incremented
static inline intptr_t elk_queue_ledger_len(ElkQueueLedger const *queue);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Array Ledger
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Mind the ELK_COLLECTION_FULL and ELK_COLLECTION_EMPTY return values.
 */
typedef struct 
{
    intptr_t capacity;
    intptr_t length;
} ElkArrayLedger;

static inline ElkArrayLedger elk_array_ledger_create(intptr_t capacity);
static inline bool elk_array_ledger_full(ElkArrayLedger *array);
static inline bool elk_array_ledger_empty(ElkArrayLedger *array);
static inline intptr_t elk_array_ledger_push_back_index(ElkArrayLedger *array);
static inline intptr_t elk_array_ledger_len(ElkArrayLedger const *array);
static inline void elk_array_ledger_reset(ElkArrayLedger *array);
static inline void elk_array_ledger_set_capacity(ElkArrayLedger *array, intptr_t capacity);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                         
 *                                                  Unordered Collections
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    Hash Map (Table)
 *---------------------------------------------------------------------------------------------------------------------------
 * The table size must be a power of two, so size_exp is used to calcualte the size of the table. If it needs to grow in 
 * size, it will grow the table, so this is only a starting point.
 *
 * The ELkHashMap does NOT copy any objects, so it only stores pointers. The user has to manage the memory for their own
 * objects.
 */
typedef struct // Internal Only
{
    uint64_t hash;
    void *key;
    void *value;
} ElkHashMapHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkHashMapHandle *handles;
    intptr_t num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
    int8_t size_exp;
} ElkHashMap;

typedef intptr_t ElkHashMapKeyIter;

static inline ElkHashMap elk_hash_map_create(int8_t size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq, ElkStaticArena *arena);
static inline void elk_hash_map_destroy(ElkHashMap *map);
static inline void *elk_hash_map_insert(ElkHashMap *map, void *key, void *value); // if return != value, key was already in the map
static inline void *elk_hash_map_lookup(ElkHashMap *map, void *key); // return NULL if not in map, otherwise return pointer to value
static inline ElkHashMapKeyIter elk_hash_map_key_iter(ElkHashMap *map);

static inline void *elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter);

/*--------------------------------------------------------------------------------------------------------------------------
 *                                            Hash Map (Table, ElkStr as keys)
 *---------------------------------------------------------------------------------------------------------------------------
 * Designed for use when keys are strings. If the key_hash is NULL, it just uses the fnv1a has function.
 *
 * Values are not copied, they are stored as pointers, so the user must manage memory.
 *
 * Uses fnv1a hash.
 */
typedef struct // Internal only
{
    uint64_t hash;
    ElkStr key;
    void *value;
} ElkStrMapHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkStrMapHandle *handles;
    intptr_t num_handles;
    int8_t size_exp;
} ElkStrMap;

typedef intptr_t ElkStrMapKeyIter;

static inline ElkStrMap elk_str_map_create(int8_t size_exp, ElkStaticArena *arena);
static inline void elk_str_map_destroy(ElkStrMap *map);
static inline void *elk_str_map_insert(ElkStrMap *map, ElkStr key, void *value); // if return != value, key was already in the map
static inline void *elk_str_map_lookup(ElkStrMap *map, ElkStr key); // return NULL if not in map, otherwise return pointer to value
static inline ElkHashMapKeyIter elk_str_map_key_iter(ElkStrMap *map);

static inline ElkStr elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                        Hash Set
 *---------------------------------------------------------------------------------------------------------------------------
 * The table size must be a power of two, so size_exp is used to calcualte the size of the table. If it needs to grow in 
 * size, it will grow the table, so this is only a starting point.
 *
 * The ELkHashSet does NOT copy any objects, so it only stores pointers. The user has to manage the memory for their own
 * objects.
 */
typedef struct // Internal only
{
    uint64_t hash;
    void *value;
} ElkHashSetHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkHashSetHandle *handles;
    intptr_t num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
    int8_t size_exp;
} ElkHashSet;

typedef intptr_t ElkHashSetIter;

static inline ElkHashSet elk_hash_set_create(int8_t size_exp, ElkSimpleHash val_hash, ElkEqFunction val_eq, ElkStaticArena *arena);
static inline void elk_hash_set_destroy(ElkHashSet *set);
static inline void *elk_hash_set_insert(ElkHashSet *set, void *value); // if return != value, value was already in the set
static inline void *elk_hash_set_lookup(ElkHashSet *set, void *value); // return NULL if not in set, otherwise return ptr to value
static inline ElkHashSetIter elk_hash_set_value_iter(ElkHashSet *set);

static inline void *elk_hash_set_value_iter_next(ElkHashSet *set, ElkHashSetIter *iter);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                          Parsing Commonly Encountered File Types
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       CSV Parsing
 *---------------------------------------------------------------------------------------------------------------------------
 * Parsing a string may modify it since tokens are returned as ElkStr and we may modify those strings in place.
 *
 * The CSV format I handle is pretty simple. Anything goes inside quoted strings. Quotes in a quoted string have to be 
 * escaped by another quote, e.g. "Frank ""The Tank"" Johnson". Comment lines are allowed, but they must be full lines, no
 * end of line comments, and they must start with a '#' character.
 */
typedef struct
{
    intptr_t row; // The number of CSV rows parsed so far, this includes blank lines.
    intptr_t col; // CSV column, that is, how many commas have we passed
    ElkStr value; // The value not including the comma or any new line character
} ElkCsvToken;

typedef struct
{
    ElkStr remaining;       // The portion of the string remaining to be parsed. Useful for diagnosing parse errors.
    intptr_t row;           // Only counts parseable rows, comment lines don't count.
    intptr_t col;           // CSV column, that is, how many commas have we passed on this line.
    intptr_t max_token_len; // CSV files usually don't have very long entries for values, set a limit beyond which is error
    bool error;             // Have we encountered an error while parsing?
} ElkCsvParser;

static inline ElkCsvParser elk_csv_create_parser(ElkStr input, intptr_t max_token_len);
static inline ElkCsvToken elk_csv_next_token(ElkCsvParser *parser);
static inline bool elk_csv_finished(ElkCsvParser *parser);
static inline ElkStr elk_csv_unquote_str(ElkStr str);

#define elk_csv_default_parser(input) elk_csv_create_parser(input, 128)

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *                                          Implementation of `inline` functions.
 *
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static inline int64_t
elk_num_leap_years_since_epoch(int64_t year)
{
    Assert(year >= 1);

    year -= 1;
    return year / 4 - year / 100 + year / 400;
}

static inline int64_t
elk_days_since_epoch(int year)
{
    // Days in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    int64_t ts = (year - 1) * DAYS_PER_YEAR + num_leap_years_since_epoch;

    return ts;
}

static inline int64_t
elk_time_to_unix_epoch(ElkTime time)
{
    return time - elk_unix_epoch_timestamp;
}

static inline ElkTime
elk_time_from_unix_timestamp(int64_t unixtime)
{
    return unixtime + elk_unix_epoch_timestamp;
}

static inline bool
elk_is_leap_year(int year)
{
    if (year % 4 != 0) { return false; }
    if (year % 100 == 0 && year % 400 != 0) { return false; }
    return true;
}

static inline ElkTime
elk_make_time(ElkStructTime tm)
{
    return elk_time_from_ymd_and_hms(tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
}

static inline ElkTime
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

static inline ElkTime
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

static inline ElkTime
elk_time_add(ElkTime time, int change_in_time)
{
    ElkTime result = time + change_in_time;
    Assert(result >= 0);
    return result;
}

// Days in a year up to beginning of month
static int64_t const sum_days_to_month[2][13] = {
    {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};


static inline ElkTime
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

static inline ElkTime 
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

static inline ElkStructTime 
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

static inline ElkStr
elk_str_from_cstring(char *src)
{
    intptr_t len;
    for (len = 0; *(src + len) != '\0'; ++len) ; // intentionally left blank.
    return (ElkStr){.start = src, .len = len};
}

static inline ElkStr
elk_str_copy(intptr_t dst_len, char *restrict dest, ElkStr src)
{
    intptr_t const src_len = src.len;
    intptr_t const copy_len = src_len < dst_len ? src_len : dst_len;
    memcpy(dest, src.start, copy_len);

    // Add a terminating zero IFF we can.
    if(copy_len < dst_len) { dest[copy_len] = '\0'; }

    intptr_t end = copy_len < dst_len ? copy_len : dst_len;

    return (ElkStr){.start = dest, .len = end};
}

static inline ElkStr
elk_str_strip(ElkStr input)
{
    char *const start = input.start;
    intptr_t start_offset = 0;
    for (start_offset = 0; start_offset < input.len; ++start_offset)
    {
        if (start[start_offset] > 0x20) { break; }
    }

    intptr_t end_offset = 0;
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

static inline 
ElkStr elk_str_substr(ElkStr str, intptr_t start, intptr_t len)
{
    Assert(start >= 0 && len > 0 && start + len <= str.len);
    char *ptr_start = (char *)((intptr_t)str.start + start);
    return (ElkStr) {.start = ptr_start, .len = len};
}

static inline int
elk_str_cmp(ElkStr left, ElkStr right)
{
    if(left.start == right.start && left.len == right.len) { return 0; }

    intptr_t len = left.len > right.len ? right.len : left.len;

    for (intptr_t i = 0; i < len; ++i) 
    {
        if (left.start[i] < right.start[i]) { return -1; }
        else if (left.start[i] > right.start[i]) { return 1; }
    }

    if (left.len == right.len) { return 0; }
    if (left.len > right.len) { return 1; }
    return -1;
}

static inline bool
elk_str_eq(ElkStr const left, ElkStr const right)
{
    if (left.len != right.len) { return false; }

    intptr_t len = left.len > right.len ? right.len : left.len;

    for (intptr_t i = 0; i < len; ++i)
    {
        if (left.start[i] != right.start[i]) { return false; }
    }

    return true;
}

_Static_assert(sizeof(intptr_t) == sizeof(uintptr_t), "intptr_t and uintptr_t aren't the same size?!");

static inline bool
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
static inline bool
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
    intptr_t len_remaining = str.len;

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

static inline bool
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

static uint64_t const fnv_offset_bias = 0xcbf29ce484222325;
static uint64_t const fnv_prime = 0x00000100000001b3;

static inline uint64_t
elk_fnv1a_hash_accumulate(intptr_t const size_bytes, void const *value, uint64_t const hash_so_far)
{
    uint8_t const *data = value;

    uint64_t hash = hash_so_far;
    for (intptr_t i = 0; i < size_bytes; ++i)
    {
        hash ^= data[i];
        hash *= fnv_prime;
    }

    return hash;
}

static inline uint64_t
elk_fnv1a_hash(intptr_t const n, void const *value)
{
    return elk_fnv1a_hash_accumulate(n, value, fnv_offset_bias);
}

static inline uint64_t
elk_fnv1a_hash_str(ElkStr str)
{
    return elk_fnv1a_hash(str.len, str.start);
}

static inline ElkStringInterner 
elk_string_interner_create(int8_t size_exp, ElkStaticArena *storage)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size_t const handles_len = 1 << size_exp;
    ElkStringInternerHandle *handles = elk_static_arena_nmalloc(storage, handles_len, ElkStringInternerHandle);
    PanicIf(!handles);

    return (ElkStringInterner)
    {
        .storage = storage,
        .handles = handles,
        .num_handles = 0, 
        .size_exp = size_exp
    };
}

static inline void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    return;
}

static inline bool
elk_hash_table_large_enough(size_t num_handles, int8_t size_exp)
{
    // Shoot for no more than 75% of slots filled.
    return num_handles < 3 * (1 << size_exp) / 4;
}

static inline uint32_t
elk_hash_lookup(uint64_t hash, int8_t exp, uint32_t idx)
{
    // Copied from https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.
    uint32_t mask = (UINT32_C(1) << exp) - 1;
    uint32_t step = (hash >> (64 - exp)) | 1;    // the | 1 forces an odd number
    return (idx + step) & mask;
}

static inline void
elk_string_interner_expand_table(ElkStringInterner *interner)
{
    int8_t const size_exp = interner->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    size_t const handles_len = 1 << size_exp;
    size_t const new_handles_len = 1 << new_size_exp;

    ElkStringInternerHandle *new_handles = elk_static_arena_nmalloc(interner->storage, new_handles_len, ElkStringInternerHandle);
    PanicIf(!new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (handle->str.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash & 0xffffffff; // truncate
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

    interner->handles = new_handles;
    interner->size_exp = new_size_exp;

    return;
}

static inline ElkStr
elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string)
{
    ElkStr str = elk_str_from_cstring(string);
    return elk_string_interner_intern(interner, str);
}

static inline ElkStr
elk_string_interner_intern(ElkStringInterner *interner, ElkStr str)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash_str(str);
    uint32_t i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, interner->size_exp, i);
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (!handle->str.start)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(interner->num_handles, interner->size_exp))
            {
                char *dest = elk_static_arena_nmalloc(interner->storage, str.len + 1, char);
                PanicIf(!dest);
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

#ifndef NDEBUG
static inline bool
elk_is_power_of_2(uintptr_t p)
{
    return (p & (p - 1)) == 0;
}
#endif

static inline uintptr_t
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

static inline void
elk_static_arena_create(ElkStaticArena *arena, intptr_t buf_size, unsigned char buffer[])
{
    Assert(arena && buffer && buf_size > 0);

    *arena = (ElkStaticArena)
    {
        .buf_size = buf_size,
        .buf_offset = 0,
        .buffer = buffer,
        .prev_ptr = NULL,
        .prev_offset = 0,
    };

    return;
}

static inline void
elk_static_arena_destroy(ElkStaticArena *arena)
{
    // no-op
    return;
}

static inline void
elk_static_arena_reset(ElkStaticArena *arena)
{
    Assert(arena->buffer);
    arena->buf_offset = 0;
    arena->prev_ptr = NULL;
    arena->prev_offset = 0;
    return;
}

static inline void *
elk_static_arena_alloc(ElkStaticArena *arena, intptr_t size, intptr_t alignment)
{
    Assert(size > 0 && alignment > 0);

    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->buf_offset;
    uintptr_t offset = elk_align_pointer(curr_ptr, alignment);
    offset -= (uintptr_t)arena->buffer; // change to relative offset

    // Check to see if there is enough space left
    if (offset + size <= arena->buf_size)
    {
        void *ptr = &arena->buffer[offset];
        memset(ptr, 0, size);
        arena->prev_offset = arena->buf_offset;
        arena->buf_offset = offset + size;
        arena->prev_ptr = ptr;

        return ptr;
    }
    else { return NULL; }
}

static inline void * 
elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, intptr_t size)
{
    Assert(size > 0);

    if(ptr == arena->prev_ptr)
    {
        // Get previous extra offset due to alignment
        uintptr_t offset = (uintptr_t)ptr - (uintptr_t)arena->buffer; // relative offset accounting for alignment

        // Check to see if there is enough space left
        if (offset + size <= arena->buf_size)
        {
            arena->buf_offset = offset + size;
            return ptr;
        }
    }

    return NULL;
}

static inline void
elk_static_arena_free(ElkStaticArena *arena, void *ptr)
{
    if(ptr == arena->prev_ptr)
    {
        arena->buf_offset = arena->prev_offset;
    }

    return;
}

static inline void
elk_static_pool_initialize_linked_list(unsigned char *buffer, intptr_t object_size,
                                       intptr_t num_objects)
{

    // Initialize the free list to a linked list.

    // start by pointing to last element and assigning it NULL
    intptr_t offset = object_size * (num_objects - 1);
    uintptr_t *ptr = (uintptr_t *)&buffer[offset];
    *ptr = (uintptr_t)NULL;

    // Then work backwards to the front of the list.
    while (offset) 
    {
        intptr_t next_offset = offset;
        offset -= object_size;
        ptr = (uintptr_t *)&buffer[offset];
        uintptr_t next = (uintptr_t)&buffer[next_offset];
        *ptr = next;
    }

    return;
}

static inline void
elk_static_pool_reset(ElkStaticPool *pool)
{
    Assert(pool && pool->buffer && pool->num_objects && pool->object_size);

    // Initialize the free list to a linked list.
    elk_static_pool_initialize_linked_list(pool->buffer, pool->object_size, pool->num_objects);
    pool->free = &pool->buffer[0];
}

static inline void
elk_static_pool_create(ElkStaticPool *pool, intptr_t object_size, intptr_t num_objects, unsigned char buffer[])
{
    Assert(object_size >= sizeof(void *));       // Need to be able to fit at least a pointer!
    Assert(object_size % _Alignof(void *) == 0); // Need for alignment of pointers.
    Assert(num_objects > 0);

    pool->buffer = buffer;
    pool->object_size = object_size;
    pool->num_objects = num_objects;

    elk_static_pool_reset(pool);
}

static inline void
elk_static_pool_destroy(ElkStaticPool *pool)
{
    memset(pool, 0, sizeof(*pool));
}

static inline void
elk_static_pool_free(ElkStaticPool *pool, void *ptr)
{
    uintptr_t *next = ptr;
    *next = (uintptr_t)pool->free;
    pool->free = ptr;
}

static inline void *
elk_static_pool_alloc(ElkStaticPool *pool)
{
    void *ptr = pool->free;
    uintptr_t *next = pool->free;

    if (ptr) 
    {
        pool->free = (void *)*next;
        memset(ptr, 0, pool->object_size);
    }

    return ptr;
}

// Just a stub so that it will work in the generic macros.
static inline void *
elk_static_pool_alloc_aligned(ElkStaticPool *pool, intptr_t size, intptr_t alignment)
{
    Assert(pool->object_size == size); // This will trip up elk_allocator_nmalloc if count > 1
    return elk_static_pool_alloc(pool);
}

static inline ElkQueueLedger
elk_queue_ledger_create(intptr_t capacity)
{
    return (ElkQueueLedger)
    {
        .capacity = capacity, 
        .length = 0,
        .front = 0, 
        .back = 0
    };
}

static inline bool 
elk_queue_ledger_full(ElkQueueLedger *queue)
{ 
    return queue->length == queue->capacity;
}

static inline bool
elk_queue_ledger_empty(ElkQueueLedger *queue)
{ 
    return queue->length == 0;
}

static inline intptr_t
elk_queue_ledger_push_back_index(ElkQueueLedger *queue)
{
    if(elk_queue_ledger_full(queue)) { return ELK_COLLECTION_FULL; }

    intptr_t idx = queue->back % queue->capacity;
    queue->back += 1;
    queue->length += 1;
    return idx;
}

static inline intptr_t
elk_queue_ledger_pop_front_index(ElkQueueLedger *queue)
{
    if(elk_queue_ledger_empty(queue)) { return ELK_COLLECTION_EMPTY; }

    intptr_t idx = queue->front % queue->capacity;
    queue->front += 1;
    queue->length -= 1;
    return idx;
}

static inline intptr_t
elk_queue_ledger_peek_front_index(ElkQueueLedger *queue)
{
    if(queue->length == 0) { return ELK_COLLECTION_EMPTY; }
    return queue->front % queue->capacity;
}

static inline intptr_t
elk_queue_ledger_len(ElkQueueLedger const *queue)
{
    return queue->length;
}

static inline ElkArrayLedger
elk_array_ledger_create(intptr_t capacity)
{
    Assert(capacity > 0);
    return (ElkArrayLedger)
    {
        .capacity = capacity,
        .length = 0
    };
}

static inline bool 
elk_array_ledger_full(ElkArrayLedger *array)
{ 
    return array->length == array->capacity;
}

static inline bool
elk_array_ledger_empty(ElkArrayLedger *array)
{ 
    return array->length == 0;
}

static inline intptr_t
elk_array_ledger_push_back_index(ElkArrayLedger *array)
{
    if(elk_array_ledger_full(array)) { return ELK_COLLECTION_FULL; }

    intptr_t idx = array->length;
    array->length += 1;
    return idx;
}

static inline intptr_t
elk_array_ledger_len(ElkArrayLedger const *array)
{
    return array->length;
}

static inline void
elk_array_ledger_reset(ElkArrayLedger *array)
{
    array->length = 0;
}

static inline void
elk_array_ledger_set_capacity(ElkArrayLedger *array, intptr_t capacity)
{
    Assert(capacity > 0);
    array->capacity = capacity;
}

static ElkHashMap 
elk_hash_map_create(int8_t size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    intptr_t const handles_len = 1 << size_exp;
    ElkHashMapHandle *handles = elk_static_arena_nmalloc(arena, handles_len, ElkHashMapHandle);
    PanicIf(!handles);

    return (ElkHashMap)
    {
        .arena = arena,
        .handles = handles,
        .num_handles = 0, 
        .hasher = key_hash,
        .eq = key_eq,
        .size_exp = size_exp
    };
}

static inline void 
elk_hash_map_destroy(ElkHashMap *map)
{
    return;
}

static inline void
elk_hash_table_expand(ElkHashMap *map)
{
    int8_t const size_exp = map->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    intptr_t const handles_len = 1 << size_exp;
    intptr_t const new_handles_len = 1 << new_size_exp;

    ElkHashMapHandle *new_handles = elk_static_arena_nmalloc(map->arena, new_handles_len, ElkHashMapHandle);
    PanicIf(!new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkHashMapHandle *handle = &map->handles[i];

        if (handle->key == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash & 0xffffffff; // truncate
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

    map->handles = new_handles;
    map->size_exp = new_size_exp;

    return;
}

static inline void *
elk_hash_map_insert(ElkHashMap *map, void *key, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = map->hasher(key);
    uint32_t i = hash & 0xffffffff; // truncate
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

static inline void *
elk_hash_map_lookup(ElkHashMap *map, void *key)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = map->hasher(key);
    uint32_t i = hash & 0xffffffff; // truncate
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

static inline ElkHashMapKeyIter 
elk_hash_map_key_iter(ElkHashMap *map)
{
    return 0;
}

static inline void *
elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter)
{
    intptr_t const max_iter = (1 << map->size_exp);
    void *next_key = NULL;
    if(*iter >= max_iter) { return next_key; }

    do
    {
        next_key = map->handles[*iter].key;
        *iter += 1;

    } while(next_key == NULL && *iter < max_iter);

    return next_key;
}


static inline ElkStrMap 
elk_str_map_create(int8_t size_exp, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    intptr_t const handles_len = 1 << size_exp;
    ElkStrMapHandle *handles = elk_static_arena_nmalloc(arena, handles_len, ElkStrMapHandle);
    PanicIf(!handles);

    return (ElkStrMap)
    {
        .arena = arena,
        .handles = handles,
        .num_handles = 0, 
        .size_exp = size_exp,
    };
}

static inline void
elk_str_map_destroy(ElkStrMap *map)
{
    return;
}

static inline void
elk_str_table_expand(ElkStrMap *map)
{
    int8_t const size_exp = map->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    intptr_t const handles_len = 1 << size_exp;
    intptr_t const new_handles_len = 1 << new_size_exp;

    ElkStrMapHandle *new_handles = elk_static_arena_nmalloc(map->arena, new_handles_len, ElkStrMapHandle);
    PanicIf(!new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkStrMapHandle *handle = &map->handles[i];

        if (handle->key.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash & 0xffffffff; // truncate
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

    map->handles = new_handles;
    map->size_exp = new_size_exp;

    return;
}


static inline void *
elk_str_map_insert(ElkStrMap *map, ElkStr key, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash_str(key);
    uint32_t i = hash & 0xffffffff; // truncate
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

static inline void *
elk_str_map_lookup(ElkStrMap *map, ElkStr key)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = elk_fnv1a_hash_str(key);
    uint32_t i = hash & 0xffffffff; // truncate
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

static inline ElkHashMapKeyIter 
elk_str_map_key_iter(ElkStrMap *map)
{
    return 0;
}

static inline ElkStr 
elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter)
{
    intptr_t const max_iter = (1 << map->size_exp);
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

static inline ElkHashSet 
elk_hash_set_create(int8_t size_exp, ElkSimpleHash val_hash, ElkEqFunction val_eq, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    intptr_t const handles_len = 1 << size_exp;
    ElkHashSetHandle *handles = elk_static_arena_nmalloc(arena, handles_len, ElkHashSetHandle);
    PanicIf(!handles);

    return (ElkHashSet)
    {
        .arena = arena,
        .handles = handles,
        .num_handles = 0, 
        .hasher = val_hash,
        .eq = val_eq,
        .size_exp = size_exp
    };
}

static inline void 
elk_hash_set_destroy(ElkHashSet *set)
{
    return;
}

static void
elk_hash_set_expand(ElkHashSet *set)
{
    int8_t const size_exp = set->size_exp;
    int8_t const new_size_exp = size_exp + 1;

    intptr_t const handles_len = 1 << size_exp;
    intptr_t const new_handles_len = 1 << new_size_exp;

    ElkHashSetHandle *new_handles = elk_static_arena_nmalloc(set->arena, new_handles_len, ElkHashSetHandle);
    PanicIf(!new_handles);

    for (uint32_t i = 0; i < handles_len; i++) 
    {
        ElkHashSetHandle *handle = &set->handles[i];

        if (handle->value == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        uint64_t const hash = handle->hash;
        uint32_t j = hash & 0xffffffff; // truncate
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkHashSetHandle *new_handle = &new_handles[j];

            if (!new_handle->value)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    set->handles = new_handles;
    set->size_exp = new_size_exp;

    return;
}

static inline void *
elk_hash_set_insert(ElkHashSet *set, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = set->hasher(value);
    uint32_t i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, set->size_exp, i);
        ElkHashSetHandle *handle = &set->handles[i];

        if (!handle->value)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(set->num_handles, set->size_exp))
            {

                *handle = (ElkHashSetHandle){.hash = hash, .value=value};
                set->num_handles += 1;

                return handle->value;
            }
            else 
            {
                // Grow the table so we have room
                elk_hash_set_expand(set);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the set.
                return elk_hash_set_insert(set, value);
            }
        }
        else if (handle->hash == hash && set->eq(handle->value, value)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}


static inline void *
elk_hash_set_lookup(ElkHashSet *set, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    uint64_t const hash = set->hasher(value);
    uint32_t i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, set->size_exp, i);
        ElkHashSetHandle *handle = &set->handles[i];

        if (!handle->value)
        {
            return NULL;
        }
        else if (handle->hash == hash && set->eq(handle->value, value)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

static inline ElkHashSetIter
elk_hash_set_value_iter(ElkHashSet *set)
{
    return 0;
}

static inline void *
elk_hash_set_value_iter_next(ElkHashSet *set, ElkHashSetIter *iter)
{
    intptr_t const max_iter = (1 << set->size_exp);
    void *next_value = NULL;
    if(*iter >= max_iter) { return next_value; }

    do
    {
        next_value = set->handles[*iter].value;
        *iter += 1;

    } while(next_value == NULL && *iter < max_iter);

    return next_value;
}

static inline ElkCsvParser 
elk_csv_create_parser(ElkStr input, intptr_t max_token_len)
{
    return (ElkCsvParser){ .remaining=input, .row=0, .col=0, .max_token_len= max_token_len, .error=false };
}

static inline bool 
elk_csv_finished(ElkCsvParser *parser)
{
    return parser->error || parser->remaining.len == 0;
}

static inline ElkCsvToken 
elk_csv_next_token(ElkCsvParser *parser)
{
    StopIf(elk_csv_finished(parser), goto ERR_RETURN);

    // Current position in parser and how much we've processed
    char *next_char = parser->remaining.start;
    int num_chars_proc = 0;
    intptr_t row = parser->row;
    intptr_t col = parser->col;

    // Handle comment lines
    while(col == 0 && *next_char == '#')
    {
        // We must be on a comment line if we got here, so read just past the end of the line
        
        while(*next_char && parser->remaining.len - num_chars_proc > 0)
        {
            char *last_char = next_char;
            ++next_char;
            ++num_chars_proc;
            if(*last_char == '\n') { break; }
        }
    }

    // The data for the next value to return
    char *next_value_start = next_char;
    intptr_t next_value_len = 0;

    // Are we in a quoted string where we should ignore commas?
    bool in_string = false;
    bool new_row = false;

    while(*next_char && parser->remaining.len - num_chars_proc > 0)
    {
        if(*next_char == '\n')  
        {
            ++next_char;
            ++num_chars_proc;
            new_row = true;
            break;
        }
        else if(*next_char == '"')
        {
            in_string = !in_string;
        }
        else if(*next_char == ',' && !in_string)
        {
            ++next_char;
            ++num_chars_proc;
            break;
        }
        else if(next_value_len > parser->max_token_len)
        {
            goto ERR_RETURN;
        }

        ++next_value_len;
        ++next_char;
        ++num_chars_proc;
    }

    if(new_row)
    {
        parser->row += 1;
        parser->col = 0;
    } 
    else
    {
        parser->col += 1;
    }

    parser->remaining.start = next_char;
    parser->remaining.len -= num_chars_proc;

    return (ElkCsvToken){ .row=row, .col=col, .value=(ElkStr){ .start=next_value_start, .len=next_value_len }};

ERR_RETURN:
    parser->error = true;
    return (ElkCsvToken){ .row=parser->row, .col=parser->col, .value=(ElkStr){.start=parser->remaining.start, .len=0}};
}

static inline ElkStr 
elk_csv_unquote_str(ElkStr str)
{
    // remove any leading and trailing white space
    str = elk_str_strip(str);

    // 
    // Handle corner cases
    //

    // Handle string that isn't even quoted!
    if(str.len < 2 || str.start[0] != '"') { return str; }


    // Handle the empty string.
    if(str.len == 2 && str.start[0] == '"' && str.start[1] == '"') 
    {
        return (ElkStr){ .start = str.start, .len=0 }; 
    }

    // Ok, now we've got a quoted non-empty string.
    char *sub_str = &str.start[1];
    int next_read = 0;
    int next_write = 0;
    intptr_t len = 0;
    while(next_read < str.len - 2)
    {
        if(sub_str[next_read] == '"')
        {
            next_read++;
            if(next_read >= str.len -2) { break; }
        }

        if(next_read != next_write)
        {
            sub_str[next_write] = sub_str[next_read];
        }

        len += 1;
        next_read += 1;
        next_write += 1;
    }

    return (ElkStr){ .start=sub_str, .len=len};
}

#endif
