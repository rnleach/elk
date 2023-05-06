#include "test.h"
/*------------------------------------------------------------------------------------------------
 *
 *                                       Tests for ElkArray
 *
 *-----------------------------------------------------------------------------------------------*/

static ElkArray
build_double_array(size_t num_elements)
{
    ElkArray arr = elk_array_new(sizeof(double));
    ElkCode ret = ELK_CODE_SUCCESS;

    for (size_t i = 0; i < num_elements; ++i) {
        double value = i;
        ret = elk_array_push_back(&arr, &value);
        assert(ret == ELK_CODE_SUCCESS);
    }

    assert(elk_array_length(&arr) == num_elements);

    return arr;
}

/*-------------------------------------------------------------------------------------------------
 *                         Test adding and retrieving data match.
 *-----------------------------------------------------------------------------------------------*/
static void
test_elk_array_data_integrity(void)
{
    size_t const list_size = 20;
    ElkCode ret = ELK_CODE_SUCCESS;
    double value_from_array = HUGE_VAL;

    ElkArray arr = build_double_array(list_size);

    // Check that the values in the list are what they should be.
    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        double *alias = elk_array_alias_index(&arr, i);
        assert(*alias == value);
    }

    // Check that pop gives the correct value back to you.
    for (size_t i = list_size - 1; elk_array_length(&arr) > 0; --i) {
        double value = i;
        ret = elk_array_pop_back(&arr, &value_from_array);

        assert(ret == ELK_CODE_SUCCESS);
        assert(value_from_array == value);
    }

    assert(elk_array_length(&arr) == 0);
    ret = elk_array_pop_back(&arr, &value_from_array);
    assert(ret == ELK_CODE_EMPTY);

    ret = elk_array_clear(&arr);
    assert(ret == ELK_CODE_SUCCESS);
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
test_elk_array_foreach(void)
{
    size_t const list_size = 20;
    ElkCode ret = ELK_CODE_SUCCESS;

    ElkArray arr = build_double_array(list_size);

    ret = elk_array_foreach(&arr, square_small_double, NULL);
    assert(ret == ELK_CODE_EARLY_TERM);

    for (size_t i = 0; i < list_size; ++i) {
        double value = i;
        square_small_double(&value, NULL);

        double *alias = elk_array_alias_index(&arr, i);
        assert(*alias == value);
    }

    ret = elk_array_clear(&arr);
    assert(ret == ELK_CODE_SUCCESS);
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
test_elk_array_filter_out(void)
{
    size_t const list_size = 20;
    ElkCode ret = ELK_CODE_SUCCESS;

    ElkArray src = build_double_array(list_size);
    ElkArray sink = elk_array_new(sizeof(double));

    ret = elk_array_filter_out(&src, &sink, delete_large_double, NULL);
    assert(ret == ELK_CODE_SUCCESS);

    assert(elk_array_length(&src) == 6);
    assert(elk_array_length(&sink) == (list_size - 6));

    for (size_t i = 0; i < list_size; ++i) {

        double *alias = 0;
        if (i < 6) {
            alias = elk_array_alias_index(&src, i);
            double value = i;
            assert(*alias == value);
        } else {
            alias = elk_array_alias_index(&sink, i - 6);
            assert(*alias >= 6.0);
        }
    }

    ret = elk_array_clear(&src);
    assert(ret == ELK_CODE_SUCCESS);
    ret = elk_array_clear(&sink);
    assert(ret == ELK_CODE_SUCCESS);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All ElkArray tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_array_tests(void)
{
    test_elk_array_data_integrity();
    test_elk_array_foreach();
    test_elk_array_filter_out();
}
