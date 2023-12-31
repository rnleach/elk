#include <inttypes.h>

#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                        Test CSV
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static char *sample_one = 
    "# This is a sample of a possible CSV string that would need parsed.\n"
    "# This sample has a couple of comments at the start, and then some made up data.\n"
    "\n"
    "col1,col2, col3 , col4,col5 ,col6\n"
    "1,2,3,4,5,6\n"
    "\"Frank \"\"The Tank\"\" Johnson\",867-5309,unquoted string, 4,5, 6\n"
    "row4-col0,row4-col1,row4-col4,row4-col3,row4-col4,row4-col5\n"
    "row5-col0,,,,,row5-col5\n";

static void
test_one(void)
{
    ElkStr sample = elk_str_from_cstring(sample_one);

    ElkCsvParser p_ = elk_csv_default_parser(sample);
    ElkCsvParser *p = &p_;

    intptr_t rows = 0;
    intptr_t cols = 0;

    ElkCsvToken t;
    while(!elk_csv_finished(p))
    {
        t = elk_csv_next_token(p);

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
    char test[] = " \"Frank \"\"The Tank\"\" Johnson\" ";
    char correct_answer[] = "Frank \"The Tank\" Johnson";
    ElkStr test_str = elk_str_from_cstring(test);
    ElkStr correct_answer_str = elk_str_from_cstring(correct_answer);

    ElkStr parsed = elk_csv_unquote_str(test_str);
    Assert(elk_str_eq(parsed, correct_answer_str));
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_csv_tests(void)
{
    test_one();
    test_unquote();
}