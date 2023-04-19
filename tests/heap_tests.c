#include "test.h"
/*-------------------------------------------------------------------------------------------------
 *
 *                                          ElkHeap
 *
 *-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
 *                                  Test ElkHeap Data Integrity
 *-----------------------------------------------------------------------------------------------*/
static int
int_priority(void *p)
{
    int *v = p;
    return *v;
}

static void
elk_heap_test_data_integrity(void)
{
    ElkHeap *heap = elk_heap_new(sizeof(int), int_priority, 3);
    ElkCode ret = ELK_CODE_SUCCESS;

    for(int i = -300; i <= 400; ++i) {
        heap = elk_heap_insert(heap, &i, &ret);
        assert(!elk_is_error(ret));
    }

    assert(elk_heap_count(heap) == 701);

    int val = 0;
    for(int i = 400; i >= -300; --i) {
        int const *top = elk_heap_peek(heap);
        assert(*top == i);
        heap = elk_heap_top(heap, &val, &ret);
        assert(val == i);
        assert(!elk_is_error(ret));
    }

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
    elk_heap_test_data_integrity();
}
