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

    // Test it several times to make sure we can put a lot through the queue
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < 50; ++i) {
            assert(ELK_CODE_SUCCESS == elk_queue_enqueue(queue, &i));
            assert(elk_queue_count(queue) == i + 1);
        }

        assert(elk_queue_full(queue));
        assert(!elk_queue_empty(queue));

        for (int i = 50; i < 100; ++i) {
            // These shouldn't get inserted!
            assert(ELK_CODE_FULL == elk_queue_enqueue(queue, &i));
        }

        assert(elk_queue_full(queue));
        assert(!elk_queue_empty(queue));

        for (int i = 0; i < 50; ++i) {
            int const *x = elk_queue_peek_alias(queue);
            assert(*x == i);

            int y;
            assert(ELK_CODE_SUCCESS == elk_queue_dequeue(queue, &y));
            assert(y == i);
        }

        assert(elk_queue_empty(queue));
        assert(!elk_queue_full(queue));
    }

    elk_queue_free(queue);
}

/*-------------------------------------------------------------------------------------------------
 *                                  Test ElkQueue foreach
 *-----------------------------------------------------------------------------------------------*/
static bool
sum_queue(void *item, void *sum)
{
    int x = *(int *)item;
    int *running_sum = sum;
    *running_sum += x;

    return true;
}

static void
elk_queue_test_foreach(void)
{
    ElkQueue *queue = elk_queue_new(sizeof(int), 50);
    assert(queue);
    assert(elk_queue_empty(queue));
    assert(!elk_queue_full(queue));

    for (int i = 0; i < 50; ++i) {
        assert(ELK_CODE_SUCCESS == elk_queue_enqueue(queue, &i));
        assert(elk_queue_count(queue) == i + 1);
    }

    assert(elk_queue_full(queue));
    assert(!elk_queue_empty(queue));

    int sum = 0;
    elk_queue_foreach(queue, sum_queue, &sum);
    assert(sum == 1225);

    assert(elk_queue_empty(queue));
    assert(!elk_queue_full(queue));

    elk_queue_free(queue);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkQueue tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_queue_tests(void)
{
    elk_queue_test_data_integrity();
    elk_queue_test_foreach();
}
