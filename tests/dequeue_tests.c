#include "test.h"
/*-------------------------------------------------------------------------------------------------
 *
 *                                          ElkDequeue
 *
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *                                  Test ElkDequeue Data Integrity
 *-----------------------------------------------------------------------------------------------*/
static void
elk_dequeue_test_data_integrity_back_to_front(void)
{
    ElkDequeue *dequeue = elk_dequeue_new(sizeof(int), 50);
    assert(dequeue);
    assert(elk_dequeue_empty(dequeue));
    assert(!elk_dequeue_full(dequeue));

    // Test it several times.
    for (int j = 0; j < 10; ++j) {
        for (int i = 0; i < 50; ++i) {
            assert(ELK_CODE_SUCCESS == elk_dequeue_enqueue_back(dequeue, &i));
            assert(elk_dequeue_count(dequeue) == i + 1);
        }

        assert(elk_dequeue_full(dequeue));
        assert(!elk_dequeue_empty(dequeue));

        for (int i = 50; i < 100; ++i) {
            // These shouldn't get inserted!
            assert(ELK_CODE_FULL == elk_dequeue_enqueue_back(dequeue, &i));
        }

        assert(elk_dequeue_full(dequeue));
        assert(!elk_dequeue_empty(dequeue));

        for (int i = 0; i < 50; ++i) {
            int const *x = elk_dequeue_peek_front_alias(dequeue);
            assert(*x == i);

            int y;
            assert(ELK_CODE_SUCCESS == elk_dequeue_dequeue_front(dequeue, &y));
            assert(y == i);
        }

        assert(elk_dequeue_empty(dequeue));
        assert(!elk_dequeue_full(dequeue));
    }

    elk_dequeue_free(dequeue);
}

static void
elk_dequeue_test_data_integrity_front_to_back(void)
{
    ElkDequeue *dequeue = elk_dequeue_new(sizeof(int), 50);
    assert(dequeue);
    assert(elk_dequeue_empty(dequeue));
    assert(!elk_dequeue_full(dequeue));

    // Test it several times.
    for (int j = 0; j < 10; ++j) {
        for (int i = 0; i < 50; ++i) {
            assert(ELK_CODE_SUCCESS == elk_dequeue_enqueue_front(dequeue, &i));
            assert(elk_dequeue_count(dequeue) == i + 1);
        }

        assert(elk_dequeue_full(dequeue));
        assert(!elk_dequeue_empty(dequeue));

        for (int i = 50; i < 100; ++i) {
            // These shouldn't get inserted!
            assert(ELK_CODE_FULL == elk_dequeue_enqueue_front(dequeue, &i));
        }

        assert(elk_dequeue_full(dequeue));
        assert(!elk_dequeue_empty(dequeue));

        for (int i = 0; i < 50; ++i) {
            int const *x = elk_dequeue_peek_back_alias(dequeue);
            assert(*x == i);

            int y;
            assert(ELK_CODE_SUCCESS == elk_dequeue_dequeue_back(dequeue, &y));
            assert(y == i);
        }

        assert(elk_dequeue_empty(dequeue));
        assert(!elk_dequeue_full(dequeue));
    }

    elk_dequeue_free(dequeue);
}

/*-------------------------------------------------------------------------------------------------
 *                                  Test ElkDequeue foreach
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
elk_dequeue_test_foreach_forwards(void)
{
    ElkDequeue *dequeue = elk_dequeue_new(sizeof(int), 50);
    assert(dequeue);
    assert(elk_dequeue_empty(dequeue));
    assert(!elk_dequeue_full(dequeue));

    for (int i = 0; i < 50; ++i) {
        assert(ELK_CODE_SUCCESS == elk_dequeue_enqueue_back(dequeue, &i));
        assert(elk_dequeue_count(dequeue) == i + 1);
    }

    assert(elk_dequeue_full(dequeue));
    assert(!elk_dequeue_empty(dequeue));

    int sum = 0;
    elk_dequeue_foreach(dequeue, sum_queue, &sum);
    assert(sum == 1225);

    assert(elk_dequeue_empty(dequeue));
    assert(!elk_dequeue_full(dequeue));

    elk_dequeue_free(dequeue);
}

static void
elk_dequeue_test_foreach_backwards(void)
{
    ElkDequeue *dequeue = elk_dequeue_new(sizeof(int), 50);
    assert(dequeue);
    assert(elk_dequeue_empty(dequeue));
    assert(!elk_dequeue_full(dequeue));

    for (int i = 0; i < 50; ++i) {
        assert(ELK_CODE_SUCCESS == elk_dequeue_enqueue_front(dequeue, &i));
        assert(elk_dequeue_count(dequeue) == i + 1);
    }

    assert(elk_dequeue_full(dequeue));
    assert(!elk_dequeue_empty(dequeue));

    int sum = 0;
    elk_dequeue_foreach(dequeue, sum_queue, &sum);
    assert(sum == 1225);

    assert(elk_dequeue_empty(dequeue));
    assert(!elk_dequeue_full(dequeue));

    elk_dequeue_free(dequeue);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkDequeue tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_dequeue_tests(void)
{
    elk_dequeue_test_data_integrity_back_to_front();
    elk_dequeue_test_data_integrity_front_to_back();
    elk_dequeue_test_foreach_forwards();
    elk_dequeue_test_foreach_backwards();
}
