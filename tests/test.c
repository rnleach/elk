//
// For testing, ensure we have some debugging tools activated.
//

// We must have asserts working for the tests to work.
#ifdef NDEBUG
#    undef NDEBUG
#endif

// In the event of a panic, this should cause a crash and let the debugger take over if the test
// is running under a debugger. Otherwise it will just exit.
#ifndef ELK_PANIC_CRASH
#    define ELK_PANIC_CRASH
#endif

// Turn on ELK_MEMORY_DEBUG from the command line to check for memory errors during testing.

#include <assert.h>
#include <math.h>

#include "../src/elk.h"

/*------------------------------------------------------------------------------------------------
 *
 *                                       Tests for Time
 *
 *-----------------------------------------------------------------------------------------------*/
static void
test_time_unix_epoch(void)
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    assert(epoch == 0);
}

static void
test_time_time_t_is_seconds(void)
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    time_t day1 = elk_time_from_ymd_and_hms(1970, 1, 2, 0, 0, 0);

    assert((day1 - epoch) == (60 * 60 * 24));
}

static void
test_time_truncate_to_hour()
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    time_t t1 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 14, 39);

    assert(elk_time_truncate_to_hour(t1) == epoch);
}

static void
test_time_truncate_to_specific_hour()
{
    time_t start = elk_time_from_ymd_and_hms(2022, 6, 20, 19, 14, 39);
    time_t target1 = elk_time_from_ymd_and_hms(2022, 6, 20, 12, 0, 0);
    time_t target2 = elk_time_from_ymd_and_hms(2022, 6, 19, 21, 0, 0);

    assert(elk_time_truncate_to_specific_hour(start, 12) == target1);
    assert(elk_time_truncate_to_specific_hour(start, 21) == target2);
}

static void
test_time_addition()
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    time_t t1 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 14, 39);

    assert((epoch + 14 * ElkMinute + 39 * ElkSecond) == t1);
    assert(elk_time_add(epoch, 14 * ElkMinute + 39 * ElkSecond) == t1);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkList tests
 *-----------------------------------------------------------------------------------------------*/
static void
elk_time_tests(void)
{
    test_time_unix_epoch();
    test_time_time_t_is_seconds();
    test_time_truncate_to_hour();
    test_time_truncate_to_specific_hour();
    test_time_addition();
}

/*------------------------------------------------------------------------------------------------
 *
 *                                       Tests for ElkList
 *
 *-----------------------------------------------------------------------------------------------*/

static ElkList *
build_double_list(size_t num_elements)
{
    ElkList *list = elk_list_new(sizeof(double));

    for (size_t i = 0; i < num_elements; ++i) {
        double value = i;
        list = elk_list_push_back(list, &value);
    }

    assert(elk_list_count(list) == num_elements);

    return list;
}

/*-------------------------------------------------------------------------------------------------
 *                         Test adding and retrieving data match.
 *-----------------------------------------------------------------------------------------------*/
static void
test_elk_list_data_integrity(void)
{
    size_t const list_size = 20;

    ElkList *list = build_double_list(list_size);

    // Check that the values in the list are what they should be.
    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        double *alias = elk_list_get_alias_at_index(list, i);
        assert(*alias == value);
    }

    // Check that pop gives the correct value back to you.
    for (size_t i = list_size - 1; elk_list_count(list) > 0; --i) {
        double value = i;
        double value_from_list = HUGE_VAL;
        list = elk_list_pop_back(list, &value_from_list);

        assert(value_from_list == value);
    }

    assert(elk_list_count(list) == 0);

    list = elk_list_free(list);
    assert(!list);
}

/*-------------------------------------------------------------------------------------------------
 *                                         Test copy
 *-----------------------------------------------------------------------------------------------*/
static void
test_elk_list_copy(void)
{
    size_t const list_size = 1000;

    ElkList *list = build_double_list(list_size);

    ElkList *copy = elk_list_copy(list);

    assert(elk_list_count(list) == elk_list_count(copy));

    for (size_t i = 0; i < elk_list_count(list); ++i) {
        double *item = elk_list_get_alias_at_index(list, i);
        double *item_copy = elk_list_get_alias_at_index(copy, i);

        assert(*item == *item_copy);
    }

    list = elk_list_free(list);
    assert(!list);

    copy = elk_list_free(copy);
    assert(!copy);
}

