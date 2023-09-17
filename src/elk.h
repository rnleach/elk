#pragma once
/**
 * \file elk.h
 * \brief A source library of useful code.
 *
 * See the main page for an overall description and the list of goals/non-goals: \ref index
 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*-------------------------------------------------------------------------------------------------
 *                                      Error Handling
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup errors Macros for clean error handling.
 *
 * @{
 */

/// \cond HIDDEN
#ifdef ELK_PANIC_CRASH
#    define HARD_EXIT (fprintf(0, "CRASH"))
#else
#    define HARD_EXIT (exit(EXIT_FAILURE))
#endif
/// \endcond HIDDEN

/** Clean error handling not removed in release builds, always prints a message to \c stderr.
 *
 * Unlike \c assert, this macro isn't compiled out if \c NDEBUG is defined. Error checking that is
 * always on. This macro will do an error action, which could be a \c goto, \c continue, \c return
 * or \c break, or any code snippet you put in there. But it will always print a message to
 * \c stderr when it is triggered.
 */
#define StopIf(assertion, error_action, ...)                                                       \
    {                                                                                              \
        if (assertion) {                                                                           \
            fprintf(stderr, "[%s %d]: ", __FILE__, __LINE__);                                      \
            fprintf(stderr, __VA_ARGS__);                                                          \
            fprintf(stderr, "\n");                                                                 \
            {                                                                                      \
                error_action;                                                                      \
            }                                                                                      \
        }                                                                                          \
    }

/** Clean and quiet error handling not removed in release builds.
 *
 * Unlike \c assert, this macro isn't compiled out if \c NDEBUG is defined. Error checking that is
 * always on. This macro will do an error action, which could be a \c goto, \c return, \c continue,
 * or \c break, or any code snippet you put in there. It's called quiet because nothing is printed.
 */
#define Bailout(assertion, error_action)                                                           \
    {                                                                                              \
        if (assertion) {                                                                           \
            error_action;                                                                          \
        }                                                                                          \
    }

/** If the assertion fails, cleanly abort the program with an error message.
 *
 * Unlike \c assert, this macro isn't compiled out if \c NDEBUG is defined. This is useful for
 * checking errors that should never happen, like running out of memory.
 *
 * If \c ELK_PANIC_CRASH is defined, then this will print to \c NULL and cause a crash that any
 * good debugger should catch so you can investigate the stack trace and the cause of the crash.
 */
#define PanicIf(assertion, ...)                                                                    \
    {                                                                                              \
        if (assertion) {                                                                           \
            fprintf(stderr, "[%s %d]: ", __FILE__, __LINE__);                                      \
            fprintf(stderr, __VA_ARGS__);                                                          \
            fprintf(stderr, "\n");                                                                 \
            HARD_EXIT;                                                                             \
        }                                                                                          \
    }

/** Cleanly abort the program with an error message.
 *
 * Unlike \c assert, this macro isn't compiled out if \c NDEBUG is defined. Useful for checking
 * unreachable code like the default statement in a \c switch statement.
 *
 * If \c ELK_PANIC_CRASH is defined, then this will print to \c NULL and cause a crash that any
 * good debugger should catch so you can investigate the stack trace and the cause of the crash.
 */
#define Panic(...)                                                                                 \
    {                                                                                              \
        fprintf(stderr, "[%s %d]: ", __FILE__, __LINE__);                                          \
        fprintf(stderr, __VA_ARGS__);                                                              \
        fprintf(stderr, "\n");                                                                     \
        HARD_EXIT;                                                                                 \
    }

