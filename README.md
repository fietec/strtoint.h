# strtoint

A simple stb-style single-header library for parsing strings into
explicit-width integers, similarily to the `strtol` function family.

## Features
- Explicit Widths: Dedicated functions for 8, 16, 32, and 64-bit signed and unsigned types.
- C23/Modern Compliance: Natively parses binary (0b/0B), hexadecimal (0x/0X), and octal (0) prefixes during auto-detection (base = 0).
- Standard Error Handling: Sets `errno` to `ERANGE` on overflow/underflow relative to the target type, and to `EINVAL` for invalid bases.
- Zero Dependencies: Compiles with standard C99/C11/C23 toolchains using only <stdint.h>, <stdbool.h>, <ctype.h>, and <errno.h>.

## Usage
To use this library, include the header normally in your project. In exactly one C or C++ file, define `STRTOINT_IMPLEMENTATION` before including the header to create the function definitions.

```c
#define STRTOINT_IMPLEMENTATION
#include "strtoint.h"
```

### Example
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