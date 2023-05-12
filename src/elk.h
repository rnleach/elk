#pragma once
/**
 * \file elk.h
 * \brief A source library of useful code.
 *
 * See the main page for an overall description and the list of goals/non-goals: \ref index
 */
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
#define QuietStopIf(assertion, error_action)                                                       \
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

/** \defgroup errorcodes Error codes returned by some Elk functions.
 *
 * @{
 */
typedef enum ElkCode {
    ELK_CODE_FULL,          /**< The collection is full and can't accept anymore items. */
    ELK_CODE_EMPTY,         /**< The collection is empty and can't return anymore items. */
    ELK_CODE_INVALID_INDEX, /**< An index provided was invalid and potentially out of bounds. */
    ELK_CODE_EARLY_TERM,    /**< An algorithm terminated early for some reason. */
    ELK_CODE_SUCCESS,       /**< There was no error, only success! */
} ElkCode;

/** Test to see if a code indicates an error. */
static inline bool
elk_is_error(ElkCode code)
{
    return code < ELK_CODE_SUCCESS;
}

/** @} */ // end of errorcodes group
/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup time Time and Dates
 *
 * Time and date functions are generally not thread safe due to their reliance on the C standard
 * library.
 * @{
 */

/** Create a \c time_t object given the date and time information.
 *
 * WARNING: The standard C library functions that this function depends on are not reentrant or
 * threadsafe, so neither is this function.
 *
 * NOTE: All inputs are in UTC.
 *
 * \param year is the 4 digit year.
 * \param month is the month number [1,12].
 * \param day is the day of the month [1,31].
 * \param hour is the hour of the day [0, 23].
 * \param minute is the minute of the day [0, 59].
 *
 * \returns The system representation of that time as a \c time_t. This is usually (but not
 * guaranteed to be) the seconds since midnight UTC on January 1st, 1970. The tests for this
 * library test to make sure it is the seconds since the Unix epoch.
 */
time_t elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds);

/** Truncate the minutes and seconds from the \p time argument.
 *
 * WARNING: The standard C library functions that this function depends on are not reentrant or
 * threadsafe, so neither is this function.
 *
 * NOTE: The non-posix version of this function depends on transformations to and from local time.
 * If you're in a timezone without a whole hour offset from UTC, this won't round you down to a
 * whole hour in UTC. At the time I wrote this comment, their are approximately 10 such time zones
 * world wide that use half hour or 45 minute offsets.
 *
 * \param time is the time you want truncated or rounded down (back) to the most recent hour.
 *
 * \returns the time truncated, or rounded down, to the most recent hour.
 */
time_t elk_time_truncate_to_hour(time_t time);

/** Round backwards in time until the desired hour is reached.
 *
 * WARNING: The standard C library functions that this function depends on are not reentrant or
 * threadsafe, so neither is this function.
 *
 * NOTE: The non-posix version of this function depends on transformations to and from local time.
 * If you're in a timezone without a whole hour offset from UTC, this won't round you down to a
 * whole hour in UTC. At the time I wrote this comment, their are approximately 10 such time zones
 * world wide that use half hour or 45 minute offsets.
 *
 * \param time is the time you want truncated or rounded down (backwards) to the requested
 *            (\p hour).
 * \param hour is the hour you want to round backwards to. This will go back to the previous day
 *             if necessary. This always assumes you're working in UTC.
 *
 * \returns the time truncated, or rounded down, to the requested hour, even if it has to go back a
 * day.
 */
time_t elk_time_truncate_to_specific_hour(time_t time, int hour);

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
    ElkWeek = 60 * 60 * 27 * 7,
} ElkTimeUnit;

/** Add a change in time.
 *
 * The type \ref ElkTimeUnit can be multiplied by an integer to make \p change_in_time more
 * readable. For subtraction, just use a negative sign on the change_in_time.
 */
time_t elk_time_add(time_t time, int change_in_time);

/** @} */ // end of time group
/*-------------------------------------------------------------------------------------------------
 *                                       Memory and Pointers
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup memory Memory management.
 *
 * Functions and macros for managing and debugging memory.
 *
 * The elk_init_memory_debug(), elk_finalize_memory_debug(), and elk_debug_mem() functions are null
 * functions unless the \c ELK_MEMORY_DEBUG macro is defined. When that macro is defined, malloc(),
 * realloc(), calloc(), and free() from stdlib.h are usurped by macros that keep track of memory
 * allocations.
 *
 * elk_init_memory_debug() can optionally check a mutex to prevent data races in a multithreaded
 * application, but it is the user's responsibility to set that up.
 *
 * @{
 */

