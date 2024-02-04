#ifndef _ELK_HEADER_
#define _ELK_HEADER_

#include <stdint.h>
#include <stddef.h>

#include <immintrin.h>

#if defined(_WIN32) || defined(_WIN64)
#define __lzcnt32(a) __lzcnt(a)
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Define simpler types
 *-------------------------------------------------------------------------------------------------------------------------*/

// stdbool.h
#ifndef bool
  #define bool int
  #define false 0
  #define true 1
#endif

/* Other libraries I may have already included may use these exact definitions too. */
#ifndef _TYPE_ALIASES_
#define _TYPE_ALIASES_
typedef char       byte;
typedef ptrdiff_t  size;
typedef size_t    usize;

typedef uintptr_t  uptr;
typedef intptr_t   iptr;

typedef float       f32;
typedef double      f64;

typedef uint8_t      u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef int8_t       i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 * Declare parts of the standard C library I use. These should almost always be implemented as compiler intrinsics anyway.
 *-------------------------------------------------------------------------------------------------------------------------*/

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

static i64 const SECONDS_PER_MINUTE = INT64_C(60);
static i64 const MINUTES_PER_HOUR = INT64_C(60);
static i64 const HOURS_PER_DAY = INT64_C(24);
static i64 const DAYS_PER_YEAR = INT64_C(365);
static i64 const SECONDS_PER_HOUR = INT64_C(60) * INT64_C(60);
static i64 const SECONDS_PER_DAY = INT64_C(60) * INT64_C(60) * INT64_C(24);
static i64 const SECONDS_PER_YEAR = INT64_C(60) * INT64_C(60) * INT64_C(24) * INT64_C(365);

typedef i64 ElkTime;

typedef struct 
{
    i16 year;        /* 1-32,767 */
    i8 month;        /* 1-12     */
    i8 day;          /* 1-31     */
    i8 hour;         /* 0-23     */
    i8 minute;       /* 0-59     */
    i8 second;       /* 0-59     */
    i16 day_of_year; /* 1-366    */
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

static inline i64 elk_time_to_unix_epoch(ElkTime time);
static inline ElkTime elk_time_from_unix_timestamp(i64 unixtime);
static inline bool elk_is_leap_year(int year);
static inline ElkTime elk_make_time(ElkStructTime tm); /* Ignores the day_of_year member. */
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
    size len;     // the length of the string (not including a null terminator if it's there)
} ElkStr;

typedef struct
{
    ElkStr left;
    ElkStr right;
} ElkStrSplitPair;

static inline ElkStr elk_str_from_cstring(char *src);
static inline ElkStr elk_str_copy(size dst_len, char *restrict dest, ElkStr src);
static inline ElkStr elk_str_strip(ElkStr input);                              // Strips leading and trailing whitespace
static inline ElkStr elk_str_substr(ElkStr str, size start, size len);         // Create a substring from a longer string
static inline int elk_str_cmp(ElkStr left, ElkStr right);                      // 0 if equal, -1 if left is first, 1 otherwise
static inline bool elk_str_eq(ElkStr const left, ElkStr const right);          // Faster than elk_str_cmp, checks length first
static inline ElkStrSplitPair elk_str_split_on_char(ElkStr str, char const split_char);

/* Parsing values from strings.
 *
 * In all cases the input string is assumed to be stripped of leading and trailing whitespace. Any suffixes that are non-
 * numeric will cause a parse error for the number parsing cases. Robust parsers check for more error cases, and fast 
 * parsers make more assumptions. 
 *
 * For f64, the fast parser assumes no NaN or Infinity values or any errors of any kind. The robust parser checks for NaN,
 * +/- Infinity, and overflow.
 *
 * Parsing datetimes assumes a format YYYY-MM-DD HH:MM:SS, YYYY-MM-DDTHH:MM:SS, YYYYDDDHHMMSS. The latter format is the 
 * year, day of the year, hours, minutes, and seconds.
 *
 * In general, these functions return true on success and false on failure. On falure the out argument is left untouched.
 */
static inline bool elk_str_parse_i64(ElkStr str, i64 *result);
static inline bool elk_str_robust_parse_f64(ElkStr str, f64 *out);
static inline bool elk_str_fast_parse_f64(ElkStr str, f64 *out);
static inline bool elk_str_fast_parse_2_f64(ElkStr str1, f64 *restrict out1, ElkStr str2, f64 *restrict out2);
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
 * Compile with -D_ELK_TRACK_MEM_USAGE to include the maximum memory tracking variables & features.
 *
 * A statically sized, non-growable arena allocator that works on top of a user supplied buffer.
 */

#ifdef _ELK_TRACK_MEM_USAGE
typedef struct
{
    size max_offset;
    bool over_allocation_attempted;
} ElkStaticArenaAllocationMetrics;
#endif

typedef struct 
{
    size buf_size;
    size buf_offset;
    byte *buffer;
    void *prev_ptr;
    size prev_offset;
#ifdef _ELK_TRACK_MEM_USAGE
    ElkStaticArenaAllocationMetrics *metrics_ptr;
#endif
} ElkStaticArena;

static inline void elk_static_arena_create(ElkStaticArena *arena, size buf_size, byte buffer[]);
static inline void elk_static_arena_destroy(ElkStaticArena *arena);
static inline void elk_static_arena_reset(ElkStaticArena *arena);  // Set offset to 0, invalidates all previous allocations
static inline void * elk_static_arena_alloc(ElkStaticArena *arena, size num_bytes, size alignment); // ret NULL if OOM
static inline void * elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, size size); // ret NULL if ptr is not most recent allocation
static inline void elk_static_arena_free(ElkStaticArena *arena, void *ptr); // Undo if it was last allocation, otherwise no-op

#define elk_static_arena_malloc(arena, type) (type *)elk_static_arena_alloc(arena, sizeof(type), _Alignof(type))
#define elk_static_arena_nmalloc(arena, count, type) (type *)elk_static_arena_alloc(arena, count * sizeof(type), _Alignof(type))
#define elk_static_arena_nrealloc(arena, ptr, count, type) (type *) elk_static_arena_realloc(arena, (ptr), sizeof(type) * (count))

#ifdef _ELK_TRACK_MEM_USAGE
static ElkStaticArenaAllocationMetrics elk_static_arena_metrics[32] = {0};
static size elk_static_arena_metrics_next = 0;
static inline f64 elk_static_arena_max_ratio(ElkStaticArena *arena);
static inline bool elk_static_arena_over_allocated(ElkStaticArena *arena);
#endif

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
    size object_size;    // The size of each object
    size num_objects;    // The capacity, or number of objects storable in the pool
    void *free;              // The head of a free list of available slots for objects
    byte *buffer;   // The buffer we actually store the data in
} ElkStaticPool;

static inline void elk_static_pool_create(ElkStaticPool *pool, size object_size, size num_objects, byte buffer[]);
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

typedef u64 (*ElkHashFunction)(size const size_bytes, void const *value);
typedef u64 (*ElkSimpleHash)(void const *object); // Already knows the size of the object to be hashed!

static inline u64 elk_fnv1a_hash(size const n, void const *value);
static inline u64 elk_fnv1a_hash_accumulate(size const size_bytes, void const *value, u64 const hash_so_far);
static inline u64 elk_fnv1a_hash_str(ElkStr str);

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
    u64 hash;
    ElkStr str;
} ElkStringInternerHandle;

typedef struct 
{
    ElkStaticArena *storage;          // This is where to store the strings

    ElkStringInternerHandle *handles; // The hash table - handles index into storage
    u32 num_handles;                  // The number of handles
    i8 size_exp;                  // Used to keep track of the length of *handles
} ElkStringInterner;


static inline ElkStringInterner elk_string_interner_create(i8 size_exp, ElkStaticArena *storage);
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

static size const ELK_COLLECTION_EMPTY = -1;
static size const ELK_COLLECTION_FULL = -2;

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Queue Ledger
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Mind the ELK_COLLECTION_FULL and ELK_COLLECTION_EMPTY return values.
 */

typedef struct 
{
    size capacity;
    size length;
    size front;
    size back;
} ElkQueueLedger;

static inline ElkQueueLedger elk_queue_ledger_create(size capacity);
static inline bool elk_queue_ledger_full(ElkQueueLedger *queue);
static inline bool elk_queue_ledger_empty(ElkQueueLedger *queue);
static inline size elk_queue_ledger_push_back_index(ElkQueueLedger *queue);  // index of next location to put an object
static inline size elk_queue_ledger_pop_front_index(ElkQueueLedger *queue);  // index of next location to take object
static inline size elk_queue_ledger_peek_front_index(ElkQueueLedger *queue); // index of next object, but not incremented
static inline size elk_queue_ledger_len(ElkQueueLedger const *queue);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Array Ledger
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Mind the ELK_COLLECTION_FULL and ELK_COLLECTION_EMPTY return values.
 */
typedef struct 
{
    size capacity;
    size length;
} ElkArrayLedger;

static inline ElkArrayLedger elk_array_ledger_create(size capacity);
static inline bool elk_array_ledger_full(ElkArrayLedger *array);
static inline bool elk_array_ledger_empty(ElkArrayLedger *array);
static inline size elk_array_ledger_push_back_index(ElkArrayLedger *array);
static inline size elk_array_ledger_pop_back_index(ElkArrayLedger *array);
static inline size elk_array_ledger_len(ElkArrayLedger const *array);
static inline void elk_array_ledger_reset(ElkArrayLedger *array);
static inline void elk_array_ledger_set_capacity(ElkArrayLedger *array, size capacity);

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
    u64 hash;
    void *key;
    void *value;
} ElkHashMapHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkHashMapHandle *handles;
    size num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
    i8 size_exp;
} ElkHashMap;

typedef size ElkHashMapKeyIter;

static inline ElkHashMap elk_hash_map_create(i8 size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq, ElkStaticArena *arena);
static inline void elk_hash_map_destroy(ElkHashMap *map);
static inline void *elk_hash_map_insert(ElkHashMap *map, void *key, void *value); // if return != value, key was already in the map
static inline void *elk_hash_map_lookup(ElkHashMap *map, void *key); // return NULL if not in map, otherwise return pointer to value
static inline size elk_hash_map_len(ElkHashMap *map);
static inline ElkHashMapKeyIter elk_hash_map_key_iter(ElkHashMap *map);

