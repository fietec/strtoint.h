# strtoint

A simple stb-style single-header library for parsing strings into fixed-width integers, following the semantics of the `strtol` / `strtoul` function family.

## Features

- **Fixed-Width Types:** Dedicated functions for 8, 16, 32, and 64-bit signed and unsigned integer types.
- **`strtol` / `strtoul` Semantics:** Skips leading whitespace, accepts an optional sign, supports bases 0 and 2-36, and reports the first unparsed character through `endptr`.
- **Range Checking:** Values outside the target type's range are clamped to the corresponding bound and reported as a range error instead of wrapping around.
- **Custom Ranges:** `*_custom` variants support arbitrary inclusive `[min, max]` bounds.
- **Binary Prefix:** Supports C23-style `0b` / `0B` prefixes when the base is 0 or 2, in addition to the usual hexadecimal and octal prefixes.
- **Unsigned Negation:** Follows the unsigned negation semantics specified for `strtoul` / `strtoull`. A leading `-` converts the parsed magnitude using unsigned negation; `*_custom` variants apply the same semantics using the supplied maximum as the range limit.
- **Structured Results:** `_s` variants report parsing metadata and errors through a result struct instead of relying on `errno`. This includes flags for leading whitespace and negative signs, allowing callers to impose stricter validation when needed.
- **Single Header:** No external dependencies; the library uses only standard C headers.

## Usage

To use the library, include the header normally in your project. In exactly one C or C++ translation unit, define `STRTOINT_IMPLEMENTATION` before including the header to create the function definitions.

```c
#define STRTOINT_IMPLEMENTATION
#include "strtoint.h"
```

### Example 1 - Basic Conversion

The legacy variants behave similarly to `strtol` / `strtoul`, including `errno`-based range reporting and `endptr` handling.
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
    int8_t value = strtoint8(hex_str, &endptr, 0);

    if (errno == ERANGE) {
        printf("Overflow detected for int8_t.\n");
    } else {
        printf("Parsed value: %"PRId8"\n", value); // Output: 127
        printf("Unparsed part: '%s'\n", endptr);   // Output: ' extra text'
    }
    return 0;
}
```
Output:
```
Parsed value: 127
Unparsed part: ' extra text'
```

### Example 2 - Structured Results

The `_s` variants provide parsing metadata and errors via the `strtoint_res_t` struct without relying on `errno`:
```c
struct strtoint_res_t {
    const char *endptr;   // Pointer to the first unparsed character

    // Parsing Flags
    bool negative;        // Set if a leading '-' sign was parsed (useful for strict unsigned checks)
    bool leading_spaces;  // Set if leading whitespace was skipped prior to conversion

    // Error Flags
    bool no_digits;       // No valid digits were found in the string
    bool out_of_range;    // The value exceeded the representable range of the target type
    bool invalid_params;  // The supplied parameters were invalid (e.g. base not 0 or 2-36, min > max etc.)
};
```
```c
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#define STRTOINT_IMPLEMENTATION
#include "strtoint.h"

int main(void){
    const char *input = "-5";
    struct strtoint_res_t res;

    uint8_t value = strtouint8_s(input, &res, 10);

    if (res.invalid_params) {
        printf("Error: Invalid parameters.\n");
    } else if (res.no_digits) {
        printf("Error: No valid digits found.\n");
    } else if (res.out_of_range) {
        printf("Error: Value is outside the range of uint8_t.\n");
    } else if (res.negative) {
        /*
         * strtouint8_s() follows strtoul() semantics, so "-5"
         * is converted using unsigned negation and produces 251.
         *
         * The negative flag allows callers to reject negative
         * input when their application requires a strictly
         * non-negative textual representation.
         */
        printf("Error: Negative values are not allowed!\n");
    } else {
        printf("Successfully parsed: %" PRIu8 "\n", value);
    }

    return 0;
}
```
Output:
```
Error: Negative values are not allowed!
```