/** Steal a pointer.
 *
 * This is useful for tracking ownership. To move an object by pointer, steal the pointer using this
 * function, and the original value will be set to \c NULL.
 */
static inline void *
elk_steal_ptr(void **ptr)
{
    void *item = *ptr;
    *ptr = NULL;
    return item;
}

/** Initialize the memory debugging system.
 *
 * This function is defined as an empty function unless the macro ELK_MEMORY_DEBUG is defined.
 *
 * Since this is only used for debugging, the mutex set up and teardown can be guarded by defines
 * and you can use functions that panic/abort if any part of the setup / teardown, locking,
 * unlocking fail on a given platform.
 *
 * \param mutex a global mutex for preventing data races in multithreaded applications. \c NULL is
 *        allowed if no global locking is needed.
 * \param lock a function to lock the \p mutex. \c NULL is allowed if no global locking is needed.
 * \param unlock a function to unlock the \p mutex. \c NULL is allowed if no global locking is
 *        needed.
 */
void elk_init_memory_debug(void *mutex, void (*lock)(void *), void (*unlock)(void *));

/** Finalize and clean up the memory debugging system.
 *
 * This function is defined as an empty function unless the macro ELK_MEMORY_DEBUG is defined.
 */
void elk_finalize_memory_debug();

/** Run the debug checks with the current state of the application's memory.
 *
 * This function is defined as an empty function unless the macro ELK_MEMORY_DEBUG is defined.
 *
 * If a buffer overrun is detected, this function will crash the program immediately. Otherwise, it
 * will print a list of "active" pointers and the file and line where they were allocated, and some
 * summary statistics of allocations.
 */
void elk_debug_mem();

/// \cond HIDDEN

/** Replacement for malloc when ELK_MEMORY_DEBUG is defined. */
void *elk_malloc(size_t size, char const *fname, char const *func, unsigned line);

/** Replacement for realloc when ELK_MEMORY_DEBUG is defined. */
void *elk_realloc(void *ptr, size_t size, char const *fname, char const *func, unsigned line);

/** Replacement for calloc when ELK_MEMORY_DEBUG is defined. */
void *elk_calloc(size_t nmemb, size_t size, char const *fname, char const *func, unsigned line);

/** Replacement for free when ELK_MEMORY_DEBUG is defined. */
void elk_free(void *ptr, char const *fname, char const *func, unsigned line);

#ifdef ELK_MEMORY_DEBUG
#    define malloc(s) elk_malloc((s), __FILE__, __func__, __LINE__)
#    define realloc(p, s) elk_realloc((p), (s), __FILE__, __func__, __LINE__)
#    define calloc(n, s) elk_calloc((n), (s), __FILE__, __func__, __LINE__)
#    define free(p) elk_free((p), __FILE__, __func__, __LINE__)
#endif

/// \endcond HIDDEN

/** @} */ // end of memory group
/*-------------------------------------------------------------------------------------------------
 *                                    Iterator Functions
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup iterator Iterator interface.
 *
 * This section defines the iterator interface for collections defined in Elk.
 *
 * @{
 */

/** An iterator function.
 *
 * This is a prototype for the functions you can pass into a collection when you're iterating over
 * it's members.
 *
 * IMPORTANT: These functions should not modify the collection they are iterating over, but they
 * can modify the elements in the collection. That is, the values of the element can be changed, but
 * elements should not be added to, removed from, or swapped around in the collection.
 *
 * \param item is the member of the collection that you're iterating over.
 * \param user_data is the same object that will be passed to the function on every call. Think of
 * it as an accumulator.
 *
 * \returns \c true if the iteration should continue and \c false if it should terminate and iterate
 * no more.
 */
typedef bool (*IterFunc)(void *item, void *user_data);

/** A filter or selection function.
 *
 * This is a prototype for the functions you can pass into a collection when you're filtering or
 * selecting items based on a criteria.
 *
 * IMPORTANT: These functions should not modify the collection they are iterating over or the
 * elements in the collection.
 *
 * \param item is the member of the collection that you're iterating over.
 * \param user_data is the same object that will be passed to the function on every call.
 *
 * \returns \c true or \c false. What exactly that means depends on the context of the function
 * using the \p FilterFunc, and should be documented in the function that receives the
 * \p FilterFunc.
 */
