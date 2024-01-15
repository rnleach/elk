#include "test.h"

#include <string.h>

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
    usize len = strlen(str) + 1;
    char *dest = elk_static_arena_nmalloc(arena, len, char);
    Assert(dest);

    strcpy(dest, str);

    return dest;
}

static char test_chars[18] = 
{
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',  'I', 'J', 'K', '1', '2', '$', '^', '&', '\t', '\0'
};

static f64 test_f64[6] = { 0.0, 1.0, 2.17, 3.14159, 9.81, 1.6666 };

static void
test_arena(void)
{
    byte arena_buffer[ELK_KB(1)] = {0};
    ElkStaticArena arena_i = {0};
    ElkStaticArena *arena = &arena_i;
    elk_static_arena_create(arena, sizeof(arena_buffer), arena_buffer);

    for (i32 trip_num = 1; trip_num <= 5; trip_num++) 
    {

        char *arena_strs[6] = {0};
        char *arena_chars[18] = {0};
        f64 *arena_f64[6] = {0};
        i32 *arena_ints[6] = {0};

        // Fill the arena
        for (i32 j = 0; j < 6; j++) 
        {
            arena_chars[j * 3 + 0] = elk_static_arena_malloc(arena, char);
            *arena_chars[j * 3 + 0] = test_chars[j * 3 + 0];

            arena_f64[j] = elk_static_arena_malloc(arena, f64);
            *arena_f64[j] = test_f64[j];

            arena_chars[j * 3 + 1] = elk_static_arena_malloc(arena, char);
            *arena_chars[j * 3 + 1] = test_chars[j * 3 + 1];

            arena_strs[j] = copy_string_to_arena(arena, tst_strings[j]);

            arena_chars[j * 3 + 2] = elk_static_arena_malloc(arena, char);
            *arena_chars[j * 3 + 2] = test_chars[j * 3 + 2];

            arena_ints[j] = elk_static_arena_malloc(arena, i32);
            *arena_ints[j] = 2 * trip_num + 3 * j;
        }

        // Test the values!
        for (i32 j = 0; j < 6; j++) 
        {
            Assert(*arena_chars[j * 3 + 0] == test_chars[j * 3 + 0]);
            Assert(*arena_f64[j] == test_f64[j]);
            Assert(*arena_chars[j * 3 + 1] == test_chars[j * 3 + 1]);
            Assert(strcmp(tst_strings[j], arena_strs[j]) == 0);
            Assert(*arena_chars[j * 3 + 2] == test_chars[j * 3 + 2]);
            Assert(*arena_ints[j] == 2 * trip_num + 3 * j);
        }

        elk_static_arena_reset(arena);
    }

#ifdef _ELK_TRACK_MEM_USAGE
    f64 pct_mem = elk_static_arena_max_ratio(arena) * 100.0;
    bool over_allocated = elk_static_arena_over_allocated(arena);
    fprintf(stdout, "%s used %.2lf%% of arena and arena %s over allocated.\n",
            __func__, pct_mem, over_allocated ? "***WAS***": "was not");
#endif

    elk_static_arena_destroy(arena);
}

static void
test_static_arena_realloc(void)
{
    _Alignas(_Alignof(f64)) byte buffer[100 * sizeof(f64)];
    ElkStaticArena arena_instance = {0};
    ElkStaticArena *arena = &arena_instance;

    elk_static_arena_create(arena, sizeof(buffer), buffer);

    f64 *ten_dubs = elk_static_arena_nmalloc(arena, 10, f64);
    Assert(ten_dubs);

    for(i32 i = 0; i < 10; ++i)
    {
        ten_dubs[i] = (f64)i;
    }

    f64 *hundred_dubs = elk_static_arena_nrealloc(arena, ten_dubs, 100, f64);

    Assert(hundred_dubs);
    Assert(hundred_dubs == ten_dubs);

    for(i32 i = 0; i < 10; ++i)
    {
        Assert(hundred_dubs[i] == (f64)i);
    }

    for(i32 i = 10; i < 100; ++i)
    {
        hundred_dubs[i] = (f64)i;
    }

    for(i32 i = 10; i < 100; ++i)
    {
        Assert(hundred_dubs[i] == (f64)i);
    }

    f64 *million_dubs = elk_static_arena_realloc(arena, hundred_dubs, 1000000 * sizeof *ten_dubs);
    Assert(!million_dubs);


#ifdef _ELK_TRACK_MEM_USAGE
    f64 pct_mem = elk_static_arena_max_ratio(arena) * 100.0;
    bool over_allocated = elk_static_arena_over_allocated(arena);
    fprintf(stdout, "%s used %.2lf%% of arena and arena %s over allocated (should be overallocated as part of test).\n",
            __func__, pct_mem, over_allocated ? "was": "***WAS NOT***");
#endif

    elk_static_arena_destroy(arena);
}

static void
test_static_arena_free(void)
{
    byte buffer[10 * sizeof(f64)];
    ElkStaticArena arena_instance = {0};
    ElkStaticArena *arena = &arena_instance;

    elk_static_arena_create(arena, sizeof(buffer), buffer);

    f64 *first = elk_static_arena_malloc(arena, f64);
    Assert(first);
    *first = 2.0;

    elk_static_arena_free(arena, first);

    f64 *second = elk_static_arena_malloc(arena, f64);
    Assert(second);
    Assert(first == second); // Since we freed 'first', that's what we should get back this time.

    f64 *third = elk_static_arena_malloc(arena, f64);
    Assert(third);

    usize offset_before = arena->buf_offset;
    elk_static_arena_free(arena, second); // should be a no op
    f64 *fourth = elk_static_arena_malloc(arena, f64);
    Assert(fourth);

    usize offset_after = arena->buf_offset;

    Assert(offset_before != offset_after);
    Assert(offset_before < offset_after);


#ifdef _ELK_TRACK_MEM_USAGE
    f64 pct_mem = elk_static_arena_max_ratio(arena) * 100.0;
    bool over_allocated = elk_static_arena_over_allocated(arena);
    fprintf(stdout, "%s used %.2lf%% of arena and arena %s over allocated.\n",
            __func__, pct_mem, over_allocated ? "***WAS***": "was not");
#endif

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
