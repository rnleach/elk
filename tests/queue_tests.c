#include "test.h"
/*-------------------------------------------------------------------------------------------------
 *
 *                                          ElkQueue
 *
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *                                  Test ElkQueue Data Integrity
 *-----------------------------------------------------------------------------------------------*/
static void
elk_queue_test_data_integrity(void)
{
    ElkQueue *queue = elk_queue_new(sizeof(int), 50);
    assert(queue);
    assert(elk_queue_empty(queue));
    assert(!elk_queue_full(queue));

    for (int i = 0; i < 50; ++i) {
        assert(elk_queue_enqueue(queue, &i));
        assert(elk_queue_count(queue) == i + 1);
    }

    assert(elk_queue_full(queue));
    assert(!elk_queue_empty(queue));

    for (int i = 50; i < 100; ++i) {
        // These shouldn't get inserted!
        assert(!elk_queue_enqueue(queue, &i));
    }

    assert(elk_queue_full(queue));
    assert(!elk_queue_empty(queue));

    for (int i = 0; i < 50; ++i) {
        int const *x = elk_queue_peek_alias(queue);
        assert(*x == i);

        int y;
        assert(elk_queue_dequeue(queue, &y));
        assert(y == i);
    }

    assert(elk_queue_empty(queue));
    assert(!elk_queue_full(queue));

    queue = elk_queue_free(queue);
    assert(!queue);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkQueue tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_queue_tests(void)
{
    elk_queue_test_data_integrity();
}