/* strtoint.h - v2.0.0 - Public Domain
 * A family of robust string-to-integer conversion functions for fixed-width types.
 * Overview:
 *   - The strtoint{8,16,32,64} / strtouint{8,16,32,64} functions provide
 *     strtol / strtoul-compatible conversions for fixed-width integer types.
 *   - The strtoint_range / strtouint_range functions additionally allow
 *     custom minimum and maximum bounds (both inclusive).
 *   - Adds support for binary numbers via the "0b" or "0B" prefix (when base is 0 or 2).
 *   - Standardizes negative inputs for unsigned types across different platforms:
 *     A negative value is converted by subtracting its magnitude from the
 *     maximum bound plus one. Values whose magnitude exceeds the maximum
 *     bound saturate to the maximum bound and report an error.
 */

#ifndef STRTOINT_H
#define STRTOINT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Detailed result metadata for the structured (_s) variants.
struct strtoint_res_t {
    const char *endptr;   // Pointer to the first unparsed character
    bool negative;        // True if the input had a leading '-' sign
    bool leading_spaces;  // True if the input had leading whitespaces

    // Error Flags
    bool no_digits;       // No valid digits were found in the string
    bool out_of_range;    // The value exceeded the representable range of the target type
    bool invalid_params;  // The supplied parameters were invalid (e.g. base not 0 or 2-36, min > max etc.)
};

/* -------------------------------------------------------------------------- */
/* Legacy / strtol-like Variants                                              */
/* -------------------------------------------------------------------------- */
/* These provide strtol/strtoul-compatible conversions for fixed-width types.
 * They set `errno` to ERANGE on overflow and EINVAL for invalid parameters.  */

// Signed Conversions
int8_t  strtoint8 (const char *str, char **endptr, int base);
int16_t strtoint16(const char *str, char **endptr, int base);
int32_t strtoint32(const char *str, char **endptr, int base);
int64_t strtoint64(const char *str, char **endptr, int base);
int64_t strtoint_range(const char *str, char **endptr, int base, int64_t min, int64_t max);

// Unsigned Conversions
uint8_t  strtouint8 (const char *str, char **endptr, int base);
uint16_t strtouint16(const char *str, char **endptr, int base);
uint32_t strtouint32(const char *str, char **endptr, int base);
uint64_t strtouint64(const char *str, char **endptr, int base);
uint64_t strtouint_range(const char *str, char **endptr, int base, uint64_t min, uint64_t max);

/* -------------------------------------------------------------------------- */
/* Safe / Structured Variants                                                 */
/* -------------------------------------------------------------------------- */
/* These variants do not rely on global `errno`. Instead, they populate the
 * `strtoint_res_t` structure with parsing metadata and errors.               */

// Signed Conversions
int8_t  strtoint8_s (const char *str, struct strtoint_res_t *res, int base);
int16_t strtoint16_s(const char *str, struct strtoint_res_t *res, int base);
int32_t strtoint32_s(const char *str, struct strtoint_res_t *res, int base);
int64_t strtoint64_s(const char *str, struct strtoint_res_t *res, int base);
int64_t strtoint_range_s(const char *str, struct strtoint_res_t *res, int base, int64_t min, int64_t max);

// Unsigned Conversions
uint8_t  strtouint8_s (const char *str, struct strtoint_res_t *res, int base);
uint16_t strtouint16_s(const char *str, struct strtoint_res_t *res, int base);
uint32_t strtouint32_s(const char *str, struct strtoint_res_t *res, int base);
uint64_t strtouint64_s(const char *str, struct strtoint_res_t *res, int base);
uint64_t strtouint_range_s(const char *str, struct strtoint_res_t *res, int base, uint64_t min, uint64_t max);

#ifdef __cplusplus
}
#endif

#endif // STRTOINT_H

#ifdef STRTOINT_IMPLEMENTATION

#include <ctype.h>
#include <errno.h>
#include <string.h>

