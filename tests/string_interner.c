#include "test.h"

#include <string.h>

/*-------------------------------------------------------------------------------------------------
 *
 *                                    Test String Interner
 *
 *-----------------------------------------------------------------------------------------------*/

char *some_strings[] = {
    "vegemite", "cantaloupe",    "poutine",    "cottonwood trees", "x",
    "y",        "peanut butter", "jelly time", "strawberries",     "and cream",
    "raining",  "cats and dogs", "sushi",      "date night",       "sour",
    "beer!",    "scotch",        "yes please", "raspberries",      "snack time",
};

static void
test_string_interner(void)
{
    size_t const NUM_TEST_STRINGS = sizeof(some_strings) / sizeof(some_strings[0]);

    ElkInternedString strs[NUM_TEST_STRINGS] = {0};

    ElkStringInterner *interner = elk_string_interner_create(3, 3);
    assert(interner);

    // Fill the interner with strings!
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) {
        char const *str = some_strings[i];
        strs[i] = elk_string_interner_intern(interner, str);
    }

    // Now see if we get the right ones back out!
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) {
        char const *str = some_strings[i];
        char const *interned_str = elk_string_interner_retrieve(interner, strs[i]);
        assert(interned_str);
        assert(strcmp(str, interned_str) == 0);
    }

    elk_string_interner_destroy(interner);

}

/*-------------------------------------------------------------------------------------------------
 *                                           All tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_string_interner_tests()
{
    test_string_interner();
}
