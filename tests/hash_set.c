#include "test.h"

#include <string.h>

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                 Test String Interner
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

static char *some_strings[] = 
{
    "vegemite", "cantaloupe",    "poutine",    "cottonwood trees", "x",
    "y",        "peanut butter", "jelly time", "strawberries",     "and cream",
    "raining",  "cats and dogs", "sushi",      "date night",       "sour",
    "beer!",    "scotch",        "yes please", "raspberries",      "snack time",
};

static uint64_t simple_str_hash(void const *str)
{
    return elk_fnv1a_hash_str(*(ElkStr *)str);
}

static bool str_eq(void const *left, void const *right)
{
    return elk_str_eq(*(ElkStr *)left, *(ElkStr *)right);
}

#define NUM_TEST_STRINGS (sizeof(some_strings) / sizeof(some_strings[0]))

static void
test_elk_hash_set(void)
{
    ElkStr strs[NUM_TEST_STRINGS] = {0};
    for(int i = 0; i < NUM_TEST_STRINGS; ++i)
    {
        strs[i] = elk_str_from_cstring(some_strings[i]);
    }


    ElkHashSet *set = elk_hash_set_create(2, simple_str_hash, str_eq);
    for(int i = 0; i < NUM_TEST_STRINGS; ++i)
    {
        ElkStr *str = elk_hash_set_insert(set, &strs[i]);
        Assert(str == &strs[i]);
    }

    for(int i = 0; i < NUM_TEST_STRINGS; ++i)
    {
        ElkStr *str = elk_hash_set_lookup(set, &strs[i]);
        Assert(str == &strs[i]);
    }

    ElkStr not_in_set = elk_str_from_cstring("green beans");
    ElkStr *str = elk_hash_set_lookup(set, &not_in_set);
    Assert(str == NULL);

    elk_hash_set_destroy(set);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_hash_set_tests()
{
    test_elk_hash_set();
}
