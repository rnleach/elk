#include "test.h"
/*-------------------------------------------------------------------------------------------------
 *
 *                                       ElkRTreeView
 *
 *-----------------------------------------------------------------------------------------------*/
struct LabeledRect {
    Elk2DRect rect;
    char *label;
};

static struct LabeledRect
labeled_rect_new(unsigned int min_x, unsigned int min_y)
{
    // All rects have width & height of 1

    char const *fmt = "%ux%u";
    int size = snprintf(0, 0, fmt, min_x, min_y);

    char *label = calloc(size + 1, sizeof(char));
    assert(label);

    int actual_size = snprintf(label, size + 1, fmt, min_x, min_y);
    assert(actual_size == size);

    return (struct LabeledRect){.rect =
                                    (Elk2DRect){.ll = (Elk2DCoord){.x = min_x, .y = min_y},
                                                .ur = (Elk2DCoord){.x = min_x + 1, .y = min_y + 1}},
                                .label = label};
}

static ElkList *
create_rectangles_for_rtree_view_test(void)
{
    ElkList *list = elk_list_new_with_capacity(40, sizeof(struct LabeledRect));

    for (unsigned int i = 1; i <= 15; i += 2) {
        for (unsigned int j = 1; j <= 9; j += 2) {
            struct LabeledRect r = labeled_rect_new(i, j);
            list = elk_list_push_back(list, &r);
        }
    }

    return list;
}

static bool
labeled_rect_cleanup(void *item, void *user_data)
{
    assert(item);
    struct LabeledRect *r = item;
    free(r->label);

    return true;
}

static ElkList *
clean_up_rectangles_for_rtree_view_test(ElkList *list)
{
    elk_list_foreach(list, labeled_rect_cleanup, NULL);
    return elk_list_free(list);
}

static Elk2DCoord
labeled_rect_centroid(void *item)
{
    assert(item);
    struct LabeledRect *lrect = item;
    return (Elk2DCoord){.x = (lrect->rect.ll.x + lrect->rect.ur.x) / 2.0,
                        .y = (lrect->rect.ll.y + lrect->rect.ur.y) / 2.0};
}

static Elk2DRect
labeled_rect_mbr(void *item)
{
    assert(item);
    struct LabeledRect *lrect = item;
    return lrect->rect;
}

/*-------------------------------------------------------------------------------------------------
 *                           Test Creating and Destroying ElkRTreeView
 *-----------------------------------------------------------------------------------------------*/
static void
elk_rtree_view_test_create_destroy(void)
{
    ElkList *list = create_rectangles_for_rtree_view_test();

    Elk2DRTreeView *rtree =
        elk_2d_rtree_view_new(list, labeled_rect_centroid, labeled_rect_mbr, NULL);

    rtree = elk_2d_rtree_view_free(rtree);
    assert(!rtree);

    list = clean_up_rectangles_for_rtree_view_test(list);
    assert(!list);
}

/*-------------------------------------------------------------------------------------------------
 *                                   Test Querying ElkRTreeView
 *-----------------------------------------------------------------------------------------------*/
static bool
count_labeled_rects(void *item, void *user_data)
{
    // struct LabeledRect *lrect = item;
    unsigned int *count = user_data;

    // fprintf(stderr, "Hit: %s\n", lrect->label);
    *count += 1;

    return true;
}

struct RTreeTestQueryPair {
    Elk2DRect qrect;
    unsigned long num_should_hit;
};

static void
elk_rtree_view_test_query(void)
{
    ElkList *list = create_rectangles_for_rtree_view_test();

    Elk2DRTreeView *rtree =
        elk_2d_rtree_view_new(list, labeled_rect_centroid, labeled_rect_mbr, NULL);

    // Check whole domain returns all the rectangles.
    struct RTreeTestQueryPair whole_domain = {.qrect =
                                                  (Elk2DRect){
                                                      .ll = (Elk2DCoord){.x = 0.0, .y = 0.0},
                                                      .ur = (Elk2DCoord){.x = 20.0, .y = 20.0},
                                                  },
                                              .num_should_hit = elk_list_count(list)};

    unsigned int count = 0;
    elk_2d_rtree_view_foreach(rtree, whole_domain.qrect, count_labeled_rects, &count);
    assert(count == whole_domain.num_should_hit);

    // Check several sub-rectangles.
    struct RTreeTestQueryPair test_pairs[] = {
        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 0.0, .y = 0.0},
                                                         .ur = (Elk2DCoord){.x = 4.5, .y = 4.5}},
                                    .num_should_hit = 4},

        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 0.0, .y = 0.0},
                                                         .ur = (Elk2DCoord){.x = 5.5, .y = 5.5}},
                                    .num_should_hit = 9},

        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = -10.0, .y = -10.0},
                                                         .ur = (Elk2DCoord){.x = 5.5, .y = 5.5}},
                                    .num_should_hit = 9},

        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 0.0, .y = 0.0},
                                                         .ur = (Elk2DCoord){.x = 4.5, .y = 4.5}},
                                    .num_should_hit = 4},

        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 7.5, .y = 5.5},
                                                         .ur = (Elk2DCoord){.x = 9.5, .y = 7.5}},
                                    .num_should_hit = 4},

        (struct RTreeTestQueryPair){.qrect =
                                        (Elk2DRect){.ll = (Elk2DCoord){.x = 14.5, .y = 8.5},
                                                    .ur = (Elk2DCoord){.x = 100.0, .y = 1000.0}},
                                    .num_should_hit = 1},

        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 3.0, .y = 0.0},
                                                         .ur = (Elk2DCoord){.x = 4.5, .y = 4.5}},
                                    .num_should_hit = 2},

        // Just graze the edges
        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 4.0, .y = 4.0},
                                                         .ur = (Elk2DCoord){.x = 5.0, .y = 5.0}},
                                    .num_should_hit = 4},

        // Hit nothing!
        (struct RTreeTestQueryPair){.qrect = (Elk2DRect){.ll = (Elk2DCoord){.x = 4.1, .y = 4.1},
                                                         .ur = (Elk2DCoord){.x = 4.9, .y = 4.9}},
                                    .num_should_hit = 0},
    };

    for (unsigned i = 0; i < sizeof(test_pairs) / sizeof(test_pairs[0]); ++i) {
        count = 0;
        elk_2d_rtree_view_foreach(rtree, test_pairs[i].qrect, count_labeled_rects, &count);
        assert(count == test_pairs[i].num_should_hit);
    }

    rtree = elk_2d_rtree_view_free(rtree);
    assert(!rtree);

    list = clean_up_rectangles_for_rtree_view_test(list);
    assert(!list);
}

/*-------------------------------------------------------------------------------------------------
 *                                    All ElkRTreeView test
 *-----------------------------------------------------------------------------------------------*/
void
elk_rtree_view_tests(void)
{
    elk_rtree_view_test_create_destroy();
    elk_rtree_view_test_query();
}
