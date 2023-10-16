#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                              Tests for the Memory Arena
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
    unsigned char arena_buffer[ELK_KB(1)] = {0};
    ElkStaticArena arena_i = {0};
    ElkStaticArena *arena = &arena_i;
    elk_static_arena_create(arena, sizeof(arena_buffer), arena_buffer);

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

static void
test_static_arena_realloc(void)
{
    _Alignas(_Alignof(double)) unsigned char buffer[100 * sizeof(double)];
    ElkStaticArena arena_instance = {0};
    ElkStaticArena *arena = &arena_instance;

    elk_static_arena_create(arena, sizeof(buffer), buffer);

    double *ten_dubs = elk_allocator_nmalloc(arena, 10, double);
    Assert(ten_dubs);

    for(int i = 0; i < 10; ++i)
    {
        ten_dubs[i] = (double)i;
    }

    double *hundred_dubs = elk_static_arena_nrealloc(arena, ten_dubs, 100, double);

    Assert(hundred_dubs);
    Assert(hundred_dubs == ten_dubs);

    for(int i = 0; i < 10; ++i)
    {
        Assert(hundred_dubs[i] == (double)i);
    }

    for(int i = 10; i < 100; ++i)
    {
        hundred_dubs[i] = (double)i;
    }

    for(int i = 10; i < 100; ++i)
    {
        Assert(hundred_dubs[i] == (double)i);
    }

    double *million_dubs = elk_static_arena_realloc(arena, hundred_dubs, 1000000 * sizeof *ten_dubs);
    Assert(!million_dubs);

    elk_static_arena_destroy(arena);
}

static void
test_static_arena_free(void)
{
    unsigned char buffer[10 * sizeof(double)];
    ElkStaticArena arena_instance = {0};
    ElkStaticArena *arena = &arena_instance;

    elk_static_arena_create(arena, sizeof(buffer), buffer);

    double *first = elk_allocator_malloc(arena, double);
    Assert(first);
    *first = 2.0;

    elk_allocator_free(arena, first);

    double *second = elk_allocator_malloc(arena, double);
    Assert(second);
    Assert(first == second); // Since we freed 'first', that's what we should get back this time.

    double *third = elk_allocator_malloc(arena, double);
    Assert(third);

    size_t offset_before = arena->buf_offset;
    elk_allocator_free(arena, second); // should be a no op
    double *fourth = elk_allocator_malloc(arena, double);
    Assert(fourth);

    size_t offset_after = arena->buf_offset;

    Assert(offset_before != offset_after);
    Assert(offset_before < offset_after);

    elk_static_arena_destroy(arena);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                All Memory Arena Tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_arena_tests(void)
{
    test_arena();
    test_static_arena_realloc();
    test_static_arena_free();
}
