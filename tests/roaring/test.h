#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#define DESCRIBE_TEST \
    do {              \
    } while (0)

#define DEFINE_TEST(name) TEST(FERMAT_ROARING_TEST_SUITE, name)

#define assert_true(x) EXPECT_TRUE(x)
#define assert_false(x) EXPECT_FALSE(x)
#define assert_int_equal(a, b) EXPECT_EQ((a), (b))
#define assert_int_not_equal(a, b) EXPECT_NE((a), (b))
#define assert_ptr_equal(a, b) EXPECT_TRUE((a) == (b))
#define assert_ptr_not_equal(a, b) EXPECT_TRUE((a) != (b))
#define assert_null(p) EXPECT_TRUE((p) == nullptr)
#define assert_non_null(p) EXPECT_TRUE((p) != nullptr)
#define assert_string_equal(a, b) EXPECT_STREQ((a), (b))
#define assert_memory_equal(a, b, n) EXPECT_EQ(memcmp((a), (b), (n)), 0)
#define assert_in_range(x, minv, maxv) \
    do {                               \
        EXPECT_GE((x), (minv));        \
        EXPECT_LE((x), (maxv));        \
    } while (0)
#define assert_uint_in_range(x, minv, maxv) assert_in_range(x, minv, maxv)

inline std::string roaring_fail_fmt(const char* fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return std::string(buf);
}

#define fail_msg(...) ADD_FAILURE() << roaring_fail_fmt(__VA_ARGS__)

#define assert_bitmap_validate(b)                                            \
    do {                                                                     \
        const char* internal_reason_buf = nullptr;                           \
        if (!roaring_bitmap_internal_validate((b), &internal_reason_buf)) {  \
            ADD_FAILURE() << "internal validation failed: "                  \
                          << internal_reason_buf;                            \
        }                                                                    \
    } while (0)
