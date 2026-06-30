/* strtoint.h - v1.2.0 - This library is in the public domain. */

#ifndef STRTOINT_H
#define STRTOINT_H

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct strtoint_res_t {
    char *endptr;        // pointer to the first unparsed character
    bool negative;       // input had a leading '-' sign
    bool leading_spaces; // input had leading whitespaces

    // error flags
    bool invalid_base;   // supplied base was invalid
    bool no_digits;      // no valid digits were found
    bool out_of_range;   // value exceeded the representable range of target type
};

int8_t  strtoint8 (const char *str, char **endptr, int base);
int16_t strtoint16(const char *str, char **endptr, int base);
int32_t strtoint32(const char *str, char **endptr, int base);
int64_t strtoint64(const char *str, char **endptr, int base);
int8_t  strtoint8_s (const char *str, struct strtoint_res_t *res, int base);
int16_t strtoint16_s(const char *str, struct strtoint_res_t *res, int base);
int32_t strtoint32_s(const char *str, struct strtoint_res_t *res, int base);
int64_t strtoint64_s(const char *str, struct strtoint_res_t *res, int base);

uint8_t  strtouint8 (const char *str, char **endptr, int base);
uint16_t strtouint16(const char *str, char **endptr, int base);
uint32_t strtouint32(const char *str, char **endptr, int base);
uint64_t strtouint64(const char *str, char **endptr, int base);
uint8_t  strtouint8_s (const char *str, struct strtoint_res_t *res, int base);
uint16_t strtouint16_s(const char *str, struct strtoint_res_t *res, int base);
uint32_t strtouint32_s(const char *str, struct strtoint_res_t *res, int base);
uint64_t strtouint64_s(const char *str, struct strtoint_res_t *res, int base);

#ifdef __cplusplus
}
#endif

#endif // STRTOINT_H

#ifdef STRTOINT_IMPLEMENTATION

static inline int strtoint__digit_value(char c, int base)
{
    int value = -1;
    if ('0' <= c && c <= '9') value = c - '0';
    if ('A' <= c && c <= 'Z') value = c - 'A' + 10;
    if ('a' <= c && c <= 'z') value = c - 'a' + 10;
    if (value < base || base == 0) return value;
    return -1;
}

static inline bool strtoint__str_starts_with_case(const char *s1, const char *s2)
{
    if (!s2 || *s2 == '\0') return true;
    if (!s1) return false;
    while (*s2) {
        unsigned char c1 = (unsigned char)tolower((unsigned char)*s1);
        unsigned char c2 = (unsigned char)tolower((unsigned char)*s2);
        if (c1 != c2) return false;
        s1++;
        s2++;
    }
    return true;
}

int64_t strtoint__parse_s(const char *str, struct strtoint_res_t *res, int base, int64_t min, int64_t max)
{
    if (res) *res = (struct strtoint_res_t){0};
    if (base < 0 || base == 1 || base > 36){
        if (res){
            res->invalid_base = true;
            res->endptr = (char*)str;
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
    if ((base == 0 || base == 2) && strtoint__str_starts_with_case(start, "0b")){
        if (strtoint__digit_value(start[2], 2) != -1) {
            base = 2;
            start += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 16) && strtoint__str_starts_with_case(start, "0x")){
        if (strtoint__digit_value(start[2], 16) != -1) {
            base = 16;
            start += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 8) && *start == '0'){
        base = 8;
    } else if (base == 0){
        base = 10;
    }

    const char *end = start;
    while (strtoint__digit_value(*end, base) != -1) end++;

    if (end == start){
        if (res){
            res->endptr = (char*) str;
            res->no_digits = true;
        }
        return 0;
    }

    const char *pdigit = start;
    while (pdigit < end){
        int64_t digit = strtoint__digit_value(*pdigit++, base);
        if (result < (INT64_MIN + digit)/base) goto range_error;
        result = result * base - digit;
    }

    if (!negative){
        if (max < 0 || result < -max) goto range_error;
        result = -result;
    } else if (result < min) goto range_error;

    if (res){
        res->endptr = (char*)end;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
    }
    return result;

range_error:
    if (res){
        res->out_of_range = true;
        res->endptr = (char*) end;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
    }
    return (negative ? min : max);
}

int64_t strtoint__parse(const char *str, char **endptr, int base, int64_t min, int64_t max)
{
    struct strtoint_res_t res;
    int64_t result = strtoint__parse_s(str, &res, base, min, max);
    if (res.invalid_base) errno = EINVAL;
    if (res.out_of_range) errno = ERANGE;
    if (endptr) *endptr = res.endptr;
    return result;
}

int8_t strtoint8(const char *str, char **endptr, int base)
{
    return strtoint__parse(str, endptr, base, INT8_MIN, INT8_MAX);
}

int8_t strtoint8_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint__parse_s(str, res, base, INT8_MIN, INT8_MAX);
}

int16_t strtoint16(const char *str, char **endptr, int base)
{
    return strtoint__parse(str, endptr, base,INT16_MIN, INT16_MAX);
}

int16_t strtoint16_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint__parse_s(str, res, base, INT16_MIN, INT16_MAX);
}

