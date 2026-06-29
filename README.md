# strtoint

A simple stb-style single-header library for parsing strings into
explicit-width integers, similarily to the `strtol` function family.

## Features
- Explicit Widths: Dedicated functions for 8, 16, 32, and 64-bit signed and unsigned types.
- C23/Modern Compliance: Natively parses binary (0b/0B), hexadecimal (0x/0X), and octal (0) prefixes during auto-detection (base = 0).
- Enhanced Safety (`_s` variants): Provides thread-safe, reentrant error handling using a result struct instead of relying on the global errno state.
- Zero Dependencies: Compiles with standard ISO-C99 using only <stdint.h>, <stdbool.h>, <ctype.h>, and <errno.h>.

## Usage
To use this library, include the header normally in your project. In exactly one C or C++ file, define `STRTOINT_IMPLEMENTATION` before including the header to create the function definitions.

```c
#define STRTOINT_IMPLEMENTATION
#include "strtoint.h"
```

### Example 1
```c
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#define STRTOINT_IMPLEMENTATION
#include "strtoint.h"

int main(void) {
    const char *hex_str = "0x7F extra text";
    char *endptr;

    // Parse directly into an 8-bit signed integer
    int8_t value = strtoint8(hex_str, &endptr, 16);

    if (errno == ERANGE) {
        printf("Overflow detected for int8_t.\n");
    } else {
        printf("Parsed value: %"PRId8"\n", value); // Output: 127
        printf("Unparsed part: '%s'\n", endptr);   // Output: ' extra text'
    }
    return 0;
}
```

### Example 2
Using the `_s` variants:
```c
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#define STRTOINT_IMPLEMENTATION
#include "strtoint.h"

int main(void) {
    const char *ip_str = "-5"; // Modulo arithmetic would usually make this 251
    struct strtoint_res_t res; // Declare a struct to hold additional data to the usual `endptr`

    uint8_t value = strtouint8_s(ip_str, &res, 10);

    if (res.invalid_base) {
        printf("Error: Unsupported numerical base.\n");
    } else if (res.out_of_range) {
        printf("Error: Value overflows uint8_t.\n");
    } else if (res.negative) {
        // Safely catch negative inputs before they wrap around
        printf("Error: Negative values are not allowed!\n");
    } else {
        printf("Successfully parsed: %"PRIu8"\n", value);
    }
    return 0;
}
```