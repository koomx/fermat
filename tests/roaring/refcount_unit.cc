#include <stdint.h>

#include <fermat/roaring/portability.h>

#define FERMAT_ROARING_TEST_SUITE Refcount
#include "test.h"

DEFINE_TEST(test_non_atomic_refcount) {
    croaring_refcount_t refcount = 2;

    assert_false(croaring_refcount_dec(&refcount));
    assert_int_equal(croaring_refcount_get(&refcount), 1);
    assert_true(croaring_refcount_dec(&refcount));
    assert_int_equal(croaring_refcount_get(&refcount), 0);
}
