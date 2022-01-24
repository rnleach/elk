#include "elk.h"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>

// Constant messages for errors.
static const char *err_out_of_mem = "out of memory";

/*-------------------------------------------------------------------------------------------------
 *                                           List
 *-----------------------------------------------------------------------------------------------*/
struct ElkList {
    size_t element_size;
    size_t len;
    size_t capacity;
    _Alignas(max_align_t) unsigned char data[];
};

ElkList *
elk_list_new(size_t element_size)
{
    size_t const initial_num_elements = 4;
    return elk_list_new_with_capacity(initial_num_elements, element_size);
}

ElkList *
elk_list_new_with_capacity(size_t capacity, size_t element_size)
{
    size_t allocation_size = sizeof(struct ElkList) + element_size * capacity;

    struct ElkList *new = malloc(allocation_size);
    Panicif(!new, "%s", err_out_of_mem);

    new->element_size = element_size;
    new->len = 0;
    new->capacity = capacity;

    return new;
}

ElkList *
elk_list_free(ElkList *list)
{
    if (list) {
        free(list);
    }

    return NULL;
}

ElkList *
elk_list_clear(ElkList *list)
{
    assert(list);
    list->len = 0;
    return list;
}

static struct ElkList *
elk_list_expand(struct ElkList *list, size_t min_capacity)
{
    assert(list);

    if (min_capacity <= list->capacity) {
        return list;
    }

    size_t new_capacity = list->capacity * 3 / 2;
    new_capacity = new_capacity < 4 ? 4 : new_capacity;

    size_t allocation_size = sizeof(struct ElkList) + list->element_size * new_capacity;

    struct ElkList *new = realloc(list, allocation_size);
    Panicif(!new, "%s", err_out_of_mem);

    new->capacity = new_capacity;

    return new;
}

ElkList *
elk_list_push_back(ElkList *list, void *item)
{
    assert(list);
    assert(item);

    if (list->capacity == list->len) {
        list = elk_list_expand(list, list->len + 1);
    }

    void *next = list->data + list->element_size * list->len;
    memcpy(next, item, list->element_size);
    list->len++;

    return list;
}

ElkList *
elk_list_pop_back(ElkList *list, void *item)
{
    assert(list);

    if (item) {
        memcpy(item, list->data + list->element_size * (list->len - 1), list->element_size);
    }

    list->len--;

    return list;
}

ElkList *
elk_list_swap_remove(ElkList *const list, size_t index, void *item)
{
    assert(list);

    void *index_ptr = list->data + list->element_size * index;
    void *last_ptr = list->data + list->element_size * (list->len - 1);

    if (item) {
        memcpy(item, index_ptr, list->element_size);
    }

    memcpy(index_ptr, last_ptr, list->element_size);
    list->len--;

    return list;
}

size_t
elk_list_count(ElkList const *const list)
{
    assert(list);
    return list->len;
}

ElkList *
elk_list_copy(ElkList const *const list)
{
    assert(list);

    struct ElkList *copy = elk_list_new_with_capacity(list->len, list->element_size);

    assert(copy->capacity <= list->capacity);

    size_t copy_size = sizeof(struct ElkList) + copy->element_size * copy->capacity;
    memcpy(copy, list, copy_size);
    copy->capacity = copy->len;

    return copy;
}

void *const
elk_list_get_alias_at_index(ElkList *const list, size_t index)
{
    assert(list);
    return list->data + index * list->element_size;
}

void
elk_list_foreach(ElkList *const list, IterFunc ifunc, void *user_data)
{
    assert(list);

    void *first = list->data;
    void *last = list->data + list->element_size * (list->len - 1);
    for (void *item = first; item <= last; item += list->element_size) {
        bool keep_going = ifunc(item, user_data);
        if (!keep_going) {
            return;
        }
    }

    return;
}

ElkList *
elk_list_filter_out(ElkList *const src, ElkList *sink, FilterFunc filter, void *user_data)
{
    assert(src);
    if (sink) {
        assert(src->element_size == sink->element_size);
    }

    for (size_t i = 0; i < src->len; ++i) {
        void *item = elk_list_get_alias_at_index(src, i);
        bool to_remove = filter(item, user_data);

        if (to_remove) {
            if (sink) {
                sink = elk_list_push_back(sink, item);
            }

            elk_list_swap_remove(src, i, NULL);
            --i;
        }
    }

    return sink;
}

/*-------------------------------------------------------------------------------------------------
 *                                       Hilbert Curves
 *-----------------------------------------------------------------------------------------------*/

struct HilbertCurve {
    /** The number of iterations to use for this curve.
     *
     * This can be a maximum of 31. If it is larger than 31, we won't have enough bits to do the
     * binary transformations correctly.
     */
    uint64_t iterations;

    /** This is the domain that the curve will cover. */
    Elk2DRect domain;
};

static struct HilbertCurve
elk_hilbert_curve_initialize(unsigned int iterations, Elk2DRect domain)
{
    Panicif(iterations < 1, "Require at least 1 iteration for Hilbert curve.");
    Panicif(iterations > 31, "Maximum 31 iterations for Hilbert curve.");
    return (struct HilbertCurve){.iterations = iterations, .domain = domain};
}

