#pragma once

// Set DEBUG to 1 to get debug printouts.
#define DEBUG 0


#if DEBUG
#define debug_print(...)    Serial.print(__VA_ARGS__)
#define debug_println(...)  Serial.println(__VA_ARGS__)
#define debug_printf(...)   Serial.printf(__VA_ARGS__)
#else
#define debug_print(...)
#define debug_println(...)
#define debug_printf(...)
#endif