#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                               Tests for the Memory Pool
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
#define TEST_BUF_COUNT 10

static void
test_full_pool(void)
{
    ElkStaticPool pool_obj = {0};
    ElkStaticPool *pool = &pool_obj;
    _Alignas(_Alignof(double)) unsigned char buffer[TEST_BUF_COUNT * sizeof(double)] = {0};

    elk_static_pool_create(pool, sizeof(double), TEST_BUF_COUNT, buffer);

    double *dubs[TEST_BUF_COUNT] = {0};

    for (int i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        dubs[i] = elk_static_pool_alloc(pool);
        Assert(dubs[i]);

        *dubs[i] = (double)i;
    }

    for (int i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        Assert(*dubs[i] == (double)i);
    }

    // Test that it's full!
    for (int i = TEST_BUF_COUNT; i < 2 * TEST_BUF_COUNT; i++) 
    {
        double *no_dub = elk_static_pool_alloc(pool);
        Assert(!no_dub);
    }

    elk_static_pool_destroy(pool);
}

static void
test_pool_freeing(void)
{
    ElkStaticPool pool_obj = {0};
    ElkStaticPool *pool = &pool_obj;
    _Alignas(_Alignof(double)) unsigned char buffer[TEST_BUF_COUNT * sizeof(double)] = {0};

    elk_static_pool_create(pool, sizeof(double), TEST_BUF_COUNT, buffer);

    double *dubs[TEST_BUF_COUNT] = {0};

    for (int i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        dubs[i] = elk_static_pool_alloc(pool);
        Assert(dubs[i]);

        *dubs[i] = (double)i;
    }

    for (int i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        Assert(*dubs[i] == (double)i);
    }

    // Half empty it!
    for (int i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        elk_static_pool_free(pool, dubs[2 * i]);
        dubs[2 * i] = NULL;
    }

    for (int i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        dubs[2 * i] = elk_static_pool_alloc(pool);
        Assert(dubs[2 * i]);

        *dubs[2 * i] = (double)i;
    }

    for (int i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        Assert(*dubs[2 * i] == (double)i);
    }

    elk_static_pool_destroy(pool);
}

/*----------------------------------------------------------------------------------------------------------------------------
 *                                                 All Memory Pool Tests
 *--------------------------------------------------------------------------------------------------------------------------*/
void
elk_pool_tests(void)
{
    test_full_pool();
    test_pool_freeing();
}
