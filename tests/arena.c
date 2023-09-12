#include "test.h"

#include <string.h>

/*------------------------------------------------------------------------------------------------
 *
 *                                  Tests for the Memory Arena
 *
 *-----------------------------------------------------------------------------------------------*/
static char *tst_strings[6] = {
    "test string 1",
    "peanut butter jelly time",
    "eat good food! not peanut butter jelly",
    "brocolli",
    "grow a vegetable garden for your health and sanity",
    "dogs are better people....except they'll poop anywhere...that's a flaw",
};

static char *
copy_string_to_arena(ElkArenaAllocator *arena, char const *str)
{
    size_t len = strlen(str) + 1;
    char *dest = elk_arena_nmalloc(arena, len, char);
    assert(dest);

    strcpy(dest, str);

    return dest;
}

static char test_chars[18] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',  'I',
                              'J', 'K', '1', '2', '$', '^', '&', '\t', '\0'};

static double test_doubles[6] = {0.0, 1.0, 2.17, 3.14159, 9.81, 1.6666};

static void
test_arena(void)
{
    // This is a really small block size, but we want to test the arena expansion as well.
    ElkArenaAllocator arena_i = {0};
    ElkArenaAllocator *arena = &arena_i;
    elk_arena_init(arena, 50);

    for (int trip_num = 1; trip_num <= 5; trip_num++) {

        char *arena_strs[6] = {0};
        char *arena_chars[18] = {0};
        double *arena_doubles[6] = {0};
        int *arena_ints[6] = {0};

        // Fill the arena
        for (int j = 0; j < 6; j++) {
            arena_chars[j * 3 + 0] = elk_arena_malloc(arena, char);
            *arena_chars[j * 3 + 0] = test_chars[j * 3 + 0];

            arena_doubles[j] = elk_arena_malloc(arena, double);
            *arena_doubles[j] = test_doubles[j];

            arena_chars[j * 3 + 1] = elk_arena_malloc(arena, char);
            *arena_chars[j * 3 + 1] = test_chars[j * 3 + 1];

            arena_strs[j] = copy_string_to_arena(arena, tst_strings[j]);

            arena_chars[j * 3 + 2] = elk_arena_malloc(arena, char);
            *arena_chars[j * 3 + 2] = test_chars[j * 3 + 2];

            arena_ints[j] = elk_arena_malloc(arena, int);
            *arena_ints[j] = 2 * trip_num + 3 * j;
        }

        // Test the values!
        for (int j = 0; j < 6; j++) {
            assert(*arena_chars[j * 3 + 0] == test_chars[j * 3 + 0]);
            assert(*arena_doubles[j] == test_doubles[j]);
            assert(*arena_chars[j * 3 + 1] == test_chars[j * 3 + 1]);
            assert(strcmp(tst_strings[j], arena_strs[j]) == 0);
            assert(*arena_chars[j * 3 + 2] == test_chars[j * 3 + 2]);
            assert(*arena_ints[j] == 2 * trip_num + 3 * j);
        }

        elk_arena_reset(arena);
    }

    elk_arena_destroy(arena);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All Memory Arena Tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_arena_tests(void)
{
    test_arena();
}