/** @} */ // end of errors group

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup time Time and Dates
 *
 * The standard C library interface for time isn't threadsafe in general, so I'm reimplementing
 * parts of it here.
 *
 * To do that is pretty difficult! So I'm specializing based on my needs.
 *  - I work entirely in UTC, so I won't bother with timezones. Timezones require interfacing with
 *    the OS, and so it belongs in a platform library anyway.
 *  - I mainly handle meteorological data, including observations and forecasts. I'm not concerned
 *    with time and dates before the 19th century or after the 21st century. Covering more is fine,
 *    but not necessary.
 *  - Given the restrictions above, I'm going to make the code as simple and fast as I can.
 *  - The end result covers the time ranging from midnight January 1st, 1 ADE to the last second of
 *    the day on December 31st, 32767.
 *
 * @{
 */

/** Succinct type used for calendar time.
 *
 * Most work with times and dates should be done using this type as it is small and natively
 * supported on most modern architectures that I will be using.
 */
typedef int64_t ElkTime;

/** The unix epoch in ElkTime.
 *
 * If you have a timestamp based on the unix epoch, add this value to get it into the elk epoch.
 */
extern ElkTime const elk_unix_epoch_timestamp;

/** Convert an \ref ElkTime to a Unix timestamp. */
static inline int64_t
elk_time_to_unix_epoch(ElkTime time)
{
    return time - elk_unix_epoch_timestamp;
}

/** Convert an Unxi timestamp to an \ref ElkTime. */
static inline ElkTime
elk_time_from_unix_timestamp(int64_t unixtime)
{
    return unixtime + elk_unix_epoch_timestamp;
}

/** Calendar time.
 *
 * Useful for constructing or deconstructing \ref ElkTime objects, and doing output.
 */
typedef struct ElkStructTime {
    int16_t year;
    int8_t month;
    int8_t day;
    int8_t hour;
    int8_t minute;
    int8_t second;
} ElkStructTime;

/** Find out if a year is a leap year. */
bool elk_is_leap_year(int year);

/** Create a \ref ElkTime object given the date and time information.
 *
 * NOTE: All inputs are in UTC.
 *
 * \param year is the 4 digit year, and it must be greater than or equal to 0 (1 BCE).
 * \param month is the month number [1,12].
 * \param day is the day of the month [1,31].
 * \param hour is the hour of the day [0, 23].
 * \param minute is the minute of the day [0, 59].
 *
 * The minimum year is year 1 and the maximum year is year 32767.
 */
ElkTime elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds);

/** Convert from \ref ElkStructTime to \ref ElkTime. */
ElkTime elk_make_time(ElkStructTime tm);

/** Convert from \ref ElkTime to \ref ElkRefTime. */
ElkStructTime elk_make_struct_time(ElkTime time);

/** Truncate the minutes and seconds from the \p time argument.
 *
 * \param time is the time you want truncated or rounded down (back) to the most recent hour.
 *
 * \returns the time truncated, or rounded down, to the most recent hour.
 */
ElkTime elk_time_truncate_to_hour(ElkTime time);

/** Round backwards in time until the desired hour is reached.
 *
 * \param time is the time you want truncated or rounded down (backwards) to the requested
 *            (\p hour).
 * \param hour is the hour you want to round backwards to. This will go back to the previous day
 *             if necessary. This always assumes you're working in UTC.
 *
 * \returns the time truncated, or rounded down, to the requested hour, even if it has to go back a
 * day.
 */
ElkTime elk_time_truncate_to_specific_hour(ElkTime time, int hour);

/** Units of time for adding / subtracting time.
 *
 * It's not straight forward to add / subtract months or years due to the different days per month
 * and leap years, so those options aren't available.
 */
typedef enum {
    ElkSecond = 1,
    ElkMinute = 60,
    ElkHour = 60 * 60,
    ElkDay = 60 * 60 * 24,
    ElkWeek = 60 * 60 * 24 * 7,
} ElkTimeUnit;

/** Add a change in time.
 *
 * The type \ref ElkTimeUnit can be multiplied by an integer to make \p change_in_time more
 * readable. For subtraction, just use a negative sign on the change_in_time.
 */
ElkTime elk_time_add(ElkTime time, int change_in_time);

/** @} */ // end of time group
/*-------------------------------------------------------------------------------------------------
 *                                         Hashes
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup hash Hash functions
 *
 * Non-cryptographically secure hash functions.
 *
 * If you're passing user supplied data to this, it's possible that a nefarious actor could send
 * data specifically designed to jam up one of these functions. So don't use them to write a web
 * browser or server, or banking software. They should be good and fast though for data munging.
 *
 * @{
 */

/** Function prototype for a hash function.
 *
 * \param n is the size of the value in bytes.
 * \param value is a pointer to the object to hash.
 *
 * \returns the hash value as an unsigned, 64 bit integer.
 */
typedef uint64_t (*ElkHashFunction)(size_t const n, void const *value);

/** Accumulate values into a hash.
 *
 * This is a useful tool for writing functions to calculate hashes of custom types. For instance
 * if you have a struct, you could write a function that creates a hash by sending the first
 * member through \ref elk_fnv1a_hash() and then sending each of the other members in turn through
 * this function. Then finally return the final value.
 *
 * \param n is the size of the value in bytes.
 * \param value is a pointer to the object to hash.
 * \param hash_so_far is the hash value calculated so far.
 *
 * \returns the hash value (so far) as an unsigned, 64 bit integer.
 */
static inline uint64_t
elk_fnv1a_hash_accumulate(size_t const n, void const *value, uint64_t const hash_so_far)
{
    uint64_t const fnv_prime = 0x00000100000001b3;

    uint8_t const *data = value;

    uint64_t hash = hash_so_far;
    for (size_t i = 0; i < n; ++i) {
        hash ^= data[i];
        hash *= fnv_prime;
    }

    return hash;
}

