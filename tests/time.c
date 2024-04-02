#include "test.h"

#include <inttypes.h>

#define TEST_EVERY_1_SEC_INTERVAL 0
#define TEST_EVERY_SECOND 0
/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                    Tests for Time
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static b32
is_in_array(int val, size items, int const array[])
{
    for (int i = 0; i < items; i++) 
    {
        if (array[i] == val) { return true; }
    }

    return false;
}

static void
test_leap_years(void)
{
    int const years_400_rule[] = 
    {
        0,     400,   800,   1200,  1600,  2000,  2400,  2800,  3200,  3600,  4000,  4400,
        4800,  5200,  5600,  6000,  6400,  6800,  7200,  7600,  8000,  8400,  8800,  9200,
        9600,  10000, 10400, 10800, 11200, 11600, 12000, 12400, 12800, 13200, 13600, 14000,
        14400, 14800, 15200, 15600, 16000, 16400, 16800, 17200, 17600, 18000, 18400, 18800,
        19200, 19600, 20000, 20400, 20800, 21200, 21600, 22000, 22400, 22800, 23200, 23600,
        24000, 24400, 24800, 25200, 25600, 26000, 26400, 26800, 27200, 27600, 28000, 28400,
        28800, 29200, 29600, 30000, 30400, 30800, 31200, 31600, 32000
    };

    size const years_400_len = sizeof(years_400_rule) / sizeof(years_400_rule[0]);

    int const years_100_rule[] =
    {
        100,   200,   300,   500,   600,   700,   900,   1000,  1100,  1300,  1400,  1500,
        1700,  1800,  1900,  2100,  2200,  2300,  2500,  2600,  2700,  2900,  3000,  3100,
        3300,  3400,  3500,  3700,  3800,  3900,  4100,  4200,  4300,  4500,  4600,  4700,
        4900,  5000,  5100,  5300,  5400,  5500,  5700,  5800,  5900,  6100,  6200,  6300,
        6500,  6600,  6700,  6900,  7000,  7100,  7300,  7400,  7500,  7700,  7800,  7900,
        8100,  8200,  8300,  8500,  8600,  8700,  8900,  9000,  9100,  9300,  9400,  9500,
        9700,  9800,  9900,  10100, 10200, 10300, 10500, 10600, 10700, 10900, 11000, 11100,
        11300, 11400, 11500, 11700, 11800, 11900, 12100, 12200, 12300, 12500, 12600, 12700,
        12900, 13000, 13100, 13300, 13400, 13500, 13700, 13800, 13900, 14100, 14200, 14300,
        14500, 14600, 14700, 14900, 15000, 15100, 15300, 15400, 15500, 15700, 15800, 15900,
    };

    size const years_100_len = sizeof(years_100_rule) / sizeof(years_100_rule[0]);

    // Divisible by 400 rule
    for (int i = 0; i < years_400_len; ++i) 
    {
        Assert(elk_is_leap_year(years_400_rule[i]));
    }

    // Divisible by 100 rule
    for (int i = 0; i < years_100_len; ++i) 
    {
        Assert(!elk_is_leap_year(years_100_rule[i]));
    }

    // Divisible by 4 or not
    Assert(elk_is_leap_year(1984));
    Assert(elk_is_leap_year(1988));
    Assert(elk_is_leap_year(1992));
    Assert(elk_is_leap_year(1996));

    Assert(!elk_is_leap_year(1985));
    Assert(!elk_is_leap_year(1989));
    Assert(!elk_is_leap_year(1993));
    Assert(!elk_is_leap_year(1997));

    Assert(!elk_is_leap_year(INT16_MAX));

    for (int i = 0; i < 11199; ++i) 
    {
        if (i % 4 == 0 && !is_in_array(i, years_100_len, years_100_rule)) {
            Assert(elk_is_leap_year(i));
        }
        else
        {
            Assert(!elk_is_leap_year(i));
        }
    }
}