typedef bool (*FilterFunc)(void const *item, void *user_data);

/** @} */ // end of iterator group
/*-------------------------------------------------------------------------------------------------
 *                                           ElkArray
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup array Array
 *
 * An array that dynamically grows in size as needed.
 *
 * @{
 */

/** An array that dynamically grows in size as needed.
 *
 * All the operations on this type assume the system will never run out of memory, so if it does
 * they will abort the program.
 *
 * This array assumes (and so it is only good for) storing types of constant size that do not
 * require any copy or delete functions. This list stores plain old data types (PODs). Of course
 * you could store pointers or something that contains pointers in the array, but you would be
 * responsible for managing the memory (including any dangling aliases that may be out there after
 * adding it to the array).
 *
 * The members of this struct SHOULD NOT be modified by user code. The length and data members are
 * here for convenience so you can easily read them and use them in a for loop.
 */
typedef struct ElkArray {
    /** The number of items stored in the array. Users SHOULD NOT CHANGE THIS VALUE. */
    size_t length;

    /** An "untyped" pointer to the objects in memory. */
    unsigned char *data;

    /** The size of each element in the array. */
    size_t element_size;

    /** The number of items, each of \ref element_size, that the array has space to store. */
    size_t capacity;
} ElkArray;

/** Create a new array.
 *
 * \param element_size is the size of each item in bytes.
 *
 * \returns a new array. This cannot practically fail unless the system runs out of memory. The out
 * of memory error is checked, and if the system is out of memory this function will exit the
 * process with an error message in both release and debug builds.
 */
ElkArray elk_array_new(size_t element_size);

/** Create a new array with an already known capacity.
 *
 * \param capacity is the minimum capacity the array should have when created.
 * \param element_size is the size of each item in bytes.
 *
 * \returns a new array. This cannot practically fail unless the system runs out of memory. The out
 * of memory error is checked, and if the system is out of memory this function will exit the
 * process with an error message in both release and debug builds.
 */
ElkArray elk_array_new_with_capacity(size_t capacity, size_t element_size);

/** Free all heap memory used by the array.
 *
 * \param arr is the array to clear. A \c NULL pointer is NOT acceptable.
 *
 * \returns \ref ELK_CODE_SUCCESS or crashes the program upon failure.
 */
ElkCode elk_array_clear(ElkArray *arr);

/** Clear out the contents of the array but keep the memory intact for reuse.
 *
 * The length of the array is set to zero without freeing any memory so the array is ready to use
 * again.
 *
 * \param arr is the array to clear out. The array must not be \c NULL.
 *
 * \returns ELK_CODE_SUCCESS
 */
ElkCode elk_array_reset(ElkArray *arr);

/** Append \p item to the end of the array.
 *
 * \param arr is the array to add the \p item too. It must not be \c NULL.
 * \param item is the item to add! The item is copied into place with \c memcpy(). \p item must not
 * be \c NULL.
 *
 * \returns ELK_CODE_SUCCESS or aborts the program if it runs out of memory.
 */
ElkCode elk_array_push_back(ElkArray *arr, void *item);

/** Remove an item from the back of the array.
 *
 * \param arr is the array to remove an item from. This must NOT be \c NULL.
 * \param item is a memory location to move the last item in the array into. If this is \c NULL,
 * then the element is just removed from the array and not copied anywhere. If the array is empty,
 * then this object will not be altered at all.
 *
 * \returns \ref ELK_CODE_SUCCESS on success or \ref ELK_CODE_EMPTY if there was nothing to return.
 */
ElkCode elk_array_pop_back(ElkArray *arr, void *item);

/** Remove an item from the array without preserving order.
 *
 * This will remove an item at the \p index by swapping its position with the element at the end of
 * the array and then decrementing the length of the array.
 *
 * \param arr is the array to remove the item from. \p arr must NOT be \c NULL.
 * \param index is the position in the array to remove.
 * \param item is a memory location that can hold the removed data. It will be copied here if
 * \p item isn't \c NULL.
 *
 * \returns \ref ELK_CODE_SUCCESS unless the array is empty, then it returns \ref ELK_CODE_EMPTY. If
 * the \p index is beyond the array length, the \ref ELK_CODE_INVALID_INDEX is returned.
 */
ElkCode elk_array_swap_remove(ElkArray *const arr, size_t index, void *item);

