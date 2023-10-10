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

static void
test_string_interner(void)
{
    size_t const NUM_TEST_STRINGS = sizeof(some_strings) / sizeof(some_strings[0]);

    ElkStr strs[sizeof(some_strings) / sizeof(some_strings[0])] = {0};

    ElkStringInterner *interner = elk_string_interner_create(3, 3);
    Assert(interner);

    // Fill the interner with strings!
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings[i];
        strs[i] = elk_string_interner_intern_cstring(interner, str);
    }

    // Now see if we get the right ones back out!
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings[i];
        ElkStr interned_str = elk_string_interner_intern_cstring(interner, str);
        Assert(strcmp(str, interned_str.start) == 0);
        Assert(strcmp(str, strs[i].start) == 0);
        Assert(interned_str.len == strs[i].len);
        Assert(interned_str.start == strs[i].start);
    }

    elk_string_interner_destroy(interner);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_string_interner_tests()
{
    test_string_interner();
}