/** FNV-1a hash function.
 *
 * WARNING: In types with padding, such as many structs, it is not safe to use this function to
 * calculate a hash value since it will also include the values of the bytes in the padding, which
 * the programmer doesn't generally have control over. This is only safe for types all the data is
 * stored in contiguous bytes with no padding (such as a string). For composite types, create a
 * custom hash function that uses this function for the first struct member and then passes the
 * resulting hash value through \ref elk_fnv1a_hash_accumulate() with the other members to build
 * up a hash value.
 *
 * \param n is the size of the value in bytes.
 * \param value is a pointer to the object to hash.
 *
 * \returns the hash value as an unsigned, 64 bit integer.
 */
static inline uint64_t
elk_fnv1a_hash(size_t const n, void const *value)
{
    uint64_t const fnv_offset_bias = 0xcbf29ce484222325;
    return elk_fnv1a_hash_accumulate(n, value, fnv_offset_bias);
}

/** @} */ // end of hash group
/*-------------------------------------------------------------------------------------------------
 *                                       String Slice
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup str String slice.
 *
 * An altenrate implementation of strings with "fat pointers" that are pointers to the start and
 * the length of the string. When strings are copied or moved, every effort is made to keep them
 * null terminated so they play nice with the standard C string implementation.
 *
 * \warning Slices are only meant to alias into larger strings and so have no built in memory
 *    management functions. It's unadvised to have an \ref ElkStr that is the only thing that
 *    contains a pointer from \c malloc(). A seperate pointer to any buffer should be kept around
 *    for memory management purposes.
 * @{
 */

/** A fat pointer type for strings.
 *
 * \note If \ref ElkStr.start is not \c NULL but \ref ElkStr.len is zero, this refers to an empty
 *     string slice. If \ref ElkStr.start is \c NULL, then it doesn't refer to anything, it's the
 *     same as a \c NULL pointer in plain C.
 */
typedef struct ElkStr {
    char *start; /// points at first character in the string.
    size_t len;  /// the length of the string (not including a null terminator if it's there).
} ElkStr;

/** Create an \ref ElkStr from a null terminated C string. */
ElkStr elk_str_from_cstring(char *src);

/** Copy a string into a buffer.
 *
 * Copies the string referred to by \p src into the buffer \p dest. If the buffer isn't big enough,
 * then it copies as much as it can. If the buffer is large enough, a null character will be
 * appended to the end. If not, the last position in the buffer will be set to the null character.
 * So it ALWAYS leaves a null terminated string in \p dest.
 *
 * \returns an ElkStr representing the destination.
 */
ElkStr elk_str_copy(size_t dst_len, char *restrict dest, ElkStr src);

/** Compare two strings character by character.
 *
 * \warning This is NOT utf-8 safe. It looks 1 byte at a time. So if you're using fancy utf-8 stuff,
 * no promises.
 *
 * \return zero (0) if the two slices are equal, less than zero (-1) if \p left alphabetically
 *     comes before \p right and greater than zero (1) if \p left is alphabetically comes after
 *     \p right.
 */
int elk_str_cmp(ElkStr left, ElkStr right);

/** Check for equality between two strings.
 *
 * We could just use \ref elk_str_cmp(), but that is doing more than necessary. If I'm just looking
 * for equal strings I can use far fewer instructions to do that.
 *
 * \warning This is NOT utf-8 safe. It looks 1 byte at a time. So if you're using fancy utf-8 stuff,
 * no promises.
 */
bool elk_str_eq(ElkStr left, ElkStr right);

/** Strip the leading and trailing white space from a string slice. */
ElkStr elk_str_strip(ElkStr input);

/** @} */ // end of str group
/*-------------------------------------------------------------------------------------------------
 *                                       String Interner
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup intern A string interner.
 *
 * @{
 */

/** Intern strings for more memory efficient storage. */
typedef struct ElkStringInterner ElkStringInterner;

/** Create a new string interner.
 *
 * \param size_exp The interner is backed by a hash table with a capacity that is a power of 2. The
 *     \p size_exp is that power of two. This value is only used initially, if the table needs to
 *     expand, it will, so it's OK to start with small values here. However, if you know it will
 *     grow larger, it's better to start larger! For most reasonable use cases, it really probably
 *     shouldn't be smaller than 5, but no checks are done for this.
 *
 * \param avg_string_size is the expected average size of the strings. This isn't really important,
 *     because we can allocate more storage if needed. But if you know you'll always be using
 *     small strings, make this a small number like 5 or 6 to prevent overallocating memory. If
 *     you aren't sure, still use a small number and the interner will grow the storage as
 *     necessary.
 *
 * \returns a pointer to an interner. If allocation fails, it will abort the program.
 */
