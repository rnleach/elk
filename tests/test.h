#ifndef ELK_TESTS
#define ELK_TESTS
//
// For testing, ensure we have some debugging tools activated.
//

// We must have asserts working for the tests to work.
#ifdef NDEBUG
#    undef NDEBUG
#endif

#define _ELK_TRACK_MEM_USAGE

#include <stdio.h>
#include "../src/elk.h"

void elk_time_tests(void);
void elk_fnv1a_tests(void);
void elk_str_tests(void);
void elk_parse_tests(void);
void elk_arena_tests(void);
void elk_pool_tests(void);
void elk_string_interner_tests(void);
void elk_queue_ledger_tests(void);
void elk_array_ledger_tests(void);
void elk_hash_table_tests(void);
void elk_hash_set_tests(void);
void elk_csv_tests(void);

#endif
