#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/elk.h"

/*------------------------------------------------------------------------------------------------
 *
 *                                       Tests for ElkList
 *
 *-----------------------------------------------------------------------------------------------*/

static ElkList *
build_double_list(size_t num_elements)
{
    ElkList *list = elk_list_new(sizeof(double));

    for (size_t i = 0; i < num_elements; ++i) {
        double value = i;
        list = elk_list_push_back(list, &value);
    }

    assert(elk_list_count(list) == num_elements);

    return list;
}

/*-------------------------------------------------------------------------------------------------
 *                         Test adding and retrieving data match.
 *-----------------------------------------------------------------------------------------------*/
static void
test_elk_list_data_integrity(void)
{
    size_t const list_size = 20;

    ElkList *list = build_double_list(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        double *alias = elk_list_get_alias_at_index(list, i);
        assert(*alias == value);
    }

    list = elk_list_free(list);

    assert(!list);
}

static bool
square_small_double(void *val_ptr, void *unused)
{
    double *val = val_ptr;

    if (*val < 6.0) {
        *val *= *val;
        return true;
    }

    return false;
}

static void
test_elk_list_foreach(void)
{
    size_t const list_size = 20;

    ElkList *list = build_double_list(list_size);

    elk_list_foreach(list, square_small_double, NULL);

    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        square_small_double(&value, NULL);

        double *alias = elk_list_get_alias_at_index(list, i);
        assert(*alias == value);
    }

    list = elk_list_free(list);
    assert(!list);
}

static bool
delete_large_double(void const *val_ptr, void *unused)
{
    double const *val = val_ptr;
    if (*val < 6.0) {
        return false;
    }

    return true;
}

static void
test_elk_list_filter_out(void)
{
    size_t const list_size = 20;

    ElkList *src = build_double_list(list_size);
    ElkList *sink = elk_list_new(sizeof(double));

    sink = elk_list_filter_out(src, sink, delete_large_double, NULL);

    assert(elk_list_count(src) == 6);
    assert(elk_list_count(sink) == (list_size - 6));

    for (size_t i = 0; i < list_size; ++i) {

        double *alias = 0;
        if (i < 6) {
            alias = elk_list_get_alias_at_index(src, i);
            double value = i;
            assert(*alias == value);
        } else {
            alias = elk_list_get_alias_at_index(sink, i - 6);
            assert(*alias >= 6.0);
        }
    }

    src = elk_list_free(src);
    assert(!src);
    sink = elk_list_free(sink);
    assert(!sink);
}

static void
elk_list_tests(void)
{
    test_elk_list_data_integrity();
    test_elk_list_foreach();
    test_elk_list_filter_out();
}

/*-------------------------------------------------------------------------------------------------
 *
 *                                       Main - Run the tests
 *
 *-----------------------------------------------------------------------------------------------*/
int
main(void)
{
    printf("Starting Tests.\n");

    elk_list_tests();

    printf("\n\n*** Tests completed successfully. ***\n\n");
    return EXIT_SUCCESS;
}
