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

// Turn on ELK_MEMORY_DEBUG from the command line to check for memory errors during testing.

#include <assert.h>
#include <math.h>

#include "../src/elk.h"

void elk_time_tests(void);
void elk_list_tests(void);
void elk_queue_tests(void);
void elk_hilbert_tests(void);
void elk_rtree_view_tests(void);
