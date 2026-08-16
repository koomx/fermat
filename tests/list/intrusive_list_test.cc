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
//

#include <gtest/gtest.h>
#include <fermat/list/intrusive_list.h>
#include <string>
#include <vector>

namespace fermat {
namespace {

// ----------------------------------------------------------------------------
// Node types derived from IntrusiveListNode.
// ----------------------------------------------------------------------------

/// Simple node storing an integer.
struct IntNode : public IntrusiveListNode {
    int value;
    explicit IntNode(int v) : value(v) {}
    bool operator==(const IntNode& other) const { return value == other.value; }
    bool operator<(const IntNode& other) const { return value < other.value; }
};

/// Node storing a string.
struct StringNode : public IntrusiveListNode {
    std::string value;
    explicit StringNode(const std::string& v) : value(v) {}
    bool operator==(const StringNode& other) const { return value == other.value; }
    bool operator<(const StringNode& other) const { return value < other.value; }
};

/// Node that will be inserted into multiple lists via multiple inheritance.
struct MultiListNode : public IntrusiveListNode {
    int id;
    explicit MultiListNode(int i) : id(i) {}
};

// ----------------------------------------------------------------------------
// Basic construction and empty checks.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, DefaultConstructEmpty) {
    IntrusiveList<IntNode> list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0u);
    EXPECT_EQ(list.begin(), list.end());
}

TEST(IntrusiveListTest, CopyConstructIsIgnored) {
    IntrusiveList<IntNode> list1;
    IntrusiveList<IntNode> list2(list1);  // copy constructor does nothing
    EXPECT_TRUE(list2.empty());
}

TEST(IntrusiveListTest, CopyAssignmentIsIgnored) {
    IntrusiveList<IntNode> list1;
    IntrusiveList<IntNode> list2;
    list2 = list1;  // assignment does nothing
    EXPECT_TRUE(list2.empty());
}

// ----------------------------------------------------------------------------
// push_front, push_back, front, back, size, iteration.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, PushFrontAndPopFront) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);

    list.push_front(n1);
    list.push_front(n2);
    list.push_front(n3);

    EXPECT_EQ(list.size(), 3u);
    EXPECT_EQ(list.front().value, 3);
    EXPECT_EQ(list.back().value, 1);

    list.pop_front();
    EXPECT_EQ(list.size(), 2u);
    EXPECT_EQ(list.front().value, 2);
    list.pop_front();
    EXPECT_EQ(list.front().value, 1);
    list.pop_front();
    EXPECT_TRUE(list.empty());
}

TEST(IntrusiveListTest, PushBackAndPopBack) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);

    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);

    EXPECT_EQ(list.size(), 3u);
    EXPECT_EQ(list.front().value, 1);
    EXPECT_EQ(list.back().value, 3);

    list.pop_back();
    EXPECT_EQ(list.size(), 2u);
    EXPECT_EQ(list.back().value, 2);
    list.pop_back();
    EXPECT_EQ(list.back().value, 1);
    list.pop_back();
    EXPECT_TRUE(list.empty());
}

TEST(IntrusiveListTest, IterationForward) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);

    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (auto it = list.begin(); it != list.end(); ++it) {
        actual.push_back(it->value);
    }
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, IterationReverse) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);

    std::vector<int> expected = {3, 2, 1};
    std::vector<int> actual;
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        actual.push_back(it->value);
    }
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, ConstIterators) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2);
    list.push_back(n1);
    list.push_back(n2);

    const auto& const_list = list;
    std::vector<int> values;
    for (auto it = const_list.cbegin(); it != const_list.cend(); ++it) {
        values.push_back(it->value);
    }
    EXPECT_EQ(values, (std::vector<int>{1, 2}));
    list.clear();
}

// ----------------------------------------------------------------------------
// Insert and erase.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, InsertBefore) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3), n4(4);
    list.push_back(n1);
    list.push_back(n3);

    auto it = list.begin();
    ++it;  // points to n3
    auto inserted = list.insert(it, n2);
    EXPECT_EQ(inserted->value, 2);

    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, EraseSingle) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);

    auto it = list.begin();
    ++it;  // points to n2
    auto next = list.erase(it);
    EXPECT_EQ(next->value, 3);
    EXPECT_EQ(list.size(), 2u);

    std::vector<int> expected = {1, 3};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, EraseRange) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3), n4(4);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);
    list.push_back(n4);

    auto first = list.begin();
    ++first;  // points to n2
    auto last = list.end();
    --last;   // points to n4
    auto next = list.erase(first, last);
    EXPECT_EQ(next->value, 4);
    EXPECT_EQ(list.size(), 2u);

    std::vector<int> expected = {1, 4};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

