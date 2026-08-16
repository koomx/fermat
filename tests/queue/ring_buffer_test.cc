// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fermat/queue/ring_buffer.h>
#include <vector>
#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <list>

namespace {
    // ============================================================================
    // TestObject – tracks construction/destruction counts (for non‑trivial types)
    // ============================================================================
    struct TestObject {
        static std::atomic<size_t> ctor_count;
        static std::atomic<size_t> dtor_count;
        static std::atomic<size_t> copy_ctor_count;
        static std::atomic<size_t> move_ctor_count;
        static std::atomic<size_t> copy_assign_count;
        static std::atomic<size_t> move_assign_count;

        int value;

        TestObject(int v = 0)
            : value(v) { ++ctor_count; }
        TestObject(const TestObject& other)
            : value(other.value) { ++copy_ctor_count; }
        TestObject(TestObject&& other) noexcept
            : value(other.value) { ++move_ctor_count; }
        ~TestObject() { ++dtor_count; }

        TestObject& operator=(const TestObject& other) {
            if (this != &other) {
                value = other.value;
                ++copy_assign_count;
            }
            return *this;
        }

        TestObject& operator=(TestObject&& other) noexcept {
            if (this != &other) {
                value = other.value;
                ++move_assign_count;
            }
            return *this;
        }

        static void Reset() {
            ctor_count = 0;
            dtor_count = 0;
            copy_ctor_count = 0;
            move_ctor_count = 0;
            copy_assign_count = 0;
            move_assign_count = 0;
        }
    };

    std::atomic<size_t> TestObject::ctor_count { 0 };
    std::atomic<size_t> TestObject::dtor_count { 0 };
    std::atomic<size_t> TestObject::copy_ctor_count { 0 };
    std::atomic<size_t> TestObject::move_ctor_count { 0 };
    std::atomic<size_t> TestObject::copy_assign_count { 0 };
    std::atomic<size_t> TestObject::move_assign_count { 0 };

    /// Verifies that the sequence [first, last) equals the given values in order.
    /// @tparam Iter iterator type
    /// @tparam Values types of the expected values
    /// @param first start of the sequence
    /// @param last end of the sequence
    /// @param expected the expected values (must match the number of elements)
    /// @return true if the sequence matches exactly the expected values.
    template <typename Iter, typename... Values>
    bool VerifySequence(Iter first, Iter last, Values... expected) {
        auto it = first;
        bool ok = true;
        auto check = [&](const auto& val) {
            if (ok && it != last && *it == val) {
                ++it;
            } else {
                ok = false;
            }
        };
        (check(expected), ...);
        return ok && it == last;
    }

    // ============================================================================
    // Tests for RingBuffer with default container (std::vector)
    // ============================================================================

    TEST(RingBufferTest, DefaultConstructor) {
        fermat::RingBuffer<int> rb;
        EXPECT_EQ(rb.size(), 0u);
        EXPECT_EQ(rb.capacity(), 0u);
        EXPECT_TRUE(rb.empty());
        EXPECT_TRUE(rb.full()); // capacity 0 => full?
        // For capacity 0, the underlying container has size 1 (sentinel).
        EXPECT_EQ(rb.get_container().size(), 1u);
        EXPECT_EQ(rb.begin(), rb.end());
    }

    TEST(RingBufferTest, CapacityConstructor) {
        fermat::RingBuffer<int> rb(10);
        EXPECT_EQ(rb.capacity(), 10u);
        EXPECT_EQ(rb.size(), 0u);
        EXPECT_TRUE(rb.empty());
        EXPECT_FALSE(rb.full());
        EXPECT_EQ(rb.get_container().size(), 11u);
    }

    TEST(RingBufferTest, ConstructWithAllocator) {
        fermat::RingBuffer<int, std::vector<int>> rb(5);
        EXPECT_EQ(rb.capacity(), 5u);
        EXPECT_TRUE(rb.empty());
    }

    TEST(RingBufferTest, ConstructWithContainer) {
        std::vector<int> vec(10, 42);
        fermat::RingBuffer<int, std::vector<int>> rb(vec);
        // The container is copied, but the ring buffer is empty.
        EXPECT_EQ(rb.capacity(), 9u); // vec.size() = 10, minus sentinel = 9
        EXPECT_EQ(rb.size(), 0u);
        EXPECT_TRUE(rb.empty());
        // The underlying container should have been copied.
        EXPECT_EQ(rb.get_container().size(), 10u);
        for (size_t i = 0; i < rb.get_container().size(); ++i) {
            EXPECT_EQ(rb.get_container()[i], 42);
        }
    }