static void
test_time_epoch(void)
{
    ElkTime epoch = elk_time_from_ymd_and_hms(1, 1, 1, 0, 0, 0);
    Assert(epoch == 0);
}

static void
test_time_time_t_is_seconds(void)
{
    ElkTime epoch = elk_time_from_ymd_and_hms(1, 1, 1, 0, 0, 0);
    ElkTime day1 = elk_time_from_ymd_and_hms(1, 1, 2, 0, 0, 0);

    Assert((day1 - epoch) == (60 * 60 * 24));
}

#if TEST_EVERY_1_SEC_INTERVAL || TEST_EVERY_SECOND
// Days in a month - first row is normal, second is leap year
static int dim[2][12] =
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};
#endif

static void
test_increments_are_1_second(void)
{
    // Individual test cases that have caused errors before.

    // This one exposed a leap year related bug
    ElkTime first = elk_time_from_ymd_and_hms(3, 12, 31, 23, 59, 59);
    ElkTime second = elk_time_from_ymd_and_hms(4, 1, 1, 0, 0, 0);
    Assert(second - first == 1);

    first = elk_time_from_ymd_and_hms(32767, 12, 31, 23, 59, 58);
    second = elk_time_from_ymd_and_hms(32767, 12, 31, 23, 59, 59);
    Assert(second - first == 1);

#if TEST_EVERY_1_SEC_INTERVAL // Loop takes a long time to run; only do it to find trouble spots.
    printf("\tIn \"test_increments_are_1_second()\".\n");
    printf("\tTesting if each time step is indeed 1 second apart.\n");

    ElkTime previous = -1;

    for (int year = 1; year <= 4000; year++) 
    {
        int leap_idx = elk_is_leap_year(year) ? 1 : 0;
        for (int month = 1; month <= 12; month++) 
        {
            for (int day = 1; day <= dim[leap_idx][month - 1]; day++) 
            {
                for (int hour = 0; hour <= 23; hour++) 
                {
                    for (int minute = 0; minute <= 59; minute++) 
                    {
                        for (int second = 0; second <= 59; second++) 
                        {
                            ElkTime current =
                                elk_time_from_ymd_and_hms(year, month, day, hour, minute, second);
                            if (current - previous != 1) 
                            {
                                printf("\nFAIL: diff=%" PRId64
                                       ", year=%d, month=%d, day=%d, hour=%d, "
                                       "minute=%d, second=%d\n",
                                       current - previous, year, month, day, hour, minute, second);
                                Assert(current - previous == 1);
                            }
                            previous = current;
                        }
                    }
                }
            }
        }

        printf("\r\t%8.4lf%% done", 100.0 * year / 4000.0);
        fflush(stdout);
    }
    printf("\n\tFinished\n");
#endif
}

static b32
struct_times_equal(ElkStructTime left, ElkStructTime right)
{
    return left.year == right.year && left.month == right.month && left.day == right.day &&
           left.hour == right.hour && left.minute == right.minute && left.second == right.second;
}

