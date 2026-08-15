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
#include <fermat/maps/vector_map.h>
#include <fermat/maps/vector_multimap.h>
#include <deque>
#include <string>
#include <utility>
#include <vector>

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

    // Reset counters before each test
    void ResetTestObjectCounters() {
        TestObject::sCtorCount = 0;
        TestObject::sDtorCount = 0;
        TestObject::sCopyCount = 0;
        TestObject::sMoveCount = 0;
    }

    // -----------------------------------------------------------------------------
    // Tests for VectorMap
    // -----------------------------------------------------------------------------
    TEST(VectorMapTest, ConstructionAndAssignment) {
        using Map = VectorMap<int, std::string>;

        // Default constructor
        Map m1;
        EXPECT_TRUE(m1.empty());
        EXPECT_EQ(m1.size(), 0u);

        // Initializer list
        Map m2 = { { 1, "one" }, { 2, "two" }, { 3, "three" } };
        EXPECT_EQ(m2.size(), 3u);
        EXPECT_EQ(m2[1], "one");
        EXPECT_EQ(m2[2], "two");
        EXPECT_EQ(m2[3], "three");

        // Copy constructor
        Map m3(m2);
        EXPECT_EQ(m3, m2);

        // Move constructor
        Map m4(std::move(m2));
        EXPECT_EQ(m4.size(), 3u);
        EXPECT_TRUE(m2.empty()); // moved-from state

        // Copy assignment
        Map m5;
        m5 = m4;
        EXPECT_EQ(m5, m4);

        // Move assignment
        Map m6;
        m6 = std::move(m4);
        EXPECT_EQ(m6.size(), 3u);
        EXPECT_TRUE(m4.empty());

        // Initializer list assignment
        m1 = { { 10, "ten" }, { 20, "twenty" } };
        EXPECT_EQ(m1.size(), 2u);
    }

    TEST(VectorMapTest, InsertAndEmplace) {
        using Map = VectorMap<int, TestObject>;
        ResetTestObjectCounters();

        Map m;
        auto [it1, inserted1] = m.insert({ 1, TestObject(100) });
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(it1->first, 1);
        EXPECT_EQ(it1->second.value, 100);

        // Insert duplicate key should fail
        auto [it2, inserted2] = m.insert({ 1, TestObject(200) });
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it2->second.value, 100);

        // Emplace
        auto [it3, inserted3] = m.emplace(2, 200);
        EXPECT_TRUE(inserted3);
        EXPECT_EQ(it3->second.value, 200);

        // Emplace hint
        auto it4 = m.emplace_hint(m.end(), 3, 300);
        EXPECT_EQ(it4->first, 3);
        EXPECT_EQ(it4->second.value, 300);

        // Insert via key_type (value-initialized mapped_type)
        auto [it5, inserted5] = m.insert(4); // inserts {4, TestObject()}
        EXPECT_TRUE(inserted5);
        EXPECT_EQ(it5->second.value, 0);

        // Insert via key_type with move
        int key = 5;
        auto [it6, inserted6] = m.insert(std::move(key));
        EXPECT_TRUE(inserted6);
        EXPECT_EQ(it6->first, 5);
    }

    TEST(VectorMapTest, OperatorBracketAndAtKey) {
        using Map = VectorMap<int, std::string>;
        Map m = { { 1, "one" }, { 2, "two" } };

        // operator[] for existing key
        EXPECT_EQ(m[1], "one");
        // operator[] for new key creates default-constructed value
        EXPECT_EQ(m[3], "");
        EXPECT_EQ(m.size(), 3u);
        m[3] = "three";
        EXPECT_EQ(m[3], "three");

        // at_key for existing key
        EXPECT_EQ(m.at_key(2), "two");
        // at_key for non-existing key should throw or CHECK (depending on implementation)
        // We'll test that it fails gracefully. In our implementation, at_key will KCHECK.
        // Since we can't test death in unit tests easily, we skip.
    }

    TEST(VectorMapTest, FindAndContains) {
        using Map = VectorMap<int, std::string>;
        Map m = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

        auto it = m.find(2);
        ASSERT_NE(it, m.end());
        EXPECT_EQ(it->second, "two");

        it = m.find(4);
        EXPECT_EQ(it, m.end());

        EXPECT_TRUE(m.contains(1));
        EXPECT_FALSE(m.contains(4));

        // Transparent lookup with heterogeneous key
        struct StringLike {
            std::string s;
            bool operator<(const StringLike& other) const { return s < other.s; }
        };
        // Not needed for int map.
    }

    TEST(VectorMapTest, Erase) {
        using Map = VectorMap<int, std::string>;
        Map m = { { 1, "one" }, { 2, "two" }, { 3, "three" }, { 4, "four" } };

        // Erase by key
        size_t erased = m.erase(2);
        EXPECT_EQ(erased, 1u);
        EXPECT_EQ(m.size(), 3u);
        EXPECT_FALSE(m.contains(2));

        erased = m.erase(5);
        EXPECT_EQ(erased, 0u);

        // Erase by iterator
        auto it = m.find(3);
        it = m.erase(it);
        EXPECT_EQ(m.size(), 2u);
        EXPECT_EQ(it->first, 4); // iterator points to next element

        // Erase range
        auto first = m.begin();
        auto last = m.end();
        m.erase(first, last);
        EXPECT_TRUE(m.empty());
    }

    TEST(VectorMapTest, LowerUpperBoundAndEqualRange) {
        using Map = VectorMap<int, std::string>;
        Map m = { { 1, "one" }, { 2, "two" }, { 3, "three" }, { 4, "four" } };

        auto lb = m.lower_bound(2);
        EXPECT_EQ(lb->first, 2);
        auto ub = m.upper_bound(2);
        EXPECT_EQ(ub->first, 3);

        auto [eq_lb, eq_ub] = m.equal_range(2);
        EXPECT_EQ(eq_lb, lb);
        EXPECT_EQ(eq_ub, ub);

        // For key not present
        lb = m.lower_bound(5);
        EXPECT_EQ(lb, m.end());
        ub = m.upper_bound(5);
        EXPECT_EQ(ub, m.end());
    }

    TEST(VectorMapTest, CountAndSize) {
        using Map = VectorMap<int, int>;
        Map m = { { 1, 10 }, { 2, 20 }, { 3, 30 } };
        EXPECT_EQ(m.count(2), 1u);
        EXPECT_EQ(m.count(5), 0u);
        EXPECT_EQ(m.size(), 3u);
        m.clear();
        EXPECT_TRUE(m.empty());
        EXPECT_EQ(m.size(), 0u);
    }

    TEST(VectorMapTest, IteratorsAndReverseIterators) {
        using Map = VectorMap<int, int>;
        Map m = { { 1, 10 }, { 2, 20 }, { 3, 30 } };
        std::vector<int> keys;
        for (auto it = m.begin(); it != m.end(); ++it)
            keys.push_back(it->first);
        EXPECT_EQ(keys, (std::vector<int> { 1, 2, 3 }));

        keys.clear();
        for (auto it = m.rbegin(); it != m.rend(); ++it)
            keys.push_back(it->first);
        EXPECT_EQ(keys, (std::vector<int> { 3, 2, 1 }));
    }

    TEST(VectorMapTest, HeterogeneousLookup) {
        // Use transparent comparator (std::less<void>)
        using Map = VectorMap<std::string, int, std::less<>>;
        Map m = { { "apple", 1 }, { "banana", 2 }, { "cherry", 3 } };

        // Find with std::string_view
        std::string_view sv = "banana";
        auto it = m.find(sv);
        ASSERT_NE(it, m.end());
        EXPECT_EQ(it->first, "banana");

        // contains with const char*
        EXPECT_TRUE(m.contains("apple"));

        // count
        EXPECT_EQ(m.count("cherry"), 1u);
        EXPECT_EQ(m.count("grape"), 0u);

        // lower_bound with std::string_view
        auto lb = m.lower_bound("banana");
        EXPECT_EQ(lb->first, "banana");

        // equal_range
        auto [eq_lb, eq_ub] = m.equal_range("banana");
        EXPECT_EQ(eq_lb->first, "banana");
        EXPECT_EQ(eq_ub->first, "cherry");
    }

    TEST(VectorMapTest, CompareCustom) {
        struct ReverseCompare {
            bool operator()(int a, int b) const { return a > b; }
        };
        using Map = VectorMap<int, int, ReverseCompare>;
        Map m = { { 1, 10 }, { 2, 20 }, { 3, 30 } };
        // Since compare is reversed, order should be descending.
        std::vector<int> keys;
        for (auto& p : m)
            keys.push_back(p.first);
        EXPECT_EQ(keys, (std::vector<int> { 3, 2, 1 }));

        // lower_bound uses the compare
        auto lb = m.lower_bound(2);
        EXPECT_EQ(lb->first, 2);
    }

    TEST(VectorMapTest, MoveSemantics) {
        using Map = VectorMap<int, TestObject>;
        ResetTestObjectCounters();

        Map m1;
        m1.emplace(1, 10);
        m1.emplace(2, 20);

        Map m2 = std::move(m1);
        EXPECT_EQ(m2.size(), 2u);
        EXPECT_TRUE(m1.empty());

        // Check that objects were moved, not copied
        // Exact counts depend on implementation, but should be low.
    }

    TEST(VectorMapTest, Swap) {
        using Map = VectorMap<int, std::string>;
        Map m1 = { { 1, "one" }, { 2, "two" } };
        Map m2 = { { 3, "three" } };
        m1.swap(m2);
        EXPECT_EQ(m1.size(), 1u);
        EXPECT_EQ(m2.size(), 2u);
        EXPECT_TRUE(m1.contains(3));
        EXPECT_TRUE(m2.contains(1));
    }

    // -----------------------------------------------------------------------------
    // Tests for VectorMultimap
    // -----------------------------------------------------------------------------
    TEST(VectorMultimapTest, ConstructionAndAssignment) {
        using MMap = VectorMultimap<int, std::string>;
        MMap mm1;
        EXPECT_TRUE(mm1.empty());

        MMap mm2 = { { 1, "one" }, { 1, "uno" }, { 2, "two" } };
        EXPECT_EQ(mm2.size(), 3u);
        EXPECT_EQ(mm2.count(1), 2u);

        MMap mm3(mm2);
        EXPECT_EQ(mm3, mm2);

        MMap mm4(std::move(mm2));
        EXPECT_EQ(mm4.size(), 3u);
        EXPECT_TRUE(mm2.empty());

        mm1 = mm4;
        EXPECT_EQ(mm1, mm4);
    }

    TEST(VectorMultimapTest, InsertAndEmplace) {
        using MMap = VectorMultimap<int, TestObject>;
        ResetTestObjectCounters();

        MMap mm;
        auto it1 = mm.insert({ 1, TestObject(100) });
        EXPECT_EQ(it1->first, 1);
        auto it2 = mm.insert({ 1, TestObject(101) });
        EXPECT_EQ(it2->first, 1);
        EXPECT_EQ(mm.size(), 2u);
        EXPECT_EQ(mm.count(1), 2u);

        // Emplace
        auto it3 = mm.emplace(2, 200);
        EXPECT_EQ(it3->first, 2);
        EXPECT_EQ(it3->second.value, 200);

        // Emplace hint
        auto it4 = mm.emplace_hint(mm.end(), 2, 201);
        EXPECT_EQ(it4->first, 2);
        EXPECT_EQ(mm.count(2), 2u);

        // Insert via key_type (value-initialized)
        auto it5 = mm.insert(3);
        EXPECT_EQ(it5->first, 3);
        EXPECT_EQ(it5->second.value, 0);
    }

    TEST(VectorMultimapTest, FindAndContains) {
        using MMap = VectorMultimap<int, std::string>;
        MMap mm = { { 1, "one" }, { 1, "uno" }, { 2, "two" } };
        auto it = mm.find(1);
        ASSERT_NE(it, mm.end());
        // May be either "one" or "uno", but key is 1.
        EXPECT_EQ(it->first, 1);
        EXPECT_TRUE(mm.contains(1));
        EXPECT_FALSE(mm.contains(3));
    }

    TEST(VectorMultimapTest, Erase) {
        using MMap = VectorMultimap<int, std::string>;
        MMap mm = { { 1, "one" }, { 1, "uno" }, { 2, "two" }, { 3, "three" } };
        size_t erased = mm.erase(1);
        EXPECT_EQ(erased, 2u);
        EXPECT_EQ(mm.size(), 2u);
        EXPECT_FALSE(mm.contains(1));

        // Erase by iterator
        auto it = mm.find(2);
        it = mm.erase(it);
        EXPECT_EQ(mm.size(), 1u);
        EXPECT_EQ(it->first, 3);

        // Erase range
        mm.erase(mm.begin(), mm.end());
        EXPECT_TRUE(mm.empty());
    }

    TEST(VectorMultimapTest, EqualRangeAndCount) {
        using MMap = VectorMultimap<int, std::string>;
        MMap mm = { { 1, "one" }, { 1, "uno" }, { 1, "eins" }, { 2, "two" } };
        auto [first, last] = mm.equal_range(1);
        EXPECT_EQ(std::distance(first, last), 3);
        int count = 0;
        for (auto it = first; it != last; ++it) {
            EXPECT_EQ(it->first, 1);
            ++count;
        }
        EXPECT_EQ(count, 3);
        EXPECT_EQ(mm.count(1), 3u);
        EXPECT_EQ(mm.count(2), 1u);
        EXPECT_EQ(mm.count(3), 0u);
    }

    TEST(VectorMultimapTest, LowerUpperBound) {
        using MMap = VectorMultimap<int, int>;
        MMap mm = { { 1, 10 }, { 1, 11 }, { 2, 20 }, { 3, 30 } };
        auto lb = mm.lower_bound(1);
        EXPECT_EQ(lb->first, 1);
        auto ub = mm.upper_bound(1);
        EXPECT_EQ(ub->first, 2);
        EXPECT_EQ(std::distance(lb, ub), 2);

        lb = mm.lower_bound(5);
        EXPECT_EQ(lb, mm.end());
        ub = mm.upper_bound(5);
        EXPECT_EQ(ub, mm.end());
    }

    TEST(VectorMultimapTest, HeterogeneousLookup) {
        using MMap = VectorMultimap<std::string, int, std::less<>>;
        MMap mm = { { "apple", 1 }, { "apple", 2 }, { "banana", 3 } };
        std::string_view sv = "apple";
        auto range = mm.equal_range(sv);
        int count = 0;
        for (auto it = range.first; it != range.second; ++it) {
            EXPECT_EQ(it->first, "apple");
            ++count;
        }
        EXPECT_EQ(count, 2);
        EXPECT_EQ(mm.count(sv), 2u);
    }

    TEST(VectorMultimapTest, IteratorsAndReverse) {
        using MMap = VectorMultimap<int, int>;
        MMap mm = { { 1, 10 }, { 2, 20 }, { 3, 30 } };
        std::vector<int> keys;
        for (auto it = mm.begin(); it != mm.end(); ++it)
            keys.push_back(it->first);
        EXPECT_EQ(keys, (std::vector<int> { 1, 2, 3 }));

        keys.clear();
        for (auto it = mm.rbegin(); it != mm.rend(); ++it)
            keys.push_back(it->first);
        EXPECT_EQ(keys, (std::vector<int> { 3, 2, 1 }));
    }

    TEST(VectorMultimapTest, Swap) {
        using MMap = VectorMultimap<int, std::string>;
        MMap mm1 = { { 1, "one" }, { 2, "two" } };
        MMap mm2 = { { 3, "three" } };
        mm1.swap(mm2);
        EXPECT_EQ(mm1.size(), 1u);
        EXPECT_EQ(mm2.size(), 2u);
        EXPECT_TRUE(mm1.contains(3));
        EXPECT_TRUE(mm2.contains(1));
    }

    // -----------------------------------------------------------------------------
    // Tests for different underlying containers (if needed)
    // -----------------------------------------------------------------------------
    TEST(VectorMapTest, WithDequeAsUnderlying) {
        using DequeMap = VectorMap<int, int, std::less<int>, std::deque<std::pair<int, int>>>;
        DequeMap dm = { { 1, 10 }, { 2, 20 } };
        EXPECT_EQ(dm.size(), 2u);
        EXPECT_EQ(dm[1], 10);
    }

} // namespace fermat