    TEST(RingBufferTest, CopyConstructor) {
        fermat::RingBuffer<int> rb1(10);
        for (int i = 0; i < 5; ++i)
            rb1.push_back(i);
        fermat::RingBuffer<int> rb2(rb1);
        EXPECT_EQ(rb1.size(), rb2.size());
        EXPECT_EQ(rb1.capacity(), rb2.capacity());
        EXPECT_TRUE(std::equal(rb1.begin(), rb1.end(), rb2.begin()));
        // The underlying containers should be equal.
        EXPECT_EQ(rb1.get_container().size(), rb2.get_container().size());
    }

    TEST(RingBufferTest, MoveConstructor) {
        fermat::RingBuffer<int> rb1(10);
        for (int i = 0; i < 5; ++i)
            rb1.push_back(i);
        auto* underlying_data = &rb1.get_container();
        fermat::RingBuffer<int> rb2(std::move(rb1));
        EXPECT_EQ(rb2.size(), 5u);
        EXPECT_EQ(rb2.capacity(), 10u);
        // rb1 is now in a valid but unspecified state; we check it's empty and destructible.
        EXPECT_TRUE(rb1.empty());
        // Underlying container may have been swapped.
    }

    TEST(RingBufferTest, InitializerListConstructor) {
        fermat::RingBuffer<int> rb = { 1, 2, 3, 4, 5 };
        EXPECT_EQ(rb.size(), 5u);
        EXPECT_EQ(rb.capacity(), 5u);
        /// initializer list"
        EXPECT_TRUE(VerifySequence(rb.begin(), rb.end(), 1, 2, 3, 4, 5));
    }

    TEST(RingBufferTest, CopyAssignment) {
        fermat::RingBuffer<int> rb1 = { 1, 2, 3 };
        fermat::RingBuffer<int> rb2;
        rb2 = rb1;
        EXPECT_EQ(rb2.size(), 3u);
        EXPECT_TRUE(std::equal(rb1.begin(), rb1.end(), rb2.begin()));
    }

    TEST(RingBufferTest, MoveAssignment) {
        fermat::RingBuffer<int> rb1 = { 1, 2, 3 };
        fermat::RingBuffer<int> rb2;
        rb2 = std::move(rb1);
        EXPECT_EQ(rb2.size(), 3u);
        EXPECT_EQ(rb2[0], 1);
        EXPECT_EQ(rb2[1], 2);
        EXPECT_EQ(rb2[2], 3);
        // rb1 is in valid but unspecified state; we can only call functions with no preconditions.
        // Do not check rb1.empty() because it's not required.
        rb1.clear(); // safe
    }

    TEST(RingBufferTest, InitializerListAssignment) {
        fermat::RingBuffer<int> rb(5);
        rb = { 10, 20 };
        EXPECT_EQ(rb.size(), 2u);
        EXPECT_EQ(rb[0], 10);
        EXPECT_EQ(rb[1], 20);
    }

    TEST(RingBufferTest, AssignFromIterators) {
        fermat::RingBuffer<int> rb(10);
        std::vector<int> src = { 1, 2, 3, 4 };
        rb.assign(src.begin(), src.end());
        EXPECT_EQ(rb.size(), 4u);
        /// "assign"
        EXPECT_TRUE(VerifySequence(rb.begin(), rb.end(), 1, 2, 3, 4));
    }

    TEST(RingBufferTest, Swap) {
        fermat::RingBuffer<int> rb1 = { 1, 2, 3 };
        fermat::RingBuffer<int> rb2 = { 4, 5, 6, 7 };
        rb1.swap(rb2);
        EXPECT_EQ(rb1.size(), 4u);
        EXPECT_EQ(rb2.size(), 3u);
        EXPECT_EQ(rb1[0], 4);
        EXPECT_EQ(rb2[0], 1);
    }

    TEST(RingBufferTest, CapacityFunctions) {
        fermat::RingBuffer<int> rb(20);
        EXPECT_EQ(rb.capacity(), 20u);
        for (int i = 0; i < 15; ++i)
            rb.push_back(i);
        EXPECT_EQ(rb.size(), 15u);
        EXPECT_FALSE(rb.full());
        // reserve should increase capacity only if larger than current.
        rb.reserve(25);
        EXPECT_EQ(rb.capacity(), 25u);
        rb.reserve(10); // should do nothing
        EXPECT_EQ(rb.capacity(), 25u);
        // set_capacity can shrink.
        rb.set_capacity(10);
        EXPECT_EQ(rb.capacity(), 10u);
        EXPECT_EQ(rb.size(), 10u); // oldest elements dropped
        EXPECT_EQ(rb[0], 5); // first element after dropping oldest (0-4 dropped)
        // resize
        rb.resize(8);
        EXPECT_EQ(rb.size(), 8u);
        rb.resize(12);
        EXPECT_EQ(rb.size(), 12u);
        // after resizing up, elements at the end are uninitialized (stale), but we can still assign.
        rb[11] = 999;
        EXPECT_EQ(rb.back(), 999);
    }

