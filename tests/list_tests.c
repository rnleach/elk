#include "test.h"
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

    // Check that the values in the list are what they should be.
    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        double *alias = elk_list_get_alias_at_index(list, i);
        assert(*alias == value);
    }

    // Check that pop gives the correct value back to you.
    for (size_t i = list_size - 1; elk_list_count(list) > 0; --i) {
        double value = i;
        double value_from_list = HUGE_VAL;
        list = elk_list_pop_back(list, &value_from_list);

        assert(value_from_list == value);
    }

    assert(elk_list_count(list) == 0);

    list = elk_list_free(list);
    assert(!list);
}

/*-------------------------------------------------------------------------------------------------
 *                                         Test copy
 *-----------------------------------------------------------------------------------------------*/
static void
test_elk_list_copy(void)
{
    size_t const list_size = 1000;

    ElkList *list = build_double_list(list_size);

    ElkList *copy = elk_list_copy(list);

    assert(elk_list_count(list) == elk_list_count(copy));

    for (size_t i = 0; i < elk_list_count(list); ++i) {
        double *item = elk_list_get_alias_at_index(list, i);
        double *item_copy = elk_list_get_alias_at_index(copy, i);

        assert(*item == *item_copy);
    }

    list = elk_list_free(list);
    assert(!list);

    copy = elk_list_free(copy);
    assert(!copy);
}

/*-------------------------------------------------------------------------------------------------
 *                                       Test foreach
 *-----------------------------------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------------------------------------
 *                                       Test filter_out
 *-----------------------------------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkList tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_list_tests(void)
{
    test_elk_list_data_integrity();
    test_elk_list_copy();
    test_elk_list_foreach();
    test_elk_list_filter_out();
}
