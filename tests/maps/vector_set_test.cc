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

#include <gtest/gtest.h>
#include <fermat/maps/vector_set.h>
#include <fermat/maps/vector_multiset.h>
#include <deque>
#include <string>
#include <utility>
#include <vector>
#include <turbo/strings/ascii.h>
#include <turbo/strings/str_cat.h>

namespace fermat {
    // -----------------------------------------------------------------------------
    // Helper: test object that counts constructions/destructions
    // -----------------------------------------------------------------------------
    class TestObject {
    public:
        static int sCtorCount;
        static int sDtorCount;
        static int sCopyCount;
        static int sMoveCount;

        int value;

        TestObject(int v = 0)
            : value(v) { ++sCtorCount; }
        TestObject(const TestObject& other)
            : value(other.value) { ++sCopyCount; }
        TestObject(TestObject&& other) noexcept
            : value(std::exchange(other.value, 0)) { ++sMoveCount; }
        ~TestObject() { ++sDtorCount; }

        TestObject& operator=(const TestObject& other) {
            value = other.value;
            ++sCopyCount;
            return *this;
        }

        TestObject& operator=(TestObject&& other) noexcept {
            value = std::exchange(other.value, 0);
            ++sMoveCount;
            return *this;
        }

        bool operator==(const TestObject& other) const { return value == other.value; }
        bool operator<(const TestObject& other) const { return value < other.value; }
    };

    int TestObject::sCtorCount = 0;
    int TestObject::sDtorCount = 0;
    int TestObject::sCopyCount = 0;
    int TestObject::sMoveCount = 0;

    void ResetTestObjectCounters() {
        TestObject::sCtorCount = 0;
        TestObject::sDtorCount = 0;
        TestObject::sCopyCount = 0;
        TestObject::sMoveCount = 0;
    }

    // Case-insensitive comparator for testing find_as and heterogeneous lookup
    struct CaseInsensitiveCompare {
        template <typename T, typename U>
        bool operator()(const T& a, const U& b) const {
            auto sa = turbo::str_to_lower(turbo::str_cat(a));
            auto sb = turbo::str_to_lower(turbo::str_cat(b));
            return sa < sb;
        }
    };

    // -----------------------------------------------------------------------------
    // Tests for VectorSet (unique keys)
    // -----------------------------------------------------------------------------
    TEST(VectorSetTest, ConstructionAndAssignment) {
        using Set = VectorSet<int>;
        Set s1;
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(s1.size(), 0u);

        Set s2 = { 1, 2, 3, 4 };
        EXPECT_EQ(s2.size(), 4u);
        EXPECT_TRUE(s2.contains(1));
        EXPECT_TRUE(s2.contains(3));

        Set s3(s2);
        EXPECT_EQ(s3, s2);

        Set s4(std::move(s2));
        EXPECT_EQ(s4.size(), 4u);
        EXPECT_TRUE(s2.empty()); // moved-from state

        Set s5;
        s5 = s4;
        EXPECT_EQ(s5, s4);

        Set s6;
        s6 = std::move(s4);
        EXPECT_EQ(s6.size(), 4u);
        EXPECT_TRUE(s4.empty());

        s1 = { 10, 20, 30 };
        EXPECT_EQ(s1.size(), 3u);
        EXPECT_TRUE(s1.contains(20));
    }

    TEST(VectorSetTest, InsertAndEmplace) {
        using Set = VectorSet<TestObject>;
        ResetTestObjectCounters();

        Set s;
        auto [it1, inserted1] = s.insert(TestObject(10));
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(it1->value, 10);

        // duplicate insert should fail
        auto [it2, inserted2] = s.insert(TestObject(10));
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it2->value, 10);

        // emplace
        auto [it3, inserted3] = s.emplace(20);
        EXPECT_TRUE(inserted3);
        EXPECT_EQ(it3->value, 20);

        // emplace_hint
        auto it4 = s.emplace_hint(s.end(), 30);
        EXPECT_EQ(it4->value, 30);