    TEST(RingBufferTest, EmptyFull) {
        fermat::RingBuffer<int> rb(3);
        EXPECT_TRUE(rb.empty());
        EXPECT_FALSE(rb.full());
        rb.push_back(1);
        EXPECT_FALSE(rb.empty());
        EXPECT_FALSE(rb.full());
        rb.push_back(2);
        rb.push_back(3);
        EXPECT_EQ(rb.size(), 3u);
        EXPECT_TRUE(rb.full());
        // Now push another: should overwrite front.
        rb.push_back(4);
        EXPECT_EQ(rb.size(), 3u);
        EXPECT_TRUE(rb.full());
        EXPECT_EQ(rb.front(), 2);
        EXPECT_EQ(rb.back(), 4);
    }

    TEST(RingBufferTest, Iterators) {
        fermat::RingBuffer<int> rb = { 10, 20, 30, 40 };
        auto it = rb.begin();
        EXPECT_EQ(*it, 10);
        ++it;
        EXPECT_EQ(*it, 20);
        it++;
        EXPECT_EQ(*it, 30);
        --it;
        EXPECT_EQ(*it, 20);
        it += 2;
        EXPECT_EQ(*it, 40);
        it -= 1;
        EXPECT_EQ(*it, 30);
        std::advance(it, 2);
        EXPECT_EQ(it, rb.end());

        const auto& crb = rb;
        auto cit = crb.cbegin();
        EXPECT_EQ(*cit, 10);
        ++cit;
        EXPECT_EQ(*cit, 20);
    }

    TEST(RingBufferTest, ReverseIterators) {
        fermat::RingBuffer<int> rb = { 1, 2, 3, 4, 5 };
        auto rit = rb.rbegin();
        EXPECT_EQ(*rit, 5);
        ++rit;
        EXPECT_EQ(*rit, 4);
        auto crit = rb.crbegin();
        EXPECT_EQ(*crit, 5);
        auto rend = rb.rend();
        --rend;
        EXPECT_EQ(*rend, 1);
    }

    TEST(RingBufferTest, ElementAccess) {
        fermat::RingBuffer<int> rb = { 5, 10, 15 };
        EXPECT_EQ(rb.front(), 5);
        EXPECT_EQ(rb.back(), 15);
        rb.front() = 99;
        rb.back() = 100;
        EXPECT_EQ(rb[0], 99);
        EXPECT_EQ(rb[2], 100);
        const auto& crb = rb;
        EXPECT_EQ(crb[0], 99);
        EXPECT_EQ(crb[1], 10);
    }

    TEST(RingBufferTest, PushBackPopBack) {
        fermat::RingBuffer<int> rb(5);
        for (int i = 0; i < 5; ++i)
            rb.push_back(i);
        EXPECT_EQ(rb.size(), 5u);
        EXPECT_EQ(rb[0], 0);
        EXPECT_EQ(rb[4], 4);
        rb.push_back(99); // overwrites front
        EXPECT_EQ(rb.size(), 5u);
        EXPECT_EQ(rb[0], 1);
        EXPECT_EQ(rb[4], 99);
        rb.pop_back();
        EXPECT_EQ(rb.size(), 4u);
        EXPECT_EQ(rb.back(), 4);
        // Test push_back() returning reference.
        int& ref = rb.push_back();
        ref = 88;
        EXPECT_EQ(rb.back(), 88);
    }

    TEST(RingBufferTest, PushFrontPopFront) {
        fermat::RingBuffer<int> rb(5);
        for (int i = 0; i < 5; ++i)
            rb.push_front(i);
        EXPECT_EQ(rb.size(), 5u);
        EXPECT_EQ(rb[0], 4); // LIFO order because push_front inserts at front.
        EXPECT_EQ(rb[4], 0);
        rb.push_front(99);
        EXPECT_EQ(rb.size(), 5u);
        EXPECT_EQ(rb[0], 99);
        EXPECT_EQ(rb[4], 1); // oldest dropped
        rb.pop_front();
        EXPECT_EQ(rb.size(), 4u);
        EXPECT_EQ(rb.front(), 4);
        // push_front returning reference.
        int& ref = rb.push_front();
        ref = 77;
        EXPECT_EQ(rb.front(), 77);
    }

