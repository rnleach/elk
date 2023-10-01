#include "test.h"
#include <math.h>

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                  Tests for Str Parsing
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
static void
test_parse_int64(void)
{
    char *valid_num_strs[]     = {"0", "1", "-1", "+2", "65356", "700", "50", "50000000000"};
    int64_t const valid_nums[] = { 0 ,  1 ,  -1 ,  +2 ,  65356 ,  700 ,  50 ,  50000000000 };

    for (int i = 0; i < sizeof(valid_nums) / sizeof(valid_nums[0]); ++i)
    {
        ElkStr str = elk_str_from_cstring(valid_num_strs[i]);
        int64_t parsed = INT64_MAX;
        bool success = elk_str_parse_int_64(str, &parsed);
        Assert(success);

        int64_t tval = valid_nums[i];
        Assert(tval == parsed);
    }

    char *invalid_num_strs[] = {"0a", "*1", "65356.020", "700U", "50L", "0x5000"};

    for (int i = 0; i < sizeof(invalid_num_strs) / sizeof(invalid_num_strs[0]); ++i)
    {
        ElkStr str = elk_str_from_cstring(invalid_num_strs[i]);
        int64_t parsed = INT64_MAX;
        bool success = elk_str_parse_int_64(str, &parsed);
        Assert(!success);
    }
}

static void
test_parse_double(void)
{
    double const precision = 1.0e-15;

    char *valid_num_strs[] =    {"1.0", "-1.0", "3.14159", "2.345e5", "-2.345e-5", "+500.23e2", "1.7876931348623157e308"};
    double const valid_nums[] = { 1.0 ,  -1.0 ,  3.14159 ,  2.345e5 ,  -2.345e-5 ,  +500.23e2 ,  1.7876931348623157e308 };

    for (int i = 0; i < sizeof(valid_nums) / sizeof(valid_nums[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(valid_num_strs[i]);
        double parsed = NAN;
        bool success = elk_str_parse_float_64(str, &parsed);
        Assert(success);

        double tval = valid_nums[i];
        // printf("success = %d str = %s parsed = %g actual = %g difference = %g\n",
        //         success, valid_num_strs[i], parsed, tval, (tval-parsed)/tval);
        Assert(fabs((tval - parsed) / tval) < precision);
    }

    char *invalid_num_strs[] = {"1.0x", " -1.0", "3.1415999999999999999", "1.0e500", "1.8e308"};

    for (int i = 0; i < sizeof(invalid_num_strs) / sizeof(invalid_num_strs[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(invalid_num_strs[i]);
        double parsed = 0.0;
        bool success = elk_str_parse_float_64(str, &parsed);
        Assert(!success);
    }

    char *inf_str[] = {"inf", "Inf", "INF", "-inf", "-Inf", "-INF"};
    for (int i = 0; i < sizeof(inf_str) / sizeof(inf_str[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(inf_str[i]);
        double val = 0.0;
        bool success = elk_str_parse_float_64(str, &val);
        Assert(success && isinf(val));
    }

    char *nan_str[] = {"nan", "NaN", "NAN", "Nan"};
    for (int i = 0; i < sizeof(nan_str) / sizeof(nan_str[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(nan_str[i]);
        double val = 0.0;
        bool success = elk_str_parse_float_64(str, &val);
        Assert(success && isnan(val));
    }
}

static void
test_parse_datetime(void)
{
    // Test something that should succeed
    ElkTime tst = elk_time_from_ymd_and_hms(1981, 4, 15, 0, 15, 16);

    ElkStr tst_str1 = elk_str_from_cstring("1981-04-15T00:15:16");
    ElkStr tst_str2 = elk_str_from_cstring("1981-04-15 00:15:16");
    ElkStr tst_str3 = elk_str_from_cstring("1981105001516");

    ElkTime out = 0;
    Assert(elk_str_parse_datetime(tst_str1, &out) && out == tst);
    Assert(elk_str_parse_datetime(tst_str2, &out) && out == tst);
    Assert(elk_str_parse_datetime(tst_str3, &out) && out == tst);

    // Test something that should fail
    tst_str1 = elk_str_from_cstring("1981-4-15T00:15:16");
    tst_str2 = elk_str_from_cstring("19810415001516");
    tst_str3 = elk_str_from_cstring("1981 105 001516");

    out = 0;
    Assert(!elk_str_parse_datetime(tst_str1, &out) && out == 0);
    Assert(!elk_str_parse_datetime(tst_str2, &out) && out == 0);
    Assert(!elk_str_parse_datetime(tst_str3, &out) && out == 0);

}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  All Str Parsing tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_parse_tests(void)
{
    test_parse_int64();
    test_parse_double();
    test_parse_datetime();
}
