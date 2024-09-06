#ifndef _ELK_HEADER_
#define _ELK_HEADER_

/* Change some warning settings for MSVC. This code is well tested and we use some "tricks" for performance, and I don't
 * want to be bothered in other projects by those warnings.
 */

#pragma warning(push)
#pragma warning(disable:4146) /* Applying '-' unary operator to unsigned. Someimes we bit fiddle! */

#include <stdint.h>
#include <stddef.h>

#include <immintrin.h>

#if defined(_WIN64) || defined(_WIN32)
#define __lzcnt32(a) __lzcnt(a)
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 * TODO: Things I'd like to add.
 *-------------------------------------------------------------------------------------------------------------------------*/
// TODO: Substring Search.

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Define simpler types
 *-------------------------------------------------------------------------------------------------------------------------*/

/* Other libraries I may have already included may use these exact definitions too. */
#ifndef _TYPE_ALIASES_
#define _TYPE_ALIASES_

typedef int32_t     b32;
#ifndef false
   #define false 0
   #define true  1
#endif
	
#if !defined(_WINDOWS_) && !defined(_INC_WINDOWS)
typedef char       byte;
#endif

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
    #define Assert(assertion) (void)(assertion)
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
typedef i64 ElkTimeDiff;

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
static inline b32 elk_is_leap_year(int year);
static inline ElkTime elk_make_time(ElkStructTime tm); /* Ignores the day_of_year member. */
static inline ElkTime elk_time_truncate_to_hour(ElkTime time);
static inline ElkTime elk_time_truncate_to_specific_hour(ElkTime time, int hour);
static inline ElkTime elk_time_add(ElkTime time, ElkTimeDiff change_in_time);
static inline ElkTimeDiff elk_time_difference(ElkTime a, ElkTime b); /* a - b */

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
    char *start;  // points at first character in the string
    size len;     // the length of the string (not including a null terminator if it's there)
} ElkStr;

typedef struct
{
    ElkStr left;
    ElkStr right;
} ElkStrSplitPair;

static inline ElkStr elk_str_from_cstring(char *src);
static inline ElkStr elk_str_copy(size dst_len, char *restrict dest, ElkStr src);
static inline ElkStr elk_str_strip(ElkStr input);                           // Strips leading and trailing whitespace
static inline ElkStr elk_str_substr(ElkStr str, size start, size len);      // Create a substring from a longer string
static inline i32 elk_str_cmp(ElkStr left, ElkStr right);                   // 0 if equal, -1 if left is first, 1 otherwise
static inline b32 elk_str_eq(ElkStr const left, ElkStr const right);        // Faster than elk_str_cmp, checks length first
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
static inline b32 elk_str_parse_i64(ElkStr str, i64 *result);
static inline b32 elk_str_robust_parse_f64(ElkStr str, f64 *out);
static inline b32 elk_str_fast_parse_f64(ElkStr str, f64 *out);
static inline b32 elk_str_parse_datetime(ElkStr str, ElkTime *out);

#define elk_str_parse_elk_time(str, result) elk_str_parse_i64((str), (result))

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
    b32 over_allocation_attempted;
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
static inline void * elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, size asize); // ret NULL if ptr is not most recent allocation
static inline void elk_static_arena_free(ElkStaticArena *arena, void *ptr); // Undo if it was last allocation, otherwise no-op

#define elk_static_arena_malloc(arena, type) (type *)elk_static_arena_alloc((arena), sizeof(type), _Alignof(type))
#define elk_static_arena_nmalloc(arena, count, type) (type *)elk_static_arena_alloc((arena), (count) * sizeof(type), _Alignof(type))
#define elk_static_arena_nrealloc(arena, ptr, count, type) (type *) elk_static_arena_realloc((arena), (ptr), sizeof(type) * (count))

#ifdef _ELK_TRACK_MEM_USAGE
static ElkStaticArenaAllocationMetrics elk_static_arena_metrics[32] = {0};
static size elk_static_arena_metrics_next = 0;
static inline f64 elk_static_arena_max_ratio(ElkStaticArena *arena);
static inline b32 elk_static_arena_over_allocated(ElkStaticArena *arena);
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
    void *free;          // The head of a free list of available slots for objects
    byte *buffer;        // The buffer we actually store the data in
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
 * If you're passing user supplied data to this, it's possible that a nefarious actor could send data specifically designed
 * to jam up one of these functions. So don't use them to write a web browser or server, or banking software. They should be
 * good and fast though for data munging.
 *
 * To build a custom "fnv1a" hash function just follow the example of the implementation of elk_fnv1a_hash below. For the 
 * first member of a struct, call the accumulate function with the fnv_offset_bias, then call the accumulate function for
 * each remaining member of the struct. This will leave the padding out of the calculation, which is good because we cannot 
 * guarantee what is in the padding.
 */
typedef b32 (*ElkEqFunction)(void const *left, void const *right);

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
    i8 size_exp;                      // Used to keep track of the length of *handles
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
 *  instance of a bookkeeping type (e.g. ElkQueueLedger) to track the state of all the arrays that back it. Further,
 *  different collections in the parallel collections can have different sized objects.
 *
 *  Complicated memory management can be a disadvantage of the ledger approach. For instance, implementing growable
 *  collections can require shuffling objects around in the backing buffer during the resize operation. A queue, for 
 *  instance, is non-trivial to expand because you have to account for wrap-around (assuming it is backed by a circular
 *  buffer). So not all the ledger types APIs will directly support resizing, and even those that do will further require 
 *  the user to make sure that as they allocate more memory in the backing buffer, they also update the ledger. Finally, if
 *  you want to pass the collection as a whole to a function, you'll have to pass the ledger and buffer(s) separately or
 *  create your own composite type to package them together in.
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
static inline b32 elk_queue_ledger_full(ElkQueueLedger *queue);
static inline b32 elk_queue_ledger_empty(ElkQueueLedger *queue);
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
static inline b32 elk_array_ledger_full(ElkArrayLedger *array);
static inline b32 elk_array_ledger_empty(ElkArrayLedger *array);
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
typedef struct
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
 *                                                        Sorting
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       Radix Sort 
 *---------------------------------------------------------------------------------------------------------------------------
 * I've only implemented radix sort for fixed size signed intengers, unsigned integers, and floating point types thus far.
 *
 * The sort requires extra memory that depends on the size of the list being sorted, so the user must provide that. In order
 * to ensure proper alignment and size, the user must provide the scratch buffer. Probably using an arena with 
 * elk_static_arena_nmalloc() to create a buffer and then freeing it after the sort would be the most efficient way to handle
 * that.
 *
 * The API is set up for sorting an array of structures. The 'offset' argument is the offset in bytes into the structure 
 * where the sort key can be found. The 'stride' argument is just the size of the structures so we know how to iterate the
 * array. The sort order (ascending / descending) and sort key type (signed integer, unsigned integer, floating) are passed
 * as arguments. The sort key type includes the size of the sort key in bits.
 */

typedef enum
{
    ELK_RADIX_SORT_UINT8,
    ELK_RADIX_SORT_INT8,
    ELK_RADIX_SORT_UINT16,
    ELK_RADIX_SORT_INT16,
    ELK_RADIX_SORT_UINT32,
    ELK_RADIX_SORT_INT32,
    ELK_RADIX_SORT_F32,
    ELK_RADIX_SORT_UINT64,
    ELK_RADIX_SORT_INT64,
    ELK_RADIX_SORT_F64,
} ElkRadixSortByType;

typedef enum { ELK_SORT_ASCENDING, ELK_SORT_DESCENDING } ElkSortOrder;

static inline void elk_radix_sort(
        void *buffer, 
        size num, 
        size offset, 
        size stride, 
        void *scratch, 
        ElkRadixSortByType sort_type, 
        ElkSortOrder order);

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
 * Parsing a string may modify it, especially if it uses quotes a lot. The only function in this API that modifies the 
 * the string in place is the elk_csv_unquote_str() function. So, this function cannot be used if you're parsing a string 
 * stored in read only memory, which is probably the case if parsing a memory mapped file. None of the other functions in 
 * this API modify the string they are parsing. However, you (the user) may modify strings (memory) returned by the 
 * elk_csv_*_next_token() functions. So be very careful using this API if you're parsing read only memory.
 *
 * The CSV format I handle is pretty simple. Anything goes inside quoted strings. Quotes in a quoted string have to be 
 * escaped by another quote, e.g. "Frank ""The Tank"" Johnson". Comment lines are allowed, but they must be full lines, no
 * end of line comments, and they must start with a '#' character.
 *
 * Comments in CSV are rare, and can really slow down the parser. But I do find comments useful when I use CSV for some
 * smaller configuration files. So I've implemented two CSV parser functions. The first is the "full" version, and it can
 * handle comments anywhere. The second is the "fast" parser, which can handle comment lines at the beginning of the file, 
 * but once it gets past there it is more agressively optimized by assuming no more comment lines. This allows for a much
 * faster parser for large CSV files without interspersed comments.
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
    b32 error;              // Have we encountered an error while parsing?
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
static inline b32 elk_csv_finished(ElkCsvParser *parser);
static inline ElkStr elk_csv_unquote_str(ElkStr str, ElkStr const buffer);
static inline ElkStr elk_csv_simple_unquote_str(ElkStr str);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                    Random Numbers
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Uniform Random Deviates
 *-------------------------------------------------------------------------------------------------------------------------*/