/** The number of items currently in the array.
 *
 * The same information can be retrieved directly on the array by accessing the length member of the
 * array. E.g \c arr->length or \c arr.length. This is a function instead of a macro because that
 * has the advantage of being type-safe.
 *
 * \param arr must NOT be \c NULL.
 *
 * \returns the number of items in the \p arr.
 */
static inline size_t
elk_array_length(ElkArray const *const arr)
{
    assert(arr);
    return arr->length;
}

/** Get an alias (pointer) to an item in the array.
 *
 * This is not bounds checked, so make sure you know your index exists in the array by checking
 * elk_array_length() first.
 *
 * \param arr the list is not modified in any way. That is, nothing is added or removed, however
 * the returned element may be modified. Must not be \c NULL.
 * \param index is the index of the item you want to get. If the index is out of range, it will
 * invoke undefined behavior, and if you're lucky cause a segmentation fault.
 *
 * \returns a pointer to the item at the given index. You may use this pointer to modify the
 * value at that index, but DO NOT try to free it. You don't own it.
 */
void *const elk_array_alias_index(ElkArray *const arr, size_t index);

/** Apply \p ifunc to each element of \p arr.
 *
 * This method assumes that whatever operation \p ifunc does is infallible. If that is not the case
 * then error information of some kind should be returned via the \p user_data.
 *
 * \param arr the array of items to iterate over.
 * \param ifunc is the function to apply to each item. If this function returns \c false, this
 * function will abort further evaluations. This function assumes this operation is infallible,
 * if that is not the case then any error information should be returned via \p user_data.
 * \param user_data will be supplied to \p ifunc as the second argument each time it is called.
 *
 * \returns \ref ELK_CODE_SUCCESS if it made it all the way through the list or
 * \ref ELK_CODE_EARLY_TERM if it stopped early.
 */
ElkCode elk_array_foreach(ElkArray *const arr, IterFunc ifunc, void *user_data);

/** Filter elements out of \p src and put them into \p sink.
 *
 * \param src is the array to potentially remove items from. It must not be \c NULL.
 * \param sink is the array to drop the filtered items into. If this is \c NULL then the items
 * selected to be filtered out will be removed from the \p src array.
 * \param filter, if this returns \c true, the element will be removed from the \p src list.
 * \param user_data is passed through to \p filter on each call.
 *
 * \returns \ref ELK_CODE_SUCCESS
 */
ElkCode elk_array_filter_out(ElkArray *const src, ElkArray *sink, FilterFunc filter,
                             void *user_data);

/** @} */ // end of array group
/*-------------------------------------------------------------------------------------------------
 *                                            Queue
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup queue A simple bounded queue.
 *
 * @{
 */

/** An array backed bounded queue.
 *
 * This queue stores POD types, and doesn't free any pointers internal to its data members. That
 * can be done by calling a function to free items if necessary through the elk_queue_foreach()
 * function.
 */
typedef struct ElkQueue ElkQueue;

/** Create a new queue.
 *
 * \param element_size is the size of each item in bytes.
 * \param capacity is the maximum capacity of the queue.
 *
 * \returns a pointer to the new queue.
 */
ElkQueue *elk_queue_new(size_t element_size, size_t capacity);

/** Free the memory used by this queue.
 *
 * This method does not free any memory pointed to by items in the queue. To do that create an
 * \ref IterFunc and use the elk_queue_foreach() function to free memory pointed to by members.
 */
void elk_queue_free(ElkQueue *queue);

/** Detect if the queue is full.
 *
 * \returns \c true if the \p queue is full and can't take anymore input.
 */
bool elk_queue_full(ElkQueue *queue);

/** Detect if the queue is empty.
 *
 * \returns \c true if the \p queue is empty and has nothing more to give.
 */
bool elk_queue_empty(ElkQueue *queue);

/** Add an item to the end of the queue.
 *
 * \param queue the queue to add something too. Cannot be \c NULL.
 * \param item the item to add to the queue which will be copied into the queue with \c memcpy
 *
 * \returns \ref ELK_CODE_SUCCESS if the operation succeeds, or \ref ELK_CODE_FULL if the queue is
 * full.
 */
ElkCode elk_queue_enqueue(ElkQueue *queue, void *item);

/** Take an item from the front of the queue.
 *
 * \param queue the queue to take something from. Cannot be \c NULL.
 * \param output is a location to hold the returned item. The item is moved there with \c memcpy.
 *
 * \returns \ref ELK_CODE_SUCCESS if the operation succeeds or \ref ELK_CODE_EMPTY if the queue was
 * empty.
 */
