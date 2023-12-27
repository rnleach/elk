#include "test.h"

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

    unsigned char buffer[ELK_KB(1)] = {0};
    ElkStaticArena arena_i = {0};
    ElkStaticArena *arena = &arena_i;
    elk_static_arena_create(arena, sizeof(buffer), buffer);

    ElkHashSet set_ = elk_hash_set_create(2, simple_str_hash, str_eq, arena);
    ElkHashSet *set = &set_;
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

    Assert(elk_len(set) == NUM_TEST_STRINGS);

    ElkStr not_in_set = elk_str_from_cstring("green beans");
    ElkStr *str = elk_hash_set_lookup(set, &not_in_set);
    Assert(str == NULL);

    elk_hash_set_destroy(set);
}

static void
test_elk_hash_set_iter(void)
{
    ElkStr strs[NUM_TEST_STRINGS] = {0};
    for(int i = 0; i < NUM_TEST_STRINGS; ++i)
    {
        strs[i] = elk_str_from_cstring(some_strings[i]);
    }

    unsigned char buffer[ELK_KB(1)] = {0};
    ElkStaticArena arena_i = {0};
    ElkStaticArena *arena = &arena_i;
    elk_static_arena_create(arena, sizeof(buffer), buffer);

    ElkHashSet set_ = elk_hash_set_create(2, simple_str_hash, str_eq, arena);
    ElkHashSet *set = &set_;
    for(int i = 0; i < NUM_TEST_STRINGS; ++i)
    {
        ElkStr *str = elk_hash_set_insert(set, &strs[i]);
        Assert(str == &strs[i]);
    }

    ElkHashSetIter iter = elk_hash_set_value_iter(set);
    ElkStr *next = elk_hash_set_value_iter_next(set, &iter);
    int found_count = 0;
    while(next)
    {
        bool found = false;
        for(int i = 0; i < NUM_TEST_STRINGS; ++i)
        {
            if(&strs[i] == next) 
            { 
                found_count += 1;
                found = true;
                break;
            }
        }

        Assert(found);

        next = elk_hash_set_value_iter_next(set, &iter);
    }
    Assert(found_count == NUM_TEST_STRINGS);

    elk_hash_set_destroy(set);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_hash_set_tests()
{
    test_elk_hash_set();
    test_elk_hash_set_iter();
}