static inline void *elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter);

/*--------------------------------------------------------------------------------------------------------------------------
 *                                            Hash Map (Table, ElkStr as keys)
 *---------------------------------------------------------------------------------------------------------------------------
 * Designed for use when keys are strings.
 *
 * Values are not copied, they are stored as pointers, so the user must manage memory.
 *
 * Uses fnv1a hash.
 */
typedef struct // Internal only
{
    u64 hash;
    ElkStr key;
    void *value;
} ElkStrMapHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkStrMapHandle *handles;
    size num_handles;
    i8 size_exp;
} ElkStrMap;

typedef size ElkStrMapKeyIter;
typedef size ElkStrMapHandleIter;

static inline ElkStrMap elk_str_map_create(i8 size_exp, ElkStaticArena *arena);
static inline void elk_str_map_destroy(ElkStrMap *map);
static inline void *elk_str_map_insert(ElkStrMap *map, ElkStr key, void *value); // if return != value, key was already in the map
static inline void *elk_str_map_lookup(ElkStrMap *map, ElkStr key); // return NULL if not in map, otherwise return pointer to value
static inline size elk_str_map_len(ElkStrMap *map);
static inline ElkStrMapKeyIter elk_str_map_key_iter(ElkStrMap *map);
static inline ElkStrMapHandleIter elk_str_map_handle_iter(ElkStrMap *map);

static inline ElkStr elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter);
static inline ElkStrMapHandle elk_str_map_handle_iter_next(ElkStrMap *map, ElkStrMapHandleIter *iter);

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
    u64 hash;
    void *value;
} ElkHashSetHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkHashSetHandle *handles;
    size num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
    i8 size_exp;
} ElkHashSet;

typedef size ElkHashSetIter;

static inline ElkHashSet elk_hash_set_create(i8 size_exp, ElkSimpleHash val_hash, ElkEqFunction val_eq, ElkStaticArena *arena);
static inline void elk_hash_set_destroy(ElkHashSet *set);
static inline void *elk_hash_set_insert(ElkHashSet *set, void *value); // if return != value, value was already in the set
static inline void *elk_hash_set_lookup(ElkHashSet *set, void *value); // return NULL if not in set, otherwise return ptr to value
static inline size elk_hash_set_len(ElkHashSet *set);
static inline ElkHashSetIter elk_hash_set_value_iter(ElkHashSet *set);

static inline void *elk_hash_set_value_iter_next(ElkHashSet *set, ElkHashSetIter *iter);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                            Generic Macros for Collections
 *---------------------------------------------------------------------------------------------------------------------------
 * These macros take any collection and return a result.
 */
#define elk_len(x) _Generic((x),                                                                                            \
        ElkQueueLedger *: elk_queue_ledger_len,                                                                             \
        ElkArrayLedger *: elk_array_ledger_len,                                                                             \
        ElkHashMap *: elk_hash_map_len,                                                                                     \
        ElkStrMap *: elk_str_map_len,                                                                                        \
        ElkHashSet *: elk_hash_set_len)(x)



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
 *
 * Comments in CSV are rare, and can really slow down the parser. But I do find comments useful when I use CSV for some
 * smaller configuration files. So I'm implemented two CSV parser functions. The first is the "full" version, and it can
 * handle comments anywhere. The second is the "fast" parser, which can handle comment lines at the beginning of the file, 
 * but once it gets past there it is more agressively optimized by assuming no more comment lines.
 */
typedef struct
{
    size row;     // The number of CSV rows parsed so far, this includes blank lines.
    size col;     // CSV column, that is, how many commas have we passed
    ElkStr value; // The value not including the comma or any new line character
} ElkCsvToken;

typedef struct
{
    ElkStr remaining;       // The portion of the string remaining to be parsed. Useful for diagnosing parse errors.
    size row;               // Only counts parseable rows, comment lines don't count.
    size col;               // CSV column, that is, how many commas have we passed on this line.
    bool error;             // Have we encountered an error while parsing?
#ifdef __AVX2__
    i32 byte_pos;

    __m256i buf;
    u32 buf_comma_bits;
    u32 buf_newline_bits;
    u32 buf_any_delimiter_bits;
    u32 carry;
#endif
} ElkCsvParser;

static inline ElkCsvParser elk_csv_create_parser(ElkStr input);
static inline ElkCsvToken elk_csv_full_next_token(ElkCsvParser *parser);
static inline ElkCsvToken elk_csv_fast_next_token(ElkCsvParser *parser);
static inline bool elk_csv_finished(ElkCsvParser *parser);
static inline ElkStr elk_csv_unquote_str(ElkStr str);
static inline ElkStr elk_csv_simple_unquote_str(ElkStr str);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                     Coyote Add Ons
 *
 *
 *---------------------------------------------------------------------------------------------------------------------------
 * These are ONLY enabled if the coyote library has also been included. It must be included first.
 *
 */
#ifdef _COYOTE_H_
static inline ElkStaticArena elk_static_arena_allocate_and_create(size num_bytes); /* Allocates using Coyote.          */
static inline void elk_static_arena_destroy_and_deallocate(ElkStaticArena *arena); /* Frees memory with Coyote.        */

/* Use Coyote file slurp with arena. */
static inline size elk_file_slurp(char const *filename, byte **out, ElkStaticArena *arena);   
static inline ElkStr elk_file_slurp_text(char const *filename, ElkStaticArena *arena);
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *                                          Implementation of `inline` functions.
 *
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static inline i64
elk_num_leap_years_since_epoch(i64 year)
{
    Assert(year >= 1);

    year -= 1;
    return year / 4 - year / 100 + year / 400;
}

static inline i64
elk_days_since_epoch(int year)
{
    // Days in the years up to now.
    i64 const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    i64 ts = (year - 1) * DAYS_PER_YEAR + num_leap_years_since_epoch;

    return ts;
}

static inline i64
elk_time_to_unix_epoch(ElkTime time)
{
    return time - elk_unix_epoch_timestamp;
}

static inline ElkTime
elk_time_from_unix_timestamp(i64 unixtime)
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

    i64 const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    i64 const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    Assert(adjusted >= 0);

    return adjusted;
}

static inline ElkTime
elk_time_truncate_to_specific_hour(ElkTime time, int hour)
{
    Assert(hour >= 0 && hour <= 23 && time >= 0);

    ElkTime adjusted = time;

    i64 const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    i64 const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    i64 actual_hour = (adjusted / SECONDS_PER_HOUR) % HOURS_PER_DAY;
    if (actual_hour < hour) { actual_hour += 24; }

    i64 change_hours = actual_hour - hour;
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
static i64 const sum_days_to_month[2][13] = {
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
    i64 const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    ElkTime ts = (year - 1) * SECONDS_PER_YEAR + num_leap_years_since_epoch * SECONDS_PER_DAY;

    // Seconds in the months up to the start of this month
    i64 const days_until_start_of_month =
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
    i64 const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
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
    i64 const days_since_epoch = time;

    // Calculate the year
    int year = days_since_epoch / (DAYS_PER_YEAR) + 1; // High estimate, but good starting point.
    i64 test_time = elk_days_since_epoch(year);
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
    i16 day_of_year = time + 1;

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
        .second = second,
        .day_of_year = day_of_year,
    };
}

static inline ElkStr
elk_str_from_cstring(char *src)
{
    size len;
    for (len = 0; *(src + len) != '\0'; ++len) ; // intentionally left blank.
    return (ElkStr){.start = src, .len = len};
}

static inline ElkStr
elk_str_copy(size dst_len, char *restrict dest, ElkStr src)
{
    size const src_len = src.len;
    size const copy_len = src_len < dst_len ? src_len : dst_len;
    memcpy(dest, src.start, copy_len);

    // Add a terminating zero IFF we can.
    if(copy_len < dst_len) { dest[copy_len] = '\0'; }

    size end = copy_len < dst_len ? copy_len : dst_len;

    return (ElkStr){.start = dest, .len = end};
}

static inline ElkStr
elk_str_strip(ElkStr input)
{
    char *const start = input.start;
    size start_offset = 0;
    for (start_offset = 0; start_offset < input.len && start[start_offset] <= 0x20; ++start_offset);

    size end_offset = 0;
    for (end_offset = input.len - 1; end_offset > start_offset && start[end_offset] <= 0x20; --end_offset);

    return (ElkStr) { .start = &start[start_offset], .len = end_offset - start_offset + 1 };
}

static inline 
ElkStr elk_str_substr(ElkStr str, size start, size len)
{
    Assert(start >= 0 && len > 0 && start + len <= str.len);
    char *ptr_start = (char *)((size)str.start + start);
    return (ElkStr) {.start = ptr_start, .len = len};
}

static inline int
elk_str_cmp(ElkStr left, ElkStr right)
{
    if(left.start == right.start && left.len == right.len) { return 0; }

    size len = left.len > right.len ? right.len : left.len;

    for (size i = 0; i < len; ++i) 
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

    size len = left.len > right.len ? right.len : left.len;

    for (size i = 0; i < len; ++i)
    {
        if (left.start[i] != right.start[i]) { return false; }
    }

    return true;
}

static inline ElkStrSplitPair
elk_str_split_on_char(ElkStr str, char const split_char)
{
    ElkStr left = { .start = str.start, .len = 0 };
    ElkStr right = { .start = NULL, .len = 0 };

    for(char const *c = str.start; *c != split_char && left.len < str.len; ++c, ++left.len);

    if(left.len + 1 < str.len)
    {
        right.start = &str.start[left.len + 1];
        right.len = str.len - left.len - 1;
    }

    return (ElkStrSplitPair) { .left = left, .right = right};
}

_Static_assert(sizeof(size) == sizeof(uptr), "intptr_t and uintptr_t aren't the same size?!");