        // insert with heterogeneous key (if transparent compare)
        using Set2 = VectorSet<std::string, std::less<>>;
        Set2 s2;
        auto [it5, inserted5] = s2.insert("hello");
        EXPECT_TRUE(inserted5);
        EXPECT_EQ(*it5, "hello");
    }

    TEST(VectorSetTest, InsertWithHint) {
        using Set = VectorSet<int>;
        Set s = { 1, 3, 5 };
        // correct hint (position before 3)
        auto it = s.insert(s.lower_bound(2), 2);
        EXPECT_EQ(*it, 2);
        EXPECT_EQ(s.size(), 4u);
        // incorrect hint falls back to regular insert
        it = s.insert(s.end(), 4);
        EXPECT_EQ(*it, 4);
        EXPECT_EQ(s.size(), 5u);
    }

    TEST(VectorSetTest, Erase) {
        using Set = VectorSet<int>;
        Set s = { 1, 2, 3, 4, 5 };
        size_t erased = s.erase(3);
        EXPECT_EQ(erased, 1u);
        EXPECT_FALSE(s.contains(3));
        EXPECT_EQ(s.size(), 4u);

        erased = s.erase(10);
        EXPECT_EQ(erased, 0u);

        // erase by iterator
        auto it = s.find(2);
        it = s.erase(it);
        EXPECT_FALSE(s.contains(2));
        EXPECT_EQ(*it, 4); // points to next element

        // erase range
        auto first = s.find(4);
        auto last = s.end();
        s.erase(first, last);
        EXPECT_EQ(s.size(), 1u);
        EXPECT_TRUE(s.contains(1));
    }

    TEST(VectorSetTest, FindAndContains) {
        using Set = VectorSet<std::string>;
        Set s = { "apple", "banana", "cherry" };
        auto it = s.find("banana");
        ASSERT_NE(it, s.end());
        EXPECT_EQ(*it, "banana");

        it = s.find("grape");
        EXPECT_EQ(it, s.end());

        EXPECT_TRUE(s.contains("apple"));
        EXPECT_FALSE(s.contains("grape"));
    }

    TEST(VectorSetTest, Count) {
        using Set = VectorSet<int>;
        Set s = { 10, 20, 30 };
        EXPECT_EQ(s.count(20), 1u);
        EXPECT_EQ(s.count(40), 0u);
    }

    TEST(VectorSetTest, LowerUpperBoundAndEqualRange) {
        using Set = VectorSet<int>;
        Set s = { 1, 2, 4, 5, 7 };
        auto lb = s.lower_bound(4);
        EXPECT_EQ(*lb, 4);
        auto ub = s.upper_bound(4);
        EXPECT_EQ(*ub, 5);

        auto [eq_lb, eq_ub] = s.equal_range(4);
        EXPECT_EQ(eq_lb, lb);
        EXPECT_EQ(eq_ub, ub);

        // key not present
        lb = s.lower_bound(3);
        EXPECT_EQ(*lb, 4);
        ub = s.upper_bound(3);
        EXPECT_EQ(*ub, 4);
        auto [eq_lb2, eq_ub2] = s.equal_range(3);
        EXPECT_EQ(eq_lb2, eq_ub2); // empty range
    }

    TEST(VectorSetTest, HeterogeneousLookup) {
        using Set = VectorSet<std::string, std::less<>>; // transparent comparator
        Set s = { "apple", "banana", "cherry" };

        std::string_view sv = "banana";
        auto it = s.find(sv);
        ASSERT_NE(it, s.end());
        EXPECT_EQ(*it, "banana");

        EXPECT_TRUE(s.contains(sv));
        EXPECT_EQ(s.count(sv), 1u);

        auto lb = s.lower_bound(sv);
        EXPECT_EQ(*lb, "banana");
        auto ub = s.upper_bound(sv);
        EXPECT_EQ(*ub, "cherry");
    }

    TEST(VectorSetTest, FindAs) {
        using Set = VectorSet<std::string>;
        Set s = { "Apple", "Banana", "Cherry" };
        // find_as with case-insensitive comparator
        auto it = s.find_as("apple", CaseInsensitiveCompare());
        ASSERT_NE(it, s.end());
        EXPECT_EQ(*it, "Apple");
    }

    TEST(VectorSetTest, IteratorsAndReverse) {
        using Set = VectorSet<int>;
        Set s = { 1, 2, 3, 4 };
        std::vector<int> forward;
        for (auto it = s.begin(); it != s.end(); ++it)
            forward.push_back(*it);
        EXPECT_EQ(forward, (std::vector<int> { 1, 2, 3, 4 }));

        std::vector<int> backward;
        for (auto it = s.rbegin(); it != s.rend(); ++it)
            backward.push_back(*it);
        EXPECT_EQ(backward, (std::vector<int> { 4, 3, 2, 1 }));
    }

    TEST(VectorSetTest, CustomComparator) {
        struct ReverseCompare {
            bool operator()(int a, int b) const { return a > b; }
        };
        using Set = VectorSet<int, ReverseCompare>;
        Set s = { 3, 1, 4, 2 };
        // order should be descending
        std::vector<int> expected = { 4, 3, 2, 1 };
        std::vector<int> actual(s.begin(), s.end());
        EXPECT_EQ(actual, expected);

        // find still works
        auto it = s.find(2);
        EXPECT_NE(it, s.end());
        EXPECT_EQ(*it, 2);
    }

    TEST(VectorSetTest, Swap) {
        using Set = VectorSet<int>;
        Set s1 = { 1, 2, 3 };
        Set s2 = { 4, 5 };
        s1.swap(s2);
        EXPECT_EQ(s1.size(), 2u);
        EXPECT_TRUE(s1.contains(4));
        EXPECT_EQ(s2.size(), 3u);
        EXPECT_TRUE(s2.contains(1));
    }

    TEST(VectorSetTest, MoveSemantics) {
        using Set = VectorSet<TestObject>;
        ResetTestObjectCounters();

        Set s1;
        s1.emplace(10);
        s1.emplace(20);
        Set s2 = std::move(s1);
        EXPECT_EQ(s2.size(), 2u);
        EXPECT_TRUE(s1.empty());
        // moves should be cheap; no deep copies
        EXPECT_LE(TestObject::sCopyCount, 0); // ensure no copies
    }

    // -----------------------------------------------------------------------------
    // Tests for VectorMultiset (allows duplicates)
    // -----------------------------------------------------------------------------
    TEST(VectorMultisetTest, ConstructionAndAssignment) {
        using MSet = VectorMultiset<int>;
        MSet ms1;
        EXPECT_TRUE(ms1.empty());

        MSet ms2 = { 1, 1, 2, 2, 3 };
        EXPECT_EQ(ms2.size(), 5u);
        EXPECT_EQ(ms2.count(1), 2u);
        EXPECT_EQ(ms2.count(2), 2u);
        EXPECT_EQ(ms2.count(3), 1u);

        MSet ms3(ms2);
        EXPECT_EQ(ms3, ms2);

        MSet ms4(std::move(ms2));
        EXPECT_EQ(ms4.size(), 5u);
        EXPECT_TRUE(ms2.empty());

        ms1 = ms4;
        EXPECT_EQ(ms1, ms4);
        ms1 = std::move(ms4);
        EXPECT_EQ(ms1.size(), 5u);
        EXPECT_TRUE(ms4.empty());
    }

    TEST(VectorMultisetTest, InsertAndEmplace) {
        using MSet = VectorMultiset<TestObject>;
        ResetTestObjectCounters();

        MSet ms;
        auto it1 = ms.insert(TestObject(10));
        EXPECT_EQ(it1->value, 10);
        auto it2 = ms.insert(TestObject(10)); // duplicate allowed
        EXPECT_EQ(it2->value, 10);
        EXPECT_EQ(ms.size(), 2u);
        EXPECT_EQ(ms.count(10), 2u);

        auto it3 = ms.emplace(20);
        EXPECT_EQ(it3->value, 20);
        auto it4 = ms.emplace_hint(ms.end(), 20);
        EXPECT_EQ(it4->value, 20);
        EXPECT_EQ(ms.count(20), 2u);
    }

    TEST(VectorMultisetTest, Erase) {
        using MSet = VectorMultiset<int>;
        MSet ms = { 1, 1, 2, 2, 3, 3 };
        size_t erased = ms.erase(2);
        EXPECT_EQ(erased, 2u);
        EXPECT_EQ(ms.count(2), 0u);
        EXPECT_EQ(ms.size(), 4u);

        // erase by iterator (erases only one element)
        auto it = ms.find(1);
        it = ms.erase(it);
        EXPECT_EQ(ms.count(1), 1u);
        EXPECT_EQ(*it, 1); // next duplicate

        // erase range
        auto first = ms.lower_bound(1);
        auto last = ms.upper_bound(1);
        ms.erase(first, last);
        EXPECT_EQ(ms.count(1), 0u);
        EXPECT_EQ(ms.size(), 2u); // {3,3}
    }

    TEST(VectorMultisetTest, FindAndContains) {
        using MSet = VectorMultiset<std::string>;
        MSet ms = { "a", "a", "b", "c" };
        auto it = ms.find("a");
        ASSERT_NE(it, ms.end());
        EXPECT_EQ(*it, "a");
        // find returns first occurrence
        it = ms.find("d");
        EXPECT_EQ(it, ms.end());

        EXPECT_TRUE(ms.contains("a"));
        EXPECT_FALSE(ms.contains("z"));
    }

    TEST(VectorMultisetTest, Count) {
        using MSet = VectorMultiset<int>;
        MSet ms = { 1, 1, 2, 3, 3, 3 };
        EXPECT_EQ(ms.count(1), 2u);
        EXPECT_EQ(ms.count(2), 1u);
        EXPECT_EQ(ms.count(3), 3u);
        EXPECT_EQ(ms.count(4), 0u);
    }

    TEST(VectorMultisetTest, EqualRangeAndBound) {
        using MSet = VectorMultiset<int>;
        MSet ms = { 1, 1, 2, 2, 3, 4, 4 };
        auto [first, last] = ms.equal_range(2);
        EXPECT_EQ(std::distance(first, last), 2);
        for (auto it = first; it != last; ++it)
            EXPECT_EQ(*it, 2);

        auto lb = ms.lower_bound(2);
        EXPECT_EQ(*lb, 2);
        auto ub = ms.upper_bound(2);
        EXPECT_EQ(*ub, 3);

        // equal_range for key not present
        auto [f2, l2] = ms.equal_range(5);
        EXPECT_EQ(f2, l2);
        EXPECT_EQ(f2, ms.end());
    }

    TEST(VectorMultisetTest, EqualRangeSmall) {
        using MSet = VectorMultiset<int>;
        MSet ms = { 1, 2, 2, 3, 4, 4, 4 };
        auto [first, last] = ms.equal_range_small(2);
        EXPECT_EQ(std::distance(first, last), 2);
        auto [f2, l2] = ms.equal_range_small(4);
        EXPECT_EQ(std::distance(f2, l2), 3);
        auto [f3, l3] = ms.equal_range_small(5);
        EXPECT_EQ(f3, l3);
        EXPECT_EQ(f3, ms.end());
    }

    TEST(VectorMultisetTest, HeterogeneousLookup) {
        using MSet = VectorMultiset<std::string, std::less<>>;
        MSet ms = { "apple", "apple", "banana", "cherry" };
        std::string_view sv = "apple";
        EXPECT_EQ(ms.count(sv), 2u);
        auto it = ms.find(sv);
        ASSERT_NE(it, ms.end());
        EXPECT_EQ(*it, "apple");

        auto [first, last] = ms.equal_range(sv);
        EXPECT_EQ(std::distance(first, last), 2);
    }

    TEST(VectorMultisetTest, IteratorsAndReverse) {
        using MSet = VectorMultiset<int>;
        MSet ms = { 1, 1, 2, 3, 3 };
        std::vector<int> forward(ms.begin(), ms.end());
        EXPECT_EQ(forward, (std::vector<int> { 1, 1, 2, 3, 3 }));

        std::vector<int> backward(ms.rbegin(), ms.rend());
        std::reverse(backward.begin(), backward.end());
        EXPECT_EQ(backward, forward);
    }

    TEST(VectorMultisetTest, Swap) {
        using MSet = VectorMultiset<int>;
        MSet ms1 = { 1, 1, 2 };
        MSet ms2 = { 3, 4, 4, 5 };
        ms1.swap(ms2);
        EXPECT_EQ(ms1.size(), 4u);
        EXPECT_EQ(ms2.size(), 3u);
        EXPECT_TRUE(ms1.contains(4));
        EXPECT_TRUE(ms2.contains(1));
    }

    // -----------------------------------------------------------------------------
    // Tests for different underlying containers
    // -----------------------------------------------------------------------------
    TEST(VectorSetTest, WithDequeUnderlying) {
        using DequeSet = VectorSet<int, std::less<int>, std::deque<int>>;
        DequeSet ds = { 5, 2, 8, 1 };
        EXPECT_EQ(ds.size(), 4u);
        EXPECT_TRUE(ds.contains(2));
        EXPECT_TRUE(ds.contains(8));
        std::vector<int> sorted(ds.begin(), ds.end());
        EXPECT_EQ(sorted, (std::vector<int> { 1, 2, 5, 8 }));
    }

    TEST(VectorMultisetTest, WithDequeUnderlying) {
        using DequeMSet = VectorMultiset<int, std::less<int>, std::deque<int>>;
        DequeMSet dms = { 1, 2, 2, 3 };
        EXPECT_EQ(dms.count(2), 2u);
        auto [first, last] = dms.equal_range(2);
        EXPECT_EQ(std::distance(first, last), 2);
    }

    // -----------------------------------------------------------------------------
    // Const correctness tests
    // -----------------------------------------------------------------------------
    TEST(VectorSetTest, ConstAccess) {
        const VectorSet<int> s = { 1, 2, 3 };
        EXPECT_EQ(s.size(), 3u);
        auto it = s.find(2);
        EXPECT_NE(it, s.end());
        EXPECT_EQ(*it, 2);
        EXPECT_EQ(s.count(3), 1u);
        auto lb = s.lower_bound(2);
        EXPECT_EQ(*lb, 2);
        auto ub = s.upper_bound(2);
        EXPECT_EQ(*ub, 3);
    }

    TEST(VectorMultisetTest, ConstAccess) {
        const VectorMultiset<int> ms = { 1, 1, 2 };
        EXPECT_EQ(ms.size(), 3u);
        EXPECT_EQ(ms.count(1), 2u);
        auto it = ms.find(2);
        EXPECT_NE(it, ms.end());
        auto range = ms.equal_range(1);
        EXPECT_EQ(std::distance(range.first, range.second), 2);
    }
} // namespace fermat
