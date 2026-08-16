#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <fermat/roaring/bitset/bitset.h>

#define FERMAT_ROARING_TEST_SUITE CbitsetUnit
#include "test.h"

int compute_cardinality(fermat::roaring::bitset_t *b) {
    size_t k = 0;
    for (size_t i = 0; bitset_next_set_bit(b, &i); i++) {
        k += 1;
    }
    return k;
}

DEFINE_TEST(test_iterate) {
    fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
    for (int k = 0; k < 1000; ++k) bitset_set(b, 3 * k);
    assert_true(bitset_count(b) == 1000);
    assert_true(compute_cardinality(b) == 1000);
    size_t k = 0;
    for (size_t i = 0; bitset_next_set_bit(b, &i); i++) {
        assert_true(i == k);
        k += 3;
    }
    assert_true(k == 3000);
    bitset_free(b);
}

DEFINE_TEST(test_next_bits_iterate) {
    fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
    for (int i = 0; i < 100; i++) bitset_set(b, i);
    for (int i = 1000; i < 1100; i += 2) bitset_set(b, i);

    // Use an odd, small buffer size
    size_t buffer[3];
    size_t howmany = 0;
    size_t i = 0;
    for (size_t startfrom = 0;
         (howmany = bitset_next_set_bits(
              b, buffer, sizeof(buffer) / sizeof(buffer[0]), &startfrom)) > 0;
         startfrom++) {
        for (size_t j = 0; j < howmany; j++) {
            size_t expected;
            if (i < 100) {
                expected = i;
            } else {
                expected = 1000 + 2 * (i - 100);
            }
            assert_int_equal(buffer[j], expected);
            ++i;
        }
    }
    assert_int_equal(i, 150);
    bitset_free(b);
}

bool increment(size_t value, void *param) {
    size_t k;
    memcpy(&k, param, sizeof(size_t));
    assert_true(value == k);
    k += 3;
    memcpy(param, &k, sizeof(size_t));
    return true;
}

DEFINE_TEST(test_iterate2) {
    fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
    for (int k = 0; k < 1000; ++k) bitset_set(b, 3 * k);
    assert_true(compute_cardinality(b) == 1000);
    assert_true(bitset_count(b) == 1000);
    size_t k = 0;
    bitset_for_each(b, increment, &k);
    assert_true(k == 3000);
    bitset_free(b);
}

DEFINE_TEST(test_construct) {
   fermat::roaring:: bitset_t *b = fermat::roaring::bitset_create();
    for (int k = 0; k < 1000; ++k) bitset_set(b, 3 * k);
    assert_true(compute_cardinality(b) == 1000);
    assert_true(bitset_count(b) == 1000);
    for (int k = 0; k < 3 * 1000; ++k)
        assert_true(bitset_get(b, k) == (k / 3 * 3 == k));
    fermat::roaring::bitset_free(b);
}

DEFINE_TEST(test_max_min) {
    fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
    assert_true(bitset_empty(b));
    for (size_t k = 100; k < 1000; ++k) {
        bitset_set(b, 3 * k);
        assert_true(bitset_minimum(b) == 3 * 100);
        assert_true(bitset_maximum(b) == 3 * k);
    }
    fermat::roaring::bitset_free(b);
}

DEFINE_TEST(test_shift_left) {
    for (size_t sh = 0; sh < 256; sh++) {
        fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
        int power = 3;
        size_t s1 = 100;
        size_t s2 = 5000;
        for (size_t k = s1; k < s2; ++k) {
            bitset_set(b, power * k);
        }
        int mycount = bitset_count(b);
        assert_true(compute_cardinality(b) == mycount);
        bitset_shift_left(b, sh);
        assert_true(bitset_count(b) == (size_t)mycount);
        assert_true(compute_cardinality(b) == mycount);
        for (size_t k = s1; k < s2; ++k) {
            assert_true(bitset_get(b, power * k + sh));
        }
        bitset_free(b);
    }
}

