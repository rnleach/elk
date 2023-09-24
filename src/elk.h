#pragma once
/**
 * \file elk.h
 * \brief A source library of useful code.
 *
 * See the main page for an overall description and the list of goals/non-goals: \ref index
 */
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define ErrorIf(assertion, error_action, ...)                                                      \
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
#define StopIf(assertion, error_action)                                                            \
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

/// \cond HIDDEN
extern int64_t const SECONDS_PER_MINUTE;
extern int64_t const MINUTES_PER_HOUR;
extern int64_t const HOURS_PER_DAY;
extern int64_t const DAYS_PER_YEAR;
extern int64_t const SECONDS_PER_HOUR;
extern int64_t const SECONDS_PER_DAY;
extern int64_t const SECONDS_PER_YEAR;

// Days in a year up to beginning of month
extern int64_t const sum_days_to_month[2][13];

inline int64_t
elk_num_leap_years_since_epoch(int64_t year)
{
    assert(year >= 1);

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

/// \endcond HIDDEN

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
inline int64_t
elk_time_to_unix_epoch(ElkTime time)
{
    return time - elk_unix_epoch_timestamp;
}

/** Convert an Unxi timestamp to an \ref ElkTime. */
inline ElkTime
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
inline bool
elk_is_leap_year(int year)
{
    if (year % 4 != 0)
        return false;
    if (year % 100 == 0 && year % 400 != 0)
        return false;
    return true;
}

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
inline ElkTime
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

/** Convert from \ref ElkStructTime to \ref ElkTime. */
inline ElkTime
elk_make_time(ElkStructTime tm)
{
    return elk_time_from_ymd_and_hms(tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
}

/** Convert from \ref ElkTime to \ref ElkRefTime. */
inline ElkStructTime
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

/** Truncate the minutes and seconds from the \p time argument.
 *
 * \param time is the time you want truncated or rounded down (back) to the most recent hour.
 *
 * \returns the time truncated, or rounded down, to the most recent hour.
 */
inline ElkTime
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
inline ElkTime
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
inline ElkTime
elk_time_add(ElkTime time, int change_in_time)
{
    ElkTime result = time + change_in_time;
    assert(result >= 0);
    return result;
}

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
inline uint64_t
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
inline uint64_t
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
inline ElkStr
elk_str_from_cstring(char *src)
{
    assert(src);

    size_t len;
    for (len = 0; *(src + len) != '\0'; ++len)
        ; // intentionally left blank.
    return (ElkStr){.start = src, .len = len};
}

/** Copy a string into a buffer.
 *
 * Copies the string referred to by \p src into the buffer \p dest. If the buffer isn't big enough,
 * then it copies as much as it can. If the buffer is large enough, a null character will be
 * appended to the end. If not, the last position in the buffer will be set to the null character.
 * So it ALWAYS leaves a null terminated string in \p dest.
 *
 * \returns an ElkStr representing the destination.
 */
inline ElkStr
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

/** Compare two strings character by character.
 *
 * \warning This is NOT utf-8 safe. It looks 1 byte at a time. So if you're using fancy utf-8 stuff,
 * no promises.
 *
 * \return zero (0) if the two slices are equal, less than zero (-1) if \p left alphabetically
 *     comes before \p right and greater than zero (1) if \p left is alphabetically comes after
 *     \p right.
 */
inline int
elk_str_cmp(ElkStr left, ElkStr right)
{
    assert(left.start && right.start);

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

/** Check for equality between two strings.
 *
 * We could just use \ref elk_str_cmp(), but that is doing more than necessary. If I'm just looking
 * for equal strings I can use far fewer instructions to do that.
 *
 * \warning This is NOT utf-8 safe. It looks 1 byte at a time. So if you're using fancy utf-8 stuff,
 * no promises.
 */
inline bool
elk_str_eq(ElkStr left, ElkStr right)
{
    assert(left.start && right.start);

    if (left.len != right.len)
        return false;

    size_t len = left.len > right.len ? right.len : left.len;

    for (size_t i = 0; i < len; ++i) {
        if (left.start[i] != right.start[i])
            return false;
    }

    return true;
}

/** Strip the leading and trailing white space from a string slice. */
inline ElkStr
elk_str_strip(ElkStr input)
{
    char *const start = input.start;
    int start_offset = 0;
    for (start_offset = 0; start_offset < input.len; ++start_offset) {
        if (start[start_offset] > 0x20)
            break;
    }

    int end_offset = 0;
    for (end_offset = input.len - 1; end_offset > start_offset; --end_offset) {
        if (start[end_offset] > 0x20)
            break;
    }

    return (ElkStr){.start = &start[start_offset], .len = end_offset - start_offset + 1};
}

/** Parse an unsigned integer.
 *
 * \returns \c false on failure and \p result is left untouched. \c true on success and \p out is
 *          the result.
 */
bool elk_str_parse_int_64(ElkStr str, int64_t *result);

/** Parse a double (64 bit floating point).
 *
 * \returns \c false on failure and \p out is \c NAN. \c true on success and \p out may be a number
 *             or \c INFINITY or \c -INFINITY or \c NAN.
 */
bool elk_str_parse_float_64(ElkStr str, double *out);

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

/// \cond HIDDEN
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

    assert(elk_is_power_of_2(align));

    uintptr_t a = (uintptr_t)align;
    uintptr_t mod = ptr & (a - 1); // Same as (ptr % a) but faster as 'a' is a power of 2

    if (mod != 0) {
        // push the address forward to the next value which is aligned
        ptr += a - mod;
    }

    return ptr;
}

/// \endcond HIDDEN

/** Initialize the static arena with a user supplied buffer. */
inline void
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
inline void *
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

/// \cond HIDDEN

inline void
elk_arena_add_block(ElkArenaAllocator *arena, size_t block_size)
{
    uint32_t max_block_size = arena->head.buf_size > block_size ? arena->head.buf_size : block_size;
    assert(max_block_size > sizeof(ElkStaticArena));

    unsigned char *buffer = calloc(max_block_size, 1);
    PanicIf(!buffer, "out of memory");

    ElkStaticArena next = arena->head;

    elk_static_arena_init(&arena->head, max_block_size, buffer);
    ElkStaticArena *next_ptr =
        elk_static_arena_alloc(&arena->head, sizeof(ElkStaticArena), _Alignof(ElkStaticArena));
    assert(next_ptr);
    *next_ptr = next;
}

inline void
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

/// \endcond HIDDEN

/** Initialize an arena allocator.
 *
 * If this fails, it aborts. If the machine runs out of memory, it aborts.
 *
 * \param starting_block_size this is the minimum block size that it will use if it needs to expand.
 *        If however a larger allocation is ever requested than the block size, that will become
 *        the new block size.
 */
inline void
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
inline void
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

/** Free all memory associated with this arena.
 *
 * It is unusable after this operation, but you can put it through \ref elk_arena_init() again if
 * you want. You may prefer to destroy and reinitialize an arena if you want it to shrink back to
 * the originally requested block size. \ref elk_arena_reset() will always use keep the arena at the
 * largest size used so far, so this is also a way to reclaim memory.
 */
inline void
elk_arena_destroy(ElkArenaAllocator *arena)
{
    assert(arena);
    elk_arena_free_blocks(arena);
    arena->head.buf_size = 0;
    arena->head.buf_offset = 0;
}

/** Free an allocation from the arena.
 *
 * Currently this is implemented as a no-op.
 *
 * TODO: A future implementation may actually free this allocation IF it was the last allocation
 * that was made. This would allow the arena to behave like a stack if the allocation pattern is
 * just right.
 */
static inline void
elk_arena_free(ElkStaticArena *arena, void *ptr)
{
    // no-op
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
inline void *
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

/** @} */ // end of growable_arena group
/*-------------------------------------------------------------------------------------------------
 *                                   Static Pool Allocator
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

/// \cond HIDDEN

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
    while (offset) {
        size_t next_offset = offset;
        offset -= object_size;
        ptr = (uintptr_t *)&buffer[offset];
        uintptr_t next = (uintptr_t)&buffer[next_offset];
        *ptr = next;
    }
}

/// \endcond HIDDEN

/** Reset the pool.
 *
 * This is useful if you don't want to return the memory to the OS because you will reuse it soon,
 * e.g. in a loop, but you're done with the objects.
 */
inline void
elk_static_pool_reset(ElkStaticPool *pool)
{
    assert(pool && pool->buffer && pool->num_objects && pool->object_size);

    // Initialize the free list to a linked list.
    elk_static_pool_initialize_linked_list(pool->buffer, pool->object_size, pool->num_objects);
    pool->free = &pool->buffer[0];
}

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
inline void
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

/** Free all memory associated with this pool.
 *
 * It is unusable after this operation, but you can put it through initialize again if you want.
 * For a static pool like this, it is really a no-op because the user owns the buffer, but this
 * function is provided for symmetry with the other allocators API's.
 */
inline void
elk_static_pool_destroy(ElkStaticPool *pool)
{
    assert(pool);
    memset(pool, 0, sizeof(*pool));
}

/** Free an allocation made on the pool. */
inline void
elk_static_pool_free(ElkStaticPool *pool, void *ptr)
{
    assert(pool && ptr);

    uintptr_t *next = ptr;
    *next = (uintptr_t)pool->free;
    pool->free = ptr;
}

/** Make an allocation on the pool.
 *
 *  \returns a pointer to a memory block. If the pool is full, it returns \c NULL;
 */
inline void *
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
extern ptrdiff_t const ELK_COLLECTION_LEDGER_EMPTY;

/** Return from a function on a ledger type that indicates the collection is empty. */
extern ptrdiff_t const ELK_COLLECTION_LEDGER_FULL;

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