static inline bool 
elk_str_helper_parse_i64(ElkStr str, i64 *result)
{
    u64 parsed = 0;
    bool neg_flag = false;
    bool in_digits = false;
    char const *c = str.start;
    while (true)
    {
        // We've reached the end of the string.
        if (!*c || c >= (str.start + (uptr)str.len)) { break; }

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
elk_str_parse_i64(ElkStr str, i64 *result)
{
    // Empty string is an error
    StopIf(str.len == 0, return false);

#if 0
    /* Check that first the string is short enough, 16 bytes, and second that it does not cross a 4 KiB memory boundary.
     * The boundary check is needed because we can't be sure that we have access to the other page. If we don't, seg-fault!
     */
    if(str.len <= 16 && ((((uptr)str.start + str.len) % ELK_KiB(4)) >= 16))
    {
        // _0_ Load, but ensure the last digit is in the last position
        u8 len = (u8)str.len;
        uptr start = (uptr)str.start + str.len - 16;
        __m128i value = _mm_loadu_si128((__m128i const *)start);
        __m128i const positions = _mm_setr_epi8(16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
        __m128i const positions_mask = _mm_cmpgt_epi8(positions, _mm_set1_epi8(len));
        value = _mm_andnot_si128(positions_mask, value);                                 /* Mask out data we didn't want.*/
 
        // _1_ detect negative sign
        __m128i const neg_sign = _mm_set1_epi8('-');
        __m128i const pos_sign = _mm_set1_epi8('+');
        __m128i const neg_mask = _mm_cmpeq_epi8(value, neg_sign);
        __m128i const pos_mask = _mm_cmpeq_epi8(value, pos_sign);
        i32 neg_flag = _mm_movemask_epi8(neg_mask);
        i32 pos_flag = _mm_movemask_epi8(pos_mask);
        neg_flag = (neg_flag >> (16 - str.len)) & 0x01; /* 0-false or 1-true, remember movemask reverses bits. */
        pos_flag = (pos_flag >> (16 - str.len)) & 0x01; /* 0-false or 1-true, remember movemask reverses bits. */

        // _2_ mask out negative sign and non-digit characters
        __m128i const not_digits = _mm_or_si128(
                _mm_cmpgt_epi8(value, _mm_set1_epi8('9')),
                _mm_cmplt_epi8(value, _mm_set1_epi8('0')));
        value = _mm_andnot_si128(not_digits, value);

        // _3_ movemask to get bits representing data
        u32 const digit_bits = (~_mm_movemask_epi8(not_digits)) & 0xFFFF;

        // _4_ _mm_popcnt_u32 to get the number of digits, depending on - sign, if less than string length, ERROR!
        i32 const num_digits = _mm_popcnt_u32(digit_bits);
        if(num_digits < ((neg_flag || pos_flag) ? len - 1 : len)) { return false; }

        // _5_ subtract '0' from fields with digits.
        __m128i const zero_char = _mm_set1_epi8('0');
        value = _mm_sub_epi8(value, zero_char);
        value = _mm_andnot_si128(not_digits, value);     /* Re-zero the bytes that had zeros before this operation */
        
        // _6_ _mm_maddubs_epi16(_2_, first constant)
        __m128i const m1 = _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
        value = _mm_maddubs_epi16(value, m1);
        
        // _7_ _mm_madd_epi16(_6_, second constant)
        __m128i const m2 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
        value = _mm_madd_epi16(value, m2);

        // _8_ _mm_packs_epi32(_7_, _7_)
        value = _mm_packs_epi32(_mm_setzero_si128(), value);

        // _9_ _mm_madd_epi16(_8_, third constant)
        __m128i const m3 = _mm_setr_epi16(0, 0, 0, 0, 10000, 1, 10000, 1);
        value = _mm_madd_epi16(value, m3);

        // _10_ _mm_mul_epu32(_10_, fourth constant multiplier)
        __m128i const m4 = _mm_set1_epi32(100000000);
        __m128i value_top = _mm_mul_epu32(value, m4);

        // _11_ shuffle value into little endian for the 64 bit add coming up. _mm_shuffle_epi32(_9_, 0xB4) 0b 00 11 00 00
        value = _mm_shuffle_epi32(value, 0x30); 

        // _12_ _mm_add_epi64(_9_, _11_)
        value = _mm_add_epi64(value, value_top);

        // _13_ __int64 val = _mm_extract_epi64(_12_, 0)
        *result = (neg_flag ? -1 : 1) * _mm_extract_epi64(value, 1);
        return true;

    }
    else
    {
        return elk_str_helper_parse_i64(str, result);
    }
#else
    return elk_str_helper_parse_i64(str, result);
#endif
}

static inline bool
elk_str_robust_parse_f64(ElkStr str, f64 *out)
{
    // The following block is required to create NAN/INF witnout using math.h on MSVC Using
    // #define NAN (0.0/0.0) doesn't work either on MSVC, which gives C2124 divide by zero error.
    static f64 const ELK_ZERO = 0.0;
    f64 const ELK_INF = 1.0 / ELK_ZERO;
    f64 const ELK_NEG_INF = -1.0 / ELK_ZERO;
    f64 const ELK_NAN = 0.0 / ELK_ZERO;

    StopIf(str.len == 0, goto ERR_RETURN);

    char const *c = str.start;
    char const *end = str.start + str.len;
    size len_remaining = str.len;

    i8 sign = 0;        // 0 is positive, 1 is negative
    i8 exp_sign = 0;    // 0 is positive, 1 is negative
    i16 exponent = 0;
    i64 mantissa = 0;
    i64 extra_exp = 0;  // decimal places after the point

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

    f64 exp_part = 1.0;
    for (int i = 0; i < exponent; ++i) { exp_part *= 10.0; }
    for (int i = 0; i > exponent; --i) { exp_part /= 10.0; }

    f64 value = (f64)mantissa * exp_part;
    StopIf(value == ELK_INF || value == ELK_NEG_INF, goto ERR_RETURN);

    *out = value;
    return true;

ERR_RETURN:
    *out = ELK_NAN;
    return false;
}

static inline bool 
elk_str_fast_parse_f64(ElkStr str, f64 *out)
{
    if(__AVX2__ && str.len <=16 && ((iptr)str.start + str.len) % 4096 >= 16)
    {
        __m128i const indexes = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        __m128i const first_multiplier = _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
        __m128i const second_multiplier = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
        __m128i const third_multiplier = _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);

        iptr start_addr = (iptr)str.start + str.len - 16;

        __m128i strbuf = _mm_loadu_si128((__m128i *)start_addr);

        /* Zero out parts of buffer that aren't in my string. */
        __m128i str_mask = _mm_cmplt_epi8(indexes, _mm_set1_epi8(16 - (i8)str.len));
        strbuf = _mm_andnot_si128(str_mask, strbuf);

        /* Find index of 'e' */
        __m128i emask = _mm_cmpeq_epi8(strbuf, _mm_set1_epi8('e'));
        int8_t e_index = 31 - __lzcnt32(_mm_movemask_epi8(emask)); /* always use with -1, so just subtract 1 now and use 31 */
        e_index = e_index < 0 ? 16 : e_index;

        /* Get the exponent */
        __m128i exp_mask = _mm_cmpgt_epi8(indexes, _mm_set1_epi8(e_index));
        __m128i exponent = _mm_and_si128(exp_mask, strbuf);
        __m128i neg_exp_mask = _mm_cmpeq_epi8(exponent, _mm_set1_epi8('-'));
        bool has_neg_exp = !!_mm_movemask_epi8(neg_exp_mask);
        exponent = _mm_sub_epi8(exponent, _mm_set1_epi8('0'));
        exponent = _mm_and_si128(_mm_andnot_si128(neg_exp_mask, exp_mask), exponent);

        /* Shuffle (shift) out exponent */
        __m128i mantissa =_mm_shuffle_epi8(strbuf, _mm_add_epi8(indexes, _mm_set1_epi8(e_index)));

        /* Find index of decimal point. */
        __m128i pmask = _mm_cmpeq_epi8(mantissa, _mm_set1_epi8('.'));
        int8_t p_index = 32 - _lzcnt_u32(_mm_movemask_epi8(pmask));
        bool has_point = !!p_index;
        int extra_exp = has_point ? 16 - p_index : 0;

        /* Shuffle out the decimal point. */
        __m128i stay_mask = _mm_cmplt_epi8(indexes, _mm_set1_epi8(p_index));
        __m128i stay_shuffle = _mm_andnot_si128(stay_mask, indexes);
        __m128i shift_mask = _mm_cmplt_epi8(indexes, _mm_set1_epi8(p_index));
        __m128i shift_shuffle = _mm_and_si128(shift_mask, _mm_sub_epi8(indexes, _mm_set1_epi8(1)));
        __m128i total_shuffle = _mm_add_epi8(shift_shuffle, stay_shuffle);
        mantissa =_mm_shuffle_epi8(mantissa, total_shuffle);

        /* Find the negative sign. */
        mantissa = _mm_andnot_si128(_mm_cmplt_epi8(indexes, _mm_set1_epi8(32 - (i8)str.len - e_index + has_point)), mantissa);
        __m128i neg_mask = _mm_cmpeq_epi8(mantissa, _mm_set1_epi8('-'));
        __m128i pos_mask = _mm_cmpeq_epi8(mantissa, _mm_set1_epi8('+'));
        i64 has_neg = !!_mm_movemask_epi8(neg_mask);
        i64 has_sign = !!_mm_movemask_epi8(pos_mask) || has_neg;

        /* Subtract out the value of the zero character. */
        mantissa = _mm_sub_epi8(mantissa, _mm_set1_epi8('0'));
        mantissa = _mm_andnot_si128(_mm_cmplt_epi8(indexes, _mm_set1_epi8(32 - (i8)str.len - e_index + has_point + (i8)has_sign)), mantissa);

        /* Combine mantissa digits into decimal value. */
        mantissa = _mm_maddubs_epi16(first_multiplier, mantissa);
        mantissa = _mm_madd_epi16(second_multiplier, mantissa);
        mantissa = _mm_packs_epi32(mantissa, mantissa);
        mantissa = _mm_madd_epi16(third_multiplier, mantissa);
        __m128i reverse_mantissa = _mm_shuffle_epi32(mantissa, 27);
        mantissa = _mm_cvtepi32_epi64(mantissa);
        reverse_mantissa = _mm_cvtepi32_epi64(reverse_mantissa);
        mantissa = _mm_add_epi64(mantissa, reverse_mantissa);
        i64 mantissai = _mm_extract_epi64(mantissa, 1);

        /* Combine exponent digits into decimal value and account for extra exponent due to decimal point. */
        exponent = _mm_maddubs_epi16(first_multiplier, exponent);
        exponent = _mm_madd_epi16(second_multiplier, exponent);
        i64 expi = _mm_extract_epi32(exponent, 3);
        expi = has_neg_exp ? -expi : expi;

        /* Account for removing the decimal point. */
        expi -= extra_exp;

        f64 expf = 1.0;
        for(int i = 0; i < expi; ++i) { expf *= 10.0; };
        for(int i = 0; i > expi; --i) { expf /= 10.0; };
        f64 ret_val = (f64) mantissai * expf;

        *out = has_neg ? -ret_val : ret_val;
        return true;
    }   
    else
    {
        // The following block is required to create NAN/INF witnout using math.h on MSVC Using
        // #define NAN (0.0/0.0) doesn't work either on MSVC, which gives C2124 divide by zero error.
        static f64 const ELK_ZERO = 0.0;
        f64 const ELK_INF = 1.0 / ELK_ZERO;
        f64 const ELK_NEG_INF = -1.0 / ELK_ZERO;
        f64 const ELK_NAN = 0.0 / ELK_ZERO;

        StopIf(str.len == 0, goto ERR_RETURN);

        char const *c = str.start;
        char const *end = str.start + str.len;

        i8 sign = 0;        // 0 is positive, 1 is negative
        i8 exp_sign = 0;    // 0 is positive, 1 is negative
        i16 exponent = 0;
        i64 mantissa = 0;
        i64 extra_exp = 0;  // decimal places after the point

        // Check & parse a sign
        if (*c == '-')      { sign =  1; ++c; }
        else if (*c == '+') { sign =  0; ++c; }

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
                exponent = exponent * 10 + digit;

                ++c;
                digit = *c - '0';
            }

            // Account for negative signs
            exponent = exp_sign == 1 ? -exponent : exponent;
        }

        // Account for decimal point location.
        exponent -= extra_exp;

        f64 exp_part = 1.0;
        for (int i = 0; i < exponent; ++i) { exp_part *= 10.0; }
        for (int i = 0; i > exponent; --i) { exp_part /= 10.0; }

        f64 value = (f64)mantissa * exp_part;
        StopIf(value == ELK_INF || value == ELK_NEG_INF, goto ERR_RETURN);

        *out = value;
        return true;

    ERR_RETURN:
        *out = ELK_NAN;
        return false;
    }
}

