#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                           Tests for the Generic Memory API
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
        dubs[i] = elk_allocator_malloc(pool, double);
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
        double *no_dub = elk_allocator_malloc(pool, double);
        Assert(!no_dub);
    }

    elk_allocator_destroy(pool);
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
        dubs[i] = elk_allocator_malloc(pool, double);
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
        elk_allocator_free(pool, dubs[2 * i]);
        dubs[2 * i] = NULL;
    }

    for (int i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        dubs[2 * i] = elk_allocator_malloc(pool, double);
        Assert(dubs[2 * i]);

        *dubs[2 * i] = (double)i;
    }

    for (int i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        Assert(*dubs[2 * i] == (double)i);
    }

    elk_allocator_destroy(pool);
}

/*----------------------------------------------------------------------------------------------------------------------------
 *                                                 All Memory Pool Tests
 *--------------------------------------------------------------------------------------------------------------------------*/
static void
elk_pool_via_generic_api_tests(void)
{
    test_full_pool();
    test_pool_freeing();
}

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                               Tests for the Memory Arena
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
static char *tst_strings[6] = 
{
    "test string 1",
    "peanut butter jelly time",
    "eat good food! not peanut butter jelly",
    "brocolli",
    "grow a vegetable garden for your health and sanity",
    "dogs are better people....except they'll poop anywhere...that's a flaw",
};

static char *
copy_string_to_arena(ElkStaticArena *arena, char const *str)
{
    size_t len = strlen(str) + 1;
    char *dest = elk_allocator_nmalloc(arena, len, char);
    Assert(dest);

    strcpy(dest, str);

    return dest;
}

static char test_chars[18] =
{
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',  'I', 'J', 'K', '1', '2', '$', '^', '&', '\t', '\0'
};

static double test_doubles[6] = { 0.0, 1.0, 2.17, 3.14159, 9.81, 1.6666 };

static void
test_arena(void)
{
	unsigned char buffer[ELK_KB(1)] = {0};
    ElkStaticArena arena_i = {0};
    ElkStaticArena *arena = &arena_i;
    elk_static_arena_create(arena, sizeof(buffer), buffer);

    for (int trip_num = 1; trip_num <= 5; trip_num++) 
    {
        char *arena_strs[6] = {0};
        char *arena_chars[18] = {0};
        double *arena_doubles[6] = {0};
        int *arena_ints[6] = {0};

        // Fill the arena
        for (int j = 0; j < 6; j++) 
        {
            arena_chars[j * 3 + 0] = elk_allocator_malloc(arena, char);
            *arena_chars[j * 3 + 0] = test_chars[j * 3 + 0];

            arena_doubles[j] = elk_allocator_malloc(arena, double);
            *arena_doubles[j] = test_doubles[j];

            arena_chars[j * 3 + 1] = elk_allocator_malloc(arena, char);
            *arena_chars[j * 3 + 1] = test_chars[j * 3 + 1];

            arena_strs[j] = copy_string_to_arena(arena, tst_strings[j]);

            arena_chars[j * 3 + 2] = elk_allocator_malloc(arena, char);
            *arena_chars[j * 3 + 2] = test_chars[j * 3 + 2];

            arena_ints[j] = elk_allocator_malloc(arena, int);
            *arena_ints[j] = 2 * trip_num + 3 * j;
        }

        // Test the values!
        for (int j = 0; j < 6; j++) 
        {
            Assert(*arena_chars[j * 3 + 0] == test_chars[j * 3 + 0]);
            Assert(*arena_doubles[j] == test_doubles[j]);
            Assert(*arena_chars[j * 3 + 1] == test_chars[j * 3 + 1]);
            Assert(strcmp(tst_strings[j], arena_strs[j]) == 0);
            Assert(*arena_chars[j * 3 + 2] == test_chars[j * 3 + 2]);
            Assert(*arena_ints[j] == 2 * trip_num + 3 * j);
        }

        elk_allocator_reset(arena);
    }

    elk_allocator_destroy(arena);
}

/*----------------------------------------------------------------------------------------------------------------------------
 *                                                All Memory Arena Tests
 *--------------------------------------------------------------------------------------------------------------------------*/
static void
elk_arena_via_generic_api_tests(void)
{
    test_arena();
}

/*----------------------------------------------------------------------------------------------------------------------------
 *                                             All Memory Generic API Tests
 *--------------------------------------------------------------------------------------------------------------------------*/
void
elk_allocator_generic_api_tests(void)
{
    elk_pool_via_generic_api_tests();
    elk_arena_via_generic_api_tests();
}