/* This random number generator is a counter based generator.
 *
 * Different versions can be used in different threads with keys or if each thread increments by the counter by the number
 * of threads they can use the same key.
 *
 * Philox and Threefish were some of the original design ideas for the counter based PRNGs, but this one is based on an
 * algorithm by Bernard Widynski. The paper for the algorithm can be found at https://arxiv.org/abs/2004.06278.
 */
typedef struct
{
    u64 key;
    u64 counter;
} ElkRandomState;

static inline ElkRandomState elk_random_state_create(u64 seed);
static inline u64 elk_random_state_uniform_u64(ElkRandomState *state); /* Includes the whole range of u64. */
static inline f64 elk_random_state_uniform_f64(ElkRandomState *state); /* Random deviate in 0.-1.          */

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                         Math
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/* Kahan summation for more accurately adding floating point numbers.
 *
 * This accumulator is best just zero initialized.
 */
typedef struct
{
    f64 sum;
    f64 err;
} ElkKahanAccumulator;

static inline ElkKahanAccumulator elk_kahan_accumulator_add(ElkKahanAccumulator acc, f64 value);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                     Coyote Add Ons
 *
 *
 *---------------------------------------------------------------------------------------------------------------------------
 * These are ONLY enabled if the coyote library has also been included. It must be included before this library.
 *
 */
#ifdef _COYOTE_H_
static inline ElkStaticArena elk_static_arena_allocate_and_create(size num_bytes); /* Allocates using Coyote.   */
static inline void elk_static_arena_destroy_and_deallocate(ElkStaticArena *arena); /* Frees memory with Coyote. */

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

static inline b32
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
elk_time_add(ElkTime time, ElkTimeDiff change_in_time)
{
    ElkTime result = time + change_in_time;
    Assert(result >= 0);
    return result;
}

static inline ElkTimeDiff
elk_time_difference(ElkTime a, ElkTime b)
{
    return a - b;
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
    int const second = (int const)(time % SECONDS_PER_MINUTE);
    time = (time - second) / SECONDS_PER_MINUTE;
    Assert(time >= 0 && second >= 0 && second <= 59);

    // Get the minutes part, trim it off and convert to hours.
    int const minute = (int const)(time % MINUTES_PER_HOUR);
    time = (time - minute) / MINUTES_PER_HOUR;
    Assert(time >= 0 && minute >= 0 && minute <= 59);

    // Get the hours part, trim it off and convert to days.
    int const hour = (int const)(time % HOURS_PER_DAY);
    time = (time - hour) / HOURS_PER_DAY;
    Assert(time >= 0 && hour >= 0 && hour <= 23);

    // Rename variable for clarity
    i64 const days_since_epoch = time;

    // Calculate the year
    int year = (int)(days_since_epoch / (DAYS_PER_YEAR) + 1); // High estimate, but good starting point.
    i64 test_time = elk_days_since_epoch(year);
    while (test_time > days_since_epoch) 
    {
        int step = (int)((test_time - days_since_epoch) / (DAYS_PER_YEAR + 1));
        step = step == 0 ? 1 : step;
        year -= step;
        test_time = elk_days_since_epoch(year);
    }
    Assert(test_time <= elk_days_since_epoch(year));
    time -= elk_days_since_epoch(year); // Now it's days since start of the year.
    Assert(time >= 0);
    i16 day_of_year = (i16)(time + 1);

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
    int const day = (int const)(time + 1);
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
    size const copy_len = src.len < dst_len ? src.len : dst_len;
    memcpy(dest, src.start, copy_len);

    // Add a terminating zero IFF we can.
    if(copy_len < dst_len) { dest[copy_len] = '\0'; }

    return (ElkStr){.start = dest, .len = copy_len};
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

static inline i32
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

static inline b32
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

static inline b32 
elk_str_helper_parse_i64(ElkStr str, i64 *result)
{
    u64 parsed = 0;
    b32 neg_flag = false;
    b32 in_digits = false;
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

static inline b32
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

#pragma warning(disable : 4723)
static inline b32
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
    i16 extra_exp = 0;  // decimal places after the point

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
#pragma warning(default : 4723)

#pragma warning(disable : 4723)
static inline b32 
elk_str_fast_parse_f64(ElkStr str, f64 *out)
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
    i16 extra_exp = 0;  // decimal places after the point

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
#pragma warning(default : 4723)

static inline b32
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
            *out = elk_time_from_ymd_and_hms((i16)year, (i8)month, (i8)day, (i8)hour, (i8)minutes, (i8)seconds);
            return true;
        }
    }

    return false;
}

static inline b32
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
        *out = elk_time_from_yd_and_hms((i16)year, (i8)day_of_year, (i8)hour, (i8)minutes, (i8)seconds);
        return true;
    }

    return false;
}

static inline b32
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

    usize const handles_len = (usize)(1 << size_exp);
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

static inline b32
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
    u32 step = (u32)((hash >> (64 - exp)) | 1);    // the | 1 forces an odd number
    return (idx + step) & mask;
}