ElkCode elk_queue_dequeue(ElkQueue *queue, void *output);

/** Peek at the next item in the queue without removing it.
 *
 * \param queue the queue to peek at.
 *
 * \returns a pointer to the element at the front of the queue. If the queue is empty, then return
 * \c NULL.
 */
void const *elk_queue_peek_alias(ElkQueue *queue);

/** Get the number of items remaining in the queue.
 *
 * \param queue the queue to get the number of remaining items in. Cannot be \c NULL.
 *
 * \returns the number of items in the \p queue.
 */
size_t elk_queue_count(ElkQueue *queue);

/** Apply a function to every item in the queue while dequeueing them.
 *
 * After this call, the \p queue will be empty.
 *
 * \param queue to empty while applying \p ifunc to each item.
 * \param ifunc the function to apply to each item in the queue.
 * \param user_data is data that is passed to each call of \p ifunc.
 */
void elk_queue_foreach(ElkQueue *queue, IterFunc ifunc, void *user_data);

/** @} */ // end of queue group
/*-------------------------------------------------------------------------------------------------
 *                                    Heap or Priority Queue
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup heap A heap or priority queue.
 *
 * @{
 */

/** Function prototype for determining priority in a heap/priority queue.
 *
 * This implementation assumes a maximum-heap, so larger return values indicate higher priority and
 * will result in an item being placed nearer the top of the queue. If you need a minimum-heap just
 * flip the sign before returning from this function.
 *
 * The use of integers as the return type is intentional to avoid the pitfalls of comparing floating
 * point values with things like NaN or infinity.
 */
typedef int (*Priority)(void *item);

/** A maximum-heap.
 *
 * All the operations on this type assume the system will never run out of memory, so if it does
 * they will abort the program. The heap is backed by an array, so it should be cache friendly.
 *
 * This heap assumes (and so it is only good for) storing types of constant size that do not require
 * any copy or delete functions. This heap stores plain old data types (PODs). Of course you could
 * store pointers or something that contains pointers in the heap, but you would be responsible
 * for managing the memory (including any dangling aliases that may be out there after adding it
 * to the list).
 **/
typedef struct ElkHeap ElkHeap;

/** Create a new unbounded heap.
 *
 * This heap will grow as needed to store new elements. Watchout though, this means it can use all
 * of the machine's memory!
 *
 * \param element_size the size of elements stored in the heap.
 * \param pri the function to use to calculate an element's priority in the queue or value in the
 *        heap.
 * \param arity the number of elements per node. Usually this is just 2 for a binary heap, but this
 *        is a "d-ary" heap implementation. Good default values are in the range of 3-5.
 *
 * \returns A newly allocated heap! Upon failure it just aborts the program. If you're that
 * desperate for memory, just give up.
 */
ElkHeap *elk_heap_new(size_t element_size, Priority pri, size_t arity);

/** Create a new bounded heap.
 *
 * All of the necessary memory will be allocated up front.
 *
 * \param element_size the size of elements stored in the heap.
 * \param capacity is the maximum number of values you can store in this heap.
 * \param pri the function to use to calculate an element's priority in the queue or value in the
 *        heap.
 * \param arity the number of elements per node. Usually this is just 2 for a binary heap, but this
 *        is a "d-ary" heap implementation. Good default values are in the range of 3-5.
 *
 * \returns A newly allocated heap! Upon failure it just aborts the program. If you're that
 * desperate for memory, just give up.
 */
ElkHeap *elk_bounded_heap_new(size_t element_size, size_t capacity, Priority pri, size_t arity);

/** Free a heap.
 *
 * \returns \c NULL, which should be assigned to the original \p heap.
 */
ElkHeap *elk_heap_free(ElkHeap *heap);

/** Insert an element onto the heap.
 *
 * \param heap the heap to insert into.
 * \param item the object to be stored on the heap. It will be moved via \c memcpy, so the memory
 *        location is copied. User beware if the type uses pointers internally as this could cause
 *        aliasing.
 * \param result returns an \ref ElkCode to indicate success or failure. For unbounded heaps, this
 *        will always be \ref ELK_CODE_SUCCESS, but for bounded heaps it may return
 *        \ref ELK_CODE_FULL. Sending in \c NULL is acceptable but not advised.
 *
 * \returns a pointer to the ElkHeap. This SHOULD be reassigned to the original \p heap, especially
 * for unbounded heaps. If the heap is grown via realloc, then the pointer may change!
 */
