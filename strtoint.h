/* strtoint.h - v3.0.0 - Public Domain - ISO C99
 * A family of string-to-integer conversion functions for fixed-width types.
 *
 * Overview:
 *   - Follows standard strtol / strtoul behavior (skips leading whitespace,
 *     parses optional sign, and resolves bases 0 or [2, 36], and sets endptr
 *     to the first unparsed character).
 *   - If the parsed value falls outside the target type's (or custom) bounds,
 *     the return value is clamped to the corresponding bound and a range error is reported.
 *   - Returns 0 on invalid parameters or if no valid digits are found.
 *
 * Extensions to strtol / strtoul:
 *   - Fixed-Width Types: Variants for int8..int64 and uint8..uint64.
 *   - Binary Prefix: Supports C23-style "0b" / "0B" prefixes (when base is 0 or 2).
 *   - Structured Variants (_s): Replace errno with a result struct for explicit
 *     parsing metadata and error handling. Exposes parsing flags (leading whitespace,
 *     negative signs) to allow strict validation of inputs otherwise permissively
 *     accepted by standard strtol / strtoul.
 *   - Unsigned Negation: Follows strtoul's two's-complement wrap-around behavior. This means:
 *      - A leading '-' wraps non-zero magnitudes relative to 'max' (the type's maximum value):
 *          result = max - magnitude + 1
 *          e.g., for strtouint8("-5"): 255 - 5 + 1 = 251
 *      - A magnitude greater than 'max' will result in the max being returned and an 'out_of_range' error set:
 *          e.g. for strtouint8("-256") = 255
 *        This matches the specified behavior of strtoul, but since not all implementations are consistent
 *        in this regard it is clearly specified here.
 *      - In the '_s' variants a leading '-' always populates res->negative = true, allowing callers in strict
 *        linear contexts (e.g., port numbers) to reject negative inputs if desired.
 *
 * Custom Bounds:
 *   Functions with the '_custom' suffix accept inclusive [min, max] bounds,
 *   treating the interval as a custom integer domain.
 *   You can set these bounds to any arbitrary values which means:
 *    - For signed conversions or positive unsigned conversions (meaning without a leading '-')
 *      inputs outside of [min, max] clamp to the nearest boundary and set an 'out_of_range' error.
 *    - For negative unsigned conversions additionally, the following rule holds:
 *      If the already wrapped result (see above) falls below 'min', 'max' is returned and an 'out_of_range' error set.
 *        e.g. for min = 10, max = 20:
 *        " 5"  -> 10 + 'out_of_range' error
 *        "30"  -> 20 + 'out_of_range' error
 *        " -1" -> 20
 *        "-11" -> 10
 *        "-12" -> 20 + 'out_of_range' error
 *   For bounds set to usual integer min and max values this leads to strtol / strtoul compliant behavior.
 *   In fact, all of the fixed-integer conversion functions in this library are simple wrappers around the '_custom' functions.
 *   This also means you can create a strtol drop-in-replacement by setting min = LONG_MIN and max = LONG_MAX.
 */

#ifndef STRTOINT_H
#define STRTOINT_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Legacy / strtol-like Variants                                              */
/* -------------------------------------------------------------------------- */
/* These set `errno` to ERANGE on overflow and EINVAL for invalid parameters. */

// Signed Conversions
int8_t  strtoint8 (const char *str, char **endptr, int base);
int16_t strtoint16(const char *str, char **endptr, int base);
int32_t strtoint32(const char *str, char **endptr, int base);
int64_t strtoint64(const char *str, char **endptr, int base);
int64_t strtoint_custom(const char *str, char **endptr, int base, int64_t min, int64_t max);

// Unsigned Conversions
uint8_t  strtouint8 (const char *str, char **endptr, int base);
uint16_t strtouint16(const char *str, char **endptr, int base);
uint32_t strtouint32(const char *str, char **endptr, int base);
uint64_t strtouint64(const char *str, char **endptr, int base);
uint64_t strtouint_custom(const char *str, char **endptr, int base, uint64_t min, uint64_t max);

/* -------------------------------------------------------------------------- */
/* Safe / Structured Variants                                                 */
/* -------------------------------------------------------------------------- */
/* These variants do not rely on `errno`. Instead, they populate the
 * `strtoint_res_t` structure with parsing metadata and errors.               */

// Detailed result metadata for the structured (_s) variants.
typedef struct strtoint_res_t{
    const char *endptr;   // Pointer to the first unparsed character

    // Parsing Flags
    bool negative;        // Set if a leading '-' sign was parsed (useful for strict unsigned checks)
    bool leading_spaces;  // Set if leading whitespace was skipped prior to conversion

    // Error Flags
    bool no_digits;       // No valid digits were found in the string
    bool out_of_range;    // The value exceeded the representable range of the target type
    bool invalid_params;  // The supplied parameters were invalid (e.g. base not 0 or 2-36, min > max etc.)
} strtoint_res_t;

