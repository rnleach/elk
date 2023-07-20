#include "test.h"

#include <string.h>

/*-------------------------------------------------------------------------------------------------
 *
 *                                    Test Hash Table Integrity
 *
 *-----------------------------------------------------------------------------------------------*/

/* clang-format off */
struct hash_table_test_pair {
    char *key;
    char *value;
} kv_pairs[] = {
    {.key="x",             .value="y"},
    {.key="peanut butter", .value="jelly time"},
    {.key="strawberries",  .value="and cream"},
    {.key="raining",       .value="cats and dogs"},
    {.key="sushi",         .value="date night"},
    {.key="sour",          .value="beer!"},
    {.key="scotch",        .value="yes please"},
    {.key="raspberries",   .value="snack time"},
    {.key="",              .value=""},
};
/* clang-format on */

char *not_in_table[] = {
    "vegemite",
    "cantaloupe",
    "poutine",
    "cottonwood trees",
};

void
test_hash_table_data_integrity_strings(void)
{
    ElkHashTable *table = elk_hash_table_create(8, elk_fnv1a_hash);
    assert(table);

    // Insert the values into the table.
    for (size_t i = 0; i < sizeof(kv_pairs) / sizeof(kv_pairs[0]); ++i) {
        struct hash_table_test_pair pair = kv_pairs[i];

        bool already_in_table =
            elk_hash_table_insert(table, strlen(pair.key), pair.key, pair.value);
        assert(!already_in_table); // Started with empty table, so none should be in there already.
    }

    // Try to insert them again!
    for (size_t i = 0; i < sizeof(kv_pairs) / sizeof(kv_pairs[0]); ++i) {
        struct hash_table_test_pair pair = kv_pairs[i];

        bool already_in_table =
            elk_hash_table_insert(table, strlen(pair.key), pair.key, pair.value);
        assert(already_in_table); // We inserted them above already, so they should be in there.
    }

    // Check that we can retrieve them and they match.
    for (size_t i = 0; i < sizeof(kv_pairs) / sizeof(kv_pairs[0]); ++i) {
        struct hash_table_test_pair pair = kv_pairs[i];

        char const *retrieved_value = elk_hash_table_retrieve(table, strlen(pair.key), pair.key);
        assert(retrieved_value);
        assert(strcmp(retrieved_value, pair.value) == 0);
    }

    // Check some strings we didn't insert in the table.
    for (size_t i = 0; i < sizeof(not_in_table) / sizeof(not_in_table[0]); ++i) {
        char *key = not_in_table[i];

        char const *value = elk_hash_table_retrieve(table, strlen(key), key);
        assert(!value);
    }

    elk_hash_table_destroy(table);
}

/*-------------------------------------------------------------------------------------------------
 *
 *                              Test Hash Table with a Numeric Key
 *
 *-----------------------------------------------------------------------------------------------*/

void
test_numeric_key_type(void)
{
    ElkHashTable *table = elk_hash_table_create(8, elk_fnv1a_hash);
    assert(table);

    for (size_t i = 0; i < sizeof(not_in_table) / sizeof(not_in_table[0]); ++i) {
        char *value = not_in_table[i];

        bool already_in_table = elk_hash_table_insert(table, sizeof i, &i, value);
        assert(!already_in_table);
    }

    for (size_t i = 0; i < sizeof(not_in_table) / sizeof(not_in_table[0]); ++i) {
        char *value = not_in_table[i];

        bool already_in_table = elk_hash_table_insert(table, sizeof i, &i, value);
        assert(already_in_table);
    }

    for (size_t i = 0; i < sizeof(not_in_table) / sizeof(not_in_table[0]); ++i) {
        char *value = not_in_table[i];

        char const *retrieved_value = elk_hash_table_retrieve(table, sizeof i, &i);
        assert(retrieved_value);
        assert(strcmp(retrieved_value, value) == 0);
    }

    elk_hash_table_destroy(table);
}

/*-------------------------------------------------------------------------------------------------
 *                                           All tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_hash_table_tests(void)
{
    test_hash_table_data_integrity_strings();
    test_numeric_key_type();
}
