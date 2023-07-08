#include "test.h"
#include <limits.h>
/*-------------------------------------------------------------------------------------------------
 *
 *                                          ElkHeap
 *
 *-----------------------------------------------------------------------------------------------*/

static int
int_priority(void *p)
{
    int *v = p;
    return *v;
}

/*-------------------------------------------------------------------------------------------------
 *                                  Test Bounded ElkHeap Data Integrity
 *-----------------------------------------------------------------------------------------------*/
static void
elk_heap_bounded_test_data_integrity(void)
{
    ElkHeap *heap = elk_bounded_heap_new(sizeof(int), 30, int_priority, 5);
    ElkCode ret = ELK_CODE_SUCCESS;

    int i = -10;
    while (ret == ELK_CODE_SUCCESS) {
        heap = elk_heap_insert(heap, &i, &ret);
        if (ret == ELK_CODE_SUCCESS)
            ++i;
    }

    assert(i == 20);
    assert(elk_heap_count(heap) == 30);

    int val = INT_MAX;
    int prev_val = INT_MAX;
    ret = ELK_CODE_SUCCESS;
    do {
        assert(val <= prev_val);
        heap = elk_heap_top(heap, &val, &ret);
    } while (ret == ELK_CODE_SUCCESS);

    assert(ret == ELK_CODE_EMPTY);
    assert(elk_heap_count(heap) == 0);

    heap = elk_heap_top(heap, &val, &ret);
    assert(ret == ELK_CODE_EMPTY);

    heap = elk_heap_free(heap);
    assert(!heap);
}
/*-------------------------------------------------------------------------------------------------
 *                                      All ElkHeap tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_heap_tests(void)
{
    elk_heap_bounded_test_data_integrity();
}
