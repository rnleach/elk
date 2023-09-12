#pragma once
//
// For testing, ensure we have some debugging tools activated.
//

// We must have asserts working for the tests to work.
#ifdef NDEBUG
#    undef NDEBUG
#endif

// In the event of a panic, this should cause a crash and let the debugger take over if the test
// is running under a debugger. Otherwise it will just exit.
#ifndef ELK_PANIC_CRASH
#    define ELK_PANIC_CRASH
#endif

#include <assert.h>

#include "../src/elk.h"

void elk_time_tests(void);
void elk_fnv1a_tests(void);
void elk_string_interner_tests(void);
void elk_arena_tests(void);
void elk_pool_tests(void);