ElkStringInterner *elk_string_interner_create(int8_t size_exp, int avg_string_size);

/** Free memory and clean up. */
void elk_string_interner_destroy(ElkStringInterner *interner);

/** Intern a string.
 *
 * \param interner must not be \c NULL.
 * \param string is the string to intern.
 *
 * \returns An \ref ElkStr. This cannot fail unless the program runs out of memory, in which case
 * it aborts the program.
 */
ElkStr elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string);

/** Intern a string.
 *
 * \param interner must not be \c NULL.
 * \param string is the string to intern.
 *
 * \returns An \ref ElkStr. This cannot fail unless the program runs out of memory, in which case
 * it aborts the program.
 */
ElkStr elk_string_interner_intern(ElkStringInterner *interner, ElkStr str);

/** @} */ // end of intern group
/*-------------------------------------------------------------------------------------------------
 *
 *                                          Memory
 *
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup memory Memory management
 *
 * Utilities for managing memory. The recommended approach is to create an allocator by declaring
 * it and using its init function, and then from there on out use the \ref generic_alloc functions
 * for allocating and freeing memory on the allocator, and for destroying the allocator. That means
 * if you ever want to change your allocation strategy for a section of code, you can just swap
 * out the code that sets up the allocator and the rest should just work.
 *
 * @{
 */
/*-------------------------------------------------------------------------------------------------
 *                                          Static Arena Allocator
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup static_arena Static Arena
 *  \ingroup memory
 *
 * A statically sized, non-growable arena allocator that works on top of a user supplied buffer.
 * @{
 */

/** A statically sized arena allocator. */
typedef struct ElkStaticArena {
    /// \cond HIDDEN
    size_t buf_size;
    size_t buf_offset;
    unsigned char *buffer;
    /// \endcond HIDDEN
} ElkStaticArena;

/** Initialize the static arena with a user supplied buffer. */
static inline void
elk_static_arena_init(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[])
{
    assert(arena);
    assert(buffer);

    *arena = (ElkStaticArena){.buf_size = buf_size, .buf_offset = 0, .buffer = buffer};
    return;
}

/** Destroy & cleanup the static arena.
 *
 * Currently this is a no-op but the function is provided for symmetry and use in type generic
 * macros.
 */
static inline void
elk_static_arena_destroy(ElkStaticArena *arena)
{
    // no-op
    return;
}

/** Reset the arena.
 *
 * Starts allocating from the beginning and invalidates all memory and pointers allocated before
 * this call.
 */
static inline void
elk_static_arena_reset(ElkStaticArena *arena)
{
    assert(arena && arena->buffer);
    arena->buf_offset = 0;
    return;
}

/** Do an allocation on the arena.
 *
 * \param size is the size of the requested allocation in bytes.
 * \param alignment is the alignment required of the memory. It must be a power of 2 and is most
 *        is most easily assigned by just passing \c _Alignof() for the type of the memory will be
 *        used for.
 * \returns a pointer to the aligned memory or \c NULL if there isn't enough memory in the buffer.
 */
void *elk_static_arena_alloc(ElkStaticArena *arena, size_t size, size_t alignment);

/** Free an allocation from the arena.
 *
 * Currently this is implemented as a no-op.
 *
 * A future implementation may actually free this allocation IF it was the last allocation that
 * was made. This would allow the arena to behave like a stack if the allocation pattern is just
 * right.
 */
static inline void
elk_static_arena_free(ElkStaticArena *arena, void *ptr)
{
    // no-op - we don't own the buffer!
    return;
}

/** Convenience macro for allocating an object on a static arena. */
#define elk_static_arena_malloc(arena, type)                                                       \
    (type *)elk_static_arena_alloc((arena), sizeof(type), _Alignof(type))

/** Convenience macro for allocating an array on a static arena. */
#define elk_static_arena_nmalloc(arena, count, type)                                               \
    (type *)elk_static_arena_alloc((arena), (count) * sizeof(type), _Alignof(type))

/** @} */ // end of static_arena group
/*-------------------------------------------------------------------------------------------------
 *                                     Growable Arena Allocator
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup growable_arena Arena Allocator
 *  \ingroup memory
 *
 * A growable arena allocator.
 * @{
 */

/** An arena allocator that adds blocks as needed. */
typedef struct ElkArenaAllocator {
    /// \cond HIDDEN
    ElkStaticArena head;
    /// \endcond HIDDEN
} ElkArenaAllocator;

/** Initialize an arena allocator.
 *
 * If this fails, it aborts. If the machine runs out of memory, it aborts.
 *
 * \param starting_block_size this is the minimum block size that it will use if it needs to expand.
 *        If however a larger allocation is ever requested than the block size, that will become
 *        the new block size.
 */
