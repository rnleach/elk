#include "test.h"

#include <string.h>

/*------------------------------------------------------------------------------------------------
 *
 *                               Tests for the Memory Pool
 *
 *-----------------------------------------------------------------------------------------------*/
static void
test_full_pool(void)
{
    ElkPoolAllocator pool_obj = {0};
    ElkPoolAllocator *pool = &pool_obj;

    elk_pool_initialize(pool, sizeof(double), 10);

    double *dubs[10] = {0};

    for (int i = 0; i < 10; ++i) {
        dubs[i] = elk_pool_alloc(pool);
        assert(dubs[i]);

        *dubs[i] = (double)i;
    }

    for (int i = 0; i < 10; ++i) {
        assert(*dubs[i] == (double)i);
    }

    // Test that it's full!
    for (int i = 10; i < 20; i++) {
        double *no_dub = elk_pool_alloc(pool);
        assert(!no_dub);
    }

    elk_pool_destroy(pool);
}

static void
test_pool_freeing(void)
{
    ElkPoolAllocator pool_obj = {0};
    ElkPoolAllocator *pool = &pool_obj;

    elk_pool_initialize(pool, sizeof(double), 10);

    double *dubs[10] = {0};

    for (int i = 0; i < 10; ++i) {
        dubs[i] = elk_pool_alloc(pool);
        assert(dubs[i]);

        *dubs[i] = (double)i;
    }

    for (int i = 0; i < 10; ++i) {
        assert(*dubs[i] == (double)i);
    }

    // Empty it!
    for (int i = 0; i < 5; i++) {
        elk_pool_free(pool, dubs[2 * i]);
        dubs[2 * i] = NULL;
    }

    for (int i = 0; i < 5; i++) {
        dubs[2 * i] = elk_pool_alloc(pool);
        assert(dubs[2 * i]);

        *dubs[2 * i] = (double)i;
    }

    for (int i = 0; i < 5; i++) {
        assert(*dubs[2 * i] == (double)i);
    }

    elk_pool_destroy(pool);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All Memory Arena Tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_pool_tests(void)
{
    test_full_pool();
    test_pool_freeing();
}
