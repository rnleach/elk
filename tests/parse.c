#include "test.h"
#include <math.h>

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                  Tests for Str Parsing
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
static void
test_parse_i64(void)
{
    char *valid_num_strs[]     = {"0", "1", "-1", "+2", "65356", "700", "50", "50000000000"};
    i64 const valid_nums[] = { 0 ,  1 ,  -1 ,  +2 ,  65356 ,  700 ,  50 ,  50000000000 };

    for (i32 i = 0; i < sizeof(valid_nums) / sizeof(valid_nums[0]); ++i)
    {
        ElkStr str = elk_str_from_cstring(valid_num_strs[i]);
        i64 parsed = INT64_MAX;
        b32 success = elk_str_parse_i64(str, &parsed);
        Assert(success);

        i64 tval = valid_nums[i];
        Assert(tval == parsed);
    }

    char *invalid_num_strs[] = {"0a", "*1", "65356.020", "700U", "50L", "0x5000"};

    for (i32 i = 0; i < sizeof(invalid_num_strs) / sizeof(invalid_num_strs[0]); ++i)
    {
        ElkStr str = elk_str_from_cstring(invalid_num_strs[i]);
        i64 parsed = INT64_MAX;
        b32 success = elk_str_parse_i64(str, &parsed);
        Assert(!success);
    }
}

static void
test_robust_parse_f64(void)
{
    f64 const precision = 1.0e-15;

    char *valid_num_strs[] =    {"1.0", "-1.0", "3.14159", "2.345e5", "-2.345e-5", "+500.23e2", "1.7876931348623157e308"};
    f64 const valid_nums[] = { 1.0 ,  -1.0 ,  3.14159 ,  2.345e5 ,  -2.345e-5 ,  +500.23e2 ,  1.7876931348623157e308 };

    for (i32 i = 0; i < sizeof(valid_nums) / sizeof(valid_nums[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(valid_num_strs[i]);
        f64 parsed = NAN;
        b32 success = elk_str_robust_parse_f64(str, &parsed);
        Assert(success);

        f64 tval = valid_nums[i];
        // printf("success = %d str = %s parsed = %g actual = %g difference = %g\n",
        //         success, valid_num_strs[i], parsed, tval, (tval-parsed)/tval);
        Assert(fabs((tval - parsed) / tval) < precision);
    }

    char *invalid_num_strs[] = {"1.0x", " -1.0", "3.1415999999999999999", "1.0e500", "1.8e308"};

    for (i32 i = 0; i < sizeof(invalid_num_strs) / sizeof(invalid_num_strs[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(invalid_num_strs[i]);
        f64 parsed = 0.0;
        b32 success = elk_str_robust_parse_f64(str, &parsed);
        Assert(!success);
    }

    char *inf_str[] = {"inf", "Inf", "INF", "-inf", "-Inf", "-INF"};
    for (i32 i = 0; i < sizeof(inf_str) / sizeof(inf_str[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(inf_str[i]);
        f64 val = 0.0;
        b32 success = elk_str_robust_parse_f64(str, &val);
        Assert(success && isinf(val));
    }

    char *nan_str[] = {"nan", "NaN", "NAN", "Nan"};
    for (i32 i = 0; i < sizeof(nan_str) / sizeof(nan_str[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(nan_str[i]);
        f64 val = 0.0;
        b32 success = elk_str_robust_parse_f64(str, &val);
        Assert(success && isnan(val));
    }
}

static void
test_fast_parse_f64(void)
{
    f64 const precision = 1.0e-15;

    char *valid_num_strs[] =    {"1.0", "-1.0", "3.14159", "2.345e5", "-2.345e-5", "+500.23e2", "1.7876931348623157e308"};
    f64 const valid_nums[] = { 1.0 ,  -1.0 ,  3.14159 ,  2.345e5 ,  -2.345e-5 ,  +500.23e2 ,  1.7876931348623157e308 };

    for (i32 i = 0; i < sizeof(valid_nums) / sizeof(valid_nums[0]); ++i) 
    {
        ElkStr str = elk_str_from_cstring(valid_num_strs[i]);
        f64 parsed = NAN;
        b32 success = elk_str_fast_parse_f64(str, &parsed);
        Assert(success);

        f64 tval = valid_nums[i];
        //printf("success = %d str = %s parsed = %g actual = %g difference = %g\n",
        //        success, valid_num_strs[i], parsed, tval, (tval-parsed)/tval);
        Assert(fabs((tval - parsed) / tval) < precision);
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
    test_parse_i64();
    test_robust_parse_f64();
    test_fast_parse_f64();
    test_parse_datetime();
}