ElkHeap *elk_heap_insert(ElkHeap *heap, void *item, ElkCode *result);

/** Remove an item from the heap.
 *
 * \param heap the heap to remove the next item from.
 * \param item A location to store the removed item into via \c memcpy. If the \p heap is empty,
 *             the memory at this location should be zeroed out.
 * \param result returns an \ref ElkCode to indicate success or failure. This will be
 *        \ref ELK_CODE_SUCCESS if an item is returned, but if the heap is empty it will be
 *        \ref ELK_CODE_EMPTY.
 *
 * \returns a pointer to the ElkHeap. This SHOULD be reassigned to the original \p heap for
 * consistency with other functions that modify heaps.
 */
ElkHeap *elk_heap_top(ElkHeap *heap, void *item, ElkCode *result);

/** Get the number of items on the heap. */
size_t elk_heap_count(ElkHeap const *const heap);

/** Peek at the next item on the top of the heap.
 *
 * \param heap the heap to peek!
 *
 * \returns an aliased pointer to the top element on the heap. This value should not be modified!
 * If the heap is empty it returns \c NULL.
 */
void const *elk_heap_peek(ElkHeap const *const heap);

/** @} */ // end of heap group
/*-------------------------------------------------------------------------------------------------
 *                                    Coordinates and Rectangles
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup geom Geometry primitives.
 *
 * @{
 */

/** A simple x-y 2 dimensional coordinate. */
typedef struct Elk2DCoord {
    double x; /**< The x coordinate. */
    double y; /**< The y cooridnate. */
} Elk2DCoord;

/** A simple x-y 2 dimensional rectangle. */
typedef struct Elk2DRect {
    Elk2DCoord ll; /**< The lower left corner of a rectangle (minimum x and minimum y) */
    Elk2DCoord ur; /**< The upper right corner of a rectangle (maximum x and maximum y) */
} Elk2DRect;

/** @} */ // end of geom group
/*-------------------------------------------------------------------------------------------------
 *                                       Hilbert Curves
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup hilbert Hilbert Curves.
 *
 * Hilbert curves are a type of space filling curve, and were originally implemented in this
 * library to support an 2-dimensional RTree implementation.
 *
 * This might seem really weird to have in a "general" library. However, as a result of goal number
 * 1 for this library, here it is. This didn't really need to be in the public API, but it makes
 * testing easier.
 *
 * I ported this code from an implementation in Python at https://github.com/galtay/hilbertcurve,
 * which is itself an implementation based on the 2004 paper "Programming the Hilbert Curve" by
 * John Skilling (http://adsabs.harvard.edu/abs/2004AIPC..707..381S)(DOI: 10.10631/1.1751381).
 *
 * @{
 */

/** A 2D Hilbert Curve.
 */
typedef struct ElkHilbertCurve ElkHilbertCurve;

/** A point in the Hilbert space. */
struct HilbertCoord {
    uint32_t x;
    uint32_t y;
};

/** Create a new Hilbert Curve description.
 *
 * The number of iterations will fill the area covered by the \p domain with and NxN grid of
 * points where N = sqrt(2^(2 * iterations)). The total number of points along the Hilbert curve
 * will be 2^(2 * iterations).
 *
 * So if iterations = 1, then the grid will be 2x2.
 *
 * If iterations = 2 then N = 4, so a 4x4 grid.
 *
 * If iterations = 3 then N = 8, so a 8x8 grid.
 *
 * If iterations = 8 then N = 256, so a 256 x 256 grid.
 *
 * If iterations = 16 then N = 65536, so a 65536 x 65536 grid.
 *
 * And if iterations = 31 then N = 2,147,483,648... so a 2,147,483,648 x 2,147,483,648 grid.
 *
 * \param iterations is a number between 1 and 31 inclusive. If the number is outside that range,
 * the program will exit and print an error message.
 *
 * \param domain is a rectanglular region to map this Hilbert curve onto.
 */
struct ElkHilbertCurve *elk_hilbert_curve_new(unsigned int iterations, Elk2DRect domain);

/** Free all memory associated with a ElkHilbertCurve
 *
 * \param hc is the object to free. A \c NULL pointer is acceptable and is ignored.
 *
 * \returns \c NULL, which should be assigned to the original object, \p hc.
 */