/*-------------------------------------------------------------------------------------------------
 *                                       Test foreach
 *-----------------------------------------------------------------------------------------------*/
static bool
square_small_double(void *val_ptr, void *unused)
{
    double *val = val_ptr;

    if (*val < 6.0) {
        *val *= *val;
        return true;
    }

    return false;
}

static void
test_elk_list_foreach(void)
{
    size_t const list_size = 20;

    ElkList *list = build_double_list(list_size);

    elk_list_foreach(list, square_small_double, NULL);

    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        square_small_double(&value, NULL);

        double *alias = elk_list_get_alias_at_index(list, i);
        assert(*alias == value);
    }

    list = elk_list_free(list);
    assert(!list);
}

/*-------------------------------------------------------------------------------------------------
 *                                       Test filter_out
 *-----------------------------------------------------------------------------------------------*/
static bool
delete_large_double(void const *val_ptr, void *unused)
{
    double const *val = val_ptr;
    if (*val < 6.0) {
        return false;
    }

    return true;
}

static void
test_elk_list_filter_out(void)
{
    size_t const list_size = 20;

    ElkList *src = build_double_list(list_size);
    ElkList *sink = elk_list_new(sizeof(double));

    sink = elk_list_filter_out(src, sink, delete_large_double, NULL);

    assert(elk_list_count(src) == 6);
    assert(elk_list_count(sink) == (list_size - 6));

    for (size_t i = 0; i < list_size; ++i) {

        double *alias = 0;
        if (i < 6) {
            alias = elk_list_get_alias_at_index(src, i);
            double value = i;
            assert(*alias == value);
        } else {
            alias = elk_list_get_alias_at_index(sink, i - 6);
            assert(*alias >= 6.0);
        }
    }

    src = elk_list_free(src);
    assert(!src);
    sink = elk_list_free(sink);
    assert(!sink);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkList tests
 *-----------------------------------------------------------------------------------------------*/
static void
elk_list_tests(void)
{
    test_elk_list_data_integrity();
    test_elk_list_copy();
    test_elk_list_foreach();
    test_elk_list_filter_out();
}

/*-------------------------------------------------------------------------------------------------
 *
 *                                       Hilbert Curves
 *
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *                                    Test Integer Coordinates
 *-----------------------------------------------------------------------------------------------*/
static void
elk_hilbert_test_integer_coordinate_conversions(void)
{
    struct Elk2DRect domain = {.ll = (struct Elk2DCoord){.x = 0.0, .y = 0.0},
                               .ur = (struct Elk2DCoord){.x = 1.0, .y = 1.0}};

    struct HilbertCoord test_coords_i1[] = {
        (struct HilbertCoord){.x = 0, .y = 0},
        (struct HilbertCoord){.x = 0, .y = 1},
        (struct HilbertCoord){.x = 1, .y = 1},
        (struct HilbertCoord){.x = 1, .y = 0},
    };

    struct HilbertCoord test_coords_i2[] = {
        (struct HilbertCoord){.x = 0, .y = 0}, (struct HilbertCoord){.x = 1, .y = 0},
        (struct HilbertCoord){.x = 1, .y = 1}, (struct HilbertCoord){.x = 0, .y = 1},

        (struct HilbertCoord){.x = 0, .y = 2}, (struct HilbertCoord){.x = 0, .y = 3},
        (struct HilbertCoord){.x = 1, .y = 3}, (struct HilbertCoord){.x = 1, .y = 2},

        (struct HilbertCoord){.x = 2, .y = 2}, (struct HilbertCoord){.x = 2, .y = 3},
        (struct HilbertCoord){.x = 3, .y = 3}, (struct HilbertCoord){.x = 3, .y = 2},

        (struct HilbertCoord){.x = 3, .y = 1}, (struct HilbertCoord){.x = 2, .y = 1},
        (struct HilbertCoord){.x = 2, .y = 0}, (struct HilbertCoord){.x = 3, .y = 0},
    };

    uint64_t test_dist[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    struct ElkHilbertCurve *hc = elk_hilbert_curve_new(1, domain);
    for (uint64_t h = 0; h < 4; ++h) {
        struct HilbertCoord coords = elk_hilbert_integer_to_coords(hc, test_dist[h]);
        assert(coords.x == test_coords_i1[h].x && coords.y == test_coords_i1[h].y);

        uint64_t h2 = elk_hilbert_coords_to_integer(hc, test_coords_i1[h]);
        assert(h2 == test_dist[h]);
    }

    hc = elk_hilbert_curve_free(hc);

    hc = elk_hilbert_curve_new(2, domain);
    for (uint64_t h = 0; h < 16; ++h) {
        struct HilbertCoord coords = elk_hilbert_integer_to_coords(hc, test_dist[h]);
        assert(coords.x == test_coords_i2[h].x && coords.y == test_coords_i2[h].y);

        uint64_t h2 = elk_hilbert_coords_to_integer(hc, test_coords_i2[h]);
        assert(h2 == test_dist[h]);
    }

    hc = elk_hilbert_curve_free(hc);
}

/*-------------------------------------------------------------------------------------------------
 *                         Test Mapping from Domain into Hilbert Distance
 *-----------------------------------------------------------------------------------------------*/
struct DomainPair {
    Elk2DCoord coord;
    uint64_t hilbert_dist;
};

static void
elk_hilbert_test_domain_mapping(void)
{
    struct Elk2DRect domain = {.ll = (struct Elk2DCoord){.x = 0.0, .y = 0.0},
                               .ur = (struct Elk2DCoord){.x = 1.0, .y = 1.0}};

    // Test values for the N=1 Hilbert curve on the unit square.
    // clang-format off
    struct DomainPair n1_pairs[] = {
        (struct DomainPair){.coord = (Elk2DCoord){.x = 0.25, .y = 0.25}, .hilbert_dist = 0},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 0.25, .y = 0.75}, .hilbert_dist = 1},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 0.75, .y = 0.75}, .hilbert_dist = 2},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 0.75, .y = 0.25}, .hilbert_dist = 3},

        // Corners.
        (struct DomainPair){.coord = (Elk2DCoord){.x = 0.00, .y = 0.00}, .hilbert_dist = 0},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 0.00, .y = 1.00}, .hilbert_dist = 1},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 1.00, .y = 1.00}, .hilbert_dist = 2},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 1.00, .y = 0.00}, .hilbert_dist = 3},
    };
    // clang-format on

    struct ElkHilbertCurve *hc = elk_hilbert_curve_new(1, domain);
    for (unsigned int i = 0; i < sizeof(n1_pairs) / sizeof(n1_pairs[0]); ++i) {
        uint64_t hilbert_dist = n1_pairs[i].hilbert_dist;
        Elk2DCoord coord = n1_pairs[i].coord;

        uint64_t hd_calc = elk_hilbert_translate_to_curve_distance(hc, coord);

        // fprintf(stderr, "Coord: (%.2lf, %.2lf)  Calc: %"PRIu64" Actual: %"PRIu64"\n", coord.x,
        //         coord.y, hd_calc, hilbert_dist);

        assert(hd_calc == hilbert_dist);
    }

    hc = elk_hilbert_curve_free(hc);
    assert(!hc);

    domain = (struct Elk2DRect){.ll = (struct Elk2DCoord){.x = 0.0, .y = 0.0},
                                .ur = (struct Elk2DCoord){.x = 10.0, .y = 10.0}};

    // Test values for the N=1 Hilbert curve on a square with edges of 10.0
    // clang-format off
    struct DomainPair n1_pairs_b[] = {
        (struct DomainPair){.coord = (Elk2DCoord){.x =  2.5, .y = 2.5}, .hilbert_dist = 0},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  2.5, .y = 7.5}, .hilbert_dist = 1},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  7.5, .y = 7.5}, .hilbert_dist = 2},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  7.5, .y = 2.5}, .hilbert_dist = 3},

        // Corners.
        (struct DomainPair){.coord = (Elk2DCoord){.x =  0.0, .y =  0.0}, .hilbert_dist = 0},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  0.0, .y = 10.0}, .hilbert_dist = 1},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 10.0, .y = 10.0}, .hilbert_dist = 2},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 10.0, .y =  0.0}, .hilbert_dist = 3},
    };
    // clang-format on

    hc = elk_hilbert_curve_new(1, domain);
    for (unsigned int i = 0; i < sizeof(n1_pairs_b) / sizeof(n1_pairs_b[0]); ++i) {
        uint64_t hilbert_dist = n1_pairs_b[i].hilbert_dist;
        Elk2DCoord coord = n1_pairs_b[i].coord;

        uint64_t hd_calc = elk_hilbert_translate_to_curve_distance(hc, coord);

        // fprintf(stderr, "Coord: (%.2lf, %.2lf)  Calc: %"PRIu64" Actual: %"PRIu64"\n", coord.x,
        //         coord.y, hd_calc, hilbert_dist);

        assert(hd_calc == hilbert_dist);
    }

    hc = elk_hilbert_curve_free(hc);
    assert(!hc);

    domain = (struct Elk2DRect){.ll = (struct Elk2DCoord){.x = -2.0, .y = 5.0},
                                .ur = (struct Elk2DCoord){.x = 10.0, .y = 17.0}};

    // Test values for the N=2 Hilbert curve on a square with edges of 12.0
    // clang-format off
    struct DomainPair n2_pairs[] = {
        (struct DomainPair){.coord = (Elk2DCoord){.x =  -0.5, .y =  5.5}, .hilbert_dist =  0},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   2.5, .y =  5.5}, .hilbert_dist =  1},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   2.5, .y =  9.5}, .hilbert_dist =  2},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  -0.5, .y =  9.5}, .hilbert_dist =  3},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  -0.5, .y = 12.5}, .hilbert_dist =  4},
        (struct DomainPair){.coord = (Elk2DCoord){.x =  -0.5, .y = 15.5}, .hilbert_dist =  5},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   2.5, .y = 15.5}, .hilbert_dist =  6},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   2.5, .y = 12.5}, .hilbert_dist =  7},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   5.5, .y = 12.5}, .hilbert_dist =  8},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   5.5, .y = 15.5}, .hilbert_dist =  9},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   8.5, .y = 15.5}, .hilbert_dist = 10},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   8.5, .y = 12.5}, .hilbert_dist = 11},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   8.5, .y =  9.5}, .hilbert_dist = 12},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   5.5, .y =  9.5}, .hilbert_dist = 13},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   5.5, .y =  5.5}, .hilbert_dist = 14},
        (struct DomainPair){.coord = (Elk2DCoord){.x =   8.5, .y =  5.5}, .hilbert_dist = 15},

        // Corners.
        (struct DomainPair){.coord = (Elk2DCoord){.x = -2.0, .y =  5.0}, .hilbert_dist =  0},
        (struct DomainPair){.coord = (Elk2DCoord){.x = -2.0, .y = 17.0}, .hilbert_dist =  5},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 10.0, .y = 17.0}, .hilbert_dist = 10},
        (struct DomainPair){.coord = (Elk2DCoord){.x = 10.0, .y =  5.0}, .hilbert_dist = 15},
    };
    // clang-format on

    hc = elk_hilbert_curve_new(2, domain);
    for (unsigned int i = 0; i < sizeof(n2_pairs) / sizeof(n2_pairs[0]); ++i) {
        uint64_t hilbert_dist = n2_pairs[i].hilbert_dist;
        Elk2DCoord coord = n2_pairs[i].coord;

        uint64_t hd_calc = elk_hilbert_translate_to_curve_distance(hc, coord);

        // fprintf(stderr, "Coord: (%.2lf, %.2lf)  Calc: %"PRIu64" Actual: %"PRIu64"\n", coord.x,
        //         coord.y, hd_calc, hilbert_dist);

        assert(hd_calc == hilbert_dist);
    }

    hc = elk_hilbert_curve_free(hc);
    assert(!hc);
}

/*-------------------------------------------------------------------------------------------------
 *                                    All ElkHilbertCurve tests
 *-----------------------------------------------------------------------------------------------*/
static void
elk_hilbert_tests(void)
{
    elk_hilbert_test_integer_coordinate_conversions();
    elk_hilbert_test_domain_mapping();
}

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

    char *label = malloc(size + 1);
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
static void
elk_rtree_view_tests(void)
{
    elk_rtree_view_test_create_destroy();
    elk_rtree_view_test_query();
}

/*-------------------------------------------------------------------------------------------------
 *
 *                                       Main - Run the tests
 *
 *-----------------------------------------------------------------------------------------------*/
int
main(void)
{
    printf("\n\n***        Starting Tests.        ***\n\n");

    elk_init_memory_debug();

    elk_time_tests();
    elk_list_tests();
    elk_hilbert_tests();
    elk_rtree_view_tests();

    elk_finalize_memory_debug();

    printf("\n\n*** Tests completed successfully. ***\n\n");
    return EXIT_SUCCESS;
}