/* The idea hear is like loop unrolling, doing two at the same time may help with instruction level parallelism. */
static inline bool 
elk_str_fast_parse_2_f64(ElkStr str1, f64 *restrict out1, ElkStr str2, f64 *restrict out2)
{
    if(__AVX2__
       && str1.len <=16
       && str2.len <=16
       && ((iptr)str1.start + str1.len) % 4096 >= 16
       && ((iptr)str2.start + str2.len) % 4096 >= 16
    )
    {
        __m128i const indexes = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        __m128i const first_multiplier = _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
        __m128i const second_multiplier = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
        __m128i const third_multiplier = _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);

        iptr start_addr1 = (iptr)str1.start + str1.len - 16;
        iptr start_addr2 = (iptr)str2.start + str2.len - 16;

        __m128i strbuf1 = _mm_loadu_si128((__m128i *)start_addr1);
        __m128i strbuf2 = _mm_loadu_si128((__m128i *)start_addr2);

        /* Zero out parts of buffer that aren't in my string. */
        __m128i str_mask1 = _mm_cmplt_epi8(indexes, _mm_set1_epi8(16 - (i8)str1.len));
        strbuf1 = _mm_andnot_si128(str_mask1, strbuf1);
        __m128i str_mask2 = _mm_cmplt_epi8(indexes, _mm_set1_epi8(16 - (i8)str2.len));
        strbuf2 = _mm_andnot_si128(str_mask2, strbuf2);

        /* Find index of 'e' */
        __m128i emask1 = _mm_cmpeq_epi8(strbuf1, _mm_set1_epi8('e'));
        int8_t e_index1 = 31 - __lzcnt32(_mm_movemask_epi8(emask1)); /* always use with -1, so just subtract 1 now and use 31 */
        e_index1 = e_index1 < 0 ? 16 : e_index1;
        __m128i emask2 = _mm_cmpeq_epi8(strbuf2, _mm_set1_epi8('e'));
        int8_t e_index2 = 31 - __lzcnt32(_mm_movemask_epi8(emask2)); /* always use with -1, so just subtract 1 now and use 31 */
        e_index2 = e_index2 < 0 ? 16 : e_index2;

        /* Get the exponent */
        __m128i exp_mask1 = _mm_cmpgt_epi8(indexes, _mm_set1_epi8(e_index1));
        __m128i exponent1 = _mm_and_si128(exp_mask1, strbuf1);
        __m128i neg_exp_mask1 = _mm_cmpeq_epi8(exponent1, _mm_set1_epi8('-'));
        bool has_neg_exp1 = !!_mm_movemask_epi8(neg_exp_mask1);
        exponent1 = _mm_sub_epi8(exponent1, _mm_set1_epi8('0'));
        exponent1 = _mm_and_si128(_mm_andnot_si128(neg_exp_mask1, exp_mask1), exponent1);
        __m128i exp_mask2 = _mm_cmpgt_epi8(indexes, _mm_set1_epi8(e_index2));
        __m128i exponent2 = _mm_and_si128(exp_mask2, strbuf2);
        __m128i neg_exp_mask2 = _mm_cmpeq_epi8(exponent2, _mm_set1_epi8('-'));
        bool has_neg_exp2 = !!_mm_movemask_epi8(neg_exp_mask2);
        exponent2 = _mm_sub_epi8(exponent2, _mm_set1_epi8('0'));
        exponent2 = _mm_and_si128(_mm_andnot_si128(neg_exp_mask2, exp_mask2), exponent2);

        /* Shuffle (shift) out exponent */
        __m128i mantissa1 =_mm_shuffle_epi8(strbuf1, _mm_add_epi8(indexes, _mm_set1_epi8(e_index1)));
        __m128i mantissa2 =_mm_shuffle_epi8(strbuf2, _mm_add_epi8(indexes, _mm_set1_epi8(e_index2)));

        /* Find index of decimal point. */
        __m128i pmask1 = _mm_cmpeq_epi8(mantissa1, _mm_set1_epi8('.'));
        int8_t p_index1 = 32 - _lzcnt_u32(_mm_movemask_epi8(pmask1));
        bool has_point1 = !!p_index1;
        int extra_exp1 = has_point1 ? 16 - p_index1 : 0;
        __m128i pmask2 = _mm_cmpeq_epi8(mantissa2, _mm_set1_epi8('.'));
        int8_t p_index2 = 32 - _lzcnt_u32(_mm_movemask_epi8(pmask2));
        bool has_point2 = !!p_index2;
        int extra_exp2 = has_point2 ? 16 - p_index2 : 0;

        /* Shuffle out the decimal point. */
        __m128i stay_mask1 = _mm_cmplt_epi8(indexes, _mm_set1_epi8(p_index1));
        __m128i stay_shuffle1 = _mm_andnot_si128(stay_mask1, indexes);
        __m128i shift_mask1 = _mm_cmplt_epi8(indexes, _mm_set1_epi8(p_index1));
        __m128i shift_shuffle1 = _mm_and_si128(shift_mask1, _mm_sub_epi8(indexes, _mm_set1_epi8(1)));
        __m128i total_shuffle1 = _mm_add_epi8(shift_shuffle1, stay_shuffle1);
        mantissa1 =_mm_shuffle_epi8(mantissa1, total_shuffle1);
        __m128i stay_mask2 = _mm_cmplt_epi8(indexes, _mm_set1_epi8(p_index2));
        __m128i stay_shuffle2 = _mm_andnot_si128(stay_mask2, indexes);
        __m128i shift_mask2 = _mm_cmplt_epi8(indexes, _mm_set1_epi8(p_index2));
        __m128i shift_shuffle2 = _mm_and_si128(shift_mask2, _mm_sub_epi8(indexes, _mm_set1_epi8(1)));
        __m128i total_shuffle2 = _mm_add_epi8(shift_shuffle2, stay_shuffle2);
        mantissa2 =_mm_shuffle_epi8(mantissa2, total_shuffle2);

        /* Find the negative sign. */
        mantissa1 = _mm_andnot_si128(_mm_cmplt_epi8(indexes, _mm_set1_epi8(32 - (i8)str1.len - e_index1 + has_point1)), mantissa1);
        __m128i neg_mask1 = _mm_cmpeq_epi8(mantissa1, _mm_set1_epi8('-'));
        __m128i pos_mask1 = _mm_cmpeq_epi8(mantissa1, _mm_set1_epi8('+'));
        i64 has_neg1 = !!_mm_movemask_epi8(neg_mask1);
        i64 has_sign1 = !!_mm_movemask_epi8(pos_mask1) || has_neg1;
        mantissa2 = _mm_andnot_si128(_mm_cmplt_epi8(indexes, _mm_set1_epi8(32 - (i8)str2.len - e_index2 + has_point2)), mantissa2);
        __m128i neg_mask2 = _mm_cmpeq_epi8(mantissa2, _mm_set1_epi8('-'));
        __m128i pos_mask2 = _mm_cmpeq_epi8(mantissa2, _mm_set1_epi8('+'));
        i64 has_neg2 = !!_mm_movemask_epi8(neg_mask2);
        i64 has_sign2 = !!_mm_movemask_epi8(pos_mask2) || has_neg2;

        /* Subtract out the value of the zero character. */
        mantissa1 = _mm_sub_epi8(mantissa1, _mm_set1_epi8('0'));
        mantissa1 = _mm_andnot_si128(_mm_cmplt_epi8(indexes, _mm_set1_epi8(32 - (i8)str1.len - e_index1 + has_point1 + (i8)has_sign1)), mantissa1);
        mantissa2 = _mm_sub_epi8(mantissa2, _mm_set1_epi8('0'));
        mantissa2 = _mm_andnot_si128(_mm_cmplt_epi8(indexes, _mm_set1_epi8(32 - (i8)str2.len - e_index2 + has_point2 + (i8)has_sign2)), mantissa2);

        /* Combine mantissa digits into decimal value. */
        mantissa1 = _mm_maddubs_epi16(first_multiplier, mantissa1);
        mantissa1 = _mm_madd_epi16(second_multiplier, mantissa1);
        mantissa1 = _mm_packs_epi32(mantissa1, mantissa1);
        mantissa1 = _mm_madd_epi16(third_multiplier, mantissa1);
        __m128i reverse_mantissa1 = _mm_shuffle_epi32(mantissa1, 27);
        mantissa1 = _mm_cvtepi32_epi64(mantissa1);
        reverse_mantissa1 = _mm_cvtepi32_epi64(reverse_mantissa1);
        mantissa1 = _mm_add_epi64(mantissa1, reverse_mantissa1);
        i64 mantissai1 = _mm_extract_epi64(mantissa1, 1);
        mantissa2 = _mm_maddubs_epi16(first_multiplier, mantissa2);
        mantissa2 = _mm_madd_epi16(second_multiplier, mantissa2);
        mantissa2 = _mm_packs_epi32(mantissa2, mantissa2);
        mantissa2 = _mm_madd_epi16(third_multiplier, mantissa2);
        __m128i reverse_mantissa2 = _mm_shuffle_epi32(mantissa2, 27);
        mantissa2 = _mm_cvtepi32_epi64(mantissa2);
        reverse_mantissa2 = _mm_cvtepi32_epi64(reverse_mantissa2);
        mantissa2 = _mm_add_epi64(mantissa2, reverse_mantissa2);
        i64 mantissai2 = _mm_extract_epi64(mantissa2, 1);

        /* Combine exponent digits into decimal value and account for extra exponent due to decimal point. */
        exponent1 = _mm_maddubs_epi16(first_multiplier, exponent1);
        exponent1 = _mm_madd_epi16(second_multiplier, exponent1);
        i64 expi1 = _mm_extract_epi32(exponent1, 3);
        expi1 = has_neg_exp1 ? -expi1 : expi1;
        exponent2 = _mm_maddubs_epi16(first_multiplier, exponent2);
        exponent2 = _mm_madd_epi16(second_multiplier, exponent2);
        i64 expi2 = _mm_extract_epi32(exponent2, 3);
        expi2 = has_neg_exp2 ? -expi2 : expi2;

        /* Account for removing the decimal point. */
        expi1 -= extra_exp1;
        expi2 -= extra_exp2;

        f64 expf1 = 1.0;
        for(int i = 0; i < expi1; ++i) { expf1 *= 10.0; };
        for(int i = 0; i > expi1; --i) { expf1 /= 10.0; };
        f64 ret_val1 = (f64) mantissai1 * expf1;
        f64 expf2 = 1.0;
        for(int i = 0; i < expi2; ++i) { expf2 *= 10.0; };
        for(int i = 0; i > expi2; --i) { expf2 /= 10.0; };
        f64 ret_val2 = (f64) mantissai2 * expf2;

        *out1 = has_neg1 ? -ret_val1 : ret_val1;
        *out2 = has_neg2 ? -ret_val2 : ret_val2;
        return true;
    }
    else
    {
        return elk_str_fast_parse_f64(str1, out1) && elk_str_fast_parse_f64(str2, out2);
    }
}