struct ElkHilbertCurve *elk_hilbert_curve_free(struct ElkHilbertCurve *hc);

/** Convert the distance along the curve to coordinates in the X-Y plane.
 *
 * In debug mode (no \c NDEBUG macro defined), assertions will check to make sure the distance
 * is within the allowable range for this curve.
 */
struct HilbertCoord elk_hilbert_integer_to_coords(struct ElkHilbertCurve const *hc, uint64_t hi);

/** Convert the coordinates into a distance along the Hilbert curve.
 *
 * In debug mode (compiled without \c NDEBUG macro defined), assertions will check to make sure
 * the \p coords are the allowable range for this curve.
 */
uint64_t elk_hilbert_coords_to_integer(struct ElkHilbertCurve const *hc,
                                       struct HilbertCoord coords);

/** Translate a point into the nearest set of coordinates for this Hilbert curve \p hc.
 *
 * \param hc the Hilbert curve. Must NOT be \c NULL.
 * \param coord the coordinate to translate.
 *
 * \returns the nearest coordinate in the coordinates of the Hilbert curve.
 */
struct HilbertCoord elk_hilbert_translate_to_curve_coords(struct ElkHilbertCurve *hc,
                                                          Elk2DCoord coord);

/** Translate a point to the distance along the curve to the nearest set of coordinates for this
 * Hilbert curve \p hc.
 *
 * This is basically a convenience method for calling elk_hilbert_translate_to_curve_coords() and
 * then calling elk_hilbert_coords_to_integer().
 *
 * \param hc the Hilbert curve. Must NOT be \c NULL.
 * \param coord the coordinate to translate.
 *
 * \returns the nearest coordinate in the coordinates of the Hilbert curve.
 */
uint64_t elk_hilbert_translate_to_curve_distance(struct ElkHilbertCurve *hc, Elk2DCoord coord);

/** @} */ // end of hilbert group
/*-------------------------------------------------------------------------------------------------
 *                                          2D RTreeView
 *-----------------------------------------------------------------------------------------------*/
/** \defgroup 2drtree A 2D RTree view of an ElkArray
 *
 * The RTree view does not store any data, but it holds pointers into a list, so that you can use
 * RTree algorithms to query the data in the list.
 *
 * @{
 */

/** An R-Tree view into a list.
 *
 * Once the view has been created, the list it was created from should not be modified again as
 * that will invalidate the tree view.
 */
typedef struct Elk2DRTreeView Elk2DRTreeView;

/** Create a new R-Tree View of the list.
 *
 * The list will not be grown, or shrunk, but it may be reordered.
 *
 * \param list is the list to create a view from.
 * \param centroid is a function that takes an item from the \p list and calculates a centroid for
 * it.
 * \param rect is a function that takes an item from the \p list and calculates a bounding rectangle
 * for it.
 * \param domain is the boundaries of data in the list. If this is \c NULL, then this constructor
 * will do an initial pass over the list and calculate it.
 *
 * \returns A view of the list.
 */
Elk2DRTreeView *elk_2d_rtree_view_new(ElkArray *const list, Elk2DCoord (*centroid)(void *),
                                      Elk2DRect (*rect)(void *), Elk2DRect *domain);

/** Free an R-Tree view of the list.
 *
 * \returns \c NULL, which should be assigned to the \p tv so there isn't a dangling pointer.
 */
Elk2DRTreeView *elk_2d_rtree_view_free(Elk2DRTreeView *tv);

/** Print a tree, which is useful for debugging. */
void elk_2d_rtree_view_print(Elk2DRTreeView *rtree);

/** Search an R-Tree and apply the function to each item that overlaps the provided rectangle.
 *
 * Iterates over any elements in the root list of \p tv that have rectangles that overlap with the
 * provided \p region and applies the function \p update to those items. If \p update returns
 * \c false, then further items will not be processed. This function assumes that \p update is
 * infallible, so if you need to return an error code of some kind you'll have to pass it through
 * the \p user_data argument.
 *
 * \param tv is the tree to search.
 * \param region is the region to search.
 * \param update is a function that will potentially update items in the view and parent list.
 * \param user_data is just passed to each invocation of \p update.
 */
void elk_2d_rtree_view_foreach(Elk2DRTreeView *tv, Elk2DRect region, IterFunc update,
                               void *user_data);

/** @} */ // end of 2drtree group
