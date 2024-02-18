#include <inttypes.h>

#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                        Test CSV
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static char *sample_one = 
    "# This is a sample of a possible CSV string that would need parsed. \n"
    "# This sample has a couple of comments at the start, and then some made up data.\n"
    "\n"
    "col1,col2, col3 , col4,col5 ,col6\n"
    "1,2,3,4,5,6\n"
    "\"Frank \"\"The Tank\"\" Johnson\",867-5309,unquoted string, 4,5, 6\n"
    "row4-col0,row4-col1,row4-col4,row4-col3,row4-col4,row4-col5\n"
    "row5-col0,,,,,row5-col5\n";

static void
test_one_full(void)
{
    ElkStr sample = elk_str_from_cstring(sample_one);

    ElkCsvParser p_ = elk_csv_create_parser(sample);
    ElkCsvParser *p = &p_;

    size rows = 0;
    size cols = 0;

    ElkCsvToken t;
    while(!elk_csv_finished(p))
    {
        t = elk_csv_full_next_token(p);

        Assert(!p->error);

        rows = t.row > rows ? t.row : rows;
        cols = t.col > cols ? t.col : cols;

        //fprintf(stderr, "row=%"PRIdPTR" col=%"PRIdPTR" error=%d value = __%.*s__\n", 
        //        t.row, t.col, p->error, (int)t.value.len, t.value.start);
    }

    rows++; // since numbers start at 0
    cols++; // since numbers start at 0

    Assert(rows == 6 && cols == 6);
}

/* Either test one or test two for the fast version will force an odd alignment so we can be sure that works with the 
 * SIMD versions.
 */
static void
test_one_fast(void)
{
    ElkStr sample = elk_str_from_cstring(sample_one);

    // Print the alignment, good to know if we're always testing 32-byte aligned strings.
    //for(u64 a = 1; a <= 64; a <<= 1)
    //{
    //    printf("Alignment = %2"PRIu64"?  %2s\n", a, (uptr)sample.start % a == 0 ? "Yes" : "No");
    //}

    ElkCsvParser p_ = elk_csv_create_parser(sample);
    ElkCsvParser *p = &p_;

    size rows = 0;
    size cols = 0;

    ElkCsvToken t;
    while(!elk_csv_finished(p))
    {
        t = elk_csv_fast_next_token(p);

        Assert(!p->error);

        rows = t.row > rows ? t.row : rows;
        cols = t.col > cols ? t.col : cols;

        //printf("row=%"PRIdPTR" col=%"PRIdPTR" error=%d value = __%.*s__\n", 
        //        t.row, t.col, p->error, (int)t.value.len, t.value.start);
    }

    rows++; // since numbers start at 0
    cols++; // since numbers start at 0

    Assert(rows == 6 && cols == 6);
}

static void
test_two_fast(void)
{
    ElkStr sample = elk_str_from_cstring(sample_one + 69);
    //fprintf(stderr, "__%s__\n", sample_one + 69);

    // Print the alignment, good to know if we're always testing 32-byte aligned strings.
    //for(u64 a = 1; a <= 64; a <<= 1)
    //{
    //    printf("Alignment = %2"PRIu64"?  %2s\n", a, (uptr)sample.start % a == 0 ? "Yes" : "No");
    //}

    ElkCsvParser p_ = elk_csv_create_parser(sample);
    ElkCsvParser *p = &p_;

    size rows = 0;
    size cols = 0;

    ElkCsvToken t;
    while(!elk_csv_finished(p))
    {
        t = elk_csv_fast_next_token(p);

        Assert(!p->error);

        rows = t.row > rows ? t.row : rows;
        cols = t.col > cols ? t.col : cols;

        //printf("row=%"PRIdPTR" col=%"PRIdPTR" error=%d value = __%.*s__\n", 
        //        t.row, t.col, p->error, (int)t.value.len, t.value.start);
    }

    rows++; // since numbers start at 0
    cols++; // since numbers start at 0

    Assert(rows == 6 && cols == 6);
}

static void
test_unquote(void)
{
    char *test[] = {" \"Frank \"\"The Tank\"\" Johnson\" ", "", "unquoted string"};
    char *correct_answer[] = {"Frank \"The Tank\" Johnson",  "", "unquoted string"};
    char buffer_storage[512] = {0};
    ElkStr buffer = { .start = buffer_storage, .len = sizeof(buffer_storage) };

    for(i32 i = 0; i < sizeof(test) / sizeof(test[0]); ++i)
    {
        ElkStr test_str = elk_str_from_cstring(test[i]);
        ElkStr correct_answer_str = elk_str_from_cstring(correct_answer[i]);

        ElkStr parsed = elk_csv_unquote_str(test_str, buffer);
        Assert(elk_str_eq(parsed, correct_answer_str));
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_csv_tests(void)
{
    test_one_full();
    test_one_fast();
    test_two_fast();
    test_unquote();
}
