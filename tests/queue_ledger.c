#include "test.h"

/*------------------------------------------------------------------------------------------------
 *
 *                               Tests for the Queue Ledger
 *
 *-----------------------------------------------------------------------------------------------*/
#define TEST_BUF_COUNT 10

static void
test_empty_full_queue(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    int ibuf[TEST_BUF_COUNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    ElkQueueLedger queue = elk_queue_ledger_create(TEST_BUF_COUNT);
    ElkQueueLedger *qp = &queue;

    // It should be empty now - for as many calls as I make.
    assert(elk_queue_ledger_empty(qp));
    assert(!elk_queue_ledger_full(qp));
    for (int i = 0; i < 5; ++i) {
        assert(elk_queue_ledger_empty(qp));
        assert(!elk_queue_ledger_full(qp));
        assert(elk_queue_ledger_pop_front_index(qp) == ELK_COLLECTION_LEDGER_EMPTY);
    }

    // Let's fill it up!
    for (int i = 0; i < TEST_BUF_COUNT; ++i) {
        assert(!elk_queue_ledger_full(qp)); // Should never be full in this loop

        size_t push_idx = elk_queue_ledger_push_back_index(qp);
        ibuf[push_idx] = i;

        assert(!elk_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
    }

    assert(elk_queue_ledger_full(qp));

    // All the rest of these should fail
    for (int i = 0; i < 5; ++i) {
        assert(elk_queue_ledger_full(qp));
        assert(!elk_queue_ledger_empty(qp));
        assert(elk_queue_ledger_push_back_index(qp) == ELK_COLLECTION_LEDGER_FULL);
    }

    // Let's empty it out.
    for (int i = 0; i < TEST_BUF_COUNT; ++i) {
        assert(!elk_queue_ledger_empty(qp)); // Should never be empty at the start of this loop.

        size_t pop_idx = elk_queue_ledger_pop_front_index(qp);
        assert(ibuf[pop_idx] == i); // They should come out in the order we put them in.

        assert(!elk_queue_ledger_full(qp)); // Should never be full after we pop
    }

    // It should be empty now - for as many calls as I make.
    assert(elk_queue_ledger_empty(qp));
    assert(!elk_queue_ledger_full(qp));
    for (int i = 0; i < 5; ++i) {
        assert(elk_queue_ledger_empty(qp));
        assert(!elk_queue_ledger_full(qp));
        assert(elk_queue_ledger_pop_front_index(qp) == ELK_COLLECTION_LEDGER_EMPTY);
    }
}

static void
test_lots_of_throughput(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    int ibuf[TEST_BUF_COUNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    ElkQueueLedger queue = elk_queue_ledger_create(TEST_BUF_COUNT);
    ElkQueueLedger *qp = &queue;

    // Let's put a few in there.
    for (int i = 0; i < TEST_BUF_COUNT / 2; ++i) {
        assert(!elk_queue_ledger_full(qp)); // Should never be full in this loop

        size_t push_idx = elk_queue_ledger_push_back_index(qp);
        ibuf[push_idx] = i;

        assert(!elk_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
    }

    // Cycle through adding and removing from the queue
    int const reps = 100;
    for (int i = 0; i < reps; ++i) {

        // Let's put a few more in there.
        for (int i = 0; i < TEST_BUF_COUNT / 2; ++i) {
            assert(!elk_queue_ledger_full(qp)); // Should never be full in this loop

            size_t push_idx = elk_queue_ledger_push_back_index(qp);
            ibuf[push_idx] = i;

            assert(!elk_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
        }

        // Let's pull a few out.
        for (int i = 0; i < TEST_BUF_COUNT / 2; ++i) {
            assert(!elk_queue_ledger_empty(qp)); // Should never be empty in this loop

            size_t pop_idx = elk_queue_ledger_pop_front_index(qp);
            assert(ibuf[pop_idx] == i);

            assert(!elk_queue_ledger_full(qp)); // Should never be empty after we've pushed.
        }
    }
}

static void
test_test_peek(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    int ibuf[TEST_BUF_COUNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    ElkQueueLedger queue = elk_queue_ledger_create(TEST_BUF_COUNT);
    ElkQueueLedger *qp = &queue;

    // It should be empty now - for as many calls as I make.
    assert(elk_queue_ledger_empty(qp));
    assert(!elk_queue_ledger_full(qp));
    for (int i = 0; i < 5; ++i) {
        assert(elk_queue_ledger_peek_front_index(qp) == ELK_COLLECTION_LEDGER_EMPTY);
    }

    // Let's fill it up!
    for (int i = 0; i < TEST_BUF_COUNT; ++i) {
        assert(!elk_queue_ledger_full(qp)); // Should never be full in this loop

        size_t push_idx = elk_queue_ledger_push_back_index(qp);
        ibuf[push_idx] = i;

        assert(!elk_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
    }

    // Let's empty it out.
    for (int i = 0; i < TEST_BUF_COUNT; ++i) {
        assert(!elk_queue_ledger_empty(qp)); // Should never be empty at the start of this loop.

        size_t peek_idx = elk_queue_ledger_peek_front_index(qp);
        assert(ibuf[peek_idx] == i); // They should come out in the order we put them in.

        size_t pop_idx = elk_queue_ledger_pop_front_index(qp);
        assert(ibuf[pop_idx] == i); // They should come out in the order we put them in.

        assert(peek_idx == pop_idx);

        assert(!elk_queue_ledger_full(qp)); // Should never be full after we pop
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                      All Queue Ledger Tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_queue_ledger_tests(void)
{
    test_empty_full_queue();
    test_lots_of_throughput();
    test_test_peek();
}