    TEST(RingBufferTest, InsertSingle) {
        fermat::RingBuffer<int> rb(10);
        for (int i = 0; i < 5; ++i)
            rb.push_back(i);
        auto it = rb.insert(rb.begin() + 2, 99);
        EXPECT_EQ(rb.size(), 6u);
        EXPECT_EQ(*it, 99);
        EXPECT_EQ(rb[2], 99);
        // Insert at end
        it = rb.insert(rb.end(), 100);
        EXPECT_EQ(rb.back(), 100);
        // Insert at full buffer
        for (int i = 0; i < 4; ++i)
            rb.push_back(0); // fill to capacity 10
        EXPECT_TRUE(rb.full());
        rb.insert(rb.begin() + 5, 111);
        EXPECT_EQ(rb.size(), 10u);
        EXPECT_EQ(rb[5], 111);
        // The oldest element (0) should have been discarded.
        EXPECT_EQ(rb[0], 1); // because original 0,1,2,3,4,99,100,... after fills
    }

    TEST(RingBufferTest, InsertMultiple) {
        fermat::RingBuffer<int> rb(20);
        rb.assign({ 1, 2, 3 });
        std::vector<int> ins = { 10, 20, 30 };
        rb.insert(rb.begin() + 1, size_t(2), 99);
        EXPECT_EQ(rb.size(), 5u);
        EXPECT_EQ(rb[1], 99);
        EXPECT_EQ(rb[2], 99);
        rb.insert(rb.begin() + 2, ins.begin(), ins.end());
        EXPECT_EQ(rb.size(), 8u);
        EXPECT_EQ(rb[2], 10);
        EXPECT_EQ(rb[3], 20);
        rb.insert(rb.end(), { 100, 200 });
        EXPECT_EQ(rb.back(), 200);
    }

    TEST(RingBufferTest, EraseSingle) {
        fermat::RingBuffer<int> rb = { 1, 2, 3, 4, 5 };
        auto it = rb.erase(rb.begin() + 2);
        EXPECT_EQ(rb.size(), 4u);
        EXPECT_EQ(*it, 4);
        EXPECT_EQ(rb[2], 4);
        it = rb.erase(rb.begin());
        EXPECT_EQ(rb.size(), 3u);
        EXPECT_EQ(rb[0], 2);
    }

    TEST(RingBufferTest, EraseRange) {
        fermat::RingBuffer<int> rb = { 1, 2, 3, 4, 5, 6, 7 };
        auto it = rb.erase(rb.begin() + 2, rb.begin() + 5);
        EXPECT_EQ(rb.size(), 4u);
        EXPECT_EQ(*it, 6);
        EXPECT_EQ(rb[2], 6);
        EXPECT_EQ(rb[3], 7);
        // Erase all
        rb.erase(rb.begin(), rb.end());
        EXPECT_TRUE(rb.empty());
    }

    TEST(RingBufferTest, Clear) {
        fermat::RingBuffer<int> rb = { 1, 2, 3 };
        rb.clear();
        EXPECT_TRUE(rb.empty());
        EXPECT_EQ(rb.size(), 0u);
        EXPECT_EQ(rb.begin(), rb.end());
        // Capacity unchanged.
        EXPECT_GT(rb.capacity(), 0u);
    }

    TEST(RingBufferTest, ComparisonOperators) {
        fermat::RingBuffer<int> a = { 1, 2, 3 };
        fermat::RingBuffer<int> b = { 1, 2, 3 };
        fermat::RingBuffer<int> c = { 1, 2, 4 };
        fermat::RingBuffer<int> d = { 1, 2 };
        EXPECT_EQ(a, b);
        EXPECT_NE(a, c);
        EXPECT_LT(a, c);
        EXPECT_GT(c, a);
        EXPECT_LT(d, a); // shorter
        EXPECT_GT(a, d);
        EXPECT_LE(a, b);
        EXPECT_GE(c, a);
    }

    TEST(RingBufferTest, GetContainer) {
        fermat::RingBuffer<int> rb(10);
        rb.push_back(42);
        auto& cont = rb.get_container();
        EXPECT_EQ(cont.size(), 11u);
        // The element at position _begin should be 42.
        EXPECT_EQ(cont[0], 42); // depends on implementation, but okay.
        const auto& ccont = rb.get_container();
        EXPECT_EQ(ccont.size(), 11u);
    }

