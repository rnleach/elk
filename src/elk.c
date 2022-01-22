#include "elk.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

/*

Notes: I'm looking for a way to generate a Hilbert curve, and then use that to create a tree view
of an ElkList. I found an algorithm for generating such a curver iteratively here:

http://blog.marcinchwedczuk.pl/iterative-algorithm-for-drawing-hilbert-curve

a better algorithm coded in python can be found here:
https://github.com/galtay/hilbertcurve/blob/main/hilbertcurve/hilbertcurve.py

*/

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
 *                                          2D RTreeView
 *-----------------------------------------------------------------------------------------------*/

/* Remember these are defined in elk.h

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