static inline bool
elk_str_parse_datetime_long_format(ElkStr str, ElkTime *out)
{
    /* Calculate the start address to load it into the buffer with just the right positions for the characters. */
    uptr start = (uptr)str.start + str.len + 7 - 32;

    /* YYYY-MM-DD HH:MM:SS and YYYY-MM-DDTHH:MM:SS formats */
    /* Check to make sure we have AVX and that the string buffer won't cross a 4 KiB boundary. */
    if(__AVX2__ && ((((uptr)str.start + str.len + 7) % ELK_KiB(4)) >= 32))
    {
        __m256i dt_string = _mm256_loadu_si256((__m256i *)start);

        /* Shuffle around the numbers to get them in position for doing math. */
        __m256i shuffle = _mm256_setr_epi8(
                0xFF, 0xFF, 0xFF, 0xFF,    6,    7,    8,    9, 0xFF, 0xFF,   11,   12, 0xFF, 0xFF,   14,   15,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,    1,    2, 0xFF, 0xFF,    4,    5, 0xFF, 0xFF,    7,    8);
        dt_string = _mm256_shuffle_epi8(dt_string, shuffle);

        /* Subtract out the zero char value and mask out lanes that hold no data we need */
        dt_string = _mm256_sub_epi8(dt_string, _mm256_set1_epi8('0'));
        __m256i data_mask = _mm256_setr_epi8(
                0xFF, 0xFF, 0xFF, 0xFF,    0,    0,    0,    0, 0xFF, 0xFF,    0,    0, 0xFF, 0xFF,    0,    0,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,    0,    0, 0xFF, 0xFF,    0,    0, 0xFF, 0xFF,    0,    0);
        dt_string = _mm256_andnot_si256(data_mask, dt_string);

        /* Multiply and add adjacent lanes */
        __m256i madd_const1 = _mm256_setr_epi8(
                0,  0,  0,  0, 10,  1, 10,  1,  0,  0, 10,  1,  0,  0, 10,  1,
                0,  0,  0,  0,  0,  0, 10,  1,  0,  0, 10,  1,  0,  0, 10,  1);
        __m256i vals16 = _mm256_maddubs_epi16(dt_string, madd_const1);

        /* Month, day, hour, minute, second are already done! Now need to calculate year. */
        __m256i madd_const2 = _mm256_setr_epi16(0, 0, 100, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        __m256i vals32 = _mm256_madd_epi16(vals16, madd_const2);

        i16 year = _mm256_extract_epi32(vals32, 1);
        i8 month = _mm256_extract_epi16(vals16, 5);
        i8 day = _mm256_extract_epi16(vals16, 7);
        i8 hour = _mm256_extract_epi16(vals16, 11);
        i8 minutes = _mm256_extract_epi16(vals16, 13);
        i8 seconds = _mm256_extract_epi16(vals16, 15);

            *out = elk_time_from_ymd_and_hms(year, month, day, hour, minutes, seconds);
            return true;
    }
    else
    {
        i64 year = INT64_MIN;
        i64 month = INT64_MIN;
        i64 day = INT64_MIN;
        i64 hour = INT64_MIN;
        i64 minutes = INT64_MIN;
        i64 seconds = INT64_MIN;

        if(
            elk_str_parse_i64(elk_str_substr(str,  0, 4), &year    ) && 
            elk_str_parse_i64(elk_str_substr(str,  5, 2), &month   ) &&
            elk_str_parse_i64(elk_str_substr(str,  8, 2), &day     ) &&
            elk_str_parse_i64(elk_str_substr(str, 11, 2), &hour    ) &&
            elk_str_parse_i64(elk_str_substr(str, 14, 2), &minutes ) &&
            elk_str_parse_i64(elk_str_substr(str, 17, 2), &seconds ))
        {
            *out = elk_time_from_ymd_and_hms(year, month, day, hour, minutes, seconds);
            return true;
        }
    }

    return false;
}

static inline bool
elk_str_parse_datetime_compact_doy(ElkStr str, ElkTime *out)
{
    // YYYYDDDHHMMSS format
    i64 year = INT64_MIN;
    i64 day_of_year = INT64_MIN;
    i64 hour = INT64_MIN;
    i64 minutes = INT64_MIN;
    i64 seconds = INT64_MIN;

    if(
        elk_str_parse_i64(elk_str_substr(str,  0, 4), &year        ) && 
        elk_str_parse_i64(elk_str_substr(str,  4, 3), &day_of_year ) &&
        elk_str_parse_i64(elk_str_substr(str,  7, 2), &hour        ) &&
        elk_str_parse_i64(elk_str_substr(str,  9, 2), &minutes     ) &&
        elk_str_parse_i64(elk_str_substr(str, 11, 2), &seconds     ))
    {
        *out = elk_time_from_yd_and_hms(year, day_of_year, hour, minutes, seconds);
        return true;
    }

    return false;
}

static inline bool
elk_str_parse_datetime(ElkStr str, ElkTime *out)
{
    // Check the length to find out what type of string we are parsing.
    switch(str.len)
    {
        // YYYY-MM-DD HH:MM:SS and YYYY-MM-DDTHH:MM:SS formats
        case 19: return elk_str_parse_datetime_long_format(str, out);

        // YYYYDDDHHMMSS format
        case 13: return elk_str_parse_datetime_compact_doy(str, out);

        default: return false;
    }

    return false;
}

static u64 const fnv_offset_bias = 0xcbf29ce484222325;
static u64 const fnv_prime = 0x00000100000001b3;

static inline u64
elk_fnv1a_hash_accumulate(size const size_bytes, void const *value, u64 const hash_so_far)
{
    u8 const *data = value;

    u64 hash = hash_so_far;
    for (size i = 0; i < size_bytes; ++i)
    {
        hash ^= data[i];
        hash *= fnv_prime;
    }

    return hash;
}

static inline u64
elk_fnv1a_hash(size const n, void const *value)
{
    return elk_fnv1a_hash_accumulate(n, value, fnv_offset_bias);
}

static inline u64
elk_fnv1a_hash_str(ElkStr str)
{
    return elk_fnv1a_hash(str.len, str.start);
}

static inline ElkStringInterner 
elk_string_interner_create(i8 size_exp, ElkStaticArena *storage)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    usize const handles_len = 1 << size_exp;
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
elk_hash_table_large_enough(usize num_handles, i8 size_exp)
{
    // Shoot for no more than 75% of slots filled.
    return num_handles < 3 * (1 << size_exp) / 4;
}

static inline u32
elk_hash_lookup(u64 hash, i8 exp, u32 idx)
{
    // Copied from https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.
    u32 mask = (UINT32_C(1) << exp) - 1;
    u32 step = (hash >> (64 - exp)) | 1;    // the | 1 forces an odd number
    return (idx + step) & mask;
}

static inline void
elk_string_interner_expand_table(ElkStringInterner *interner)
{
    i8 const size_exp = interner->size_exp;
    i8 const new_size_exp = size_exp + 1;

    usize const handles_len = 1 << size_exp;
    usize const new_handles_len = 1 << new_size_exp;

    ElkStringInternerHandle *new_handles = elk_static_arena_nmalloc(interner->storage, new_handles_len, ElkStringInternerHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (handle->str.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
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

    u64 const hash = elk_fnv1a_hash_str(str);
    u32 i = hash & 0xffffffff; // truncate
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
elk_is_power_of_2(uptr p)
{
    return (p & (p - 1)) == 0;
}
#endif

static inline uptr
elk_align_pointer(uptr ptr, usize align)
{

    Assert(elk_is_power_of_2(align));

    uptr a = (uptr)align;
    uptr mod = ptr & (a - 1); // Same as (ptr % a) but faster as 'a' is a power of 2

    if (mod != 0)
    {
        // push the address forward to the next value which is aligned
        ptr += a - mod;
    }

    return ptr;
}

static inline void
elk_static_arena_create(ElkStaticArena *arena, size buf_size, byte buffer[])
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

#ifdef _ELK_TRACK_MEM_USAGE
        arena->metrics_ptr = &elk_static_arena_metrics[elk_static_arena_metrics_next++];
        *arena->metrics_ptr = (ElkStaticArenaAllocationMetrics){0};
#endif
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
elk_static_arena_alloc(ElkStaticArena *arena, size num_bytes, size alignment)
{
    Assert(num_bytes > 0 && alignment > 0);

    // Align 'curr_offset' forward to the specified alignment
    uptr curr_ptr = (uptr)arena->buffer + (uptr)arena->buf_offset;
    uptr offset = elk_align_pointer(curr_ptr, alignment);
    offset -= (uptr)arena->buffer; // change to relative offset

    // Check to see if there is enough space left
    if (offset + num_bytes <= arena->buf_size)
    {
        void *ptr = &arena->buffer[offset];
        memset(ptr, 0, num_bytes);
        arena->prev_offset = arena->buf_offset;
        arena->buf_offset = offset + num_bytes;
        arena->prev_ptr = ptr;

#ifdef _ELK_TRACK_MEM_USAGE
        arena->metrics_ptr->max_offset = arena->buf_offset > (arena->metrics_ptr->max_offset) ?
            arena->buf_offset : (arena->metrics_ptr->max_offset);
#endif

        return ptr;
    }
    else
    {
#ifdef _ELK_TRACK_MEM_USAGE
        arena->metrics_ptr->over_allocation_attempted = true;
#endif
        return NULL;
    }
}

static inline void * 
elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, size size)
{
    Assert(size > 0);

    if(ptr == arena->prev_ptr)
    {
        // Get previous extra offset due to alignment
        uptr offset = (uptr)ptr - (uptr)arena->buffer; // relative offset accounting for alignment

        // Check to see if there is enough space left
        if (offset + size <= arena->buf_size)
        {
            arena->buf_offset = offset + size;

#ifdef _ELK_TRACK_MEM_USAGE
            arena->metrics_ptr->max_offset = arena->buf_offset > (arena->metrics_ptr->max_offset) ?
                arena->buf_offset : (arena->metrics_ptr->max_offset);
#endif

            return ptr;
        }
    }

#ifdef _ELK_TRACK_MEM_USAGE
            arena->metrics_ptr->over_allocation_attempted = true;
#endif
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

#ifdef _ELK_TRACK_MEM_USAGE

static inline f64 
elk_static_arena_max_ratio(ElkStaticArena *arena)
{
    return (f64)arena->metrics_ptr->max_offset / (f64)arena->buf_size;
}

static inline bool 
elk_static_arena_over_allocated(ElkStaticArena *arena)
{
    return arena->metrics_ptr->over_allocation_attempted;
}

#endif

static inline void
elk_static_pool_initialize_linked_list(byte *buffer, size object_size, size num_objects)
{

    // Initialize the free list to a linked list.

    // start by pointing to last element and assigning it NULL
    size offset = object_size * (num_objects - 1);
    uptr *ptr = (uptr *)&buffer[offset];
    *ptr = (uptr)NULL;

    // Then work backwards to the front of the list.
    while (offset) 
    {
        size next_offset = offset;
        offset -= object_size;
        ptr = (uptr *)&buffer[offset];
        uptr next = (uptr)&buffer[next_offset];
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
elk_static_pool_create(ElkStaticPool *pool, size object_size, size num_objects, byte buffer[])
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
    uptr *next = ptr;
    *next = (uptr)pool->free;
    pool->free = ptr;
}

static inline void *
elk_static_pool_alloc(ElkStaticPool *pool)
{
    void *ptr = pool->free;
    uptr *next = pool->free;

    if (ptr) 
    {
        pool->free = (void *)*next;
        memset(ptr, 0, pool->object_size);
    }

    return ptr;
}

// Just a stub so that it will work in the generic macros.
static inline void *
elk_static_pool_alloc_aligned(ElkStaticPool *pool, size num_bytes, size alignment)
{
    Assert(pool->object_size == num_bytes); // This will trip up elk_allocator_nmalloc if count > 1
    return elk_static_pool_alloc(pool);
}

static inline ElkQueueLedger
elk_queue_ledger_create(size capacity)
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

static inline size
elk_queue_ledger_push_back_index(ElkQueueLedger *queue)
{
    if(elk_queue_ledger_full(queue)) { return ELK_COLLECTION_FULL; }

    size idx = queue->back % queue->capacity;
    queue->back += 1;
    queue->length += 1;
    return idx;
}

static inline size
elk_queue_ledger_pop_front_index(ElkQueueLedger *queue)
{
    if(elk_queue_ledger_empty(queue)) { return ELK_COLLECTION_EMPTY; }

    size idx = queue->front % queue->capacity;
    queue->front += 1;
    queue->length -= 1;
    return idx;
}

static inline size
elk_queue_ledger_peek_front_index(ElkQueueLedger *queue)
{
    if(queue->length == 0) { return ELK_COLLECTION_EMPTY; }
    return queue->front % queue->capacity;
}

static inline size
elk_queue_ledger_len(ElkQueueLedger const *queue)
{
    return queue->length;
}

static inline ElkArrayLedger
elk_array_ledger_create(size capacity)
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

static inline size
elk_array_ledger_push_back_index(ElkArrayLedger *array)
{
    if(elk_array_ledger_full(array)) { return ELK_COLLECTION_FULL; }

    size idx = array->length;
    array->length += 1;
    return idx;
}

static inline size
elk_array_ledger_pop_back_index(ElkArrayLedger *array)
{
    if(elk_array_ledger_empty(array)) { return ELK_COLLECTION_EMPTY; }

    return --array->length;
}

static inline size
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
elk_array_ledger_set_capacity(ElkArrayLedger *array, size capacity)
{
    Assert(capacity > 0);
    array->capacity = capacity;
}

static ElkHashMap 
elk_hash_map_create(i8 size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size const handles_len = 1 << size_exp;
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
    i8 const size_exp = map->size_exp;
    i8 const new_size_exp = size_exp + 1;

    size const handles_len = 1 << size_exp;
    size const new_handles_len = 1 << new_size_exp;

    ElkHashMapHandle *new_handles = elk_static_arena_nmalloc(map->arena, new_handles_len, ElkHashMapHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkHashMapHandle *handle = &map->handles[i];

        if (handle->key == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
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

    u64 const hash = map->hasher(key);
    u32 i = hash & 0xffffffff; // truncate
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

    u64 const hash = map->hasher(key);
    u32 i = hash & 0xffffffff; // truncate
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

static inline size 
elk_hash_map_len(ElkHashMap *map)
{
    return map->num_handles;
}

static inline ElkHashMapKeyIter 
elk_hash_map_key_iter(ElkHashMap *map)
{
    return 0;
}

static inline void *
elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter)
{
    size const max_iter = (1 << map->size_exp);
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
elk_str_map_create(i8 size_exp, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size const handles_len = 1 << size_exp;
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
    i8 const size_exp = map->size_exp;
    i8 const new_size_exp = size_exp + 1;

    size const handles_len = 1 << size_exp;
    size const new_handles_len = 1 << new_size_exp;

    ElkStrMapHandle *new_handles = elk_static_arena_nmalloc(map->arena, new_handles_len, ElkStrMapHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkStrMapHandle *handle = &map->handles[i];

        if (handle->key.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
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

    u64 const hash = elk_fnv1a_hash_str(key);
    u32 i = hash & 0xffffffff; // truncate
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

    u64 const hash = elk_fnv1a_hash_str(key);
    u32 i = hash & 0xffffffff; // truncate
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

static inline size
elk_str_map_len(ElkStrMap *map)
{
    return map->num_handles;
}

static inline ElkHashMapKeyIter 
elk_str_map_key_iter(ElkStrMap *map)
{
    return 0;
}

static inline ElkStrMapHandleIter 
elk_str_map_handle_iter(ElkStrMap *map)
{
    return 0;
}

static inline ElkStr 
elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter)
{
    size const max_iter = (1 << map->size_exp);
    if(*iter < max_iter) 
    {
        ElkStr next_key = map->handles[*iter].key;
        *iter += 1;
        while(next_key.start == NULL && *iter < max_iter)
        {
            next_key = map->handles[*iter].key;
            *iter += 1;
        }

        return next_key;
    }
    else
    {
        return (ElkStr){.start=NULL, .len=0};
    }

}

static inline ElkStrMapHandle 
elk_str_map_handle_iter_next(ElkStrMap *map, ElkStrMapHandleIter *iter)
{
    size const max_iter = (1 << map->size_exp);
    if(*iter < max_iter) 
    {
        ElkStrMapHandle next = map->handles[*iter];
        *iter += 1;
        while(next.key.start == NULL && *iter < max_iter)
        {
            next = map->handles[*iter];
            *iter += 1;
        }

        return next;
    }
    else
    {
        return (ElkStrMapHandle){ .key = (ElkStr){.start=NULL, .len=0}, .hash = 0, .value = NULL };
    }
}

static inline ElkHashSet 
elk_hash_set_create(i8 size_exp, ElkSimpleHash val_hash, ElkEqFunction val_eq, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size const handles_len = 1 << size_exp;
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
    i8 const size_exp = set->size_exp;
    i8 const new_size_exp = size_exp + 1;

    size const handles_len = 1 << size_exp;
    size const new_handles_len = 1 << new_size_exp;

    ElkHashSetHandle *new_handles = elk_static_arena_nmalloc(set->arena, new_handles_len, ElkHashSetHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkHashSetHandle *handle = &set->handles[i];

        if (handle->value == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
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

    u64 const hash = set->hasher(value);
    u32 i = hash & 0xffffffff; // truncate
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

    u64 const hash = set->hasher(value);
    u32 i = hash & 0xffffffff; // truncate
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

static inline size 
elk_hash_set_len(ElkHashSet *set)
{
    return set->num_handles;
}

static inline ElkHashSetIter
elk_hash_set_value_iter(ElkHashSet *set)
{
    return 0;
}

static inline void *
elk_hash_set_value_iter_next(ElkHashSet *set, ElkHashSetIter *iter)
{
    size const max_iter = (1 << set->size_exp);
    void *next_value = NULL;
    if(*iter >= max_iter) { return next_value; }

    do
    {
        next_value = set->handles[*iter].value;
        *iter += 1;

    } while(next_value == NULL && *iter < max_iter);

    return next_value;
}

#if __AVX2__
static inline void elk_csv_helper_load_new_buffer_aligned(ElkCsvParser *p, i8 skip_bytes);
#endif

static inline ElkCsvParser 
elk_csv_create_parser(ElkStr input)
{
    ElkCsvParser parser = { .remaining=input, .row=0, .col=0, .error=false };

    // Scan past leading comment lines.
    while(*parser.remaining.start == '#')
    {
        // We must be on a comment line if we got here, so read just past the end of the line
        while(*parser.remaining.start && parser.remaining.len > 0)
        {
            char last_char = *parser.remaining.start;
            ++parser.remaining.start;
            --parser.remaining.len;
            if(last_char == '\n') { break; }
        }
    }

#if __AVX2__
    uptr skip_bytes = (uptr)parser.remaining.start - ((uptr)parser.remaining.start & ~0x1F); 
    parser.remaining.start = (char *)((uptr)parser.remaining.start & ~0x1F); /* Force 32 byte alignment */
    parser.carry = 0;
    elk_csv_helper_load_new_buffer_aligned(&parser, skip_bytes);
#endif

    return parser;
}

static inline bool 
elk_csv_finished(ElkCsvParser *parser)
{
    return parser->error || parser->remaining.len == 0;
}

static inline ElkCsvToken 
elk_csv_full_next_token(ElkCsvParser *parser)
{
    StopIf(elk_csv_finished(parser), goto ERR_RETURN);

    // Current position in parser and how much we've processed
    char *next_char = parser->remaining.start;
    int num_chars_proc = 0;
    size row = parser->row;
    size col = parser->col;

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
    size next_value_len = 0;

    // Are we in a quoted string where we should ignore commas?
    bool stop = false;
    u32 carry = 0;

#if __AVX2__
    /* Do SIMD */

    __m256i quotes = _mm256_set1_epi8('"');
    __m256i commas = _mm256_set1_epi8(',');
    __m256i newlines = _mm256_set1_epi8('\n');

    __m256i S = _mm256_setr_epi64x(0x0000000000000000, 0x0101010101010101, 0x0202020202020202, 0x0303030303030303);
    __m256i M = _mm256_set1_epi64x(0x7fbfdfeff7fbfdfe);
    __m256i A = _mm256_set1_epi64x(0xffffffffffffffff);

    while(!stop && parser->remaining.len > num_chars_proc + 32)
    {
        __m256i chars = _mm256_lddqu_si256((__m256i *)next_char);

        __m256i quote_mask = _mm256_cmpeq_epi8(chars, quotes);
        __m256i comma_mask = _mm256_cmpeq_epi8(chars, commas);
        __m256i newline_mask = _mm256_cmpeq_epi8(chars, newlines);

        /* https://nullprogram.com/blog/2021/12/04/ - public domain code to create running mask */
        u32 running_quote_mask = _mm256_movemask_epi8(quote_mask);
        u32 r = running_quote_mask;

        while(running_quote_mask)
        {
            r ^= -running_quote_mask ^ running_quote_mask;
            running_quote_mask &= running_quote_mask - 1;
        }

        running_quote_mask = r ^ carry;
        carry = -(running_quote_mask >> 31);
        quote_mask = _mm256_set1_epi32(running_quote_mask);
        quote_mask = _mm256_shuffle_epi8(quote_mask, S);
        quote_mask = _mm256_or_si256(quote_mask, M);
        quote_mask = _mm256_cmpeq_epi8(quote_mask, A);

        comma_mask = _mm256_andnot_si256(quote_mask, comma_mask);
        u32 comma_bits = _mm256_movemask_epi8(comma_mask);
        newline_mask = _mm256_andnot_si256(quote_mask, newline_mask);
        u32 newline_bits = _mm256_movemask_epi8(newline_mask);

        __m256i comma_or_newline_mask = _mm256_or_si256(comma_mask, newline_mask);
        u32 comma_or_newline_bits = _mm256_movemask_epi8(comma_or_newline_mask);

        u32 first_non_zero_bit_lsb = comma_or_newline_bits & ~(comma_or_newline_bits - 1);
        i32 bit_pos = 31 - __lzcnt32(first_non_zero_bit_lsb);
        bit_pos = bit_pos > 31 || bit_pos < 0 ? 31 : bit_pos;

        bool comma = (comma_bits >> bit_pos) & 1;
        bool newline = (newline_bits >> bit_pos) & 1;
        bool comma_or_newline = (comma_or_newline_bits >> bit_pos) & 1;

        parser->row += newline;
        parser->col += -col * newline + comma;
        next_value_len += bit_pos + 1 - comma_or_newline;

        next_char += bit_pos + 1 ;
        num_chars_proc += bit_pos + 1 ;

        stop = comma_or_newline;
    }

#endif

    /* Finish up when not 32 remaining. */
    bool in_string = carry > 0;
    while(!stop && parser->remaining.len > num_chars_proc)
    {
        switch(*next_char)
        {
            case '\n':
            {
                parser->row += 1;
                parser->col = 0;
                --next_value_len;
                stop = true;
            } break;

            case '"':
            {
                in_string = !in_string;
            } break;

            case ',':
            {
                if(!in_string) 
                {
                    parser->col += 1;
                    --next_value_len;
                    stop = true;
                }
            } break;

            default: break;
        }

        ++next_value_len;
        ++next_char;
        ++num_chars_proc;
    }

    parser->remaining.start = next_char;
    parser->remaining.len -= num_chars_proc;

    return (ElkCsvToken){ .row=row, .col=col, .value=(ElkStr){ .start=next_value_start, .len=next_value_len }};

ERR_RETURN:
    parser->error = true;
    return (ElkCsvToken){ .row=parser->row, .col=parser->col, .value=(ElkStr){.start=parser->remaining.start, .len=0}};
}

#if __AVX2__
static inline void
elk_csv_helper_load_new_buffer_aligned(ElkCsvParser *p, i8 skip_bytes)
{
    Assert((uptr)p->remaining.start % 32 == 0); /* Always aligned on a 32 byte boundary. */

    /* SIMD constants for expanding a bit as index mask to a byte mask. */
    __m256i S = _mm256_setr_epi64x(0x0000000000000000, 0x0101010101010101, 0x0202020202020202, 0x0303030303030303);
    __m256i M = _mm256_set1_epi64x(0x7fbfdfeff7fbfdfe);
    __m256i A = _mm256_set1_epi64x(0xffffffffffffffff);

    /* Constants for matching delimiters & quotes. */
    __m256i quotes = _mm256_set1_epi8('"');
    __m256i commas = _mm256_set1_epi8(',');
    __m256i newlines = _mm256_set1_epi8('\n');

    /* Load data into the buffer. */
    p->buf = _mm256_load_si256((__m256i *)(p->remaining.start));

    /* Zero out leading bytes if necessary so they don't accidently match a delimiter. */
    if(skip_bytes)
    {
        Assert(skip_bytes > 0 && skip_bytes < 32);
        switch(skip_bytes - 1)
        {
            case 30: p->buf = _mm256_insert_epi8(p->buf, 0, 30); /* fall through */
            case 29: p->buf = _mm256_insert_epi8(p->buf, 0, 29); /* fall through */
            case 28: p->buf = _mm256_insert_epi8(p->buf, 0, 28); /* fall through */
            case 27: p->buf = _mm256_insert_epi8(p->buf, 0, 27); /* fall through */
            case 26: p->buf = _mm256_insert_epi8(p->buf, 0, 26); /* fall through */
            case 25: p->buf = _mm256_insert_epi8(p->buf, 0, 25); /* fall through */
            case 24: p->buf = _mm256_insert_epi8(p->buf, 0, 24); /* fall through */
            case 23: p->buf = _mm256_insert_epi8(p->buf, 0, 23); /* fall through */
            case 22: p->buf = _mm256_insert_epi8(p->buf, 0, 22); /* fall through */
            case 21: p->buf = _mm256_insert_epi8(p->buf, 0, 21); /* fall through */
            case 20: p->buf = _mm256_insert_epi8(p->buf, 0, 20); /* fall through */
            case 19: p->buf = _mm256_insert_epi8(p->buf, 0, 19); /* fall through */
            case 18: p->buf = _mm256_insert_epi8(p->buf, 0, 18); /* fall through */
            case 17: p->buf = _mm256_insert_epi8(p->buf, 0, 17); /* fall through */
            case 16: p->buf = _mm256_insert_epi8(p->buf, 0, 16); /* fall through */
            case 15: p->buf = _mm256_insert_epi8(p->buf, 0, 15); /* fall through */
            case 14: p->buf = _mm256_insert_epi8(p->buf, 0, 14); /* fall through */
            case 13: p->buf = _mm256_insert_epi8(p->buf, 0, 13); /* fall through */
            case 12: p->buf = _mm256_insert_epi8(p->buf, 0, 12); /* fall through */
            case 11: p->buf = _mm256_insert_epi8(p->buf, 0, 11); /* fall through */
            case 10: p->buf = _mm256_insert_epi8(p->buf, 0, 10); /* fall through */
            case  9: p->buf = _mm256_insert_epi8(p->buf, 0,  9); /* fall through */
            case  8: p->buf = _mm256_insert_epi8(p->buf, 0,  8); /* fall through */
            case  7: p->buf = _mm256_insert_epi8(p->buf, 0,  7); /* fall through */
            case  6: p->buf = _mm256_insert_epi8(p->buf, 0,  6); /* fall through */
            case  5: p->buf = _mm256_insert_epi8(p->buf, 0,  5); /* fall through */
            case  4: p->buf = _mm256_insert_epi8(p->buf, 0,  4); /* fall through */
            case  3: p->buf = _mm256_insert_epi8(p->buf, 0,  3); /* fall through */
            case  2: p->buf = _mm256_insert_epi8(p->buf, 0,  2); /* fall through */
            case  1: p->buf = _mm256_insert_epi8(p->buf, 0,  2); /* fall through */
            case  0: p->buf = _mm256_insert_epi8(p->buf, 0,  0); /* fall through */
        }
    }

    /* If at the end of the string, no worries just force all trailing characters in the buffer to be delimiters. */
    if(p->remaining.len < 32)
    {
        size start = p->remaining.len;
        Assert(start > 0 && start < 32);
        switch(start)
        {
            case  1: p->buf = _mm256_insert_epi8(p->buf, '\n',  2); /* fall through */
            case  2: p->buf = _mm256_insert_epi8(p->buf, '\n',  2); /* fall through */
            case  3: p->buf = _mm256_insert_epi8(p->buf, '\n',  3); /* fall through */
            case  4: p->buf = _mm256_insert_epi8(p->buf, '\n',  4); /* fall through */
            case  5: p->buf = _mm256_insert_epi8(p->buf, '\n',  5); /* fall through */
            case  6: p->buf = _mm256_insert_epi8(p->buf, '\n',  6); /* fall through */
            case  7: p->buf = _mm256_insert_epi8(p->buf, '\n',  7); /* fall through */
            case  8: p->buf = _mm256_insert_epi8(p->buf, '\n',  8); /* fall through */
            case  9: p->buf = _mm256_insert_epi8(p->buf, '\n',  9); /* fall through */
            case 10: p->buf = _mm256_insert_epi8(p->buf, '\n', 10); /* fall through */
            case 11: p->buf = _mm256_insert_epi8(p->buf, '\n', 11); /* fall through */
            case 12: p->buf = _mm256_insert_epi8(p->buf, '\n', 12); /* fall through */
            case 13: p->buf = _mm256_insert_epi8(p->buf, '\n', 13); /* fall through */
            case 14: p->buf = _mm256_insert_epi8(p->buf, '\n', 14); /* fall through */
            case 15: p->buf = _mm256_insert_epi8(p->buf, '\n', 15); /* fall through */
            case 16: p->buf = _mm256_insert_epi8(p->buf, '\n', 16); /* fall through */
            case 17: p->buf = _mm256_insert_epi8(p->buf, '\n', 17); /* fall through */
            case 18: p->buf = _mm256_insert_epi8(p->buf, '\n', 18); /* fall through */
            case 19: p->buf = _mm256_insert_epi8(p->buf, '\n', 19); /* fall through */
            case 20: p->buf = _mm256_insert_epi8(p->buf, '\n', 20); /* fall through */
            case 21: p->buf = _mm256_insert_epi8(p->buf, '\n', 21); /* fall through */
            case 22: p->buf = _mm256_insert_epi8(p->buf, '\n', 22); /* fall through */
            case 23: p->buf = _mm256_insert_epi8(p->buf, '\n', 23); /* fall through */
            case 24: p->buf = _mm256_insert_epi8(p->buf, '\n', 24); /* fall through */
            case 25: p->buf = _mm256_insert_epi8(p->buf, '\n', 25); /* fall through */
            case 26: p->buf = _mm256_insert_epi8(p->buf, '\n', 26); /* fall through */
            case 27: p->buf = _mm256_insert_epi8(p->buf, '\n', 27); /* fall through */
            case 28: p->buf = _mm256_insert_epi8(p->buf, '\n', 28); /* fall through */
            case 29: p->buf = _mm256_insert_epi8(p->buf, '\n', 29); /* fall through */
            case 30: p->buf = _mm256_insert_epi8(p->buf, '\n', 30); /* fall through */
            case 31: p->buf = _mm256_insert_epi8(p->buf, '\n', 31); /* fall through */
        }
    }

    __m256i quote_mask = _mm256_cmpeq_epi8(p->buf, quotes);
    __m256i comma_mask = _mm256_cmpeq_epi8(p->buf, commas);
    __m256i newline_mask = _mm256_cmpeq_epi8(p->buf, newlines);

    /* https://nullprogram.com/blog/2021/12/04/ - public domain code to create running mask */
    u32 running_quote_mask = _mm256_movemask_epi8(quote_mask);
    u32 r = running_quote_mask;

    while(running_quote_mask)
    {
        r ^= -running_quote_mask ^ running_quote_mask;
        running_quote_mask &= running_quote_mask - 1;
    }

    running_quote_mask = r ^ p->carry;
    p->carry = -(running_quote_mask >> 31);
    quote_mask = _mm256_set1_epi32(running_quote_mask);
    quote_mask = _mm256_shuffle_epi8(quote_mask, S);
    quote_mask = _mm256_or_si256(quote_mask, M);
    quote_mask = _mm256_cmpeq_epi8(quote_mask, A);

    comma_mask = _mm256_andnot_si256(quote_mask, comma_mask);
    p->buf_comma_bits = _mm256_movemask_epi8(comma_mask);
    newline_mask = _mm256_andnot_si256(quote_mask, newline_mask);
    p->buf_newline_bits = _mm256_movemask_epi8(newline_mask);

    __m256i comma_or_newline_mask = _mm256_or_si256(comma_mask, newline_mask);
    p->buf_any_delimiter_bits = _mm256_movemask_epi8(comma_or_newline_mask);
    p->remaining.start += skip_bytes;
    p->byte_pos = skip_bytes;
}

static inline ElkCsvToken 
elk_csv_fast_next_token(ElkCsvParser *parser)
{
    size row = parser->row;
    size col = parser->col;

    StopIf(elk_csv_finished(parser), goto ERR_RETURN);

    char *start = parser->remaining.start;
    size next_value_len = 0;
    bool stop = false;
    
    /* Stop will signal when a new delimiter is found, but we may need to process a few buffers worth of data to find one. */
    while(!stop)
    {
        u32 any_delim = parser->buf_any_delimiter_bits;

        /* Find the position of the next delimiter or the end of the buffer */
        u32 first_non_zero_bit_lsb = any_delim & ~(any_delim - 1);
        i32 bit_pos = 31 - __lzcnt32(first_non_zero_bit_lsb);
        bit_pos = bit_pos > 31 || bit_pos < 0 ? 31 : bit_pos;
        i32 run_len = bit_pos - parser->byte_pos;

        /* Detect type of delimiter, or maybe no delimiter and end of buffer. */
        bool comma = (parser->buf_comma_bits >> bit_pos) & 1;
        bool newline = (parser->buf_newline_bits >> bit_pos) & 1;
        bool comma_or_newline = (any_delim >> bit_pos) & 1;

        /* Turn off that bit so we don't find it again! This delimiter has been processed. */
        parser->buf_comma_bits &= ~first_non_zero_bit_lsb;
        parser->buf_newline_bits &= ~first_non_zero_bit_lsb;
        parser->buf_any_delimiter_bits &= ~first_non_zero_bit_lsb;

        /* Update parser state based on position of next delimiter, or end of buffer. */
        parser->row += newline;
        parser->col += -col * newline + comma;
        next_value_len += run_len + 1 - comma_or_newline;
        parser->remaining.start += run_len + 1;
        parser->remaining.len -= run_len + 1;
        parser->byte_pos = bit_pos + 1;

        /* Signal need to stop if we found a delimiter */
        stop = comma_or_newline;

        /* If we finished the buffer and need to scan the next one, load a new buffer! */
        if(bit_pos + 1 > 31)
        {
            elk_csv_helper_load_new_buffer_aligned(parser, 0);
        }
    }

    return (ElkCsvToken){ .row=row, .col=col, .value=(ElkStr){.start=start, .len=next_value_len}};

ERR_RETURN:
    parser->error = true;
    return (ElkCsvToken){ .row=row, .col=col, .value=(ElkStr){.start=parser->remaining.start, .len=0}};
}

#else

static inline ElkCsvToken 
elk_csv_fast_next_token(ElkCsvParser *parser)
{
    return elk_csv_full_next_token(parser);
}

#endif


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
    size len = 0;
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

static inline ElkStr 
elk_csv_simple_unquote_str(ElkStr str)
{
    // Handle string that isn't even quoted!
    if(str.len < 2 || str.start[0] != '"') { return str; }

    return (ElkStr){ .start = str.start + 1, .len = str.len - 2};
}

#ifdef _COYOTE_H_

static inline ElkStaticArena 
elk_static_arena_allocate_and_create(size num_bytes)
{
    ElkStaticArena arena = {0};
    CoyMemoryBlock mem = coy_memory_allocate(num_bytes);

    if(mem.valid)
    {
        elk_static_arena_create(&arena, mem.size, mem.mem);
    }

    return arena;
}

static inline void 
elk_static_arena_destroy_and_deallocate(ElkStaticArena *arena)
{
    CoyMemoryBlock mem = { .mem = arena->buffer, .size = arena->buf_size, .valid = true };
    coy_memory_free(&mem);
    arena->buffer = NULL;
    arena->buf_size = 0;
    return;
}

static inline size 
elk_file_slurp(char const *filename, byte **out, ElkStaticArena *arena)
{
    size fsize = coy_file_size(filename);
    StopIf(fsize < 0, goto ERR_RETURN);

    *out = elk_static_arena_nmalloc(arena, fsize, byte);
    StopIf(!*out, goto ERR_RETURN);

    size size_read = coy_file_slurp(filename, fsize, *out);
    StopIf(fsize != size_read, goto ERR_RETURN);

    return fsize;

ERR_RETURN:
    *out = NULL;
    return -1;
}

static inline ElkStr 
elk_file_slurp_text(char const *filename, ElkStaticArena *arena)
{

    size fsize = coy_file_size(filename);
    StopIf(fsize < 0, goto ERR_RETURN);

    byte *out = elk_static_arena_nmalloc(arena, fsize, byte);
    StopIf(!out, goto ERR_RETURN);

    size size_read = coy_file_slurp(filename, fsize, out);
    StopIf(fsize != size_read, goto ERR_RETURN);

    return (ElkStr){ .start = out, .len = fsize };

ERR_RETURN:
    return (ElkStr){ .start = NULL, .len = 0 };
}

#endif // Coyote

#endif
