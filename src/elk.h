#pragma once
/**
 * \file elk.h
 * \brief A source library of useful code.
 *
 * See the main page for an overall description and the list of goals/non-goals: \ref index
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*-------------------------------------------------------------------------------------------------
 *                                    Error Handling Macros
 *-----------------------------------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
/** Create a \c time_t object given the date information.
 *
 * WARNING: The standard C library functions that this function depends on are not reentrant or
 * threadsafe, so neither is this function.
 *
 * \param year is the 4 digit year.
 * \param month is the month number [1,12].
 * \param day is the day of the month [1,31].
 *
 * \returns The system representation of that time as a \c time_t. This is usually (but not
 * guaranteed to be) the seconds since midnight UTC on January 1st, 1970. This tests for this
 * library test to make sure it is the seconds since the Unix epoch.
 */
time_t elk_time_from_ymd(int year, int month, int day);

/** Create a \c time_t object given the date and time information.
 *
 * WARNING: The standard C library functions that this function depends on are not reentrant or
 * threadsafe, so neither is this function.
 *
 * \param year is the 4 digit year.
 * \param month is the month number [1,12].
 * \param day is the day of the month [1,31].
 * \param hour is the hour of the day [0, 23].
 * \param minute is the minute of the day [0, 59].
 *
 * \returns The system representation of that time as a \c time_t. This is usually (but not
 * guaranteed to be) the seconds since midnight UTC on January 1st, 1970. This tests for this
 * library test to make sure it is the seconds since the Unix epoch.
 */
time_t elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds);

/** Truncate the minutes and seconds from the \p time argument.
 *
 * WARNING: The standard C library functions that this function depends on are not reentrant or
 * threadsafe, so neither is this function.
 *
 * NOTE: The non-posix version of this function depends on transformations to and from local time.
 * If you're in a timezone without a whole hour offset from UTC, that this won't round you down to
 * a whole hour in UTC. At the time I wrote this comment, their are approximately 10 such time zones
 * world wide that use half hour or 45 minute offsets.
 *
 * \param time is the time you want truncated or rounded down (back) to the most recent hour.
 *
 * \returns the time truncated, or rounded down, to the most recent hour.
 */
time_t elk_time_truncate_to_hour(time_t time);
/*-------------------------------------------------------------------------------------------------
 *                                         Memory and Pointers
 *-----------------------------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------------------------------
 *                                    Iterator Functions
 *-----------------------------------------------------------------------------------------------*/
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
 * \returns \c true if the item should be selected or filtered out and \c false if it should be not
 * be selected or filtered out.
 */
typedef bool (*FilterFunc)(void const *item, void *user_data);

/*-------------------------------------------------------------------------------------------------
 *                                           List
 *-----------------------------------------------------------------------------------------------*/
/** An array backed list that dynamically grows in size as needed.
 *
 * All the operations on this type assume the system will never run out of memory, so if it does
 * they will abort the program. The list is backed by an array, so it should be cache friendly.
 *
 * This list assumes (and so it is only good for) storing types of constant size that do not require
 * any copy or delete functions. This list stores plain old data types (PODs). Of course you could
 * store pointers or something that contains pointers in the list, but you would be responsible
 * for managing the memory (including any dangling aliases that may be out there after adding it
 * to the list).
 */
typedef struct ElkList ElkList;

/** Create a new list.
 *
 * \param element_size is the size of each item in bytes.
 *
 * \returns a pointer to the new list. This cannot practically fail unless the system runs out of
 * memory. The out of memory error is checked, and if the system is out of memory this function will
 * exit the process with an error message in both release and debug builds.
 */
ElkList *elk_list_new(size_t element_size);

/** Create a new list with an already known capacity.
 *
 * \param capacity is the minimum capacity the list should have when created.
 * \param element_size is the size of each item in bytes.
 *
 * \returns a pointer to the new list. This cannot practically fail unless the system runs out of
 * memory. The out of memory error is checked, and if the system is out of memory this function will
 * exit the process with an error message in both release and debug builds.
 */
ElkList *elk_list_new_with_capacity(size_t capacity, size_t element_size);

/** Free all memory associated with a list.
 *
 * \param list is the list to free. A \c NULL pointer is acceptable and is ignored.
 *
 * \returns \c NULL, which should be assigned to the original list.
 */
ElkList *elk_list_free(ElkList *list);

/** Clear out the contents of the list but keep the memory intact for reuse.
 *
 * The length of the list is set to zero without freeing any memory so the list is ready to use
 * again.
 *
 * \param list is the list to clear out. The list must not be \c NULL.
 *
 * \returns a pointer to \p list. Assigning \p list the return value is not strictly necessary,
 * since shrinking an array is not cause for a reallocation, however, the return value is set up
 * this way to maintain consistency with other functions that operate on \ref ElkList items.
 */
ElkList *elk_list_clear(ElkList *list);

/** Append \p item to the end of the list.
 *
 * \param list is the list to add the \p item too. It must not be \c NULL.
 * \param item is the item to add! The item is copied into place with \c memcpy(). \p item must not
 * be \c NULL.
 *
 * \returns a (possibly different) pointer to \p list. This may be different if a reallocation was
 * needed. As a result, this value should be reassigned to \p list.
 */
ElkList *elk_list_push_back(ElkList *list, void *item);