DEFINE_TEST(test_set_to_val) {
    fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
    fermat::roaring::bitset_set_to_value(b, 1, true);
    fermat::roaring::bitset_set_to_value(b, 1, false);
    fermat::roaring::bitset_set_to_value(b, 10, false);
    fermat::roaring::bitset_set_to_value(b, 10, true);
    assert_true(fermat::roaring::bitset_get(b, 10));
    assert_true(!fermat::roaring::bitset_get(b, 1));
    fermat::roaring::bitset_free(b);
}

DEFINE_TEST(test_shift_right) {
    for (size_t sh = 0; sh < 256; sh++) {
        fermat::roaring::bitset_t *b = fermat::roaring::bitset_create();
        int power = 3;
        size_t s1 = 100 + sh;
        size_t s2 = s1 + 5000;
        for (size_t k = s1; k < s2; ++k) {
            bitset_set(b, power * k);
        }
        size_t mycount = bitset_count(b);
        fermat::roaring::bitset_shift_right(b, sh);
        assert_true(fermat::roaring::bitset_count(b) == mycount);
        for (size_t k = s1; k < s2; ++k) {
            assert_true(fermat::roaring::bitset_get(b, power * k - sh));
        }
        fermat::roaring::bitset_free(b);
    }
}

DEFINE_TEST(test_union_intersection) {
    fermat::roaring::bitset_t *b1 = fermat::roaring::bitset_create();
    fermat::roaring::bitset_t *b2 = fermat::roaring::bitset_create();

    for (int k = 0; k < 1000; ++k) {
        fermat::roaring::bitset_set(b1, 2 * k);
        fermat::roaring::bitset_set(b2, 2 * k + 1);
    }
    // calling xor twice should leave things unchanged
   fermat::roaring::bitset_inplace_symmetric_difference(b1, b2);
    assert_true(fermat::roaring::bitset_count(b1) == 2000);
    fermat::roaring::bitset_inplace_symmetric_difference(b1, b2);
    assert_true(fermat::roaring::bitset_count(b1) == 1000);
    fermat::roaring::bitset_inplace_difference(b1, b2);  // should make no difference
    assert_true(fermat::roaring::bitset_count(b1) == 1000);
    fermat::roaring::bitset_inplace_union(b1, b2);
    assert_true(bitset_count(b1) == 2000);
    fermat::roaring::bitset_inplace_intersection(b1, b2);
    assert_true(fermat::roaring::bitset_count(b1) == 1000);
    fermat::roaring::bitset_inplace_difference(b1, b2);
    assert_true(fermat::roaring::bitset_count(b1) == 0);
    fermat::roaring::bitset_inplace_union(b1, b2);
    fermat::roaring::bitset_inplace_difference(b2, b1);
    assert_true(fermat::roaring::bitset_count(b2) == 0);
    fermat::roaring::bitset_free(b1);
    fermat::roaring::bitset_free(b2);
}

DEFINE_TEST(test_counts) {
    fermat::roaring::bitset_t *b1 = fermat::roaring::bitset_create();
    fermat::roaring::bitset_t *b2 = fermat::roaring::bitset_create();

    for (int k = 0; k < 1000; ++k) {
        fermat::roaring::bitset_set(b1, 2 * k);
        fermat::roaring::bitset_set(b2, 3 * k);
    }
    assert_true(fermat::roaring::bitset_intersection_count(b1, b2) == 334);
    assert_true(fermat::roaring::bitset_union_count(b1, b2) == 1666);
    fermat::roaring::bitset_free(b1);
    fermat::roaring::bitset_free(b2);
}