// ----------------------------------------------------------------------------
// Splice operations.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, SpliceSingleElement) {
    IntrusiveList<IntNode> list1, list2;
    IntNode n1(1), n2(2), n3(3);
    list1.push_back(n1);
    list1.push_back(n3);
    list2.push_back(n2);

    auto pos = list1.begin();
    ++pos;  // after n1, before n3
    list1.splice(pos, n2);  // move n2 from list2 to list1

    EXPECT_TRUE(list2.empty());
    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (const auto& node : list1) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list1.clear();
    list2.clear();
}

TEST(IntrusiveListTest, SpliceWholeList) {
    IntrusiveList<IntNode> list1, list2;
    IntNode a(1), b(2);
    IntNode c(3), d(4);
    list1.push_back(a);
    list1.push_back(b);
    list2.push_back(c);
    list2.push_back(d);

    auto pos = list1.begin();
    ++pos;  // after a, before b
    list1.splice(pos, list2);

    EXPECT_TRUE(list2.empty());
    std::vector<int> expected = {1, 3, 4, 2};
    std::vector<int> actual;
    for (const auto& node : list1) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list1.clear();
    list2.clear();
}

TEST(IntrusiveListTest, SpliceElementFromAnotherList) {
    IntrusiveList<IntNode> list1, list2;
    IntNode n1(1), n2(2), n3(3);
    list1.push_back(n1);
    list1.push_back(n3);
    list2.push_back(n2);

    auto pos = list1.begin();
    ++pos;  // after n1
    auto it = list2.begin();
    list1.splice(pos, list2, it);  // move n2 from list2 to list1

    EXPECT_TRUE(list2.empty());
    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (const auto& node : list1) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list1.clear();
    list2.clear();
}

    TEST(IntrusiveListTest, SpliceRange) {
    IntrusiveList<IntNode> list1, list2;
    IntNode a(1), b(2), c(3), d(4), e(5);
    list1.push_back(a);
    list1.push_back(e);
    list2.push_back(b);
    list2.push_back(c);
    list2.push_back(d);

    auto pos = list1.begin();
    ++pos;  // after a, before e
    auto first = list2.begin();
    auto last = list2.end();
    --last;  // points to d
    list1.splice(pos, list2, first, last);  // move b,c (not d)

    EXPECT_EQ(list2.size(), 1u);
    EXPECT_EQ(list2.front().value, 4);      // d remains, value is 4
    std::vector<int> expected = {1, 2, 3, 5};
    std::vector<int> actual;
    for (const auto& node : list1) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list1.clear();
    list2.clear();
}

// ----------------------------------------------------------------------------
// remove (static)
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, StaticRemove) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);

    IntrusiveList<IntNode>::remove(n2);  // remove n2 from whichever list it belongs to

    std::vector<int> expected = {1, 3};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    // n2 should have null pointers
    EXPECT_EQ(n2.mpNext, nullptr);
    EXPECT_EQ(n2.mpPrev, nullptr);
    list.clear();
}

// ----------------------------------------------------------------------------
// contains and locate.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, ContainsAndLocate) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3);
    list.push_back(n1);
    list.push_back(n2);

    EXPECT_TRUE(list.contains(n1));
    EXPECT_TRUE(list.contains(n2));
    EXPECT_FALSE(list.contains(n3));

    auto it = list.locate(n2);
    EXPECT_EQ(it->value, 2);
    auto const_it = list.locate(n1);
    EXPECT_EQ(const_it->value, 1);
    list.clear();
}

// ----------------------------------------------------------------------------
// swap.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, Swap) {
    IntrusiveList<IntNode> list1, list2;
    IntNode a(1), b(2);
    IntNode c(3), d(4);
    list1.push_back(a);
    list1.push_back(b);
    list2.push_back(c);
    list2.push_back(d);

    list1.swap(list2);

    EXPECT_EQ(list1.size(), 2u);
    EXPECT_EQ(list1.front().value, 3);
    EXPECT_EQ(list2.front().value, 1);
    list1.clear();
    list2.clear();
}

// ----------------------------------------------------------------------------
// sort and merge.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, SortAscending) {
    IntrusiveList<IntNode> list;
    IntNode n3(3), n1(1), n4(4), n2(2);
    list.push_back(n3);
    list.push_back(n1);
    list.push_back(n4);
    list.push_back(n2);

    list.sort();

    std::vector<int> expected = {1, 2, 3, 4};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, SortDescending) {
    IntrusiveList<IntNode> list;
    IntNode n3(3), n1(1), n4(4), n2(2);
    list.push_back(n3);
    list.push_back(n1);
    list.push_back(n4);
    list.push_back(n2);

    list.sort([](const IntNode& a, const IntNode& b) { return a.value > b.value; });

    std::vector<int> expected = {4, 3, 2, 1};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, Merge) {
    IntrusiveList<IntNode> list1, list2;
    IntNode a(1), c(3), e(5);
    IntNode b(2), d(4), f(6);
    list1.push_back(a);
    list1.push_back(c);
    list1.push_back(e);
    list2.push_back(b);
    list2.push_back(d);
    list2.push_back(f);

    list1.merge(list2);  // merges sorted lists (both are already sorted)

    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    std::vector<int> actual;
    for (const auto& node : list1) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    EXPECT_TRUE(list2.empty());
    list1.clear();
    list2.clear();
}