static inline int strtoint__digit_value(char c, int base)
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
        if (strtoint__digit_value(s[2], 2) != -1) {
            base = 2;
            *str += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'X' || s[1] == 'x')){
        if (strtoint__digit_value(s[2], 16) != -1) {
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

int64_t strtoint_range_s(const char *str, struct strtoint_res_t *res, int base, int64_t min, int64_t max)
{
    if (res) memset(res, 0, sizeof(*res));
    if (!str || min > max || base < 0 || base == 1 || base > 36){
        if (res){
            res->invalid_params = true;
            res->endptr = str;
        }
        return 0;
    }
    int64_t result = 0;
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

    const char *end = start;
    while (strtoint__digit_value(*end, base) != -1) end++;

    if (end == start){
        if (res){
            res->endptr = str;
            res->no_digits = true;
            res->leading_spaces = leading_spaces;
        }
        return 0;
    }

    bool out_of_range = false;

    const char *pdigit = start;
    int64_t cutoff = INT64_MIN / base;
    int64_t cutoff_digit = -(INT64_MIN % base);
    while (pdigit < end){
        int64_t digit = strtoint__digit_value(*pdigit++, base);
        if (result < cutoff || (result == cutoff && digit > cutoff_digit)){
            result = negative? min : max;
            out_of_range = true;
            goto end;
        }
        result = result * base - digit;
    }

    if (!negative){
        if (result < -INT64_MAX){
            result = INT64_MAX;
            out_of_range = true;
            goto end;
        }
        result = -result;
    }

    if (result > max){
        result = max;
        out_of_range = true;
    }
    if (result < min){
        result = min;
        out_of_range = true;
    }

end:
    if (res){
        res->endptr = end;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
        res->out_of_range = out_of_range;
    }
    return result;
}

int64_t strtoint_range(const char *str, char **endptr, int base, int64_t min, int64_t max)
{
    struct strtoint_res_t res;
    int64_t result = strtoint_range_s(str, &res, base, min, max);
    if (res.invalid_params) errno = EINVAL;
    else if (res.out_of_range) errno = ERANGE;
    if (endptr) *endptr = (char*) res.endptr;
    return result;
}

int8_t strtoint8(const char *str, char **endptr, int base)
{
    return strtoint_range(str, endptr, base, INT8_MIN, INT8_MAX);
}

int8_t strtoint8_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint_range_s(str, res, base, INT8_MIN, INT8_MAX);
}

int16_t strtoint16(const char *str, char **endptr, int base)
{
    return strtoint_range(str, endptr, base,INT16_MIN, INT16_MAX);
}

int16_t strtoint16_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint_range_s(str, res, base, INT16_MIN, INT16_MAX);
}

int32_t strtoint32(const char *str, char **endptr, int base)
{
    return strtoint_range(str, endptr, base, INT32_MIN, INT32_MAX);
}

int32_t strtoint32_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint_range_s(str, res, base, INT32_MIN, INT32_MAX);
}

int64_t strtoint64(const char *str, char **endptr, int base)
{
    return strtoint_range(str, endptr, base, INT64_MIN, INT64_MAX);
}

int64_t strtoint64_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint_range_s(str, res, base, INT64_MIN, INT64_MAX);
}

uint64_t strtouint_range_s(const char *str, struct strtoint_res_t *res, int base, uint64_t min, uint64_t max)
{
    if (res) memset(res, 0, sizeof(*res));
    if (!str || min > max || base < 0 || base == 1 || base > 36){
        if (res){
            res->endptr = str;
            res->invalid_params = true;
        }
        return 0;
    }
    uint64_t result = 0;
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

    const char *end = start;
    while (strtoint__digit_value(*end, base) != -1) end++;

    if (end == start){
        if (res){
            res->endptr = str;
            res->no_digits = true;
            res->leading_spaces = leading_spaces;
        }
        return 0;
    }

    bool out_of_range = false;
    const char *pdigit = start;
    while (pdigit < end){
        uint64_t digit = strtoint__digit_value(*pdigit++, base);
        if (digit > max || result > (max - digit) / base){
            result = max;
            out_of_range = true;
            goto end;
        }
        result = result * base + digit;
    }

    if (negative && result > 0) {
        result = max - result + 1;
    }
    if (result < min){
        out_of_range = true;
        result = min;
    }
end:
    if (res){
        res->endptr = end;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
        res->out_of_range = out_of_range;
    }
    return result;
}

uint64_t strtouint_range(const char *str, char **endptr, int base, uint64_t min, uint64_t max)
{
    struct strtoint_res_t res;
    uint64_t result = strtouint_range_s(str, &res, base, min, max);
    if (res.invalid_params) errno = EINVAL;
    else if (res.out_of_range) errno = ERANGE;
    if (endptr) *endptr = (char*) res.endptr;
    return result;
}

uint8_t strtouint8(const char *str, char **endptr, int base)
{
    return strtouint_range(str, endptr, base, 0, UINT8_MAX);
}

uint8_t strtouint8_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint_range_s(str, res, base, 0, UINT8_MAX);
}

uint16_t strtouint16(const char *str, char **endptr, int base)
{
    return strtouint_range(str, endptr, base, 0, UINT16_MAX);
}

uint16_t strtouint16_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint_range_s(str, res, base, 0, UINT16_MAX);
}

uint32_t strtouint32(const char *str, char **endptr, int base)
{
    return strtouint_range(str, endptr, base, 0, UINT32_MAX);
}

uint32_t strtouint32_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint_range_s(str, res, base, 0, UINT32_MAX);
}

uint64_t strtouint64(const char *str, char **endptr, int base)
{
    return strtouint_range(str, endptr, base, 0, UINT64_MAX);
}

uint64_t strtouint64_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint_range_s(str, res, base, 0, UINT64_MAX);
}

#endif // STRTOINT_IMPLEMENTATION
