# elk - A general purpose C11 library.

  I like elk. They're really interesting, majestic animals. And did you know they have ivory?! So
  I decided to name my ~~general purpose~~ utility C library after them. 
 
 Goals and non-Goals:
  1. I only implement the things I need. If it's in here, I needed it at some point.

  2. Single header + single source file. Keeps it as simple as possible while also keeping the 
     implementation seperate from the API; just drop 2 files into your source tree and make sure
     they're configured in your build system. (See design notes section below.)

  3. NOT threadsafe. Access to any objects will need to be protected by the user of those objects
     with a mutex or other means to prevent data races.

  4. NO global mutable state and reentrant functions. All state related to any objects created by
     this library should be stored in that object. No global state and reentrant functions makes it
     possible to use it in multithreaded applications so long as the user ensures the data they
     need protected is protected. 

  5. As a result of number 4, functions are thread safe so as long as the input parameters are
     protected from data races before being passed into the function, the function itself will not
     introduce any data races.

  6. Does not rely on system specific code. Also trying to minimalize dependencies on the C runtime
     and the C standard library. However I sometimes need string.h (memcpy, memmove), stdio.h 
     (printf and friends) and stdint.h (integer types with specified widths).

## Design Notes

### Time
  The time related functions in this library do NOT account for local time or timezones. I almost 
  always only need to work in the UTC timezone, so I don't worry about it. 

  Know your time and what you need from it! THE TIME FUNCTIONALITY IN THIS LIBRARY IS NOT WIDELY 
  APPLICABLE. I don't even know if something like that exists.

  My use cases typically involve meteorological forecasts and/or observations. The current 
  implementation of this library uses January 1st, 1 AD as the epoch. It cannot handle times
  before that. The maximum time that can be handled by all the functions is December 31st, 32767.
  So this more than covers the useful period of meteorological observations and forecasts.

### Header (.h) & Source (.c) File Layout
  Most of the code is in the header, which is awkward. But I wanted to favor doing work on the
  stack and enable as much inlining as possible. So I've let alot of my implementation go into the
  header. I've given most of the inline functions (except the smallest) external linkage so they
  only get compiled into the final executable once, but have the best opportunities to get inlined.
  Some of the smallest functions are just \c static \c inline. This makes it hard to know what's
  intended as public API and what isn't from the library. But if you can read comments formatted for
  Doxygen, then you can follow along. If you have Doxygen, just use it to create the documentation
  and read that.

## Releases

### Version 2.1.0 - IN PROGRESS
  - (XXXX-XX-XX) NOTES ON REVISIONS
  - Added functions to parse string into int64_t or double values.

### Version 2.0.0
  - (2023-09-23) Major revision.
  - Minimalistic, general purpose tools.
  - Removed a lot of stuff from V1.2.0
  - Created my own types for handling calendar time.
  - Added a string type (gotta reinvent the C-string)
  - Added a string interner.
  - Added pool and arena allocators.
  - Added collection ledger types (queue & array ledgers)

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