void elk_arena_init(ElkArenaAllocator *arena, size_t starting_block_size);

/** Reset the arena.
 *
 * This is useful if you don't want to return the memory to the OS because you will reuse it soon,
 * e.g. in a loop, but you're done with the objects allocated off it so far.
 *
 * If there are multiple blocks allocated in the arena, they will be freed and a new block the same
 * size as the sum of all the previous blocks will be allocated in their place. If you want the
 * memory to shrink, then use \ref elk_arena_destroy() followed by a call to \ref elk_arena_init()
 * to ensure that it doesn't remain larger than needed.
 *
 * TODO: Consider adding a function that resets and only keeps the first page as a way to keep the
 * arena from gobbling up memory and returning it.
 */
void elk_arena_reset(ElkArenaAllocator *arena);

/** Free all memory associated with this arena.
 *
 * It is unusable after this operation, but you can put it through \ref elk_arena_init() again if
 * you want. You may prefer to destroy and reinitialize an arena if you want it to shrink back to
 * the originally requested block size. \ref elk_arena_reset() will always use keep the arena at the
 * largest size used so far, so this is also a way to reclaim memory.
 */
void elk_arena_destroy(ElkArenaAllocator *arena);

/** Free an allocation from the arena.
 *
 * Currently this is implemented as a no-op.
 *
 * A future implementation may actually free this allocation IF it was the last allocation that
 * was made. This would allow the arena to behave like a stack if the allocation pattern is just
 * right.
 */
static inline void
elk_arena_free(ElkStaticArena *arena, void *ptr)
{
    // no-op - we don't own the buffer!
    return;
}

/** Make an allocation on the \p arena.
 *
 * \param arena is the arena to use.
 * \param bytes is the size in bytes of the requested allocation.
 * \param alignment the required alignment for the allocation. This must be a power of 2. The power
 *  of 2 requirement is checked for in an assert, so it can be compiled out with -DNDEBUG.
 *
 *  \returns a pointer to a memory block of the requested size and alignment. If there isn't enough
 *  space, then a new block of at least the required size is allocated from the operating system.
 *  If the requested memory isn't available from the operating system, then this function aborts
 *  the program.
 */
void *elk_arena_alloc(ElkArenaAllocator *arena, size_t bytes, size_t alignment);

/** Convenience macro for allocating an object on an arena. */
#define elk_arena_malloc(arena, type) (type *)elk_arena_alloc(arena, sizeof(type), _Alignof(type))

/** Convenience macro for allocating an array on an arena. */
#define elk_arena_nmalloc(arena, count, type)                                                      \
    (type *)elk_arena_alloc(arena, (count) * sizeof(type), _Alignof(type))

/** @} */ // end of growable_arena group
/*-------------------------------------------------------------------------------------------------
 *                                      Pool Allocator
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup static_pool Static Pool Allocator
 *  \ingroup memory
 *
 * A static pool allocator that does not grow.
 * @{
 */

/** A static pool allocator.
 *
 * A pool stores objects all of the same size and alignment. This pool implementation does NOT
 * automatically expand if it runs out of space; it is statically sized at runtime.
 */
typedef struct ElkStaticPool {
    /// \cond HIDDEN
    size_t object_size;    // The size of each object.
    size_t num_objects;    // The capacity, or number of objects storable in the pool.
    void *free;            // The head of a free list of available slots for objects.
    unsigned char *buffer; // The buffer we actually store the data in.
    /// \endcond HIDDEN
} ElkStaticPool;

/** Initialize a static pool allocator.
 *
 * If this fails, it aborts. If the machine runs out of memory, it aborts.
 *
 * This is an error prone and brittle type. If you get it all working, a refactor or code edit
 * later is likely to break it.
 *
 * \warning It is the user's responsibility to make sure that there is at least
 * \p object_size * \p num_objects bytes in the backing \p buffer. If that isn't true, you'll
 * probably get a seg-fault during initialization.
 *
 * \warning \p object_size must be a multiple of \c sizeof(void*) enable to ensure the buffer
 * is aligned to hold pointers also. That also means \p object_size must be at least
 * \c sizeof(void*).
 *
 * \warning It is the user's responsibility to make sure the buffer is correctly aligned for the
 * type of objects they will be storing in it. This isn't a concern if the memory came from
 * \c malloc() et al as they return memory with the most pessimistic alignment. However, if using
 * a stack allocated or static memory section, you should use an \c _Alignas to force the alignment.
 *
 * \param object_size is the size of the objects to store in the pool.
 * \param num_objects is the capacity of the pool.
 * \param buffer A user provided buffer to store the objects.
 */
void elk_static_pool_init(ElkStaticPool *pool, size_t object_size, size_t num_objects,
                          unsigned char buffer[]);

