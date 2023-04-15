#include "test.h"
/*------------------------------------------------------------------------------------------------
 *
 *                                       Tests for Time
 *
 *-----------------------------------------------------------------------------------------------*/
static void
test_time_unix_epoch(void)
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    assert(epoch == 0);
}

static void
test_time_time_t_is_seconds(void)
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    time_t day1 = elk_time_from_ymd_and_hms(1970, 1, 2, 0, 0, 0);

    assert((day1 - epoch) == (60 * 60 * 24));
}

static void
test_time_truncate_to_hour()
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    time_t t1 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 14, 39);

    assert(elk_time_truncate_to_hour(t1) == epoch);
}

static void
test_time_truncate_to_specific_hour()
{
    time_t start = elk_time_from_ymd_and_hms(2022, 6, 20, 19, 14, 39);
    time_t target1 = elk_time_from_ymd_and_hms(2022, 6, 20, 12, 0, 0);
    time_t target2 = elk_time_from_ymd_and_hms(2022, 6, 19, 21, 0, 0);

    assert(elk_time_truncate_to_specific_hour(start, 12) == target1);
    assert(elk_time_truncate_to_specific_hour(start, 21) == target2);
}

static void
test_time_addition()
{
    time_t epoch = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 0, 0);
    time_t t1 = elk_time_from_ymd_and_hms(1970, 1, 1, 0, 14, 39);

    assert((epoch + 14 * ElkMinute + 39 * ElkSecond) == t1);
    assert(elk_time_add(epoch, 14 * ElkMinute + 39 * ElkSecond) == t1);
}

/*-------------------------------------------------------------------------------------------------
 *                                      All time tests
 *-----------------------------------------------------------------------------------------------*/
void
elk_time_tests(void)
{
    test_time_unix_epoch();
    test_time_time_t_is_seconds();
    test_time_truncate_to_hour();
    test_time_truncate_to_specific_hour();
    test_time_addition();
}
