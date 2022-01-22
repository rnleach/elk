#include "elk.h"

#include <assert.h>

/*

Notes: I'm looking for a way to generate a Hilbert curve, and then use that to create a tree view
of an ElkList. I found an algorithm for generating such a curver iteratively here:

http://blog.marcinchwedczuk.pl/iterative-algorithm-for-drawing-hilbert-curve

*/

/*-------------------------------------------------------------------------------------------------
 *                                           List
 *-----------------------------------------------------------------------------------------------*/
struct ElkList {
};

ElkList *
elk_list_new(size_t element_size)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_new_with_capacity(size_t capacity, size_t element_size)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_free(ElkList *list)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_clear(ElkList *list)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_push_back(ElkList *list, void *item)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_pop_back(ElkList *list, void *item)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_swap_remove(ElkList *const list, size_t index, void *item)
{
    // TODO implement
    assert(false);
    return 0;
}

size_t
elk_list_count(ElkList const *const list)
{
    // TODO implement
    assert(false);
    return 0;
}

ElkList *
elk_list_copy(ElkList const *const list)
{
    // TODO implement
    assert(false);
    return 0;
}

void *const
elk_list_get_alias_at_index(ElkList *const list, size_t index)
{
    // TODO implement
    assert(false);
    return 0;
}

void
elk_list_foreach(ElkList const *const list, IterFunc ifunc, void *user_data)
{
    // TODO implement
    assert(false);
    return;
}

ElkList *
elk_list_filter_out(ElkList *const src, ElkList *sink, FilterFunc filter)
{
    // TODO implement
    assert(false);
    return 0;
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