/** Reset the pool.
 *
 * This is useful if you don't want to return the memory to the OS because you will reuse it soon,
 * e.g. in a loop, but you're done with the objects.
 */
void elk_static_pool_reset(ElkStaticPool *pool);

/** Free all memory associated with this pool.
 *
 * It is unusable after this operation, but you can put it through initialize again if you want.
 * For a static pool like this, it is really a no-op because the user owns the buffer, but this
 * function is provided for symmetry with the other allocators API's.
 */
void elk_static_pool_destroy(ElkStaticPool *pool);

/** Free an allocation made on the pool. */
void elk_static_pool_free(ElkStaticPool *pool, void *ptr);

/** Make an allocation on the pool.
 *
 *  \returns a pointer to a memory block. If the pool is full, it returns \c NULL;
 */
void *elk_static_pool_alloc(ElkStaticPool *pool);

/// \cond HIDDEN
// Just a stub so that it will work in the generic macros.
static inline void *
elk_static_pool_alloc_aligned(ElkStaticPool *pool, size_t size, size_t alignment)
{
    assert(pool && pool->object_size == size);
    return elk_static_pool_alloc(pool);
}
/// \endcond HIDDEN
/** @} */ // end of static_pool group
/*-------------------------------------------------------------------------------------------------
 *                                      Generic Allocator API
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup generic_alloc Generic Allocator API
 *  \ingroup memory
 *
 * A C11 _Generic based API for swapping out allocators.
 *
 * The API doesn't include any initialization functions, those are unique enough to each allocator
 * and they are tied closely to the type.
 * @{
 */

/// \cond HIDDEN

// These are stubs for the default case in the generic macros below that have the same API, these
// are there so that it compiles, but it will abort the program. FAIL FAST.

static inline void
elk_panic_allocator_reset(void *alloc)
{
    Panic("allocator not configured: %s", __FUNCTION__);
}

static inline void
elk_panic_allocator_destroy(void *arena)
{
    Panic("allocator not configured: %s", __FUNCTION__);
}

static inline void
elk_panic_allocator_free(void *arena, void *ptr)
{
    Panic("allocator not configured: %s", __FUNCTION__);
}

static inline void
elk_panic_allocator_alloc_aligned(void *arena, size_t size, size_t alignment)
{
    Panic("allocator not configured: %s", __FUNCTION__);
}

/// \endcond HIDDEN

// clang-format off

/** Allocate an item.
 *
 * \param type is the type of object the memory location will be used for.
 */
#define elk_allocator_malloc(alloc, type) (type *)_Generic((alloc),                                \
        ElkStaticArena*: elk_static_arena_alloc,                                                   \
        ElkArenaAllocator*: elk_arena_alloc,                                                       \
        ElkStaticPool*: elk_static_pool_alloc_aligned,                                             \
        default: elk_panic_allocator_alloc_aligned)(alloc, sizeof(type), _Alignof(type))

/** Allocate an array.
 *
 * \param count is the number of items that will be in the array.
 * \param type is the type of the items that will be in the array.
 */
#define elk_allocator_nmalloc(alloc, count, type) (type *)_Generic((alloc),                        \
        ElkStaticArena*: elk_static_arena_alloc,                                                   \
        ElkArenaAllocator*: elk_arena_alloc,                                                       \
        ElkStaticPool*: elk_static_pool_alloc_aligned,                                             \
        default:                                                                                   \
            elk_panic_allocator_alloc_aligned)(alloc, count * sizeof(type), _Alignof(type))

/** Free an allocation made on this allocator.
 *
 * Results may vary, not all allocators support freeing. For those, it's just a no-op. 
 *
 * \warning Most (all?) of the allocators don't check to see if this pointer was originally 
 * allocated on this allocator instance. So if you send in a pointer that didn't come out of this
 * allocator, the behavior is undefined.
 */
#define elk_allocator_free(alloc, ptr) _Generic((alloc),                                           \
        ElkStaticArena*: elk_static_arena_free,                                                    \
        ElkArenaAllocator*: elk_arena_free,                                                        \
        ElkStaticPool*: elk_static_pool_free,                                                      \
        default: elk_panic_allocator_free)(alloc, ptr)

/** Reset the allocator.
 *
 * This invalidates all pointers previously returned from this allocator, but the backing memory
 * remains and can be reused.
 */
#define elk_allocator_reset(alloc) _Generic((alloc),                                               \
        ElkStaticArena*: elk_static_arena_reset,                                                   \
        ElkArenaAllocator*: elk_arena_reset,                                                       \
        ElkStaticPool*: elk_static_pool_reset,                                                     \
        default: elk_panic_allocator_reset)(alloc)

