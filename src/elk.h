#pragma once
/**
 * \file elk.h
 * \brief A source library of useful code.
 *
 * See the main page for an overall description and the list of goals/non-goals: \ref index
 */
#include <assert.h>
#include <stdbool.h>
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
 *                                       String Interner
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup intern A string interner.
 *
 * @{
 */

/** Intern strings for more memory efficient storage. */
typedef struct ElkStringInterner ElkStringInterner;

/** A handle to an interned string.
 *
 *  Values of this type are unique to a particular \ref ElkStringInterner, but if you have multiple
 *  \ref ElkStringInterner objects, you cannot swap handles back and forth. Each handle is
 *  associated with a particular \ref ElkStringInterner object.
 */
typedef uint32_t ElkInternedString;

/** Create a new string interner.
 *
 * \param size_exp The interner is backed by a hash table with a capacity that is a power of 2. The
 *                 \p size_exp is that power of two. This value is only used initially, if the table
 *                 needs to expand, it will, so it's OK to start with small values here. However, if
 *                 you know it will grow larger, it's better to start larger! For most reasonable
 *                 use cases, it really probably shouldn't be smaller than 5, but no checks are done
 *                 for this.
 *
 * \param avg_string_size is the expected average size of the strings. This isn't really important,
 *                        because we can reallocate the storage if needed. But if you know you'll
 *                        always be using small strings, make this a small number like 5 or 6 to
 *                        prevent overallocating memory. If you aren't sure, still use a small
 *                        number and the interner will grow the storage as necessary.
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
 * \returns a handle to the interned string that can be used to retrieve it later. This cannot fail
 * unless the program runs out of memory, in which case it aborts the probram.
 */
ElkInternedString elk_string_interner_intern(ElkStringInterner *interner, char const *string);

/** Fetch an interned string.
 *
 * \param interner must not be \c NULL.
 * \param handle is the handle to the interned string. This value must be a value that came from
 *        \ref elk_string_interner_intern().
 *
 * \returns an alias to the string. The \p interner owns this string, so do not try to free it. If
 *          you use an invalid handle that didn't come from a previous call to
 *          \ref elk_string_interner_intern(), then it may return \c NULL or it may return in the
 *          middle of a random string. If \c NDEBUG is not defined, then \c assert calls will be
 *          used to do some checks which will help if you're debugging.
 */
char const *elk_string_interner_retrieve(ElkStringInterner const *interner,
                                         ElkInternedString const handle);

/** @} */ // end of intern group
/*-------------------------------------------------------------------------------------------------
 *
 *                                          Memory
 *
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup memory Memory management
 *
 * Utilities for managing memory.
 *
 * @{
 */
/*-------------------------------------------------------------------------------------------------
 *                                          Static Arena Allocator
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup static_arena Statically Sized Arena
 *  \ingroup memory
 *
 * A statically sized, non-growable arena allocator.
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
elk_static_arena_init(ElkStaticArena *arena, size_t buf_size, unsigned char buffer[buf_size])
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
elk_static_arena_free(ElkStaticArena *arena)
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
/** \defgroup growable_arena Arena
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
/** A pool allocator.
 *
 * A pool stores objects all of the same size and alignment. This pool implementation does NOT
 * automatically expand if it runs out of space.
 */
typedef struct ElkPoolAllocator {
    /// \cond HIDDEN
    size_t object_size;
    size_t num_objects;
    void *free;
    unsigned char *buffer;
    /// \endcond HIDDEN
} ElkPoolAllocator;

/** Initialize a pool allocator.
 *
 * If this fails, it aborts. If the machine runs out of memory, it aborts.
 *
 * \param pool The pool to initialize. This cannot be \c NULL.
 * \param object_size is the size of the objects to store in the pool.
 * \param num_objects is the capacity of the pool.
 */
void elk_pool_initialize(ElkPoolAllocator *pool, size_t object_size, size_t num_objects);

/** Reset the pool.
 *
 * This is useful if you don't want to return the memory to the OS because you will reuse it soon,
 * e.g. in a loop, but you're done with the objects.
 */
void elk_pool_reset(ElkPoolAllocator *pool);

/** Free all memory associated with this pool.
 *
 * It is unusable after this operation, but you can put it through initialize again if you want.
 */
void elk_pool_destroy(ElkPoolAllocator *pool);

/** Make an allocation on the \p pool.
 *
 * \param pool is the pool to use.
 *
 *  \returns a pointer to a memory block. If the pool is full, it returns \c NULL;
 */
void *elk_pool_alloc(ElkPoolAllocator *pool);

/** Free an allocation made on the pool. */
void elk_pool_free(ElkPoolAllocator *pool, void *ptr);
/** @} */ // end of memory group