struct HilbertCurve *
elk_hilbert_curve_new(unsigned int iterations, Elk2DRect domain)
{
    struct HilbertCurve *new = malloc(sizeof(struct HilbertCurve));
    Panicif(!new, "%s", err_out_of_mem);

    *new = elk_hilbert_curve_initialize(iterations, domain);

    return new;
}

struct HilbertCurve *
elk_hilbert_curve_free(struct HilbertCurve *hc)
{
    if (hc) {
        free(hc);
    }

    return NULL;
}

struct HilbertCoord
elk_hilbert_integer_to_coords(struct HilbertCurve const *hc, uint64_t hi)
{
    assert(hc);
    assert(hi <= ((UINT64_C(1) << (2 * hc->iterations)) - 1));

    uint32_t x = 0;
    uint32_t y = 0;

    // This is the "transpose" operation.
    for (uint32_t b = 0; b < hc->iterations; ++b) {
        uint64_t x_mask = UINT64_C(1) << (2 * b + 1);
        uint64_t y_mask = UINT64_C(1) << (2 * b);

        uint32_t x_val = (hi & x_mask) >> (b + 1);
        uint32_t y_val = (hi & y_mask) >> (b);

        x |= x_val;
        y |= y_val;
    }

    // Gray decode
    uint32_t z = UINT32_C(2) << (hc->iterations - 1);
    uint32_t t = y >> 1;

    y ^= x;
    x ^= t;

    // Undo excess work
    uint32_t q = 2;
    while (q != z) {
        uint32_t p = q - 1;

        if (y & q) {
            x ^= p;
        } else {
            t = (x ^ y) & p;
            x ^= t;
            y ^= t;
        }

        if (x & q) {
            x ^= p;
        } else {
            t = (x ^ x) & p;
            x ^= t;
            x ^= t;
        }

        q <<= 1;
    }

    assert(x <= ((UINT32_C(1) << hc->iterations) - 1));
    assert(y <= ((UINT32_C(1) << hc->iterations) - 1));

    return (struct HilbertCoord){.x = x, .y = y};
}

uint64_t
elk_hilbert_coords_to_integer(struct HilbertCurve const *hc, struct HilbertCoord coords)
{
    assert(hc);
    assert(coords.x <= ((UINT32_C(1) << hc->iterations) - 1));
    assert(coords.y <= ((UINT32_C(1) << hc->iterations) - 1));

    uint32_t x = coords.x;
    uint32_t y = coords.y;

    uint32_t m = UINT32_C(1) << (hc->iterations - 1);

    // Inverse undo excess work
    uint32_t q = m;
    while (q > 1) {
        uint32_t p = q - 1;

        if (x & q) {
            x ^= p;
        } else {
            uint32_t t = (x ^ x) & p;
            x ^= t;
            x ^= t;
        }

        if (y & q) {
            x ^= p;
        } else {
            uint32_t t = (x ^ y) & p;
            x ^= t;
            y ^= t;
        }

        q >>= 1;
    }

    // Gray encode
    y ^= x;
    uint32_t t = 0;
    q = m;
    while (q > 1) {
        if (y & q) {
            t ^= (q - 1);
        }

        q >>= 1;
    }

    x ^= t;
    y ^= t;

    // This is the transpose operation
    uint64_t hi = 0;
    for (uint32_t b = 0; b < hc->iterations; ++b) {

        uint64_t x_val = (((UINT64_C(1) << b) & x) >> b) << (2 * b + 1);
        uint64_t y_val = (((UINT64_C(1) << b) & y) >> b) << (2 * b);

        hi |= x_val;
        hi |= y_val;
    }

    assert(hi <= ((UINT64_C(1) << (2 * hc->iterations)) - 1));
    return hi;
}

struct HilbertCoord
elk_hilbert_translate_to_curve_coords(struct HilbertCurve *hc, Elk2DCoord coord)
{
    assert(hc);

    // TODO implement
    assert(false);

    struct HilbertCoord hcoords = {0};
    return hcoords;
}

uint64_t
elk_hilbert_translate_to_curve_distance(struct HilbertCurve *hc, Elk2DCoord coord)
{
    assert(hc);

    // TODO implement
    assert(false);
    return 0;
}

/*-------------------------------------------------------------------------------------------------
 *                                          2D RTreeView
 *-----------------------------------------------------------------------------------------------*/

/*

NOTE: Remember these are defined in elk.h

typedef struct Elk2DCoord {
    double x;
    double y;
} Elk2DCoord;

typedef struct Elk2DRect {
    Elk2DCoord ll;
    Elk2DCoord ur;
} Elk2DRect;

*/

struct Elk2DRTreeView {
};

Elk2DRTreeView *
elk_2d_rtree_view_new(ElkList *const list, Elk2DCoord (*centroid)(void *),
                      Elk2DRect (*rect)(void *))
{
    // TODO implement
    assert(false);
    return 0;
}

Elk2DRTreeView *
elk_2d_rtree_view_free(Elk2DRTreeView *tv)
{
    // TODO implement
    assert(false);
    return 0;
}

void
elk_2d_rtree_view_foreach(Elk2DRTreeView *tv, Elk2DRect region, IterFunc update, void *user_data)
{
    // TODO implement
    assert(false);
    return;
}