// Signed Conversions
int8_t  strtoint8_s (const char *str, strtoint_res_t *res, int base);
int16_t strtoint16_s(const char *str, strtoint_res_t *res, int base);
int32_t strtoint32_s(const char *str, strtoint_res_t *res, int base);
int64_t strtoint64_s(const char *str, strtoint_res_t *res, int base);
int64_t strtoint_custom_s(const char *str, strtoint_res_t *res, int base, int64_t min, int64_t max);

// Unsigned Conversions
uint8_t  strtouint8_s (const char *str, strtoint_res_t *res, int base);
uint16_t strtouint16_s(const char *str, strtoint_res_t *res, int base);
uint32_t strtouint32_s(const char *str, strtoint_res_t *res, int base);
uint64_t strtouint64_s(const char *str, strtoint_res_t *res, int base);
uint64_t strtouint_custom_s(const char *str, strtoint_res_t *res, int base, uint64_t min, uint64_t max);

#ifdef __cplusplus
}
#endif

#endif // STRTOINT_H

#ifdef STRTOINT_IMPLEMENTATION

#include <ctype.h>

static inline int strtoint__digit_value(int c, int base)
{
    int value = -1;
    if ('0' <= c && c <= '9') value = c - '0';
    else if ('A' <= c && c <= 'Z') value = c - 'A' + 10;
    else if ('a' <= c && c <= 'z') value = c - 'a' + 10;
    if (0 <= value && value < base) return value;
    return -1;
}

static inline int strtoint__handle_base(const char **str, int base)
{
    const char *s = *str;
    if ((base == 0 || base == 2) && s[0] == '0' && (s[1] == 'B' || s[1] == 'b')){
        if (strtoint__digit_value((unsigned char)s[2], 2) != -1) {
            base = 2;
            *str += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'X' || s[1] == 'x')){
        if (strtoint__digit_value((unsigned char)s[2], 16) != -1) {
            base = 16;
            *str += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 8) && s[0] == '0'){
        base = 8;
    } else if (base == 0){
        base = 10;
    }
    return base;
}

/* Signed Parsing */

int64_t strtoint_custom_s(const char *str, strtoint_res_t *res, int base, int64_t min, int64_t max)
{
    if (!str || min > max || base < 0 || base == 1 || base > 36){
        if (res){
            res->endptr = str;
            res->negative = false;
            res->leading_spaces = false;
            res->no_digits = false;
            res->out_of_range = false;
            res->invalid_params = true;
        }
        return 0;
    }
    const char *start = str;
    while (isspace((unsigned char)*start)) start++;

    bool leading_spaces = (start != str);
    bool negative = false;
    if (*start == '-'){
        negative = true;
        start++;
    } else if (*start == '+'){
        start++;
    }

    base = strtoint__handle_base(&start, base);

    bool out_of_range = false;
    int64_t cutoff = INT64_MIN / base;
    int64_t cutoff_digit = -(INT64_MIN % base);
    const char *pdigit = start;
    int digit;
    int64_t result = 0;
    while ((digit = strtoint__digit_value((unsigned char)*pdigit, base)) != -1){
        pdigit++;
        if (out_of_range || (result == 0 && digit == 0)) continue;
        if (result < cutoff || (result == cutoff && digit > cutoff_digit)){
            result = negative? min : max;
            out_of_range = true;
            continue;
        }
        result = result * base - digit;
    }

    if (pdigit == start){
        if (res){
            res->endptr = str;
            res->negative = negative;
            res->leading_spaces = leading_spaces;
            res->no_digits = true;
            res->out_of_range = false;
            res->invalid_params = false;
        }
        return 0;
    }
    if (!out_of_range){
        if (!negative){
            if (result < -INT64_MAX){
                result = max;
                out_of_range = true;
            } else{
                result = -result;
            }
        }
        if (result > max){
            result = max;
            out_of_range = true;
        } else if (result < min){
            result = min;
            out_of_range = true;
        }
    }

    if (res){
        res->endptr = pdigit;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
        res->no_digits = false;
        res->out_of_range = out_of_range;
        res->invalid_params = false;
    }
    return result;
}

int64_t strtoint_custom(const char *str, char **endptr, int base, int64_t min, int64_t max)
{
    strtoint_res_t res;
    int64_t result = strtoint_custom_s(str, &res, base, min, max);
    if (res.invalid_params) errno = EINVAL;
    else if (res.out_of_range) errno = ERANGE;
    if (endptr) *endptr = (char*) res.endptr;
    return result;
}

int8_t strtoint8(const char *str, char **endptr, int base)
{
    return (int8_t) strtoint_custom(str, endptr, base, INT8_MIN, INT8_MAX);
}

int8_t strtoint8_s(const char *str, strtoint_res_t *res, int base)
{
    return (int8_t) strtoint_custom_s(str, res, base, INT8_MIN, INT8_MAX);
}