TEST(IntrusiveListTest, MergeWithComparator) {
    IntrusiveList<StringNode> list1, list2;
    StringNode s1("apple"), s3("carrot"), s5("egg");
    StringNode s2("banana"), s4("date"), s6("fig");
    list1.push_back(s1);
    list1.push_back(s3);
    list1.push_back(s5);
    list2.push_back(s2);
    list2.push_back(s4);
    list2.push_back(s6);

    list1.merge(list2, [](const StringNode& a, const StringNode& b) { return a.value < b.value; });

    std::vector<std::string> expected = {"apple", "banana", "carrot", "date", "egg", "fig"};
    std::vector<std::string> actual;
    for (const auto& node : list1) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list1.clear();
    list2.clear();
}

// ----------------------------------------------------------------------------
// unique.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, UniqueAdjacent) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(1), n3(2), n4(2), n5(2), n6(3);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);
    list.push_back(n4);
    list.push_back(n5);
    list.push_back(n6);

    list.unique();

    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

TEST(IntrusiveListTest, UniqueWithPredicate) {
    // Use predicate based on absolute value.
    struct AbsNode : public IntrusiveListNode {
        int value;
        explicit AbsNode(int v) : value(v) {}
    };
    IntrusiveList<AbsNode> list;
    AbsNode n1(-1), n2(1), n3(-2), n4(2), n5(3);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);
    list.push_back(n4);
    list.push_back(n5);

    list.unique([](const AbsNode& a, const AbsNode& b) { return std::abs(a.value) == std::abs(b.value); });

    std::vector<int> expected = {-1, -2, 3};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

// ----------------------------------------------------------------------------
// reverse.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, Reverse) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2), n3(3), n4(4);
    list.push_back(n1);
    list.push_back(n2);
    list.push_back(n3);
    list.push_back(n4);

    list.reverse();

    std::vector<int> expected = {4, 3, 2, 1};
    std::vector<int> actual;
    for (const auto& node : list) actual.push_back(node.value);
    EXPECT_EQ(actual, expected);
    list.clear();
}

// ----------------------------------------------------------------------------
// clear.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, Clear) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2);
    list.push_back(n1);
    list.push_back(n2);
    EXPECT_EQ(list.size(), 2u);

    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0u);
    // Nodes should have null pointers after clear.
    EXPECT_EQ(n1.mpNext, nullptr);
    EXPECT_EQ(n1.mpPrev, nullptr);
    EXPECT_EQ(n2.mpNext, nullptr);
    EXPECT_EQ(n2.mpPrev, nullptr);
    list.clear();
}

// ----------------------------------------------------------------------------
// Node can be in multiple lists via multiple inheritance.
// ----------------------------------------------------------------------------
    TEST(IntrusiveListTest, NodeInMultipleLists) {
    struct ListNodeA : public IntrusiveListNode {};
    struct ListNodeB : public IntrusiveListNode {};
    struct Object : public ListNodeA, public ListNodeB {
        int value;
        explicit Object(int v) : value(v) {}
    };

    IntrusiveList<ListNodeA> listA;
    IntrusiveList<ListNodeB> listB;
    Object obj(42);

    listA.push_back(obj);
    listB.push_back(obj);

    EXPECT_EQ(listA.size(), 1u);
    EXPECT_EQ(listB.size(), 1u);

    // Retrieve from listA and cast to Object& to access value.
    ListNodeA& nodeA = listA.front();
    Object& objFromA = static_cast<Object&>(nodeA);
    EXPECT_EQ(objFromA.value, 42);

    // Similarly for listB.
    ListNodeB& nodeB = listB.front();
    Object& objFromB = static_cast<Object&>(nodeB);
    EXPECT_EQ(objFromB.value, 42);
    listA.clear();
    listB.clear();
}
// ----------------------------------------------------------------------------
// Validation.
// ----------------------------------------------------------------------------

TEST(IntrusiveListTest, Validate) {
    IntrusiveList<IntNode> list;
    IntNode n1(1), n2(2);
    list.push_back(n1);
    list.push_back(n2);
    EXPECT_TRUE(list.validate());
    // Deliberately corrupt the list (should still be valid if we don't break it).
    // No easy way to test false case without violating invariants; we trust the implementation.
    list.clear();
}

} // namespace
} // namespace fermat