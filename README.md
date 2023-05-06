# elk - A general purpose C11 library.

  I like elk. They're really interesting, majestic animals. And did you know they have ivory?! So
  I decided to name my general purpose C library after them. Maybe I'll make a testing library at
  some point and call it wolf, but I haven't made any plans yet.
 
 Goals and non-Goals:
  1. I only implement the things I need. If it's in here, I needed it at some point. Or I thought it
     would be fun to implement and test, and I might use it some day. But if I didn't need it or
     just want to make it, it isn't in there. As a result, some of the designs will be tuned to my
     use cases.

  2. Single header + single source file. Keeps it simple, just drop 2 files into your source tree
     and make sure they're configured in your build system.

  3. NOT threadsafe. Access to any objects will need to be protected by the user of those objects
     with a mutex or other means to prevent data races.

  4. Avoid global state. All state related to any objects created by this library should be stored 
     in that object. No global state makes it possible to use it in multithreaded applications. 

  5. As a result of number 4, most functions are thread safe so as long as the input parameters are
     protected from data races before being passed into the function, the function itself will not
     introduce any data races.

     NOTE: Sometimes I depend on the standard C-library, so this cannot be enforced for some
     functions, e.g. time related ones. I'll place a warning in the documentation when that is the
     case.

  6. Does not rely on system specific code, only C11 standard library functions and API's.

## Notes

### Time
  The time related functions in this library do NOT account for local time or timezones. I almost 
  always only need to work in the UTC timezone, so I don't worry about it. 

  In this library I also assume I'm working on a system that uses seconds since the unix epoch for
  the values it stores in `time_t` objects. This is checked in the tests, and if you run the tests
  on another system where that is not the case, they should fail.

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

### Memory
  The memory debugging functions can be activated by specifying ELK_MEMORY_DEBUG as a macro on the
  compiler command line. E.g. "-DELK_MEMORY_DEBUG" will turn it on, and then anywhere `malloc()`, 
  `realloc()`, 'calloc()`, or `free()` is called it will be replaced with an internal version of
  those that tracks memory allocations. Buffer overruns are detected when memory is reallocated or
  freed and causes the application to crash immediately with a message printed to `stderr`. 

  The `elk_init_memory_debug()` function takes optional mutex parameters that it will use 
  behind the scenes to prevent data races for the memory debugger. It is the user's responsibility
  to plug those in when working in a multi-threaded application. If only working single threaded,
  then just pass `NULL` in and it will work fine.

  The functions that initialize, finalize, and report for the memory debugger are no-ops if 
  the ELK_MEMORY_DEBUG macro is not defined.

## Releases

### Version 2.0.0 - IN PROGRESS
  - Removed some less widely applicable types.
  - More consistent error handling.
  - Changed some types around, for instance ElkList becomes ElkArray and a list type is added.
  - Make more use of pointers for a cleaner API.

### Version 1.2.0
  - (2023-02-14) Multiple upgrades.
  - Removed dependency on some feature test macros.
  - Updated a lot of documentation and organized it into "modules" (per Doxygen)
  - Added the memory debugging functionality.

### Version 1.1.0
  - (2022-06-19) Adjustments to time functions.
  - Removed calendar day only time creation, just left behind the one that uses date and time.
  - Added enum for easier math with times.
  - Added function for rounding down to a specific hour.
  - Major bugfix for when I calculate the timezone offset (internally) that affected several
    time functions.

### Version 1.0.0
  - (2022-06-19) Initial release.