static inline void
elk_string_interner_expand_table(ElkStringInterner *interner)
{
    i8 const size_exp = interner->size_exp;
    i8 const new_size_exp = size_exp + 1;

    usize const handles_len = (usize)(1 << size_exp);
    usize const new_handles_len = (usize)(1 << new_size_exp);

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

static inline b32
elk_is_power_of_2(uptr p)
{
    return (p & (p - 1)) == 0;
}

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
    if ((size)(offset + num_bytes) <= arena->buf_size)
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
elk_static_arena_realloc(ElkStaticArena *arena, void *ptr, size asize)
{
    Assert(asize > 0);

    if(ptr == arena->prev_ptr)
    {
        // Get previous extra offset due to alignment
        uptr offset = (uptr)ptr - (uptr)arena->buffer; // relative offset accounting for alignment

        // Check to see if there is enough space left
        if ((size)(offset + asize) <= arena->buf_size)
        {
            arena->buf_offset = offset + asize;

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

static inline b32 
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

static inline b32 
elk_queue_ledger_full(ElkQueueLedger *queue)
{ 
    return queue->length == queue->capacity;
}

static inline b32
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

static inline b32 
elk_array_ledger_full(ElkArrayLedger *array)
{ 
    return array->length == array->capacity;
}

static inline b32
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

    size const handles_len = (size)(1 << size_exp);
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

    size const handles_len = (size)(1 << size_exp);
    size const new_handles_len = (size)(1 << new_size_exp);

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
    size const max_iter = (size)(1 << map->size_exp);
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

    size const handles_len = (size)(1 << size_exp);
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

    size const handles_len = (size)(1 << size_exp);
    size const new_handles_len = (size)(1 << new_size_exp);

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
            // found it! Replace value
            void *tmp = handle->value;
            handle->value = value;

            return tmp;
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
    size const max_iter = (size)(1 << map->size_exp);
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
    size const max_iter = (size)(1 << map->size_exp);
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

    size const handles_len = (size)(1 << size_exp);
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

    size const handles_len = (size)(1 << size_exp);
    size const new_handles_len = (size)(1 << new_size_exp);

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
    size const max_iter = (size)(1 << set->size_exp);
    void *next_value = NULL;
    if(*iter >= max_iter) { return next_value; }

    do
    {
        next_value = set->handles[*iter].value;
        *iter += 1;

    } while(next_value == NULL && *iter < max_iter);

    return next_value;
}

#define ELK_I8_FLIP(x) ((x) ^ UINT8_C(0x80))
#define ELK_I8_FLIP_BACK(x) ELK_I8_FLIP(x)

#define ELK_I16_FLIP(x) ((x) ^ UINT16_C(0x8000))
#define ELK_I16_FLIP_BACK(x) ELK_I16_FLIP(x)

#define ELK_I32_FLIP(x) ((x) ^ UINT32_C(0x80000000))
#define ELK_I32_FLIP_BACK(x) ELK_I32_FLIP(x)

#define ELK_I64_FLIP(x) ((x) ^ UINT64_C(0x8000000000000000))
#define ELK_I64_FLIP_BACK(x) ELK_I64_FLIP(x)

#define ELK_F32_FLIP(x) ((x) ^ (-(i32)(((u32)(x)) >> 31) | UINT32_C(0x80000000)))
#define ELK_F32_FLIP_BACK(x) ((x) ^ (((((u32)(x)) >> 31) - 1) | UINT32_C(0x80000000)))

#define ELK_F64_FLIP(x) ((x) ^ (-(i64)(((u64)(x)) >> 63) | UINT64_C(0x8000000000000000)))
#define ELK_F64_FLIP_BACK(x) ((x) ^ (((((u64)(x)) >> 63) - 1) | UINT64_C(0x8000000000000000)))

static inline void
elk_radix_pre_sort_transform(void *buffer, size num, size offset, size stride, ElkRadixSortByType sort_type)
{
    Assert(
           sort_type == ELK_RADIX_SORT_UINT64 || sort_type == ELK_RADIX_SORT_INT64 || sort_type == ELK_RADIX_SORT_F64
        || sort_type == ELK_RADIX_SORT_UINT32 || sort_type == ELK_RADIX_SORT_INT32 || sort_type == ELK_RADIX_SORT_F32
        || sort_type == ELK_RADIX_SORT_UINT16 || sort_type == ELK_RADIX_SORT_INT16
        || sort_type == ELK_RADIX_SORT_UINT8  || sort_type == ELK_RADIX_SORT_INT8
    );

    for(size i = 0; i < num; ++i)
    {
        switch(sort_type)
        {
            case ELK_RADIX_SORT_F64:
            {
                /* Flip bits so doubles sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F64_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT64:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I64_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_F32:
            {
                /* Flip bits so doubles sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F32_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT32:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I32_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT16:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u16 *v = (u16 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I16_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT8:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u8 *v = (u8 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I8_FLIP(*v);
            } break;

            /* Fall through without doing anything. */
            case ELK_RADIX_SORT_UINT64:
            case ELK_RADIX_SORT_UINT32:
            case ELK_RADIX_SORT_UINT16:
            case ELK_RADIX_SORT_UINT8: {}
        }
    }
}

static inline void
elk_radix_post_sort_transform(void *buffer, size num, size offset, size stride, ElkRadixSortByType sort_type)
{
    Assert(
           sort_type == ELK_RADIX_SORT_UINT64 || sort_type == ELK_RADIX_SORT_INT64 || sort_type == ELK_RADIX_SORT_F64
        || sort_type == ELK_RADIX_SORT_UINT32 || sort_type == ELK_RADIX_SORT_INT32 || sort_type == ELK_RADIX_SORT_F32
        || sort_type == ELK_RADIX_SORT_UINT16 || sort_type == ELK_RADIX_SORT_INT16
        || sort_type == ELK_RADIX_SORT_UINT8  || sort_type == ELK_RADIX_SORT_INT8
    );

    for(size i = 0; i < num; ++i)
    {
        switch(sort_type)
        {
            case ELK_RADIX_SORT_F64:
            {
                /* Flip bits so doubles sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F64_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT64:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I64_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_F32:
            {
                /* Flip bits so doubles sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F32_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT32:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I32_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT16:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u16 *v = (u16 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I16_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT8:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u8 *v = (u8 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I8_FLIP_BACK(*v);
            } break;

            /* Fall through with no-op */
            case ELK_RADIX_SORT_UINT64:
            case ELK_RADIX_SORT_UINT32:
            case ELK_RADIX_SORT_UINT16:
            case ELK_RADIX_SORT_UINT8: {}
        }
    }
}

static inline void
elk_radix_sort_8(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256] = {0};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    for(size i = 0; i < num; ++i)
    {
        u8 b0 = *(u8 *)position;
        counts[b0] += 1;
        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i] += counts[i - 1];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i] += counts[i + 1];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    for(size i = num - 1; i >= 0; --i)
    {
        void *val_src = (byte *)source + i * stride;
        u8 cnts_idx = *((u8 *)val_src + offset);
        void *val_dest = (byte *)dest + (--counts[cnts_idx]) * stride;

        memcpy(val_dest, val_src, stride);
    }

    /* Copy back to the original buffer. */
    memcpy(buffer, scratch, stride * num);
}

static inline void
elk_radix_sort_16(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256][2] = {0};
    i32 skips[2] = {1, 1};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    u16 val = *(u16 *)position;
    u8 b0 = UINT16_C(0xFF) & (val >>  0);
    u8 b1 = UINT16_C(0xFF) & (val >>  8);

    counts[b0][0] += 1;
    counts[b1][1] += 1;

    position += stride;

    u8 initial_values[2] = {b0, b1};

    for(size i = 1; i < num; ++i)
    {
        val = *(u16 *)position;
        b0 = UINT16_C(0xFF) & (val >>  0);
        b1 = UINT16_C(0xFF) & (val >>  8);

        counts[b0][0] += 1;
        counts[b1][1] += 1;

        skips[0] &= initial_values[0] == b0;
        skips[1] &= initial_values[1] == b1;

        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i][0] += counts[i - 1][0];
            counts[i][1] += counts[i - 1][1];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i][0] += counts[i + 1][0];
            counts[i][1] += counts[i + 1][1];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    size num_skips = 0;
    for(size b = 0; b < 2; ++b)
    {
        if(!skips[b])
        {
            for(size i = num - 1; i >= 0; --i)
            {
                void *val_src = (byte *)source + i * stride;
                u16 val = *(u16 *)((byte *)val_src + offset);
                u8 cnts_idx = UINT16_C(0xFF) & (val >> (b * 8));
                void *val_dest = (byte *)dest + (--counts[cnts_idx][b]) * stride;

                memcpy(val_dest, val_src, stride);
            }

            /* Flip the source & destination buffers. */
            dest = dest == scratch ? buffer : scratch;
            source = source == scratch ? buffer : scratch;
        }
        else
        {
            num_skips++;
        }
    }

    /* Swap back into the original buffer. */
    if(num_skips % 2)
    {
        memcpy(buffer, scratch, num * stride);
    }
}

static inline void
elk_radix_sort_32(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256][4] = {0};
    i32 skips[4] = {1, 1, 1, 1};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    u32 val = *(u32 *)position;
    u8 b0 = UINT32_C(0xFF) & (val >>  0);
    u8 b1 = UINT32_C(0xFF) & (val >>  8);
    u8 b2 = UINT32_C(0xFF) & (val >> 16);
    u8 b3 = UINT32_C(0xFF) & (val >> 24);

    counts[b0][0] += 1;
    counts[b1][1] += 1;
    counts[b2][2] += 1;
    counts[b3][3] += 1;

    position += stride;

    u8 initial_values[4] = {b0, b1, b2, b3};

    for(size i = 1; i < num; ++i)
    {
        val = *(u32 *)position;
        b0 = UINT32_C(0xFF) & (val >>  0);
        b1 = UINT32_C(0xFF) & (val >>  8);
        b2 = UINT32_C(0xFF) & (val >> 16);
        b3 = UINT32_C(0xFF) & (val >> 24);

        counts[b0][0] += 1;
        counts[b1][1] += 1;
        counts[b2][2] += 1;
        counts[b3][3] += 1;

        skips[0] &= initial_values[0] == b0;
        skips[1] &= initial_values[1] == b1;
        skips[2] &= initial_values[2] == b2;
        skips[3] &= initial_values[3] == b3;

        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i][0] += counts[i - 1][0];
            counts[i][1] += counts[i - 1][1];
            counts[i][2] += counts[i - 1][2];
            counts[i][3] += counts[i - 1][3];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i][0] += counts[i + 1][0];
            counts[i][1] += counts[i + 1][1];
            counts[i][2] += counts[i + 1][2];
            counts[i][3] += counts[i + 1][3];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    size num_skips = 0;
    for(size b = 0; b < 4; ++b)
    {
        if(!skips[b])
        {
            for(size i = num - 1; i >= 0; --i)
            {
                void *val_src = (byte *)source + i * stride;
                u32 val = *(u32 *)((byte *)val_src + offset);
                u8 cnts_idx = UINT32_C(0xFF) & (val >> (b * 8));
                void *val_dest = (byte *)dest + (--counts[cnts_idx][b]) * stride;

                memcpy(val_dest, val_src, stride);
            }

            /* Flip the source & destination buffers. */
            dest = dest == scratch ? buffer : scratch;
            source = source == scratch ? buffer : scratch;
        }
        else
        {
            num_skips++;
        }
    }

    /* Swap back into the original buffer. */
    if(num_skips % 2)
    {
        memcpy(buffer, scratch, num * stride);
    }
}

