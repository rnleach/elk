#include "test.h"
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
void
elk_hilbert_tests(void)
{
    elk_hilbert_test_integer_coordinate_conversions();
    elk_hilbert_test_domain_mapping();
}
