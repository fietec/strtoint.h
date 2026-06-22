/* strtoint.h - v1.0.0 - This library is in the public domain. */

#ifndef STRTOINT_H
#define STRTOINT_H

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

int8_t  strtoint8 (const char *str, char **endptr, int base);
int16_t strtoint16(const char *str, char **endptr, int base);
int32_t strtoint32(const char *str, char **endptr, int base);
int64_t strtoint64(const char *str, char **endptr, int base);

uint8_t  strtouint8 (const char *str, char **endptr, int base);
uint16_t strtouint16(const char *str, char **endptr, int base);
uint32_t strtouint32(const char *str, char **endptr, int base);
uint64_t strtouint64(const char *str, char **endptr, int base);

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

int8_t strtoint8(const char *str, char **endptr, int base)
{
    int64_t result = strtoint64(str, endptr, base);
    if (result < INT8_MIN){
        errno = ERANGE;
        return INT8_MIN;
    }
    if (result > INT8_MAX){
        errno = ERANGE;
        return INT8_MAX;
    }
    return (int8_t) result;
}

int16_t strtoint16(const char *str, char **endptr, int base)
{
    int64_t result = strtoint64(str, endptr, base);
    if (result < INT16_MIN){
        errno = ERANGE;
        return INT16_MIN;
    }
    if (result > INT16_MAX){
        errno = ERANGE;
        return INT16_MAX;
    }
    return (int16_t) result;
}

int32_t strtoint32(const char *str, char **endptr, int base)
{
    int64_t result = strtoint64(str, endptr, base);
    if (result < INT32_MIN){
        errno = ERANGE;
        return INT32_MIN;
    }
    if (result > INT32_MAX){
        errno = ERANGE;
        return INT32_MAX;
    }
    return (int32_t) result;
}

int64_t strtoint64(const char *str, char **endptr, int base)
{
    if (base < 0 || base == 1 || base > 36){
        errno = EINVAL;
        if (endptr) *endptr = (char*)str;
        return 0;
    }
    int64_t result = 0;
    const char *start = str;
    while (isspace((unsigned char)*start)) start++;

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
        if (endptr) *endptr = (char*) str;
        return 0;
    }

    const char *pdigit = start;
    while (pdigit < end){
        int64_t digit = strtoint__digit_value(*pdigit++, base);
        if (result < (INT64_MIN + digit)/base) goto range_error;
        result = result * base - digit;
    }

    if (!negative){
        if (result < -INT64_MAX) goto range_error;
        result = -result;
    }

    if (endptr) *endptr = (char*)end;
    return result;

range_error:
    errno = ERANGE;
    if (endptr) *endptr = (char*)end;
    return (negative ? INT64_MIN : INT64_MAX);
}

uint8_t strtouint8(const char *str, char **endptr, int base)
{
    uint64_t value = strtouint64(str, endptr, base);
    uint8_t result = (uint8_t) value;
    if ((uint64_t) result != value){
        errno = ERANGE;
        return UINT8_MAX;
    }
    return result;
}

uint16_t strtouint16(const char *str, char **endptr, int base)
{
    uint64_t value = strtouint64(str, endptr, base);
    uint16_t result = (uint16_t) value;
    if ((uint64_t) result != value){
        errno = ERANGE;
        return UINT16_MAX;
    }
    return result;
}

uint32_t strtouint32(const char *str, char **endptr, int base)
{
    uint64_t value = strtouint64(str, endptr, base);
    uint32_t result = (uint32_t) value;
    if ((uint64_t) result != value){
        errno = ERANGE;
        return UINT32_MAX;
    }
    return result;
}

uint64_t strtouint64(const char *str, char **endptr, int base)
{
    if (base < 0 || base == 1 || base > 36){
        errno = EINVAL;
        if (endptr) *endptr = (char*)str;
        return 0;
    }
    uint64_t result = 0;
    const char *start = str;
    while (isspace((unsigned char)*start)) start++;

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
        if (endptr) *endptr = (char*) str;
        return 0;
    }

    const char *pdigit = start;
    while (pdigit < end){
        uint64_t digit = strtoint__digit_value(*pdigit++, base);
        if (result > (UINT64_MAX - digit) / base) goto range_error;
        result = result * base + digit;
    }

    if (negative) {
        result = -result;
    }

    if (endptr) *endptr = (char*)end;
    return result;

range_error:
    errno = ERANGE;
    if (endptr) *endptr = (char*)end;
    return UINT64_MAX;
}
#endif // STRTOINT_IMPLEMENTATION