static inline void
elk_radix_sort_64(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256][8] = {0};
    i32 skips[8] = {1, 1, 1, 1, 1, 1, 1, 1};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    u64 val = *(u64 *)position;
    u8 b0 = UINT64_C(0xFF) & (val >>  0);
    u8 b1 = UINT64_C(0xFF) & (val >>  8);
    u8 b2 = UINT64_C(0xFF) & (val >> 16);
    u8 b3 = UINT64_C(0xFF) & (val >> 24);
    u8 b4 = UINT64_C(0xFF) & (val >> 32);
    u8 b5 = UINT64_C(0xFF) & (val >> 40);
    u8 b6 = UINT64_C(0xFF) & (val >> 48);
    u8 b7 = UINT64_C(0xFF) & (val >> 56);

    counts[b0][0] += 1;
    counts[b1][1] += 1;
    counts[b2][2] += 1;
    counts[b3][3] += 1;
    counts[b4][4] += 1;
    counts[b5][5] += 1;
    counts[b6][6] += 1;
    counts[b7][7] += 1;

    position += stride;

    u8 initial_values[8] = {b0, b1, b2, b3, b4, b5, b6, b7};

    for(size i = 1; i < num; ++i)
    {
        val = *(u64 *)position;
        b0 = UINT64_C(0xFF) & (val >>  0);
        b1 = UINT64_C(0xFF) & (val >>  8);
        b2 = UINT64_C(0xFF) & (val >> 16);
        b3 = UINT64_C(0xFF) & (val >> 24);
        b4 = UINT64_C(0xFF) & (val >> 32);
        b5 = UINT64_C(0xFF) & (val >> 40);
        b6 = UINT64_C(0xFF) & (val >> 48);
        b7 = UINT64_C(0xFF) & (val >> 56);

        counts[b0][0] += 1;
        counts[b1][1] += 1;
        counts[b2][2] += 1;
        counts[b3][3] += 1;
        counts[b4][4] += 1;
        counts[b5][5] += 1;
        counts[b6][6] += 1;
        counts[b7][7] += 1;

        skips[0] &= initial_values[0] == b0;
        skips[1] &= initial_values[1] == b1;
        skips[2] &= initial_values[2] == b2;
        skips[3] &= initial_values[3] == b3;
        skips[4] &= initial_values[4] == b4;
        skips[5] &= initial_values[5] == b5;
        skips[6] &= initial_values[6] == b6;
        skips[7] &= initial_values[7] == b7;

        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i][0] += counts[i - 1][0];
            counts[i][1] += counts[i - 1][1];
            counts[i][2] += counts[i - 1][2];
            counts[i][3] += counts[i - 1][3];
            counts[i][4] += counts[i - 1][4];
            counts[i][5] += counts[i - 1][5];
            counts[i][6] += counts[i - 1][6];
            counts[i][7] += counts[i - 1][7];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i][0] += counts[i + 1][0];
            counts[i][1] += counts[i + 1][1];
            counts[i][2] += counts[i + 1][2];
            counts[i][3] += counts[i + 1][3];
            counts[i][4] += counts[i + 1][4];
            counts[i][5] += counts[i + 1][5];
            counts[i][6] += counts[i + 1][6];
            counts[i][7] += counts[i + 1][7];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    size num_skips = 0;
    for(size b = 0; b < 8; ++b)
    {
        if(!skips[b])
        {
            for(size i = num - 1; i >= 0; --i)
            {
                void *val_src = (byte *)source + i * stride;
                u64 val = *(u64 *)((byte *)val_src + offset);
                u8 cnts_idx = UINT64_C(0xFF) & (val >> (b * 8));
                void *val_dest = (byte *)dest + (--counts[cnts_idx][b]) * stride;

                memcpy(val_dest, val_src, stride);
            }

            /* Flip the source & destination buffers. */
            dest = dest == scratch ? buffer : scratch;
            source = source == scratch ? buffer : scratch;
        }
        else
        {
            num_skips++;
        }
    }

    /* Swap back into the original buffer. */
    if(num_skips % 2)
    {
        memcpy(buffer, scratch, num * stride);
    }
}

static inline void
elk_radix_sort(void *buffer, size num, size offset, size stride, void *scratch, ElkRadixSortByType sort_type, ElkSortOrder order)
{
    elk_radix_pre_sort_transform(buffer, num, offset, stride, sort_type);

    switch(sort_type)
    {
        case ELK_RADIX_SORT_UINT64:
        case ELK_RADIX_SORT_INT64:
        case ELK_RADIX_SORT_F64:
        {
            elk_radix_sort_64(buffer, num, offset, stride, scratch, order);
        } break;

        case ELK_RADIX_SORT_UINT32:
        case ELK_RADIX_SORT_INT32:
        case ELK_RADIX_SORT_F32:
        {
            elk_radix_sort_32(buffer, num, offset, stride, scratch, order);
        } break;

        case ELK_RADIX_SORT_UINT16:
        case ELK_RADIX_SORT_INT16:
        {
            elk_radix_sort_16(buffer, num, offset, stride, scratch, order);
        } break;

        case ELK_RADIX_SORT_UINT8:
        case ELK_RADIX_SORT_INT8:
        {
            elk_radix_sort_8(buffer, num, offset, stride, scratch, order);
        } break;

        default: Panic();
    }
    
    elk_radix_post_sort_transform(buffer, num, offset, stride, sort_type);
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
    i8 skip_bytes = (i8)((uptr)parser.remaining.start - ((uptr)parser.remaining.start & ~0x1F));
    parser.remaining.start = (char *)((uptr)parser.remaining.start & ~0x1F); /* Force 32 byte alignment */
    parser.carry = 0;
    elk_csv_helper_load_new_buffer_aligned(&parser, skip_bytes);
#endif

    return parser;
}

static inline b32 
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
    b32 stop = false;
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

        b32 comma = (comma_bits >> bit_pos) & 1;
        b32 newline = (newline_bits >> bit_pos) & 1;
        b32 comma_or_newline = (comma_or_newline_bits >> bit_pos) & 1;

        parser->row += newline;
        parser->col += -col * newline + comma;
        next_value_len += bit_pos + 1 - comma_or_newline;

        next_char += bit_pos + 1 ;
        num_chars_proc += bit_pos + 1 ;

        stop = comma_or_newline;
    }

#endif

    /* Finish up when not 32 remaining. */
    b32 in_string = carry > 0;
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
    b32 stop = false;
    
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
        b32 comma = (parser->buf_comma_bits >> bit_pos) & 1;
        b32 newline = (parser->buf_newline_bits >> bit_pos) & 1;
        b32 comma_or_newline = (any_delim >> bit_pos) & 1;

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
            if(parser->remaining.len > 0)
            {
                elk_csv_helper_load_new_buffer_aligned(parser, 0);
            }
            else
            {
                stop = true;
            }
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
elk_csv_unquote_str(ElkStr str, ElkStr const buffer)
{
    // remove any leading and trailing white space
    str = elk_str_strip(str);

    if(str.len >= 2 && str.start[0] == '"')
    {
        // Ok, now we've got a quoted non-empty string.
        int next_read = 1;
        int next_write = 0;
        size len = 0;

        while(next_read < str.len && next_write < buffer.len)
        {
            if(str.start[next_read] == '"')
            {
                next_read++;
                if(next_read >= str.len - 2) { break; }
            }

            buffer.start[next_write] = str.start[next_read];

            len += 1;
            next_read += 1;
            next_write += 1;
        }

        return (ElkStr){ .start=buffer.start, .len=len};
    }

    return str;
}

static inline ElkStr 
elk_csv_simple_unquote_str(ElkStr str)
{
    // Handle string that isn't even quoted!
    if(str.len < 2 || str.start[0] != '"') { return str; }

    return (ElkStr){ .start = str.start + 1, .len = str.len - 2};
}