/** Remove an item from the back of the list.
 *
 * \param list is the list to remove an item from. This must NOT be \c NULL.
 * \param item is a memory location to move the last item in the list into. If this is \c NULL, then
 * the element is just removed from the list and not copied anywhere. If the list is empty, then
 * this will fill the location pointed to by \p item to all zeroes, so it is the user's
 * responsibility to ensure the list still has values in it before calling this function.
 *
 * \returns a pointer to \p list. Assigning \p list the return value is not strictly necessary,
 * since shrinking an array is not cause for a reallocation, however, the return value is set up
 * this way to maintain consistency with other functions that operate on \ref ElkList items.
 */
ElkList *elk_list_pop_back(ElkList *list, void *item);

/** Remove an item from the list without preserving order.
 *
 * This will remove an item at the \p index by swapping its position with the element at the end of
 * the list and then decrementing the length of the list.
 *
 * \param list is the list to remove the item from. \p list must NOT be \c NULL.
 * \param index is the position in the list to remove. There is no check to make sure this is in
 * bounds for the list, so the user must be sure the index isn't out of bounds (e.g. by using
 * elk_list_count()).
 * \param item is a memory location that can hold the removed data. It will be copied here if
 * \p item isn't \c NULL.
 *
 * \returns a pointer to \p list. Assigning \p list the return value is not strictly necessary,
 * since shrinking an array is not cause for a reallocation, however, the return value is set up
 * this way to maintain consistency with other functions that operate on \ref ElkList items.
 */
ElkList *elk_list_swap_remove(ElkList *const list, size_t index, void *item);

/** The number of items currently in the list.
 *
 * \param list must NOT be \c NULL.
 *
 * \returns the number of items in the \p list.
 */
size_t elk_list_count(ElkList const *const list);

/** Create a copy of this list.
 *
 * Since this list is assumed to hold plain old data (e.g. no pointers), there is no attempt at a
 * deep copy. So if the items in the list do contain pointers, this will create an alias for each of
 * those items.
 *
 * \param list is the list to create a copy of. This must not be \c NULL.
 *
 * \returns a new list that is a copy of \p list. This cannot fail unless the system runs out of
 * memory, and if that happens this function will exit the process.
 */
ElkList *elk_list_copy(ElkList const *const list);

/** Get an item from the list.
 *
 * This is not bounds checked, so make sure you know your index exists in the array by checking
 * elk_list_count() first.
 *
 * \param list the list is not modified in any way. That is nothing is added or removed, however
 * the returned element may be modified. Must not be \c NULL.
 * \param index is the index of the item you want to get. If the index is out of range, it will
 * invoke undefined behavior, and if you're lucky cause a segmentation fault.
 *
 * \returns a pointer to the item at the given index. You may use this pointer to modify the
 * value at that index, but DO NOT try to free it. You don't own it.
 */
void *const elk_list_get_alias_at_index(ElkList *const list, size_t index);

/** Apply \p ifunc to each element of \p list.
 *
 * This method assumes that whatever operation \p ifunc does is infallible. If that is not the case
 * then an error code of some kind should be returned via the \p user_data.
 *
 * \param list the list of items to iterate over.
 * \param ifunc is the function to apply to each item. If this function returns \c false, this
 * function will abort further evaluations. This function assumes this operation is infallible,
 * if that is not the case then any error codes should be returned via \p user_data.
 * \param user_data will be supplied to \p ifunc as the second argument each time it is called.
 */
void elk_list_foreach(ElkList *const list, IterFunc ifunc, void *user_data);

/** Filter elements out of \p src and put them into \p sink.
 *
 * \param src is the list to potentially remove items from. This list will never be reallocated so
 * the pointer will never move. It must not be \c NULL.
 * \param sink is the list to drop the filtered items into. If this is \c NULL then the items
 * selected to be filtered out will be removed from the list.
 * \param filter, if this returns \c true, the element will be removed from the \p src list.
 * \param user_data is passed through to \p filter on each call.
 *
 * \returns a (potentially new and different) pointer to \p sink since it may have been reallocated.
 * If the passed in \p sink was \c NULL, then \c NULL is returned.
 */
ElkList *elk_list_filter_out(ElkList *const src, ElkList *sink, FilterFunc filter, void *user_data);

/*-------------------------------------------------------------------------------------------------
 *                                    Coordinates and Rectangles
 *-----------------------------------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------------------------------------
 *                                       Hilbert Curves
 *-----------------------------------------------------------------------------------------------*/
/** A 2D Hilbert Curve.
 *
 * This might seem really weird to have in a "general" library. However, as a result of goal number
 * 1 above, here it is. This didn't really need to be in the public API, but it makes testing
 * easier.
 *
 * I ported this code from an implementation in Python at https://github.com/galtay/hilbertcurve,
 * which is itself an implementation based on the 2004 paper "Programming the Hilbert Curve" by
 * John Skilling (http://adsabs.harvard.edu/abs/2004AIPC..707..381S)(DOI: 10.10631/1.1751381).
 */
typedef struct ElkHilbertCurve ElkHilbertCurve;

/** A point in the Hilbert space. */
struct HilbertCoord {
    uint32_t x;
    uint32_t y;
};
/** Create a new Hilbert Curve description.
 *
 * \param iterations is a number between 1 and 31 inclusive. If the number is outside that range,
 * the program will exit and print an error message.
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

/*-------------------------------------------------------------------------------------------------
 *                                          2D RTreeView
 *-----------------------------------------------------------------------------------------------*/
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
Elk2DRTreeView *elk_2d_rtree_view_new(ElkList *const list, Elk2DCoord (*centroid)(void *),
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