int16_t strtoint16(const char *str, char **endptr, int base)
{
    return (int16_t) strtoint_custom(str, endptr, base,INT16_MIN, INT16_MAX);
}

int16_t strtoint16_s(const char *str, strtoint_res_t *res, int base)
{
    return (int16_t) strtoint_custom_s(str, res, base, INT16_MIN, INT16_MAX);
}

int32_t strtoint32(const char *str, char **endptr, int base)
{
    return (int32_t) strtoint_custom(str, endptr, base, INT32_MIN, INT32_MAX);
}

int32_t strtoint32_s(const char *str, strtoint_res_t *res, int base)
{
    return (int32_t) strtoint_custom_s(str, res, base, INT32_MIN, INT32_MAX);
}

int64_t strtoint64(const char *str, char **endptr, int base)
{
    return strtoint_custom(str, endptr, base, INT64_MIN, INT64_MAX);
}

int64_t strtoint64_s(const char *str, strtoint_res_t *res, int base)
{
    return strtoint_custom_s(str, res, base, INT64_MIN, INT64_MAX);
}

/* Unsigned Parsing */

uint64_t strtouint_custom_s(const char *str, strtoint_res_t *res, int base, uint64_t min, uint64_t max)
{
    if (!str || min > max || base < 0 || base == 1 || base > 36){
        if (res){
            res->endptr = str;
            res->negative = false;
            res->leading_spaces = false;
            res->no_digits = false;
            res->out_of_range = false;
            res->invalid_params = true;
        }
        return 0;
    }
    const char *start = str;
    while (isspace((unsigned char)*start)) start++;

    bool leading_spaces = (start != str);
    bool negative = false;
    if (*start == '-'){
        negative = true;
        start++;
    } else if (*start == '+'){
        start++;
    }

    base = strtoint__handle_base(&start, base);

    bool out_of_range = false;
    const char *pdigit = start;
    uint64_t result = 0;
    uint64_t udigit;
    int digit;
    while ((digit = strtoint__digit_value((unsigned char)*pdigit, base)) != -1){
        udigit = (uint64_t) digit;
        pdigit++;
        if (out_of_range || (result == 0 && udigit == 0)) continue;
        if (udigit > max || result > (max - udigit) / base){
            result = max;
            out_of_range = true;
            continue;
        }
        result = result * base + udigit;
    }
    if (pdigit == start){
        if (res){
            res->endptr = str;
            res->negative = negative;
            res->leading_spaces = leading_spaces;
            res->no_digits = true;
            res->out_of_range = false;
            res->invalid_params = false;
        }
        return 0;
    }

    if (!out_of_range){
        if (negative && result > 0) {
            if (max - result + 1 < min){
                result = max;
                out_of_range = true;
            } else{
                result = max - result + 1;
            }
        } else if (result < min){
            result = min;
            out_of_range = true;
        }
    }

    if (res){
        res->endptr = pdigit;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
        res->no_digits = false;
        res->out_of_range = out_of_range;
        res->invalid_params = false;
    }
    return result;
}

uint64_t strtouint_custom(const char *str, char **endptr, int base, uint64_t min, uint64_t max)
{
    strtoint_res_t res;
    uint64_t result = strtouint_custom_s(str, &res, base, min, max);
    if (res.invalid_params) errno = EINVAL;
    else if (res.out_of_range) errno = ERANGE;
    if (endptr) *endptr = (char*) res.endptr;
    return result;
}

uint8_t strtouint8(const char *str, char **endptr, int base)
{
    return (uint8_t) strtouint_custom(str, endptr, base, 0, UINT8_MAX);
}

uint8_t strtouint8_s(const char *str, strtoint_res_t *res, int base)
{
    return (uint8_t) strtouint_custom_s(str, res, base, 0, UINT8_MAX);
}

uint16_t strtouint16(const char *str, char **endptr, int base)
{
    return (uint16_t) strtouint_custom(str, endptr, base, 0, UINT16_MAX);
}

uint16_t strtouint16_s(const char *str, strtoint_res_t *res, int base)
{
    return (uint16_t) strtouint_custom_s(str, res, base, 0, UINT16_MAX);
}

uint32_t strtouint32(const char *str, char **endptr, int base)
{
    return (uint32_t) strtouint_custom(str, endptr, base, 0, UINT32_MAX);
}

uint32_t strtouint32_s(const char *str, strtoint_res_t *res, int base)
{
    return (uint32_t) strtouint_custom_s(str, res, base, 0, UINT32_MAX);
}

uint64_t strtouint64(const char *str, char **endptr, int base)
{
    return strtouint_custom(str, endptr, base, 0, UINT64_MAX);
}

uint64_t strtouint64_s(const char *str, strtoint_res_t *res, int base)
{
    return strtouint_custom_s(str, res, base, 0, UINT64_MAX);
}

#endif // STRTOINT_IMPLEMENTATION