static inline ElkRandomState
elk_random_state_create(u64 seed)
{
    static u64 const keys[] = {
        0x4176239dc291f5d7, 0x21346bafefa61745, 0xa76d41f32f9835b1, 0xdc592b643d8a542f, 0xc36b27a54c7d528b, 0x4e9b8a165a8e61f7,
        0xdf318b5cba829f65, 0x3e81d95cb6759ec1, 0xa29437ced467ac3f, 0xfbe467c1f258dc9b, 0xe7fac4b3236deb19, 0xf56e34a763921b85,
        0x8b29acf76f7539e1, 0xd594271a9d67385f, 0xe6d1fa898d5946cb, 0x5978c1bdcb5d7639, 0xd1a365bed96f84a5, 0x3c9b7ae1f984b513,
        0xab8467521865b27f, 0x3d75c6a43457c1eb, 0x58e3b6c7625adf49, 0x3e6b2f78714cfeb5, 0x4fc6d12baf714e23, 0x34e5cafbad543c8f,
        0x87b24c9edc364afb, 0xa937fde1fc3a5b69, 0x95a48e621a3c79d5, 0xf8e51293284fa953, 0x83ea95b65742b6af, 0xf315872a9745c62d,
        0x61f358bcb237e489, 0x6d8e935cb14ae3f5, 0xc853e46dcf5e1473, 0x4c72d5efed5142cf, 0xae5b97c32e35614d, 0x1fe76ca43c265fa9,
        0x312fabc65b3a8e17, 0x8d46ce27672c9d83, 0x67fabc58752e8adf, 0x275ea63cb632ca5d, 0xc745362ed324e8c9, 0xea91283fe128f937,
        0x912af356524c18a3, 0x3fe129a76f3e4821, 0x71e4f6598e21457d, 0x1cf42da98c1364e9, 0x83f1b5acbc189357, 0xc417b3aed81a92c3,
        0xc79bd4afe83db241, 0xbc28639dc41eaf8d, 0x32bec8143512cdf9, 0x52738ab65314fd67, 0xbd159af874281cd3, 0x162d4be87efa3c51,
        0xf35cb4876bfc39ad, 0x3adf17898aef592b, 0x8bd569fedbf26897, 0x1b76c35ed8e597f3, 0x84f56db328f9b671, 0x5cad62e325f9a4cd,
        0x45b92d1765fed34b, 0xd5cb713984e1f2b7, 0x8dfab49cb4f71325, 0xa25e4f9cbfe84291, 0xc36842ba9ab83fed, 0xbe2a8c31f9dc4e5b,
        0x47d635f219bf5dc7, 0x4918dfe54ae29c35, 0x394cd8e547e59ba1, 0xab8c431765d7b91f, 0x4da92c6763ead87b, 0xe8f64d5a92bcd6e7,
        0xc2715dfed3e21865, 0xc7254a9fefd536c1, 0x358bfc121ec8453f, 0x7169b8e32eda539b, 0xbc9fa5465bcd7319, 0x9ca2ef576acf9185,
        0x64578fb987b39fd1, 0xc28743fa96b5ad4f, 0xb97a28dfe396cdab, 0xacb79681f3baec29, 0x3eb687d212adfb95, 0x9cba8d5764c23af1,
        0xdc538b265eb4396f, 0xdb5294365ea647db, 0xbd4f7c598dcb6749, 0x6afb3c1dc9ad86b5, 0xd8f5972ed8bfa623, 0x76d4b93fe6a2b38f,
        0xe51ba6921695c2fb, 0x3d6b81a434b8e169, 0x62c7e8b54289efc5, 0xed236a7982ad1f43, 0x5a3297687e813d9f, 0x85def6cbae945c1d,
        0xd2ac37bfeda65c79, 0xd5abe6c1fb897ae5, 0xf5e1ba321b8d9a63, 0x418ed9c3256e97bf, 0x28ac14765692c73d, 0xe1f37b587384e5a9,
        0x53fdb87cb3a9f517, 0x98f4675ed2ac1583, 0x78f36cabae7c23df, 0x7d3bf4e1fe91625d, 0x5a8ce2732e8461c9, 0x6af9d3143c968f27,
        0x4ab2cf854b6a8e93, 0x4eb23cf5465a8bef, 0x23cfda59845fab6d, 0x78d346fcb462ead9, 0x62b83cfed285fa47, 0xd863a2c1f27819b3,
        0xd97b81a6528c4931, 0xbfd28e532f5e368d, 0xd87c9f454d7165f9, 0x8372cfebad839467, 0x9e3f48cbab6794d3, 0xad1b793ed97ab351,
        0x53b24a1fe65cb1ad, 0xa563f7ced45ecf1b, 0x834b91d21452fe87, 0x8dab325654761de3, 0xb216a57982793d61, 0xf941cd676f493abd,
        0xaf71cb298d4e5a3b, 0x6b28735dcd4169a7, 0x4a3e692fec64a915, 0xb23dea91fa46b781, 0x9af574321738a5ed, 0xea576493273cd45b,
        0x1952bcd7654fe3c7, 0x9e2ad61cb7861435, 0x68974f1dc26743a1, 0x942fde3cbe49521f, 0xb216f9edcd2a4f6b, 0x48b6c7e1fa2c5ed7,
        0xa79fb8d32c319d45, 0x23c54ea65a349cb1, 0x74b281f65836ba2f, 0xf169c8376527d98b, 0x16cd485a942bd8f7, 0x659f173cb32ef875,
        0x2a79614fe25437d1, 0xb5a74cfede17364f, 0xf68b1ae32f1854ab, 0xc2913b465e1c7429, 0xb183ac976c1e8295, 0x7c28d5698b43a2f1,
        0x964f538a9813ae5f, 0xdb2c791a9615aecb, 0xba96158dc518ed39, 0x516a8b7fe31bfca5, 0xeca925b6564f2c13, 0xfa9e1b4764123a7f,
        0x3f47b1265df348eb, 0xc2a6f4b54df76859, 0xc4f6b5176bfa87c5, 0x71da9b2cb9fda743, 0xbf71869cb6dea49f, 0x4269a5efe7f3c41d,
        0xc39ed7a216f5e279, 0x9fda6e8434d8f1e5, 0xbadf168985fc2163, 0x893c514761dc2eaf, 0x2d654cb98fc15e2d, 0xd573916bace35d89,
        0xf9c7e4adcad67bf5, 0xf7d4bea1f8ea9b73, 0xe1d9c7f216db98cf, 0x859dcf7326cfb84d, 0x472dcbf545d2e6b9, 0xf859be6a94d5f627,
        0xe71db38cb4ea1693, 0x5298c3ea91ca24ef, 0xc81574dbafbe436d, 0x68feb791fec162d9, 0x8da91f232ed59147, 0x5f7de1b32bc68fa3,
        0x4d7b158658c9ae21, 0xe8d671b5459bac7d, 0x6f3e2ad7648fcbe9, 0xc1256faba4c2fb57, 0xca67839fe4d61ac3, 0x38b45fa1f2d93a41,
        0x8edf19c321cb379d, 0x387b5c143fae571b, 0xe8fd2a754ec19587, 0x794af5287cb495e3, 0x65e9c48a9ac8b461, 0xe81bc53dc698a2bd,
        0x7bc43e6fe69cd13b, 0x164c3e9dc38edf97, 0x9fd56ab326a41ef3, 0x8ecb5f1654b63e71, 0x81a429e651973bcd, 0x6ecbd3476f8a5c4b,
        0xc431b8f76d8e5ab7, 0x452817adcdb19a25, 0x9542e8afeca4b891, 0x5d3e2b6fe785a6fd, 0xe54891b21789d56b, 0x4efc57b3258be4d7,
        0x6a5b31f8769ef345, 0x13fbde7a96a534b1, 0x6738d1fcb1a7532f, 0xc5a6371dcf79518b, 0x479f2e3cbc6a5fe7, 0x345be2a1fb7d8e65,
        0x9f3adc143b729dc1, 0xcf67be832974ab3f, 0xd37af8143765da9b, 0xbfd4ae287568ea19, 0xde38a47ba36cf985, 0x148de79cb47f28e1,
        0xabd4592ed192375f, 0x5ea83c1fef7645cb, 0xb87f16d21f6a7539, 0xb19e58d32e6c83a5, 0xb1925cd76c7ea413, 0x8ca517b98a62b17f,
        0x2154df398753afdb, 0x95fd7e6cb756de49, 0x68fb5eadc348fdb5, 0x2d734a61f57c1d23, 0xdfcb94a2135e2b8f, 0x4123e896516249fb,
        0x15da76c87f345a69, 0xd693fc476e3978d5, 0xdbcf6e4a9d4ca853, 0x8d24957cb83d95af, 0x8b5c93dfe961c52d, 0xed2b8a91f754e389,
        0x39b65fd21847e2f5, 0x54eb13f6586b1273, 0xb3e4f9d8745b31cf, 0xecd672f9815e4f3d, 0xf4a6c2b98f325ea9, 0x3846efbcbe358d17,
        0xdb91846fed279c83, 0x4beda75fe72a89df, 0x697815f2182eb95d, 0x29137eb54831e7c9, 0xed3fb9476924f837, 0x9e5d831cb64817a3,
        0x1fae59cdc65b4721, 0x915fe74cb12c347d, 0xef69813cbd1f53e9, 0x3d9a2b71fe139257, 0x4d63e7854e2691c3, 0x8e54bc165c28af31,
        0xe6cf92454719ae8d, 0xfb1249d6561dbcf9, 0xcae396fba821fc67, 0x2eac6f1dc7251bd3, 0x5173ac4fe6283b51, 0xe6bfc4a1f21938ad,
        0xa7c36145431d582b, 0xedc271632dfe5697, 0xb8dfa1565ce196f3, 0xf82147a98cf4b571, 0xebaf729878f6a3cd, 0x5a3e1f7ba7f9d24b,
        0xecf6814ba5fce1b7, 0xb398fdacb4cdef15, 0xec89f1a326e43f81, 0x7e8fca3542f53ced, 0x5f98e3d761f84d5b, 0xb19fc4254fea5bc7,
        0xf7a3d2187cde8b35, 0x26874ab98ce2b9a1, 0x5b7af4cdc9d4b81f, 0x4d72c6b1f8e5d67b, 0x7a1d495216c9d5e7, 0x69e5cfd327dcf465,
        0x12fc764876df25c1, 0x6ec5439a96d2543f, 0xafdc328ba1e6529b, 0x6954f7acbfda7219, 0x64128c5badcb8f75, 0x74df8ba1f9ac8ed1,
        0x7e861af329b1ac4f, 0x9b148fc32793dcab, 0x8629acd547c6eb29, 0x4be589c764c8fa95, 0x74c12b5985bc29f1, 0x3d764a2ba3be286f,
        0x827dca1fe1c246db, 0x7be8619fefc57649, 0x3fd849121fc984b5, 0xad32e6b54ebca523, 0x32fd6e154b9da28f, 0xce395f176892c1fb,
        0x4cae573987a3df59, 0x62f75edba496fec5, 0xd831c4bed6ba1e43, 0x823e7dafe38b2c9f, 0x5f78cba3239f4b1d, 0x37fab9e541c25b79,
        0x1e762b543fa479e5, 0x24fd8c365eb8a963, 0xf8c9de565c7a96bf, 0xfd6a12cba98eb63d, 0x7f632b8ed781e4a9, 0x49e3c58fe8a5f417,
        0x4b6c8a2327b91483, 0xf4d28175458932df, 0xfa56e438739c415d, 0xb46c9a57617e4fb9, 0x794f816baf938e27, 0x897d3eacbe758d93,
        0x4527e6adcb568aef, 0xc6572fb1f859ba6d, 0xcbd6f942175ec8d9, 0x18f62ed43a81f947, 0x85bcaf37679518b3, 0xe894dbacb6a94831,
        0x92b417ecb27a358d, 0xfa61247cb17c54f9, 0x215934fdce5f8367, 0xbf34e6a1fe7492d3, 0x28dbce943e87b251, 0x95dfe7432b46af9d,
        0x8c274da54859ce1b, 0xb82aed76564ced87, 0xdf3e864ba8721ce3, 0xcb1743ded7853c61, 0x3e7c921fe25639bd, 0x829b47d1f16a593b,
        0xef84d9c3214d58a7, 0xc356f9243e4f9815, 0xf9a3b7d87e53b681, 0xac9b1d587b35a4ed, 0x26e8db1a9a48d35b, 0x946b175cb84ae2c7,
        0x2ae47fcdc86df135, 0xd732bc61f87342a1, 0x3dfb9c7216543efd, 0x9bf417d652374e6b, 0x9dc1b3a761395cd7, 0x3ef17ab87f2d8c45,
        0xb4a769187d3f8ab1, 0xc2475debac43b92f, 0x91a27c4fea24d78b, 0x35a7b68fe928d6f7, 0xb349c1f2192bf675, 0xac18bd56584e26d1,
        0x7c2b8dea9861354f, 0x9c72e65a962453ab, 0x7c25a1edc1487329, 0xb354ae8dcf2b8195, 0x864397cbac2b8fe1, 0xef5a16cdca1e9d5f,
        0x51ca8d943912adcb, 0x7cdb31454a15ec39, 0x87af6cb54817fba5, 0x1897ed6a984c2b13, 0x9ec5f6dba51d297f, 0x6eb57f1dc52147eb,
        0x95eb783fe2136759, 0x4671b5aedef486c5, 0xc374edb21ef9a643, 0x8b67dfe32ceba39f, 0x31497da659feb31d, 0x84f95a276af2e179,
        0x8e712ac987c3efd5, 0x37b1df2cb7e81f53, 0x59ae7f8cb4e82daf, 0x3b9268dfe3fb4c2d, 0xf546a8d212ef4c89, 0xfe16379541e27af5,
        0x76fcb3954fe59a73, 0xc12867f54bd697cf, 0x2f3da56769eab74d, 0xe37a8df987ced5b9, 0x729f5acdc9d1f527, 0xb765ead1f8e61593,
        0xbaf9136216c723ef, 0x6781ec5435ea426d, 0x491a8d2763ec51d9, 0x7a68ec4981ce6f37, 0x6c47fb587fc28ea3, 0x79e153da9ec6ad21,
        0xc2b7453fe9c6ab7d, 0x7ade6591f6b8cae9, 0x5fe984d218bdea57, 0x8eab29f658d219c3, 0xa19f7c6878d63941, 0x5fc831d984c7369d,
        0x1d564fba93da561b, 0xd3f597bcb1cd6487, 0x65e8dbacbdaf84e3, 0x5c82f9b1fec4b361, 0x12ba84932e95a1bd, 0x76159a843a97cf2b,
        0x21e568c4368ade97, 0xcf54b186568becf3, 0xcae2831ba8b23d71, 0xf28ae96cb5943acd, 0x537129afe2c75a4b, 0x5e374b21f1ca59b7,
        0x3257fc6431bd8925, 0x59be63c43e8fa791, 0xd38149754c82a5fd, 0xca17d4e87c95d46b, 0xfeba85d76a98e3d7, 0xe34c72fdc89bf245,
        0xb641783ed79e23b1, 0x1dcb845fe7a3512f, 0x3986de4215934f7b, 0x61fa8d7432965de7, 0x6e713f45419a8d65, 0xb843f1c65f6d8bc1,
        0xe916d5b87c6e9a3f, 0x73ac841bac62d89b, 0x8953721edb75e819, 0x87becd9fea69f785, 0x9a6e3dc4398b27e1, 0x8abf91d4378d265f,
        0x6d19cfe8768154cb, 0x612f9b4cb2a57439, 0x52ae468dc19782a5, 0xb32c7f9dcf7ba213, 0x35692febac4b9e6f, 0xa79346cdc84f9edb,
        0xde6843a21b62ed49, 0x5864db132964fcb5, 0x39851d476a791c23, 0xda9e6f78765b2a8f, 0xb96328fa945d38fb, 0x853df2ced3715869,
        0xa3e8d12fe16387d5, 0x12bd6a8fef48a753, 0x731849521e3894af, 0xc16ed8b43e5cb42d, 0xaf4e23165b4fd289, 0xfc16a7298a63e1f5,
        0x3f89c56dcb682173, 0x7832e15ba6472ebf, 0x2e3469cdc65a4e3d, 0x85da42b1f23e4da9, 0x9b831ea434528c17, 0xf1ed96c761549b83,
        0xe2b4f9754e2598df, 0x9f2654c76e29b85d, 0xeb9a46d87a2bd6c9, 0x93ae57bcba2fe637, 0x4a6b295feb5416a3, 0x748be3a1fa584621,
        0x3c61bae21829437d, 0x6c2a4815463b52e9, 0x57db84ca943e8157, 0xb4d71fea92418fb3, 0x6437b92a9f25ae31, 0x532e16ca9e15ac8d,
        0x8bdc235eda17cbf9, 0xf8a374b1f91ceb67, 0xdfa9c453281ef9d3, 0x2ce93b587b243a51, 0x5ec12fa7681537ad, 0xae1f35cba619572b,
        0xabe1536cb31c6597, 0x73681afcb21d85f3, 0xd91b62ecbdefa471, 0x83d7c52febf3a2cd, 0x714526932cf6d14b, 0xb74982f329e6dfa7,
        0xf7da631548dafe15, 0x8b9e1c4767fd2e81, 0xd12cfa6986f13bed, 0xc278563ba6f34c5b, 0x96d514fed2f65ac7, 0x69c4b82fe1fb8a35,
        0x16a8c97432fd98a1, 0x2fa3cb943dcea71f, 0xb1368ac54ce2d57b, 0xbd346cf76ac5d4e7, 0x1b82ad498ad9f365, 0xfc5e63aba8eb24c1,
        0xd341872cb7cd423f, 0xc549fae1f6e2519b, 0xbce4613214c35ef7, 0xb93fea8432d68e75, 0x8cdb431652e98cd1, 0xf48ab2e65eac9b4f,
        0x7dc3fa565d8fc9ab, 0x47ced2898bc3ea29, 0x7148cf9edac4f895, 0x864d352fe7c928f1, 0x7b18cfa218cb276f, 0x4a82ce1326ae35db,
        0xd7a62ce986d17549, 0x72fb891a92d483b5, 0xf25d193cb1d8a423, 0x67ceab987eb9a18f, 0x87164ef98c7a9feb, 0x7fdb6c9eda8fce59,
        0x5eda46121a92fdc5, 0x9cfe71365ab71d43, 0x39d21f6546a72b9f, 0x46c51978769b4a1d, 0x5ef8742983be4a79, 0x28a1cf6dc3b178e5,
        0x5bd4168fe1c4a863, 0x982bfa4ede8695bf, 0x41be9cf21f89b53d, 0x5e24f8b32e7cd3a9, 0xced9a3265b9fe317, 0xb728f49a9cb51283,
        0x83e7a42ba89631df, 0x5928fb1cb6983f4d, 0x5b8a3f6ed37a4eb9, 0x3d79eba1f39e6d27, 0x9fd5b72324a18c93, 0xbd934ef5417389ef,
        0xadb356243f75b96d, 0x7f29b3854e58c7d9, 0xfd63187a9b6ce847, 0xae5d28fba97ef6b3, 0x9c6b5e8feba54731, 0xbc936571f796348d,
        0xa96f5ce2167853f9, 0xf1d9e864349c8267, 0x53bd4617647e81d3, 0x2de4b17a9382af41, 0x1b96c3287f53ae9d, 0x163bc5fa9e56cd1b,
        0xb6ac283edb49ec87, 0x71b85f6ed85bfae3, 0xbd9ac6743a7f2b61, 0x8cf794b5496238bd, 0xe84162b54976583b, 0xb2ef79dcb34956a7,
        0x5284379cb36c9715, 0x7843916dc26ea581, 0x24bf3d8cbf51a3ed, 0x84db5261fe64d25b, 0x59a84b232d46e1c7, 0xb72d89643b49ef25,
        0xb82f63c76a5c2f91, 0x8d63b197674e2cfd, 0x7f56123ba8524d6b, 0x1294e5cdc6345bd7, 0x4c269adfe2398b45, 0xc975ab81f24c89b1,
        0x27c6bfd4314da82f, 0x6f49c3e54f31d68b, 0x3cbae8f76c23d5f7, 0x25b318ea9c28f475, 0xb642975a9b4a25d1, 0xd7fe189cb95c234f,
        0x24a9c65ed73f42ab, 0x7e9ca6f1f8547129, 0x4f518ea216247f85, 0xc93481f654378de1, 0x8c3954b7612a9c5f, 0x342859d65f1e9acb,
        0xf7e246398d21eb39, 0x98421b3cbc13f9a5, 0xe73d124fec482a13, 0xb5739e2fe91a287f, 0xf7c29a43281c36eb, 0xc91f3874381f4659,
        0x73e85d2a971284c5, 0x69b38d2cb617a543, 0x8ef31d9cb317a29f, 0x7321bd898ef9b21d, 0xb65182498aebcf69, 0x26afed8ba8bfced5,
        0xaebcf5432bf41e53, 0x9e5163d438e52caf, 0x97c8def657f84b2d, 0xc2f914a765fc4b89, 0xcd8e2a3983ed69f5, 0x41a93ebdc4f1a973,
        0x2f83b74cb1e396cf, 0xa73f16edcfe7b64d, 0xaf7189521fe8d4b9, 0xc2a536f43bdce427, 0xa4937df65acef293, 0x95ec16d879c432ef,
        0x9f6d745ba9e7416d, 0x3e42c76ba5d74fc9, 0xc298435dc4db6e37, 0xea4d6f71f2ce7da3, 0xc9b3d86545e3ac21, 0x2f743ad541e3ba7d,
        0xc3e29af43fb5c8e9, 0xe96134b54dcae957, 0x2b841c6769abf7c3, 0x385dfa1cbacf2841, 0x653a2bcfe8c2359d, 0x9476fb81f8d7651b,
        0xc634ba8215da6387, 0xfb61a47545cb82e3, 0x5c693a1874cea261, 0x82f69b3762b19fad, 0xf195e3687f94ce2b, 0x625f8b498ca5ed97,
        0xa719cb2dca98ebf3, 0x354c26a1f9ad2c71, 0xfa45c712178f29cd, 0xe9bda1c438c2594b, 0x79625fe436c458b7, 0xfde34c2874ca9825,
        0x78d3f46983bca691, 0x9843627ba18c94fd, 0x75b93acdcf91d36b, 0x56eb2971fc94e2d7, 0x769e58232da8f145, 0x5fe8b7a65d9a32b1,
        0xc7591da6598b3e1f, 0x2894fb16568f3e7b, 0x7a63491ba6925ce7, 0x523db89ed3a48c65, 0x9cd238efe3978ac1, 0xbc41ad7fe17ba93f,
        0x1b5f6283216ec79b, 0x7df3eb654f82e719, 0x2d316c854d93f685, 0xf1dce7487c9826e1, 0x6825c73a9b8a245f, 0xecd2851ba87d43cb,
        0x264dbc8ed78f6239, 0x462f8de1f69381a5, 0xb6e152a324758ef1, 0x63a128d432579d6f, 0x387dcb1541589cdb, 0xd69b45e76f5edc49,
        0xfad751698d61fab5, 0x2a51fcedcd861b23, 0xb53914aedb56298f, 0x4b7c386fe85937fb, 0x4daf59821a5d4769, 0x9857ecd3284f76d5,
        0x1e7db89a9782a653, 0xe5c672b9826593af, 0xef54679ba168b32d, 0x8da4f7cbaf4bd189, 0x97653d4a9a3bcfe5, 0x5dca9ebfec721f63,
        0x9b6c8e521a542dbf, 0xb2c9d7832a574c3d, 0x9a478bc657384ca9, 0xdc594f6a984d6b17, 0xd93e625cb54f7a83, 0xf28695ecb34297df,
        0xba68493fe134b75d, 0xd413eabfef27d5c9, 0x4cb375821f2be537, 0xe9f3a2d65c2df4a3, 0x9def4c5a9d734521, 0xc5941d2a9b35427d,
        0x219d356cba3751e9, 0xb8e91afdc82a6f47, 0x59dbf47ed52c7eb3, 0x54c129afe54f9d31, 0x9c6f37121432ab8d, 0x23a8b4d65124caf9,
        0xcfb67ae54f18ea67, 0x6da2bf576c1af8d3, 0x7adef46bac3e2951, 0xf8c367adcb2136ad, 0x3a5486dfea14562b, 0xb9345f81fa186497,
        0xf71a6c43271a84f3, 0xde28a416562da371, 0x6f1a4ed7642f91cd, 0xd82c15b98412cf3b, 0xab1d86276cf3dea7, 0x2d49c61a9be7fd15,
        0x85619a3dcbfa2d81, 0xc62e78fdc7fb2aed, 0x34617ad218ef3a5b, 0x6b5e783438f259c7, 0x9671c32547f68935, 0x93d4c5f764f897a1,
        0x84cad5b983eba61f, 0x31bf68ea91fec47b, 0x57f9682bafc1d3e7, 0xd2b95a1fecd4f265, 0xbe7c86f54de823c1, 0x3dab2f765bda413f,
        0xae6c132547dc3f8b, 0xe239f18656be4df7, 0xe6f2814ba6e28d75, 0x98ced72ba4e58bd1, 0x72319cbdc2d79a4f, 0xc4b7fa21f1e9c8ab,
        0x3f57db9431ced829, 0x637b25d43ec1f795, 0xda23f4876dc527f1, 0xf6be9c587bc8256f, 0x97f4b61878c934db, 0x6dab951cb8cd6349,
        0x36ca124ed6cf72b5, 0x4d856231f7d5a123, 0x9db4c8a213b49e7f, 0xf586b7a432a69deb, 0xce32df9541bacd59, 0xc34719654f8debc5,
        0x843f762a9eb21c43, 0xf83127698cb42a9f, 0x175643bfeab7491d, 0x35d2f9c1f6cb4879, 0x1d9456c218ac67e5, 0x65479df325bf9763,
        0xdab935c765a294bf, 0xdc6f31e982a6b43d, 0x568fdac981b7d2a9, 0xec9df86baf9be217, 0x82319debab8cef73, 0xbf7a1c4eda812ecf,
        0xe59b34621b953e4d, 0x4c71fb832a864db9, 0x1f7c2d85489a6c27, 0x6b8d4c17658d7b93, 0x9fdec67a936d78ef, 0xe793f58dc392b86d,
        0xe79814bfe184c6d9, 0xa782d59edf89e647, 0xa54fcb721e7af5b3, 0xc1f3da965d9e2631, 0xa6241c376c91438d, 0x5e297c487b8452f9,
        0xabec531dcba98167, 0xf52986acb5697fc3, 0xbdcaf19ed57c9e41, 0x48e6957fe25f8c9d, 0x7cfe6b532473dc1b, 0x3a182e743164eb87,
        0xebf321454d57f9e3, 0xec357a898e7c2a61, 0x5b6ec3d98a5e27bd, 0x47f312bedc81573b, 0x68cbd24feb8365a7, 0xfea72cd1fa789615,
        0x8dc6e193276ba481, 0x7ed628b4345b92ed, 0x5fbd61a8736fc15b, 0xb15d82498152dfb7, 0xb53762fa9d46fe25, 0x245ba6fcbd582e91,
        0x7295e16dca492bfd, 0xf98de7c1f94e3c6b, 0x9d2c87b32b415ad7, 0xc6da81732b638a45, 0x59c2ae76574698b1, 0x2a536c198549a72f,
        0xb764213a923dc58b, 0x981c564ba13ec4f7, 0xa3e1f5cdce42f375, 0x7bcd9641fe5724d1, 0xea917b421d59324f, 0xfc1734a65d3c41ab,
        0x928d4fc76a3d5f19, 0xfd58624a9a317e85, 0x82164d9ba7348ce1, 0x6c4928bdc4269b5f, 0x24cd7b6fe327a9cb, 0x24b79ec1f12cda39,
        0x67bc51f4312fe8a5, 0x4672f5a87f452913, 0x145b68a98d15267f, 0x197356d87c1835eb, 0xca52183bab1c6459, 0xa75942bcb91e73c5,
        0xcbad4e5fe951a243, 0x9a6481dfe624a19f, 0x64a27df324159efb, 0xf9b642754318ce69, 0xad593f17621aecd5, 0xc2bd95176bedfc53,
        0xb42e9ca87ce12baf, 0x94718e2a9bf54a2d, 0xfc6b38aedbf64a89, 0x3f915d6ed7ea68f5, 0xecb8f64216fd9873, 0xa645ec1324de85cf,
        0x496c3ea876f2b54d, 0x371ba2d872f5d3b9, 0x34ca892ba1f9e327, 0x58ecf7d98edbf193, 0x156ebc8a9abc21ef, 0xda17bc2feac13f5d,
        0xd93a17421ad34ec9, 0x598d7ce32ae86d37, 0xd637b41546e97ca3, 0x49b1ac3876ed9b21, 0x63a915f763cfa97d, 0xf4c526eba3d2c7e9,
        0xa352c48dc1d4e857, 0x13fdb7cedeb8f6c3, 0xf53c96b43fdc2741, 0x7a32d1943cbe249d, 0xdb9a63187cd2641b, 0x69d2734a9bd46287,
        0x8c15df4ba9c781e3, 0x76213f4cb6ca9f51, 0x69fa184dc39b8ead, 0x13e7bd2fe39fad2b, 0x8d1b5f3213b2ec97, 0x217cd6f541b5eaf3,
        0xec29d3f65fc92b71, 0xe791db265e9a28cd, 0xce5db9387cae384b, 0x4bc56f9edac156b7, 0x85d1e47feac49725, 0x8ec62ad1f6c7a591,
        0x5a894bf215a893fd, 0x1e95af84349dc26b, 0xe2b3fd46539fc1d7, 0xe23cbd5981a2ef35, 0x536b1fa98e962fa1, 0x7243df1bac983d1f,
        0xade5c6bfec8a3d7b, 0x26539871f98d4be7, 0x5eb26a932ca18b65, 0xbc6135f437a489c1, 0x84d3ba754596a83f, 0xca42fb765378c69b,
        0x8f4e623ba38cd619, 0xb38a679cb19fe485, 0xa61c257dcf9425e1, 0xc6d9b4afed97235f, 0xaf5348721e7842cb, 0x231e6f754d8c6139,
        0x136a59e43a7d6f95, 0xfea1945769728df1, 0xf175468ba5739c6f, 0xc437589dc3659adb, 0xd361a78fe268db49, 0xa7845fd1f16ce9b5,
        0xc6d9e3a5417ef923, 0xa8649ec54e63278f, 0x6bef43254d7536fb, 0xf4c762898d7a4569, 0xf612375a9a6b74d5, 0x1b67c45dca7e9453,
        0xa7c96f5ed76192af, 0xdfac6381f675b12d, 0x53cfe1621345cf79, 0x9b712e643248cde5, 0x97cd6438715bfd63, 0xa6b52d376d4e1cbf,
        0x215496ba9e624b3d, 0xc538b67dcd354ba9, 0x3ae8d76fec496a17, 0x46b238a1fa4c7983, 0x4e6fd182173c86df, 0x1eaf3b743841b65d,
        0x5a3d91687653d4c9, 0x4ac95feba258e437, 0x68917f4cb159f2a3, 0xd35f6b8cbf6d3421, 0x539a2b6cbc4f317d, 0x139427cfeb324fd9,
        0xf6d1be421c256e47, 0x643af9c54a287db3, 0x62f138e7684b9c31, 0xc83bf947652e9a8d, 0x82d5ce4ba531c8f9, 0x1eca467dc423e967,
        0x1f95ba8dc126f7d3, 0x69a35bdfef3b2851, 0x5e6afb243f1c25ad, 0xc4f5a9876d1f452b, 0xe5bd16398d126397, 0xed47129a9b1682f3,
        0xc762f53dcb39a271, 0xd18fa65ba5198fbd, 0x8eb146fdc51dae3b, 0x7265b4d1f421eda7, 0xafc1e2543624fc15, 0xa18ecf3764372c81,
        0xf9a2de454cf729ed, 0xdebc12a76dfc395b, 0xb8635cd76afe48c7, 0xfa1b327dcbf19835, 0xa2b16d3ed8f496a1, 0x9485d73ed6e7a51f,
        0x8b13e79215f9c37b, 0xde19f7b434fbc2e7, 0x7cf1d24873dfe165, 0x18eb6d5a94f532c1, 0x3bc8de498dc63e2f, 0x6e28743a9cd73e8b,
        0x9fed346cb9da4cf7, 0xaf3c9861f9de6c75, 0x9e238c6439e28ad1, 0xfce7d28327d3a94f, 0x8db2f3a434e5c7ab, 0x2ad53c4764ead729,
        0xdf3e156982dce695, 0x7ef2c49a92bde5f1, 0x8a72fe3cbec3246f, 0xe453b981fec643db, 0xcbf7ad621eda6249, 0xb3ca2e943adc71b5,
        0x48e7259659bd8f13, 0x5df927a656b19d7f, 0x6f9a531875a39ceb, 0x5d47832ba3b6dc59, 0x8a371cbed2b8eac5, 0x4b2ed36fe1bcfa43,
        0x8a1db3f432be189f, 0x359f1d265fb3481d, 0xcd6ef1a43ec54679, 0x28ceaf154cb976e5, 0xd43b26f98abc9563, 0xc648da1ba69d83bf,
        0x571e438fe7a1b23d, 0xf864a7e1f5b4d1a9, 0x9bc46fe21396cef5, 0xc8d326a542a9fe73, 0xa74c98e6528a1dcf, 0x46f985b76e8f2c4d,
        0x5e6c12b87e824cb9, 0xe6954c2dcda56b27, 0x38ab794fecb87a93, 0xba28147ed56a87ef, 0xf9ceda52158ea76d, 0xdb9f26743681c5d9,
        0x46d287e986a3e547, 0x45c238b98297f4b3, 0x596b12fdc4ab2531, 0xcf16947baf7c328d, 0xe6b89fdcbc7e41f9, 0x6793d4afec926f57,
        0xe34961f21b857ec3, 0x9adfe6543b789d41, 0xa26357b4365a8b9d, 0x42b31568756eca1b, 0x746a3decb461ea87, 0x2963da8cb274f8e3,
        0x658eb12fe3a82961, 0x4cd361efef5926bd, 0x7653ebd21f7d463b, 0x47a821e32d6f54a7, 0x2615fe487d839415, 0x9b3417ca9c76a381,
        0x9c8d57eba85791ed, 0x12a6c34dc659af4b, 0x7143625ba34dceb7, 0x9172e85fe471fd25, 0xb3589f2434752d91, 0x4e21b87541562afd,
        0xe215f9465f493a6b, 0xf2dc51876e3c49d7, 0x627859ebac3f7945, 0xa4378fbedc5397b1, 0x3b67154ed945a62f, 0xe3cdb7a1f637c48b,
        0xc374fa82164ac3f7, 0x2bf7ce84364de275, 0xd2f1638a967423d1, 0x731c25dba276314f, 0xc15d2be98e263f9b, 0x1e872a6cbd3a5e19,
        0xa81bd2cfeb2c6d85, 0xd824739ed93f7be1, 0x9146bfa21b329a5f, 0x273856d43724a8cb, 0x4d2bafe65627d839, 0xedfb7369842be7a5,
        0x259a386ba54df713, 0xf16e94cdc341257f, 0x7ecf596dcf1534eb, 0x62ced941fe186359, 0xa952e7143e1a72c5, 0x85b6afc76c2d9143,
        0x7f82e4d5491d7e8f, 0xcd15f9a767129dfb, 0x8426fc3ba714cd69, 0x17ace43ba417ebd5, 0x1782b5ced41afb53, 0xb45c613fe32d19af,
        0x5c41a2d76531492d, 0xa9b365743ef24789, 0xd51e93f43ce467f5, 0x283aec165cfa9673, 0x89bce12767ea84cf, 0x85214cba97fea34d,
        0x541a3f7dc6f1d2b9, 0x3861245ed6f5e127, 0xdae3b51214d6ef83, 0x683147e543c81edf, 0x3a29e5f761fb2e5d, 0xb32489565ebf3dc9,
        0x7cb35efa9de26c37, 0x4f53dc7a9ce57ba3, 0x41cb562edbe89a21, 0x368bda21f5eba87d, 0x357684a215cdb6e9, 0x793acb8438d1e657,
        0x69372f5876d4f5c3, 0x4ae59fdba5e92641, 0x3cef8da981ea239d, 0x913685da9fcd531b, 0xd947ab3cbdcf5187, 0x81d42e5feab27fd3,
        0x96871ec32bc69e51, 0x61b785c327b68cad, 0x83f7124546d9ac2b, 0xc231db4654aedb97, 0x65483df985b1e9f3, 0xe71983adc4d52a71,
        0xcbd7395fe1b627cd, 0x3d8769bfefca374b, 0xc794fbd21fad45b7, 0xc6e2a1743dbf8625, 0x2fd68a376dc3a491, 0xce53a4f768a592fd,
        0xbd5f13ea97a8c16b, 0x74c1328ba489bfc7, 0x6421e7fcb49cde35, 0x6fadc541f5b32ea1, 0xe7cbd1f324a53c1f, 0x6c149b7541a63c7b,
        0xe5a6d9f43f894ae7, 0xb43ade654e9d7a65, 0xe1bfd9376b9f78c1, 0x48bfe25dcb92a73f
    };

    u64 key = keys[seed % (sizeof(keys)/sizeof(keys[0]))];

    return (ElkRandomState){ .key = key, .counter = seed };
}

static inline u64
elk_random_state_uniform_u64(ElkRandomState *state)
{
    u64 cnt = state->counter++;
    u64 key = state->key;
    
    u64 y = cnt * key;
    u64 x = y;
    u64 z = y + key;
    x = x * x + y;     x = (x>>32) | (x<<32);       /* round 1 */
    x = x * x + z;     x = (x>>32) | (x<<32);       /* round 2 */
    x = x * x + y;     x = (x>>32) | (x<<32);       /* round 3 */
    u64 t = x * x + z;
    x = t;             x = (x>>32) | (x<<32);       /* round 4 */

   return t ^ ((x * x + y) >> 32);                  /* round 5 */
}

static inline f64
elk_random_state_uniform_f64(ElkRandomState *state)
{
    return 5.42101086242752217e-20 * elk_random_state_uniform_u64(state);
}

static inline ElkKahanAccumulator
elk_kahan_accumulator_add(ElkKahanAccumulator acc, f64 value)
{
    f64 y = value - acc.err;
    volatile f64 t = acc.sum + value;
    volatile f64 z = t - acc.sum;
    acc.err = z - y;
    acc.sum = t;

    return acc;
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

#pragma warning(pop)
#endif
