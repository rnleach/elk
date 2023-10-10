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
test_elk_str_table(void)
{
    size_t const NUM_TEST_STRINGS = sizeof(some_strings) / sizeof(some_strings[0]);

    ElkStr strs[sizeof(some_strings) / sizeof(some_strings[0])] = {0};
	int64_t values[sizeof(some_strings) / sizeof(some_strings[0])] = {0};
	int64_t values2[sizeof(some_strings) / sizeof(some_strings[0])] = {0};

	ElkStrMap *map = elk_str_map_create(2, NULL); // Use a crazy small size_exp to force it to grow, this IS a test!
    Assert(map);

    // Fill the map
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings[i];
        strs[i] = elk_str_from_cstring(str);
		values[i] = i;
		values2[i] = i; // We'll use this later!
		
		int64_t *vptr = elk_str_map_insert(map, strs[i], &values[i]);
		Assert(vptr == &values[i]);
    }

    // Now see if we get the right ones back out!
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
		int64_t *vptr = elk_str_map_lookup(map, strs[i]);
		Assert(vptr == &values[i]);
		Assert(*vptr == i);
    }

    // Fill the map with NEW values
    for (size_t i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
		int64_t *vptr = elk_str_map_insert(map, strs[i], &values2[i]);
		Assert(vptr == &values[i]); // should get the old value pointer back
		Assert(vptr != &values2[i]); // should not point at the pointer we passed in, because we replaced a value
    }

    elk_str_map_destroy(map);
}

static uint64_t 
id_hash(void const *value)
{
    return *(int64_t const*)value;
}

static bool
int64_eq(void *left, void *right)
{
    int64_t *ileft = left;
    int64_t *iright = right;
    return *ileft == *iright;
}

#define NUM_KEYS 20
static void
test_elk_hash_table(void)
{
	ElkTime keys[NUM_KEYS] = {0};
	int64_t values[NUM_KEYS] = {0};
	int64_t values2[NUM_KEYS] = {0};

	// Initialize the keys and values
	for(int i = 0; i < NUM_KEYS; ++i)
	{
		keys[i] = elk_time_from_ymd_and_hms(2000 + i, 1 + i % 12, 1 + i, i, i, i);
		values[i] = i;
		values2[i] = i;
	}

	// Fill the hashmap
	ElkHashMap *map = elk_hash_map_create(2, id_hash, int64_eq);
	for(int i = 0; i < NUM_KEYS; ++i)
	{
		int64_t *vptr = elk_hash_map_insert(map, &keys[i], &values[i]);
		Assert(vptr == &values[i]);
	}

	// check the values
	for(int i = 0; i < NUM_KEYS; ++i)
	{
		int64_t *vptr = elk_hash_map_lookup(map, &keys[i]);
		Assert(vptr == &values[i]);
	}

	// Fill the hashmap with something else
	for(int i = 0; i < NUM_KEYS; ++i)
	{
		int64_t *vptr = elk_hash_map_insert(map, &keys[i], &values2[i]);
		Assert(vptr == &values[i]);
		Assert(vptr != &values2[i]);
	}

	// Clean up
	elk_hash_map_destroy(map);
}
#undef NUM_KEYS

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_hash_table_tests()
{
    test_elk_str_table();
    test_elk_hash_table();
}