/* Creates 2 bitsets, one containing even numbers the other odds.
Checks bitsets_disjoint() returns that they are disjoint, then sets a common
bit between both sets and checks that they are no longer disjoint. */
DEFINE_TEST(test_disjoint) {
    fermat::roaring::bitset_t *evens = fermat::roaring::bitset_create();
   fermat::roaring:: bitset_t *odds = fermat::roaring::bitset_create();

    for (int i = 0; i < 1000; i++) {
        if (i % 2 == 0)
            fermat::roaring::bitset_set(evens, i);
        else
            fermat::roaring::bitset_set(odds, i);
    }

    assert_true(bitsets_disjoint(evens, odds));

    fermat::roaring::bitset_set(evens, 501);
    fermat::roaring::bitset_set(odds, 501);

    assert_true(!fermat::roaring::bitsets_disjoint(evens, odds));

    fermat::roaring::bitset_free(evens);
    fermat::roaring::bitset_free(odds);
}

/* Creates 2 bitsets, one containing even numbers the other odds.
Checks that bitsets_intersect() returns that they do not intersect, then sets
a common bit and checks that they now intersect. */
DEFINE_TEST(test_intersects) {
    fermat::roaring::bitset_t *evens = fermat::roaring::bitset_create();
    fermat::roaring::bitset_t *odds = fermat::roaring::bitset_create();

    for (int i = 0; i < 1000; i++) {
        if (i % 2 == 0)
            fermat::roaring::bitset_set(evens, i);
        else
            fermat::roaring::bitset_set(odds, i);
    }

    assert_true(!fermat::roaring::bitsets_intersect(evens, odds));

    fermat::roaring::bitset_set(evens, 1001);
    fermat::roaring::bitset_set(odds, 1001);

    assert_true(fermat::roaring::bitsets_intersect(evens, odds));

    fermat::roaring::bitset_free(evens);
    fermat::roaring::bitset_free(odds);
}
/* Create 2 bitsets with different capacity, where the bigger superset
contains the subset bits plus additional bits after the subset arraysize.
Checks that the bitset_contains_all() returns false when checking if
the superset contains all the subset bits, and true in the opposite case. */
DEFINE_TEST(test_contains_all_different_sizes) {
    const size_t superset_size = 10;
    const size_t subset_size = 5;

    fermat::roaring::bitset_t *superset = fermat::roaring::bitset_create_with_capacity(superset_size);
    fermat::roaring::bitset_t *subset = fermat::roaring::bitset_create_with_capacity(subset_size);

    fermat::roaring::bitset_set(superset, 1);
    fermat::roaring::bitset_set(superset, subset_size - 1);
    fermat::roaring::bitset_set(superset, subset_size + 1);

    fermat::roaring::bitset_set(subset, 1);
    fermat::roaring::bitset_set(subset, subset_size - 1);

    assert_true(fermat::roaring::bitset_contains_all(superset, subset));
    assert_true(!fermat::roaring::bitset_contains_all(subset, superset));

    fermat::roaring::bitset_free(superset);
    fermat::roaring::bitset_free(subset);
}

/* Creates 2 bitsets, one with all bits from 0->1000 set, the other with only
even bits set in the same range. Checks that the bitset_contains_all()
returns true, then sets a single bit at 1001 in the prior subset and checks that
bitset_contains_all() returns false. */
DEFINE_TEST(test_contains_all) {
    fermat::roaring::bitset_t *superset = fermat::roaring::bitset_create();
    fermat::roaring::bitset_t *subset = fermat::roaring::bitset_create();

    for (int i = 0; i < 1000; i++) {
        fermat::roaring::bitset_set(superset, i);
        if (i % 2 == 0) fermat::roaring::bitset_set(subset, i);
    }

    assert_true(fermat::roaring::bitset_contains_all(superset, subset));
    assert_true(!fermat::roaring::bitset_contains_all(subset, superset));

    fermat::roaring::bitset_set(subset, 1001);

    assert_true(!fermat::roaring::bitset_contains_all(superset, subset));
    assert_true(!fermat::roaring::bitset_contains_all(subset, superset));

    fermat::roaring::bitset_free(superset);
    fermat::roaring::bitset_free(subset);
}
