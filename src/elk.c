#include "elk.h"

#include <assert.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
static int64_t const SECONDS_PER_MINUTE = INT64_C(60);
static int64_t const MINUTES_PER_HOUR = INT64_C(60);
static int64_t const HOURS_PER_DAY = INT64_C(24);
static int64_t const DAYS_PER_YEAR = INT64_C(365);
static int64_t const SECONDS_PER_HOUR = MINUTES_PER_HOUR * SECONDS_PER_MINUTE;
static int64_t const SECONDS_PER_DAY = SECONDS_PER_HOUR * HOURS_PER_DAY;
static int64_t const SECONDS_PER_YEAR = SECONDS_PER_DAY * DAYS_PER_YEAR;

// Days in a year up to beginning of month
static int64_t const sum_days_to_month[2][13] = {
    {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

static int64_t
elk_num_leap_years_since_epoch(int64_t year)
{
    assert(year >= 1);

    year -= 1;
    return year / 4 - year / 100 + year / 400;
}

ElkTime const elk_unix_epoch_timestamp = INT64_C(62135596800);

bool
elk_is_leap_year(int year)
{
    if (year % 4 != 0)
        return false;
    if (year % 100 == 0 && year % 400 != 0)
        return false;
    return true;
}

ElkTime
elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds)
{
    assert(year >= 1 && year <= INT16_MAX);
    assert(day >= 1 && day <= 31);
    assert(month >= 1 && month <= 12);
    assert(hour >= 0 && hour <= 23);
    assert(minutes >= 0 && minutes <= 59);
    assert(seconds >= 0 && seconds <= 59);

    // Seconds in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    ElkTime ts = (year - 1) * SECONDS_PER_YEAR + num_leap_years_since_epoch * SECONDS_PER_DAY;

    // Seconds in the months up to the start of this month
    int64_t const days_until_start_of_month =
        elk_is_leap_year(year) ? sum_days_to_month[1][month] : sum_days_to_month[0][month];
    ts += days_until_start_of_month * SECONDS_PER_DAY;

    // Seconds in the days of the month up to this one.
    ts += (day - 1) * SECONDS_PER_DAY;

    // Seconds in the hours, minutes, & seconds so far this day.
    ts += hour * SECONDS_PER_HOUR;
    ts += minutes * SECONDS_PER_MINUTE;
    ts += seconds;

    assert(ts >= 0);

    return ts;
}

ElkTime
elk_make_time(ElkStructTime tm)
{
    return elk_time_from_ymd_and_hms(tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
}

static int64_t
elk_days_since_epoch(int year)
{
    // Days in the years up to now.
    int64_t const num_leap_years_since_epoch = elk_num_leap_years_since_epoch(year);
    int64_t ts = (year - 1) * DAYS_PER_YEAR + num_leap_years_since_epoch;

    return ts;
}

ElkStructTime
elk_make_struct_time(ElkTime time)
{
    assert(time >= 0);

    // Get the seconds part and then trim it off and convert to minutes
    int const second = time % SECONDS_PER_MINUTE;
    time = (time - second) / SECONDS_PER_MINUTE;
    assert(time >= 0 && second >= 0 && second <= 59);

    // Get the minutes part, trim it off and convert to hours.
    int const minute = time % MINUTES_PER_HOUR;
    time = (time - minute) / MINUTES_PER_HOUR;
    assert(time >= 0 && minute >= 0 && minute <= 59);

    // Get the hours part, trim it off and convert to days.
    int const hour = time % HOURS_PER_DAY;
    time = (time - hour) / HOURS_PER_DAY;
    assert(time >= 0 && hour >= 0 && hour <= 23);

    // Rename variable for clarity
    int64_t const days_since_epoch = time;

    // Calculate the year
    int year = days_since_epoch / (DAYS_PER_YEAR) + 1; // High estimate, but good starting point.
    int64_t test_time = elk_days_since_epoch(year);
    while (test_time > days_since_epoch) {
        int step = (test_time - days_since_epoch) / (DAYS_PER_YEAR + 1);
        step = step == 0 ? 1 : step;
        year -= step;
        test_time = elk_days_since_epoch(year);
    }
    assert(test_time <= elk_days_since_epoch(year));
    time -= elk_days_since_epoch(year); // Now it's days since start of the year.
    assert(time >= 0);

    // Calculate the month
    int month = 0;
    int leap_year_idx = elk_is_leap_year(year) ? 1 : 0;
    for (month = 1; month <= 11; month++) {
        if (sum_days_to_month[leap_year_idx][month + 1] > time) {
            break;
        }
    }
    assert(time >= 0 && month > 0 && month <= 12);
    time -= sum_days_to_month[leap_year_idx][month]; // Now in days since start of month

    // Calculate the day
    int const day = time + 1;
    assert(day > 0 && day <= 31);

    return (ElkStructTime){
        .year = year, .month = month, .day = day, .hour = hour, .minute = minute, .second = second};
}

ElkTime
elk_time_truncate_to_specific_hour(ElkTime time, int hour)
{
    assert(hour >= 0 && hour <= 23 && time >= 0);

    ElkTime adjusted = time;

    int64_t const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    int64_t const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    int64_t actual_hour = (adjusted / SECONDS_PER_HOUR) % HOURS_PER_DAY;
    if (actual_hour < hour) {
        actual_hour += 24;
    }

    int64_t change_hours = actual_hour - hour;
    assert(change_hours >= 0);
    adjusted -= change_hours * SECONDS_PER_HOUR;

    assert(adjusted >= 0);

    return adjusted;
}

ElkTime
elk_time_truncate_to_hour(ElkTime time)
{
    ElkTime adjusted = time;

    int64_t const seconds = adjusted % SECONDS_PER_MINUTE;
    adjusted -= seconds;

    int64_t const minutes = (adjusted / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    adjusted -= minutes * SECONDS_PER_MINUTE;

    assert(adjusted >= 0);

    return adjusted;
}

ElkTime
elk_time_add(ElkTime time, int change_in_time)
{
    ElkTime result = time + change_in_time;
    assert(result >= 0);
    return result;
}

/*-------------------------------------------------------------------------------------------------
 *                                       String Interner
 *-----------------------------------------------------------------------------------------------*/
struct ElkStringInterner {
};

ElkStringInterner *
elk_string_interner_create(int size_exp)
{
    assert(false);
    return NULL;
}

void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    assert(false);
    return;
}

ElkInternedString
elk_string_interner_intern(ElkStringInterner *interner, char const *string)
{
    assert(false);
    return 0;
}

char const *
elk_string_interner_retrieve(ElkStringInterner *interner, ElkInternedString handle)
{
    assert(false);
    return NULL;
}
