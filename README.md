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
     will not introduce any data races.
  6. Does not rely on system specific code, only C11 standard library functions and API's.

