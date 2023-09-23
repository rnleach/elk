#include "test.h"

/*------------------------------------------------------------------------------------------------
 *
 *                               Tests for the Array Ledger
 *
 *-----------------------------------------------------------------------------------------------*/
#define TEST_BUF_COUNT 10

static void
test_empty_full_array(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    int ibuf[TEST_BUF_COUNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    ElkArrayLedger array = elk_array_ledger_create(TEST_BUF_COUNT);
    ElkArrayLedger *ap = &array;

    // It should be empty now - for as many calls as I make.
    assert(elk_array_ledger_empty(ap));
    assert(!elk_array_ledger_full(ap));
    assert(elk_array_ledger_len(ap) == 0);
    for (int i = 0; i < 5; ++i) {
        assert(elk_array_ledger_empty(ap));
        assert(!elk_array_ledger_full(ap));
        assert(elk_array_ledger_len(ap) == 0);
    }

    // Let's fill it up!
    for (int i = 0; i < TEST_BUF_COUNT; ++i) {
        assert(!elk_array_ledger_full(ap)); // Should never be full in this loop

        size_t push_idx = elk_array_ledger_push_back_index(ap);
        ibuf[push_idx] = i;

        assert(!elk_array_ledger_empty(ap)); // Should never be empty after we've pushed.
    }

    assert(elk_array_ledger_full(ap));
    assert(elk_array_ledger_len(ap) == TEST_BUF_COUNT);

    // All the rest of the pushes should fail
    for (int i = 0; i < 5; ++i) {
        assert(elk_array_ledger_full(ap));
        assert(!elk_array_ledger_empty(ap));
        assert(elk_array_ledger_len(ap) == TEST_BUF_COUNT);
        assert(elk_array_ledger_push_back_index(ap) == ELK_COLLECTION_LEDGER_FULL);
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                      All Array Ledger Tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_array_ledger_tests(void)
{
    test_empty_full_array();
}
