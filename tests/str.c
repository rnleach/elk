#include "test.h"

/*------------------------------------------------------------------------------------------------
 *
 *                                       Tests for Str
 *
 *-----------------------------------------------------------------------------------------------*/
static void
test_from_cstring(void)
{
    char *sample = "a sample string";
    ElkStr str = elk_str_from_cstring(sample);

    assert(str.start == sample);
    assert(*(str.start + str.len - 1) == 'g');
    assert(*(str.start + str.len) == '\0');
}

static void
test_str_len(void)
{
    char *sample = "a sample string";
    ElkStr str = elk_str_from_cstring(sample);
    assert(str.len == 15);
}

static void
test_str_copy(void)
{
    char *sample = "a sample string";
    ElkStr str = elk_str_from_cstring(sample);

    char dest[20] = {0};
    char too_short[10] = {0};

    elk_str_copy(sizeof(dest), dest, str);
    elk_str_copy(sizeof(too_short), too_short, str);

    assert(dest[14] == 'g');
    assert(dest[15] == '\0');

    assert(too_short[8] == ' ');
    assert(too_short[9] == '\0');
}

static void
test_str_cmp(void)
{
    char *sample = "a sample string";
    ElkStr sample_str = elk_str_from_cstring(sample);
    char *cap_sample = "A sample string";
    ElkStr cap_sample_str = elk_str_from_cstring(cap_sample);

    assert(elk_str_cmp(cap_sample_str, sample_str) == -1);
    assert(elk_str_cmp(sample_str, cap_sample_str) == 1);

    char sample_copy[16] = {0};
    ElkStr sample_copy_str = elk_str_copy(sizeof(sample_copy), sample_copy, sample_str);
    assert(elk_str_cmp(sample_copy_str, sample_str) == 0);
    assert(elk_str_cmp(sample_str, sample_copy_str) == 0);

    char *short_sample = "a sample";
    ElkStr short_sample_str = elk_str_from_cstring(short_sample);
    assert(elk_str_cmp(short_sample_str, sample_str) == -1);
    assert(elk_str_cmp(sample_str, short_sample_str) == 1);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All Str tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_str_tests(void)
{
    test_from_cstring();
    test_str_len();
    test_str_copy();
    test_str_cmp();
}
