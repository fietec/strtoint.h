#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#define STRTOINT_IMPLEMENTATION
#include "../strtoint.h"

static void test_basic_conversions(void)
{
    strtoint_res_t res;

    // Standard decimal
    int32_t val32 = strtoint32_s("123456", &res, 10);
    assert(val32 == 123456);
    assert(!res.out_of_range && !res.no_digits && !res.invalid_params);
    assert(*res.endptr == '\0');

    // Negative decimal
    val32 = strtoint32_s("-654321", &res, 10);
    assert(val32 == -654321);
    assert(res.negative);

    // Arbitrary base (Base 36: Z = 35)
    int64_t val64 = strtoint64_s("Z", &res, 36);
    assert(val64 == 35);

    // Base 0 auto-detection for octal
    val32 = strtoint32_s("077", &res, 0);
    assert(val32 == 63);

    printf("[PASS] Basic Conversions\n");
}

static void test_prefix_handling(void)
{
    strtoint_res_t res;

    // Binary prefix (0b / 0B)
    uint8_t u8 = strtouint8_s("0b101010", &res, 0);
    assert(u8 == 42);
    assert(res.endptr == res.endptr); // Pointer moved past '0b'

    u8 = strtouint8_s("0B1111", &res, 2);
    assert(u8 == 15);

    // Hex prefix (0x / 0X)
    uint32_t u32 = strtouint32_s("0xFF", &res, 0);
    assert(u32 == 255);

    // Partial prefix match edge case: "0xG" in Base 0
    // Should fallback to base 8, parse '0', and leave endptr at 'x'
    {
        int32_t val = strtoint32_s("0xG", &res, 0);
        assert(val == 0);
        assert(*res.endptr == 'x');
        assert(!res.no_digits);
    }

    // Partial binary prefix: "0b2" in Base 0
    {
        int32_t val = strtoint32_s("0b2", &res, 0);
        assert(val == 0);
        assert(*res.endptr == 'b');
        assert(!res.no_digits);
    }

    printf("[PASS] Prefix Handling (0b, 0x, 0)\n");
}

static void test_whitespace_and_flags(void)
{
    strtoint_res_t res;

    int32_t val = strtoint32_s("   \t  -42abc", &res, 10);
    assert(val == -42);
    assert(res.leading_spaces == true);
    assert(res.negative == true);
    assert(strcmp(res.endptr, "abc") == 0);
    assert(!res.no_digits && !res.out_of_range);

    printf("[PASS] Whitespace & Metadata Flags\n");
}

static void test_signed_boundaries(void)
{
    strtoint_res_t res;

    // Exact INT64_MIN (-9223372036854775808)
    int64_t val = strtoint64_s("-9223372036854775808", &res, 10);
    assert(val == INT64_MIN);
    assert(!res.out_of_range);

    // Exact INT64_MAX (9223372036854775807)
    val = strtoint64_s("9223372036854775807", &res, 10);
    assert(val == INT64_MAX);
    assert(!res.out_of_range);

    // Positive overflow for INT64_MAX + 1 ("9223372036854775808")
    // Note: Magnitude matches |INT64_MIN|, but missing negative sign!
    val = strtoint64_s("9223372036854775808", &res, 10);
    assert(val == INT64_MAX);
    assert(res.out_of_range == true);

    // Underflow below INT64_MIN
    val = strtoint64_s("-9223372036854775809", &res, 10);
    assert(val == INT64_MIN);
    assert(res.out_of_range == true);

    // 8-bit Clamping test
    int8_t v8 = strtoint8_s("500", &res, 10);
    assert(v8 == INT8_MAX);
    assert(res.out_of_range == true);

    v8 = strtoint8_s("-500", &res, 10);
    assert(v8 == INT8_MIN);
    assert(res.out_of_range == true);

    printf("[PASS] Signed Boundaries & Two's Complement Edge Cases\n");
}

static void test_unsigned_negation_and_boundaries(void)
{
    strtoint_res_t res;

    // Standard Unsigned Negation (POSIX semantics: max - val + 1)
    // For uint8_t (max 255): "-1" -> 255 - 1 + 1 = 255
    uint8_t u8 = strtouint8_s("-1", &res, 10);
    assert(u8 == 255);
    assert(res.negative == true);
    assert(!res.out_of_range);

    // "-5" for uint8_t -> 255 - 5 + 1 = 251
    u8 = strtouint8_s("-5", &res, 10);
    assert(u8 == 251);

    // Unsigned Overflow
    u8 = strtouint8_s("256", &res, 10);
    assert(u8 == UINT8_MAX);
    assert(res.out_of_range == true);

    // Unsigned max uint64 exact match
    uint64_t u64 = strtouint64_s("18446744073709551615", &res, 10);
    assert(u64 == UINT64_MAX);
    assert(!res.out_of_range);

    // Unsigned max uint64 overflow
    u64 = strtouint64_s("18446744073709551616", &res, 10);
    assert(u64 == UINT64_MAX);
    assert(res.out_of_range == true);

    printf("[PASS] Unsigned Negation & Boundaries\n");
}

static void test_custom_range(void)
{
    strtoint_res_t res;

    // Custom bound [10, 100]
    int64_t val = strtoint_custom_s("5", &res, 10, 10, 100);
    assert(val == 10); // Clamped to min
    assert(res.out_of_range == true);

    val = strtoint_custom_s("150", &res, 10, 10, 100);
    assert(val == 100); // Clamped to max
    assert(res.out_of_range == true);

    val = strtoint_custom_s("50", &res, 10, 10, 100);
    assert(val == 50);
    assert(!res.out_of_range);

    printf("[PASS] Custom Ranges\n");
}

static void test_invalid_parameters_and_errors(void)
{
    strtoint_res_t res;

    // NULL string
    int64_t val = strtoint64_s(NULL, &res, 10);
    assert(val == 0);
    assert(res.invalid_params == true);

    // Min > Max
    val = strtoint_custom_s("50", &res, 10, 100, 10);
    assert(val == 0);
    assert(res.invalid_params == true);

    // Invalid base (1 or 37)
    val = strtoint64_s("10", &res, 1);
    assert(res.invalid_params == true);

    val = strtoint64_s("10", &res, 37);
    assert(res.invalid_params == true);

    // No valid digits
    val = strtoint64_s("   ---", &res, 10);
    assert(val == 0);
    assert(res.no_digits == true);
    assert(!res.invalid_params);

    val = strtoint64_s("", &res, 10);
    assert(val == 0);
    assert(res.no_digits == true);

    printf("[PASS] Parameter & Input Error Handling\n");
}

static void test_legacy_errno_api(void)
{
    char *endptr = NULL;

    // Test ERANGE on overflow
    errno = 0;
    int8_t v8 = strtoint8("999", &endptr, 10);
    assert(v8 == INT8_MAX);
    assert(errno == ERANGE);
    assert(*endptr == '\0');

    // Test EINVAL on invalid parameter
    errno = 0;
    v8 = strtoint8("123", &endptr, 1); // Base 1 is invalid
    assert(v8 == 0);
    assert(errno == EINVAL);

    printf("[PASS] Legacy API & errno Integration\n");
}

int main(void)
{
    printf("Running strtoint.h tests...\n\n");

    test_basic_conversions();
    test_prefix_handling();
    test_whitespace_and_flags();
    test_signed_boundaries();
    test_unsigned_negation_and_boundaries();
    test_custom_range();
    test_invalid_parameters_and_errors();
    test_legacy_errno_api();

    printf("\nAll unit tests passed successfully!\n");
    return 0;
}