int32_t strtoint32(const char *str, char **endptr, int base)
{
    return strtoint__parse(str, endptr, base, INT32_MIN, INT32_MAX);
}

int32_t strtoint32_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint__parse_s(str, res, base, INT32_MIN, INT32_MAX);
}

int64_t strtoint64(const char *str, char **endptr, int base)
{
    return strtoint__parse(str, endptr, base, INT64_MIN, INT64_MAX);
}

int64_t strtoint64_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtoint__parse_s(str, res, base, INT64_MIN, INT64_MAX);
}

uint64_t strtouint__parse_s(const char *str, struct strtoint_res_t *res, int base, uint64_t max)
{
    if (res) *res = (struct strtoint_res_t){0};
    if (base < 0 || base == 1 || base > 36){
        if (res){
            res->endptr = (char*) str;
            res->invalid_base = true;
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
    if ((base == 0 || base == 2) && strtoint__str_starts_with_case(start, "0b")){
        if (strtoint__digit_value(start[2], 2) != -1) {
            base = 2;
            start += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 16) && strtoint__str_starts_with_case(start, "0x")){
        if (strtoint__digit_value(start[2], 16) != -1) {
            base = 16;
            start += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if ((base == 0 || base == 8) && *start == '0'){
        base = 8;
    } else if (base == 0){
        base = 10;
    }

    const char *end = start;
    while (strtoint__digit_value(*end, base) != -1) end++;

    if (end == start){
        if (res){
            res->endptr = (char*) str;
            res->no_digits = true;
        }
        return 0;
    }

    const char *pdigit = start;
    while (pdigit < end){
        uint64_t digit = strtoint__digit_value(*pdigit++, base);
        if (result > (UINT64_MAX - digit) / base) goto range_error;
        result = result * base + digit;
    }

    if (result > max) goto range_error;
    if (negative) {
        result = -result;
    }

    if (res){
        res->endptr = (char*)end;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
    }
    return result;

range_error:
    if (res){
        res->endptr = (char*) end;
        res->out_of_range = true;
        res->negative = negative;
        res->leading_spaces = leading_spaces;
    }
    return max;
}

uint64_t strtouint__parse(const char *str, char **endptr, int base, uint64_t max)
{
    struct strtoint_res_t res;
    uint64_t result = strtouint__parse_s(str, &res, base, max);
    if (res.invalid_base) errno = EINVAL;
    if (res.out_of_range) errno = ERANGE;
    if (endptr) *endptr = res.endptr;
    return result;
}

uint8_t strtouint8(const char *str, char **endptr, int base)
{
    return strtouint__parse(str, endptr, base, UINT8_MAX);
}

uint8_t strtouint8_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint__parse_s(str, res, base, UINT8_MAX);
}

uint16_t strtouint16(const char *str, char **endptr, int base)
{
    return strtouint__parse(str, endptr, base, UINT16_MAX);
}

uint16_t strtouint16_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint__parse_s(str, res, base, UINT16_MAX);
}

uint32_t strtouint32(const char *str, char **endptr, int base)
{
    return strtouint__parse(str, endptr, base, UINT32_MAX);
}

uint32_t strtouint32_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint__parse_s(str, res, base, UINT32_MAX);
}

uint64_t strtouint64(const char *str, char **endptr, int base)
{
    return strtouint__parse(str, endptr, base, UINT64_MAX);
}

uint64_t strtouint64_s(const char *str, struct strtoint_res_t *res, int base)
{
    return strtouint__parse_s(str, res, base, UINT64_MAX);
}

#endif // STRTOINT_IMPLEMENTATION