static void
test_time_struct(void)
{
    ElkStructTime testVals[] = 
    {
        {.year =     1, .month =  1, .day =  1, .hour =  0, .minute =  0, .second =  0, .day_of_year =   1 },
        {.year =     4, .month = 12, .day = 31, .hour =  0, .minute =  0, .second =  0, .day_of_year = 366 },
        {.year =  1970, .month =  1, .day =  1, .hour =  0, .minute =  0, .second =  0, .day_of_year =   1 },
        {.year = 32767, .month = 12, .day = 31, .hour = 23, .minute = 59, .second = 59, .day_of_year = 365 },
    };

    for (int i = 0; i < sizeof(testVals) / sizeof(testVals[0]); i++) 
    {
        ElkStructTime forward = testVals[i];
        i16 forward_doy = testVals[i].day_of_year;
        ElkTime middle = elk_make_time(testVals[i]);
        ElkStructTime back = elk_make_struct_time(middle);
        if (!struct_times_equal(forward, back) || forward_doy != back.day_of_year) 
        {
            printf("%+05d-%02d-%02d %02d:%02d:%02d %12" PRId64 " %+05d-%02d-%02d %02d:%02d:%02d \n",
                   forward.year, forward.month, forward.day, forward.hour, forward.minute,
                   forward.second, middle, back.year, back.month, back.day, back.hour, back.minute,
                   back.second);
            Assert(struct_times_equal(forward, back));
        }
    }

#if TEST_EVERY_SECOND // This loop takes a very long time to run, only do it to find trouble spots.
    printf("\tIn \"function test_time_struct()\"\n");
    for (int year = 1; year <= 4000; year++) 
    {
        int leap_idx = elk_is_leap_year(year) ? 1 : 0;
        i16 day_of_year = 1;
        for (int month = 1; month <= 12; month++) 
        {
            for (int day = 1; day <= dim[leap_idx][month - 1]; day_of_year++, day++) 
            {
                for (int hour = 0; hour <= 23; hour++) 
                {
                    for (int minute = 0; minute <= 59; minute++) 
                    {
                        for (int second = 0; second <= 59; second++) 
                        {
                            ElkStructTime forward = {.year = year,
                                                     .month = month,
                                                     .day = day,
                                                     .hour = hour,
                                                     .minute = minute,
                                                     .second = second,
                                                     .day_of_year = day_of_year,
                                                    };
                            ElkTime middle = elk_make_time(forward);
                            ElkStructTime back = elk_make_struct_time(middle);

                            if (!struct_times_equal(forward, back) || forward.day_of_year != back.day_of_year) 
                            {
                                printf("%+05d-%02d-%02d %02d:%02d:%02d %12" PRId64
                                       " %+05d-%02d-%02d "
                                       "%02d:%02d:%02d \n",
                                       forward.year, forward.month, forward.day, forward.hour,
                                       forward.minute, forward.second, middle, back.year,
                                       back.month, back.day, back.hour, back.minute, back.second);

                                Assert(struct_times_equal(forward, back));
                            }
                        }
                    }
                }
            }
        }

        printf("\r\t%8.4lf%% done", 100.0 * year / 4000.0);
        fflush(stdout);
    }
    printf("\n\tDone\n");
#endif
}

static void
test_time_linux_timestamp(void)
{
    ElkTime t0 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    i64 unix_t0 = elk_time_to_unix_epoch(t0);

    Assert(unix_t0 == 0);

    Assert(elk_time_from_unix_timestamp(0) == elk_unix_epoch_timestamp);
}

static void
test_time_truncate_to_hour()
{
    ElkTime t0 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    ElkTime t1 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 14, 39);

    Assert(elk_time_truncate_to_hour(t1) == t0);
}

static void
test_time_truncate_to_specific_hour()
{
    ElkTime start = elk_time_from_ymd_and_hms(2022, 6, 20, 19, 14, 39);
    ElkTime target1 = elk_time_from_ymd_and_hms(2022, 6, 20, 12, 0, 0);
    ElkTime target2 = elk_time_from_ymd_and_hms(2022, 6, 19, 21, 0, 0);

    Assert(elk_time_truncate_to_specific_hour(start, 12) == target1);
    Assert(elk_time_truncate_to_specific_hour(start, 21) == target2);
}

static void
test_time_addition()
{
    ElkTime epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    ElkTime t1 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 14, 39);

    Assert((epoch + 14 * ElkMinute + 39 * ElkSecond) == t1);
    Assert(elk_time_add(epoch, 14 * ElkMinute + 39 * ElkSecond) == t1);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     All time tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_time_tests(void)
{
    test_leap_years();
    test_time_epoch();
    test_time_time_t_is_seconds();
    test_increments_are_1_second();
    test_time_struct();
    test_time_linux_timestamp();
    test_time_truncate_to_hour();
    test_time_truncate_to_specific_hour();
    test_time_addition();
}