/** Destroy the allocator.
 *
 * If it has any backing memory that it got from the operating system, it will be returned via
 * \c free() here.
 */
#define elk_allocator_destroy(alloc) _Generic((alloc),                                             \
        ElkStaticArena*: elk_static_arena_destroy,                                                 \
        ElkArenaAllocator*: elk_arena_destroy,                                                     \
        ElkStaticPool*: elk_static_pool_destroy,                                                   \
        default: elk_panic_allocator_destroy)(alloc)

// clang-format off

/** @} */ // end of generic_alloc group
/** @} */ // end of memory group
/*-------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                         Collections
 *
 *
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup collections Collections
 *
 * Collections are any grouping of objects. This library breaks them up into ordered (sometimes 
 * called sequences) and unordered collections. Some examples of ordered collections are: arrays or
 * vectors, queues, dequeues, and heaps. Some examples of unordered collections are hashsets, 
 * hashtables or dictionaries, and bags.
 *
 * @{
 */

/*-------------------------------------------------------------------------------------------------
 *                                         
 *                                     Ordered Collections
 *
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup ordered_adjacent_collections Ordered and Adjacent Collections
 *  \ingroup collections
 *
 *  Ordered collections are things like arrays (sometimes called vectors), queues, dequeues, and 
 *  heaps. I'm taking a different approach to how I implement them. They will be implemented in two
 *  parts, the second of which might not be necessary. 
 *
 *  The first part is the ledger, which just does all the bookkeeping about how full the collection 
 *  is, which elements are valid, and the order of elements. A user can use these with their own 
 *  supplied buffer for storing objects. The ledger types only track indexes.
 *
 *  One advantage of the ledger approach is that the user can manage their own memory, allowing 
 *  them to use the most efficient allocation strategy. 
 *
 *  Another advantage of this is that if you have several parallel collections (e.g. parallel 
 *  arrays), you can use a single instance of a bookkeeping type (e.g. \ref ElkQueueLedger) to
 *  track the state of all the arrays that back it. Further, different collections in the parallel 
 *  collections can have different sized objects.
 *
 *  Complicated memory management can be a disadvantage of the ledger approach. For instance, 
 *  implementing growable collections can require shuffling objects around in the backing buffer 
 *  during the resize operation. A queue, for instance, is non-trivial to expand because you have 
 *  to account for wrap-around (assuming it is backed by a circular buffer). So not all the ledger 
 *  types APIs will directly support resizing, and even those that do will further require the user
 *  to make sure as they allocate more memory in the backing buffer, then also update the 
 *  ledger. Finally, if you want to pass the collection as a whole to a function, you'll have to
 *  pass the ledger and buffer separately or create your own composite type to package them 
 *  together in.
 *
 *  The second part is an implementation that manages memory for the user. This gives the user less
 *  control over how memory is allocated / freed, but it also burdens them less with the 
 *  responsibility of doing so. These types can reliably be expanded on demand. These types will
 *  internally use the ledger types.
 *
 *  The more full featured types that manage memory also require the use of \c void pointers for
 *  adding and retrieving data from the collections. So they are less type safe, whereas when using
 *  just the ledger types they only deal in indexes, so you can make the backing buffer have any
 *  type you want.
 *
 *  Adjacent just means that the objects are stored adjacently in memory. All of the collection
 *  ledger types must be backed by an array or a block of contiguous memory.
 *
 *  \note The resizable collections may not be implemented unless I need them. I've found that I 
 *  need dynamically expandable collections much less frequently.
 *
 * @{
 */

/** Return from a function on a ledger type that indicates the collection is empty. */
static ptrdiff_t const ELK_COLLECTION_LEDGER_EMPTY = -1;

/** Return from a function on a ledger type that indicates the collection is empty. */
static ptrdiff_t const ELK_COLLECTION_LEDGER_FULL = -2;

/** Check the index returned from a ledger type function for any errors. */
static inline bool elk_collection_ledger_error(ptrdiff_t index) { return index < 0; }
/*-------------------------------------------------------------------------------------------------
 *                                         Queue Ledger
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup queueledger Queue Ledger
 *  \ingroup ordered_collections
 *
 * The bookkeeping parts of a queue.
 *
 * @{
 */

/** A bookkeeping type for a queue. */
typedef struct ElkQueueLedger {
    /// \cond HIDDEN
    size_t capacity;
    size_t length;
    size_t front;
    size_t back;
    /// \endcond HIDDEN
} ElkQueueLedger;

/** Create a ledger for a queue with \p capacity. */
static inline ElkQueueLedger
elk_queue_ledger_create(size_t capacity)
{
    return (ElkQueueLedger){.capacity = capacity, .length=0, .front=0, .back=0};
}

