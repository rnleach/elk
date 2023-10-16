#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                      Tests for Str
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
static void
test_from_cstring(void)
{
    // Test a regular string
    char *sample = "a sample string";
    ElkStr str = elk_str_from_cstring(sample);

    Assert(str.start == sample);
    Assert(str.len == 15);
    Assert(*(str.start + str.len - 1) == 'g');
    Assert(*(str.start + str.len) == '\0');

    // Test an empty string
    char *empty = "";
    ElkStr empty_str = elk_str_from_cstring(empty);

    Assert(empty_str.start == empty);
    Assert(empty_str.len == 0);
    Assert(*(empty_str.start + empty_str.len) == '\0');
}

static void
test_str_strip(void)
{
    // Test with a basic string.
    char *sample = "a sample string with puncuation. \" ' ? < > $ % & * ) ( - + = 0123456";
    ElkStr sample_str = elk_str_from_cstring(sample);

    char *extra = "    a sample string with puncuation. \" ' ? < > $ % & * ) ( - + = 0123456  ";
    ElkStr extra_str = elk_str_from_cstring(extra);

    ElkStr extra_stripped = elk_str_strip(extra_str);
    Assert(elk_str_cmp(sample_str, extra_stripped) == 0);

    // Test with an empty string
    char *empty = "";
    ElkStr empty_str = elk_str_from_cstring(empty);
    ElkStr empty_strip = elk_str_strip(empty_str);
    Assert(elk_str_cmp(empty_str, empty_strip) == 0);
}

static void
test_str_eq(void)
{
    // Capitalization comparisons
    char *sample = "a sample string";
    ElkStr sample_str = elk_str_from_cstring(sample);
    char *cap_sample = "A sample string";
    ElkStr cap_sample_str = elk_str_from_cstring(cap_sample);
    char *capz_sample = "Z sample string";
    ElkStr capz_sample_str = elk_str_from_cstring(capz_sample);

    Assert(!elk_str_eq(cap_sample_str, sample_str));
    Assert(!elk_str_eq(sample_str, cap_sample_str));
    Assert(!elk_str_eq(capz_sample_str, sample_str));
    Assert(!elk_str_eq(sample_str, capz_sample_str));

    // Comparison of shorter & longer strings.
    char *short_sample = "a sample";
    ElkStr short_sample_str = elk_str_from_cstring(short_sample);
    Assert(!elk_str_eq(short_sample_str, sample_str));
    Assert(!elk_str_eq(sample_str, short_sample_str));

    // Empty string comparisons
    char *empty = "";
    ElkStr empty_str = elk_str_from_cstring(empty);

    Assert(!elk_str_eq(empty_str, sample_str));
    Assert(!elk_str_eq(sample_str, empty_str));
    Assert(elk_str_eq(empty_str, empty_str));
}

static void
test_str_cmp(void)
{
    // Capitalization comparisons
    char *sample = "a sample string";
    ElkStr sample_str = elk_str_from_cstring(sample);
    char *cap_sample = "A sample string";
    ElkStr cap_sample_str = elk_str_from_cstring(cap_sample);
    char *capz_sample = "Z sample string";
    ElkStr capz_sample_str = elk_str_from_cstring(capz_sample);

    Assert(elk_str_cmp(cap_sample_str, sample_str) == -1);
    Assert(elk_str_cmp(sample_str, cap_sample_str) == 1);
    Assert(elk_str_cmp(capz_sample_str, sample_str) == -1); // Z comes before a (case sensitive!)
    Assert(elk_str_cmp(sample_str, capz_sample_str) == 1);

    // Comparison of shorter & longer strings.
    char *short_sample = "a sample";
    ElkStr short_sample_str = elk_str_from_cstring(short_sample);
    Assert(elk_str_cmp(short_sample_str, sample_str) == -1);
    Assert(elk_str_cmp(sample_str, short_sample_str) == 1);

    // Empty string comparisons
    char *empty = "";
    ElkStr empty_str = elk_str_from_cstring(empty);

    Assert(elk_str_cmp(empty_str, sample_str) == -1);
    Assert(elk_str_cmp(sample_str, empty_str) == 1);
    Assert(elk_str_cmp(empty_str, empty_str) == 0);

    // Comparison of copies
    char sample_copy[16] = {0};
    ElkStr sample_copy_str = elk_str_copy(sizeof(sample_copy), sample_copy, sample_str);
    Assert(elk_str_cmp(sample_copy_str, sample_str) == 0);
    Assert(elk_str_cmp(sample_str, sample_copy_str) == 0);
}

static void
test_str_copy(void)
{
    char *sample = "a sample string";
    ElkStr str = elk_str_from_cstring(sample);

    char dest[20] = {0};
    char too_short[10] = {0};

    ElkStr dest_str = elk_str_copy(sizeof(dest), dest, str);
    ElkStr too_short_str = elk_str_copy(sizeof(too_short), too_short, str);

    Assert(dest_str.len == str.len);           // They're the same size
    Assert(dest_str.start != str.start);       // They're not the same location in memory
    Assert(elk_str_eq(dest_str, str));         // They should compare as equals
    Assert(dest[14] == 'g');
    Assert(dest[15] == '\0');

    Assert(too_short_str.len < str.len);       // Too short is smaller!
    Assert(too_short_str.start != str.start);  // They're not the same location in memory
    Assert(!elk_str_eq(too_short_str, str));   // They NOT should compare as equals
    Assert(too_short[8] == ' ');
    Assert(too_short[9] == 's');               // NOT a '\0' because we had no extra room.

    // Test an empty string
    char *empty = "";
    ElkStr empty_str = elk_str_from_cstring(empty);

    dest_str = elk_str_copy(sizeof(dest), dest, empty_str);

    Assert(dest_str.len == empty_str.len);     // They're the same size
    Assert(dest_str.start != empty_str.start); // They're not the same location in memory
    Assert(elk_str_eq(dest_str, empty_str));   // They should compare as equals
    Assert(dest_str.len == 0);
    Assert(dest[0] == '\0');
}

static void
test_str_substr(void)
{
    char *sample = "a sample string with puncuation. \" ' ? < > $ % & * ) ( - + = 0123456";
    ElkStr sample_str = elk_str_from_cstring(sample);

    char *extra = "    a sample string with puncuation. \" ' ? < > $ % & * ) ( - + = 0123456  ";
    ElkStr extra_str = elk_str_from_cstring(extra);

    ElkStr extra_stripped = elk_str_strip(extra_str);
    Assert(elk_str_cmp(sample_str, extra_stripped) == 0);
    Assert(elk_str_eq(sample_str, extra_stripped));

    ElkStr extra_substr = elk_str_substr(extra_str, 4, 68);
    Assert(elk_str_cmp(sample_str, extra_substr) == 0);
    Assert(elk_str_eq(sample_str, extra_substr));
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      All Str tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_str_tests(void)
{
    test_from_cstring();
    test_str_strip();
    test_str_eq();
    test_str_cmp();
    test_str_copy();
    test_str_substr();
}
