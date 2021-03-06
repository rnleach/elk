# elk - A general purpose C11 library.

  I like elk. They're really interesting, majestic animals. And did you know they have ivory?! So
  I decided to name my general purpose C library after them. Maybe I'll make a testing library at
  some point and call it wolf, but I haven't made any plans yet.
 
 Goals and non-Goals:
  1. I only implement the things I need. If it's in here, I needed it at some point.
  2. Single header + single source file. Keeps it simple.
  3. NOT threadsafe. Access to any objects will need to be protected by the user of those objects
     with a mutex or other means to prevent data races.
  4. NO global state. All state related to any objects created by this library is stored in that
     object. This makes it simple to protect variables in multi-threaded scenarios.
  5. As a result of number 4, all functions must be re-entrant so as long as the input parameters
     are protected from data races before being passed into the function, the function itself
     will not introduce any data races. (NOTE: Sometimes we depend on the standard C-library, so
     this cannot be enforced for some functions, e.g. time related ones. I'll place a warning in the
     documentation when that is the case.)
  6. Does not rely on system specific code, only C11 standard library functions and API's.

## Notes

### Time
  The time related functions in this library do NOT account for local time or timezones. I almost 
  always only need to work in the UTC timezone, so I don't worry about it. 

  In this library I also assume I'm working on a system that uses seconds since the unix epoch for
  the values it stores in time_t objects. This is checked in the tests, and if you run the tests on
  another system where that is not the case, they should fail.

  Time is a very complicated subject, and robust libraries are rarely (if ever) robust for all uses.
  There's government time (statutory), daylight savings, time zones, solar time, different calendar 
  systems, leap seconds, and it goes on and on. I mostly (always?) work with scientific data that 
  makes good sense in the UTC timezone, and I have yet to knowingly run into an issue with leap 
  seconds. 

  I mentioned leap seconds above. I tested on both OSX and Ubuntu and found that the system time
  functions from `<time.h>` don't take leap seconds into account. So if you're only interested in
  calendar days, you can do math on `time_t` assuming 60 seconds per minute, 60 minutes per hour,
  and 24 hours per day.

  Know your time and what you need from it! THE TIME FUNCTIONALITY IN THIS LIBRARY IS NOT WIDELY 
  APPLICABLE. I don't even know if something like that exists.

## Releases

### Version 1.0.0
  - (2022-06-19) Initial release.
### Version 1.1.0
  - (2022-06-19) Adjustments to time functions.
  - Removed calendar day only time creation, just left behind the one that uses date and time.
  - Added enum for easier math with times.
  - Added function for rounding down to a specific hour.
  - Major bugfix for when I calculate the timezone offset (internally) that affected several
    time functions.