/** Is the queue full? */
static inline bool 
elk_queue_ledger_full(ElkQueueLedger *queue)
{ 
    return queue->length == queue->capacity;
}

/** Is the queue empty? */
static inline bool
elk_queue_ledger_empty(ElkQueueLedger *queue)
{ 
    return queue->length == 0;
}

/** Get the index to place the next item to be pushed back. 
 *
 * This assumes you actually copy/move the item into the buffer, and so on the next call it will 
 * return the next index position!
 *
 * \returns The index of the next position to put something into the queue. If the queue is full,
 * then it returns \ref ELK_COLLECTION_LEDGER_FULL.
 */
static inline ptrdiff_t
elk_queue_ledger_push_back_index(ElkQueueLedger *queue)
{
    assert(queue);
    if(elk_queue_ledger_full(queue)) return ELK_COLLECTION_LEDGER_FULL;

    ptrdiff_t idx = queue->back % queue->capacity;
    queue->back += 1;
    queue->length += 1;
    return idx;
}

/** Get the index of the next item to take out of the queue.
 *
 * This assumes you actually copy/move the item out of the buffer, and so on the next call it will
 * return the next index position!
 *
 * \returns The index of the next position to remove something from the queue. If the queue is 
 * empty it returns \ref ELK_COLLECTION_LEDGER_EMPTY.
 */
static inline ptrdiff_t
elk_queue_ledger_pop_front_index(ElkQueueLedger *queue)
{
    if(elk_queue_ledger_empty(queue)) return ELK_COLLECTION_LEDGER_EMPTY;

    ptrdiff_t idx = queue->front % queue->capacity;
    queue->front += 1;
    queue->length -= 1;
    return idx;
}

/** Get the index of the next item to take out of the queue, without advancing it.
 *
 * This assumes you DO NOT copy/move the item out of the buffer so it remains at the head.
 *
 * \returns The index of the next position to remove something from the queue without removing. If
 * the queue is empty it returns \ref ELK_COLLECTION_LEDGER_EMPTY.
 */
static inline ptrdiff_t
elk_queue_ledger_peek_front_index(ElkQueueLedger *queue)
{
    if(queue->length == 0) return ELK_COLLECTION_LEDGER_EMPTY;
    return queue->front % queue->capacity;
}

/** Get the number of items in the queue. */
static inline size_t
elk_queue_ledger_len(ElkQueueLedger const *queue)
{
    return queue->length;
}

/** @} */ // end of queueledger group
/*-------------------------------------------------------------------------------------------------
 *                                            Array Ledger
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup arrayledger Array Ledger
 *  \ingroup ordered_collections
 *
 * The bookkeeping parts of an array.
 *
 * @{
 */

/** A bookkeeping type for an array. */
typedef struct ElkArrayLedger {
    /// \cond HIDDEN
    size_t capacity;
    size_t length;
    /// \endcond HIDDEN
} ElkArrayLedger;

/** Create a ledger for an array with \p capacity. */
static inline ElkArrayLedger
elk_array_ledger_create(size_t capacity)
{
    return (ElkArrayLedger){.capacity = capacity, .length=0};
}

/** Is the array full? */
static inline bool 
elk_array_ledger_full(ElkArrayLedger *array)
{ 
    return array->length == array->capacity;
}

/** Is the array empty? */
static inline bool
elk_array_ledger_empty(ElkArrayLedger *array)
{ 
    return array->length == 0;
}

/** Get the index to place the next item to be pushed back. 
 *
 * This assumes you actually copy/move the item into the buffer, and so on the next call it will 
 * return the next index position!
 *
 * \returns The index of the next position to put something into the array. If the array is full,
 * then it returns \ref ELK_COLLECTION_LEDGER_FULL.
 */
static inline ptrdiff_t
elk_array_ledger_push_back_index(ElkArrayLedger *array)
{
    assert(array);
    if(elk_array_ledger_full(array)) return ELK_COLLECTION_LEDGER_FULL;

    ptrdiff_t idx = array->length;
    array->length += 1;
    return idx;
}

/** Get the number of items in the array. */
static inline size_t
elk_array_ledger_len(ElkArrayLedger const *array)
{
    assert(array);
    return array->length;
}

/** Reset the array so it's empty. */
static inline void
elk_array_ledger_reset(ElkArrayLedger *array)
{
    assert(array);
    array->length = 0;
}

/** Change the capcity of the array.
 *
 * \warning Make sure you adjust the size of the backing buffer before you do this!
 */
static inline void
elk_array_ledger_set_capacity(ElkArrayLedger *array, size_t capacity)
{
    assert(array);
    array->capacity = capacity;
}

/** @} */ // end of arrayledger group
/** @} */ // end of ordered_adjacent_collections.
/** @} */ // end of collections group