    TEST(RingBufferTest, OverwriteBehavior) {
        // When full, push_back overwrites front.
        fermat::RingBuffer<int> rb(3);
        rb.push_back(1);
        rb.push_back(2);
        rb.push_back(3);
        rb.push_back(4);
        EXPECT_EQ(rb.size(), 3u);
        EXPECT_EQ(rb[0], 2);
        EXPECT_EQ(rb[1], 3);
        EXPECT_EQ(rb[2], 4);
        // push_front similarly overwrites back.
        fermat::RingBuffer<int> rb2(3);
        rb2.push_front(1);
        rb2.push_front(2);
        rb2.push_front(3);
        rb2.push_front(4);
        EXPECT_EQ(rb2.size(), 3u);
        EXPECT_EQ(rb2[0], 4);
        EXPECT_EQ(rb2[1], 3);
        EXPECT_EQ(rb2[2], 2);
    }

    // ============================================================================
    // Tests with non‑trivial type TestObject
    // ============================================================================
    /*
    TEST(RingBufferTest, NonTrivialType) {
        TestObject::Reset();
        {
            fermat::RingBuffer<TestObject> rb(10);
            for (int i = 0; i < 8; ++i) rb.push_back(std::move(TestObject(i)));
            EXPECT_EQ(rb.size(), 8u);
            EXPECT_EQ(TestObject::ctor_count, 11u);
            EXPECT_EQ(TestObject::move_ctor_count, 8u); // push_back may move
            TestObject::Reset();
            rb.pop_back();
            EXPECT_EQ(TestObject::dtor_count, 1u);
            rb.pop_front();
            EXPECT_EQ(TestObject::dtor_count, 2u);
            // Insert
            rb.insert(rb.begin() + 1, TestObject(99));
            EXPECT_EQ(TestObject::copy_ctor_count, 1u);
            // Clear
            rb.clear();
            EXPECT_EQ(rb.size(), 0u);
            // Destruction of elements should happen.
            EXPECT_EQ(TestObject::dtor_count, 7u); // remaining 5 + 2 already removed?
            // Actually after clear, all remaining elements destroyed.
        }
        EXPECT_EQ(TestObject::dtor_count,
                  TestObject::ctor_count + TestObject::copy_ctor_count + TestObject::move_ctor_count);
    }
    */
    // ============================================================================
    // Tests with different underlying container (e.g., std::vector)
    // ============================================================================
    TEST(RingBufferTest, WithStdVector) {
        fermat::RingBuffer<int, std::vector<int>> rb(10);
        for (int i = 0; i < 12; ++i)
            rb.push_back(i);
        EXPECT_EQ(rb.capacity(), 10u);
        EXPECT_EQ(rb.size(), 10u);
        EXPECT_EQ(rb.front(), 2);
        EXPECT_EQ(rb.back(), 11);
    }

    // ============================================================================
    // Tests with std::list as underlying container (bidirectional iterator)
    // ============================================================================
    TEST(RingBufferTest, WithStdList) {
        fermat::RingBuffer<int, std::list<int>> rb(10);
        for (int i = 0; i < 15; ++i)
            rb.push_back(i);
        EXPECT_EQ(rb.size(), 10u);
        EXPECT_EQ(rb.front(), 5);
        EXPECT_EQ(rb.back(), 14);
        auto it = rb.begin();
        std::advance(it, 3);
        EXPECT_EQ(*it, 8);
        rb.insert(it, 999);
        EXPECT_EQ(rb.size(), 10u);
        EXPECT_EQ(rb[3], 999); // operator[] still works with list? Yes, it uses advance.
    }

    // ============================================================================
    // Edge cases: capacity 0
    // ============================================================================
    TEST(RingBufferTest, ZeroCapacity) {
        fermat::RingBuffer<int> rb(0);
        EXPECT_EQ(rb.capacity(), 0u);
        EXPECT_TRUE(rb.empty());
        EXPECT_TRUE(rb.full());
        // push_back should be a no-op? Actually with capacity 0, push_back overwrites nothing.
        rb.push_back(42);
        EXPECT_TRUE(rb.empty()); // because buffer cannot hold any element
        EXPECT_EQ(rb.size(), 0u);
        // insert/erase should also be safe (but likely no effect)
        rb.insert(rb.begin(), 99);
        EXPECT_TRUE(rb.size() == 0);
    }
} // namespace
