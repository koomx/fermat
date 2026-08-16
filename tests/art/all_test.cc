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

#include <fermat/art/art.h>
#include <gtest/gtest.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <utility>
#include <vector>

namespace fermat {

using namespace std;
using art_internal::LeafNode;
using art_internal::node_4;
using art_internal::node_16;
using art_internal::node_48;
using art_internal::node_256;
template <class T>
using node = art_internal::Node<T>;

TEST(InnerNode, iteration) {
    node_4<void*> m;

    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);

    m.set_child(0, &n0);
    m.set_child(5, &n1);
    m.set_child(6, &n2);
    m.set_child(127, &n3);

    auto it = m.begin();
    auto it_end = m.end();

    // 0
    ASSERT_TRUE(it < it_end);
    ASSERT_TRUE(it <= it_end);
    ASSERT_TRUE(it_end > it);
    ASSERT_TRUE(it_end >= it);
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(0, *it);

    ++it;
    // 1
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(5, *it);

    ++it;
    // 2
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(6, *it);

    ++it;
    // 3
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(127, *it);

    ++it;
    // 4 (overflow)
    ASSERT_TRUE(it == it_end);
    ASSERT_TRUE(it <= it_end);
    ASSERT_TRUE(it >= it_end);

    --it;
    // 3
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(127, *it);

    --it;
    // 2
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(6, *it);

    --it;
    // 1
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(5, *it);

    --it;
    // 0
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(0, *it);

    --it;
    // -1 (underflow)
    ASSERT_TRUE(it < it_end);

    ++it;
    // 0
    ASSERT_TRUE(it < it_end);
    ASSERT_EQ(0, *it);
  }
TEST(InnerNode, reverse_iteration) {
    node_4<void*> m;

    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);

    m.set_child(0, &n0);
    m.set_child(5, &n1);
    m.set_child(6, &n2);
    m.set_child(127, &n3);

    auto it = m.rbegin();
    auto it_end = m.rend();

    // 0
    ASSERT_TRUE(it < it_end);
    ASSERT_TRUE(it <= it_end);
    ASSERT_TRUE(it_end > it);
    ASSERT_TRUE(it_end >= it);
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(127, (int) *it);

    ++it;
    // 1
    ASSERT_EQ(6, (int) *it);

    ++it;
    // 2
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(5, (int) *it);

    ++it;
    // 3
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(0, *it);

    ++it;
    // 4 (overflow)
    ASSERT_TRUE(it == it_end);
    ASSERT_TRUE(it <= it_end);
    ASSERT_TRUE(it >= it_end);

    --it;
    // 3
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(0, *it);

    --it;
    // 2
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(5, (int) *it);

    --it;
    // 1
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(6, (int) *it);

    --it;
    // 0
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(127, (int) *it);

    --it;
    // -1 (underflow)
    ASSERT_TRUE(it != it_end);

    ++it;
    // 0
    ASSERT_TRUE(it != it_end);
    ASSERT_EQ(127, (int) *it);
  }
TEST(ArtNode, check_prefix) {
        LeafNode<int*> node(nullptr);
        string key = "000100001";
        int key_len = key.length() + 1; // +1 for \0
        string prefix = "0000";
        int prefix_len = prefix.length() + 1; // +1 for \0

        node.prefix_ = (char *) prefix.c_str();
        node.prefix_len_ = prefix_len;

        EXPECT_EQ(3, node.check_prefix(key.c_str() + 0, key_len - 0));
        EXPECT_EQ(2, node.check_prefix(key.c_str() + 1, key_len - 1));
        EXPECT_EQ(1, node.check_prefix(key.c_str() + 2, key_len - 2));
        EXPECT_EQ(0, node.check_prefix(key.c_str() + 3, key_len - 3));
        EXPECT_EQ(4, node.check_prefix(key.c_str() + 4, key_len - 4));
        EXPECT_EQ(3, node.check_prefix(key.c_str() + 5, key_len - 5));
        EXPECT_EQ(2, node.check_prefix(key.c_str() + 6, key_len - 6));
        EXPECT_EQ(1, node.check_prefix(key.c_str() + 7, key_len - 7));
        EXPECT_EQ(0, node.check_prefix(key.c_str() + 8, key_len - 8));
        EXPECT_EQ(0, node.check_prefix(key.c_str() + 9, key_len - 9));
    }
TEST(ArtNode4, monte_carlo_insert) {
    /* set up */
    array<uint8_t, 256> partial_keys;
    array<node<void*> *, 256> children;

    for (int i = 0; i < 256; i += 1) {
      /* populate partial_keys with all values in the partial_keys_t domain */
      partial_keys[i] = i;

      /* populate child nodes */
      children[i] = new LeafNode<void*>(nullptr);
    }

    /* rng */
    random_device rd;
    mt19937 g(rd());

    for (int experiment = 0; experiment < 10000; experiment += 1) {
      /* test subject */
      node_4<void*> node;

      /* shuffle in order to make a seemingly random insertion order */
      shuffle(partial_keys.begin(), partial_keys.end(), g);
      for (int i = 0; i < 4; i += 1) {
        ASSERT_FALSE(node.is_full());

        auto partial_key = partial_keys[i];
        auto child = children[partial_key];
        node.set_child(partial_key, child);

        for (int j = 0; j <= i; j += 1) {
          auto p_k = partial_keys[j];
          auto expected_child = children[p_k];
          auto actual_child_ptr = node.find_child(p_k);
          ASSERT_TRUE(actual_child_ptr != nullptr);
          auto actual_child = *actual_child_ptr;
          ASSERT_EQ(expected_child, actual_child);
        }
      }
      ASSERT_TRUE(node.is_full());
    }

    /* tear down */
    for (int i = 0; i < 256; i += 1) {
      delete children[i];
    }
  }
TEST(ArtNode4, delete_child_delete_child_that_doesn_t_exist_0) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(0) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode4, delete_child_delete_min_1) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(1) == nullptr);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode4, delete_child_delete_inner_2) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(2) == nullptr);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode4, delete_child_delete_child_that_doesn_t_exist_3) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(3) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode4, delete_child_delete_inner_4) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(subject.find_child(4) == nullptr);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode4, delete_child_delete_max_5) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(5) == &n5);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(subject.find_child(5) == nullptr);
    }
TEST(ArtNode4, delete_child_delete_child_that_doesn_t_exist_6) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_4<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(6) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode4, next_partial_key_completely_empty_node) {
    node_4<void*> n;

    
      ASSERT_THROW(n.next_partial_key(0), std::out_of_range);
    }
TEST(ArtNode4, next_partial_key_child_at_128) {
    node_4<void*> n;

    
      n.set_child(-128, nullptr);
      ASSERT_EQ(-128, n.next_partial_key(-128));
      for (int i = 1; i < 256; ++i) {
        ASSERT_THROW(n.next_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode4, next_partial_key_child_at_127) {
    node_4<void*> n;

    
      n.set_child(127, nullptr);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(127, n.next_partial_key(i - 128));
      }
    }
TEST(ArtNode4, next_partial_key_dense_children) {
    node_4<void*> n;

    
      n.set_child(0, nullptr);
      n.set_child(1, nullptr);
      n.set_child(2, nullptr);
      n.set_child(3, nullptr);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(1, n.next_partial_key(1));
      ASSERT_EQ(2, n.next_partial_key(2));
      ASSERT_EQ(3, n.next_partial_key(3));
      ASSERT_THROW(n.next_partial_key(4), std::out_of_range);
    }
TEST(ArtNode4, next_partial_key_sparse_children) {
    node_4<void*> n;

    
      n.set_child(0, nullptr);
      n.set_child(5, nullptr);
      n.set_child(10, nullptr);
      n.set_child(100, nullptr);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(5, n.next_partial_key(1));
      ASSERT_EQ(10, n.next_partial_key(6));
      ASSERT_EQ(100, n.next_partial_key(11));
      ASSERT_THROW(n.next_partial_key(101), std::out_of_range);
    }
TEST(ArtNode4, previous_partial_key_completely_empty_node) {
    node_4<void*> n;

    
      ASSERT_THROW(n.prev_partial_key(127), std::out_of_range);
    }
TEST(ArtNode4, previous_partial_key_child_at_128) {
    node_4<void*> n;

    
      n.set_child(-128, nullptr);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(-128, n.prev_partial_key(i - 128));
      }
    }
TEST(ArtNode4, previous_partial_key_child_at_127) {
    node_4<void*> n;

    
      n.set_child(127, nullptr);
      ASSERT_EQ(127, n.prev_partial_key(127));
      for (int i = 0; i < 255; ++i) {
        ASSERT_THROW(n.prev_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode4, previous_partial_key_dense_children) {
    node_4<void*> n;

    
      n.set_child(1, nullptr);
      n.set_child(2, nullptr);
      n.set_child(3, nullptr);
      n.set_child(4, nullptr);
      ASSERT_EQ(1, n.prev_partial_key(1));
      ASSERT_EQ(2, n.prev_partial_key(2));
      ASSERT_EQ(3, n.prev_partial_key(3));
      ASSERT_EQ(4, n.prev_partial_key(4));
      ASSERT_EQ(4, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode4, previous_partial_key_sparse_children) {
    node_4<void*> n;

    
      n.set_child(1, nullptr);
      n.set_child(5, nullptr);
      n.set_child(10, nullptr);
      n.set_child(100, nullptr);
      ASSERT_EQ(1, n.prev_partial_key(4));
      ASSERT_EQ(5, n.prev_partial_key(9));
      ASSERT_EQ(10, n.prev_partial_key(99));
      ASSERT_EQ(100, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode16, monte_carlo) {
    /* set up */
    array<uint8_t, 256> partial_keys;
    array<node<void*> *, 256> children;

    for (int i = 0; i < 256; i += 1) {
      /* populate partial_keys with all values in the partial_keys_t domain */
      partial_keys[i] = i;

      /* populate child nodes */
      children[i] = new LeafNode<void*>(nullptr);
    }

    /* rng */
    random_device rd;
    mt19937 g(rd());

    for (int experiment = 0; experiment < 1000; experiment += 1) {
      /* test subject */
      node_16<void*> node;

      /* shuffle in order to make a seemingly random insertion order */
      shuffle(partial_keys.begin(), partial_keys.end(), g);
      for (int i = 0; i < 16; i += 1) {
        ASSERT_FALSE(node.is_full());

        auto partial_key = partial_keys[i];
        auto child = children[partial_key];
        node.set_child(partial_key, child);

        for (int j = 0; j <= i; j += 1) {
          auto p_k = partial_keys[j];
          auto expected_child = children[p_k];
          auto actual_child_ptr = node.find_child(p_k);
          ASSERT_TRUE(actual_child_ptr != nullptr);
          auto actual_child = *actual_child_ptr;
          ASSERT_EQ(expected_child, actual_child);
        }
      }
      ASSERT_TRUE(node.is_full());
    }

    /* tear down */
    for (int i = 0; i < 256; i += 1) {
      delete children[i];
    }
  }
TEST(ArtNode16, delete_child_delete_child_that_doesn_t_exist_0) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(0) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode16, delete_child_delete_min_1) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(1) == nullptr);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode16, delete_child_delete_inner_2) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(2) == nullptr);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode16, delete_child_delete_child_that_doesn_t_exist_3) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(3) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode16, delete_child_delete_inner_4) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(subject.find_child(4) == nullptr);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode16, delete_child_delete_max_5) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(5) == &n5);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(subject.find_child(5) == nullptr);
    }
TEST(ArtNode16, delete_child_delete_child_that_doesn_t_exist_6) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_16<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(6) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode16, next_partial_key_completely_empty_node) {
    node_16<void*> n;

    
      ASSERT_THROW(n.next_partial_key(0), std::out_of_range);
    }
TEST(ArtNode16, next_partial_key_child_at_128) {
    node_16<void*> n;

    
      n.set_child(-128, nullptr);
      ASSERT_EQ(-128, n.next_partial_key(-128));
      for (int i = 1; i < 256; ++i) {
        ASSERT_THROW(n.next_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode16, next_partial_key_child_at_127) {
    node_16<void*> n;

    
      n.set_child(127, nullptr);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(127, n.next_partial_key(i - 128));
      }
    }
TEST(ArtNode16, next_partial_key_dense_children) {
    node_16<void*> n;

    
      n.set_child(0, nullptr);
      n.set_child(1, nullptr);
      n.set_child(2, nullptr);
      n.set_child(3, nullptr);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(1, n.next_partial_key(1));
      ASSERT_EQ(2, n.next_partial_key(2));
      ASSERT_EQ(3, n.next_partial_key(3));
      ASSERT_THROW(n.next_partial_key(4), std::out_of_range);
    }
TEST(ArtNode16, next_partial_key_sparse_children) {
    node_16<void*> n;

    
      n.set_child(0, nullptr);
      n.set_child(5, nullptr);
      n.set_child(10, nullptr);
      n.set_child(100, nullptr);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(5, n.next_partial_key(1));
      ASSERT_EQ(10, n.next_partial_key(6));
      ASSERT_EQ(100, n.next_partial_key(11));
      ASSERT_THROW(n.next_partial_key(101), std::out_of_range);
    }
TEST(ArtNode16, previous_partial_key_completely_empty_node) {
    node_16<void*> n;

    
      ASSERT_THROW(n.prev_partial_key(127), std::out_of_range);
    }
TEST(ArtNode16, previous_partial_key_child_at_128) {
    node_16<void*> n;

    
      n.set_child(-128, nullptr);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(-128, n.prev_partial_key(i - 128));
      }
    }
TEST(ArtNode16, previous_partial_key_child_at_127) {
    node_16<void*> n;

    
      n.set_child(127, nullptr);
      ASSERT_EQ(127, n.prev_partial_key(127));
      for (int i = 0; i < 255; ++i) {
        ASSERT_THROW(n.prev_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode16, previous_partial_key_dense_children) {
    node_16<void*> n;

    
      n.set_child(1, nullptr);
      n.set_child(2, nullptr);
      n.set_child(3, nullptr);
      n.set_child(4, nullptr);
      ASSERT_EQ(1, n.prev_partial_key(1));
      ASSERT_EQ(2, n.prev_partial_key(2));
      ASSERT_EQ(3, n.prev_partial_key(3));
      ASSERT_EQ(4, n.prev_partial_key(4));
      ASSERT_EQ(4, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode16, previous_partial_key_sparse_children) {
    node_16<void*> n;

    
      n.set_child(1, nullptr);
      n.set_child(5, nullptr);
      n.set_child(10, nullptr);
      n.set_child(100, nullptr);
      ASSERT_EQ(1, n.prev_partial_key(4));
      ASSERT_EQ(5, n.prev_partial_key(9));
      ASSERT_EQ(10, n.prev_partial_key(99));
      ASSERT_EQ(100, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode16, grow_to_node_48_preserves_offset_PR_20_next_partial_key_after_grow) {
    // This test reproduces the bug reported in PR #20:
    // When node_16 grows to node_48, the indexes_ array must use 128 offset

    
      LeafNode<void*>* dummy_children[17];
      for (int i = 0; i < 17; ++i) {
        dummy_children[i] = new LeafNode<void*>(nullptr);
      }
      node_16<void*>* n16 = new node_16<void*>();
      char test_keys[17];
      for (int i = 0; i < 17; ++i) {
        test_keys[i] = 'a' + i; // a, b, c, ..., q
      }
      for (int i = 0; i < 16; ++i) {
        n16->set_child(test_keys[i], dummy_children[i]);
      }
      ASSERT_TRUE(n16->is_full());
      auto* n48 = static_cast<node_48<void*>*>(n16->grow());
      ASSERT_TRUE(n48 != nullptr);
      for (int i = 0; i < 16; ++i) {
        auto** child_ptr = n48->find_child(test_keys[i]);
        ASSERT_TRUE(child_ptr != nullptr);
        ASSERT_TRUE(*child_ptr == dummy_children[i]);
      }
      n48->set_child(test_keys[16], dummy_children[16]);
      for (int i = 0; i < 17; ++i) {
        auto** child_ptr = n48->find_child(test_keys[i]);
        ASSERT_TRUE(child_ptr != nullptr);
        ASSERT_TRUE(*child_ptr == dummy_children[i]);
      }
      // Starting from first key 'a', we should find all keys in order
      char current = n48->next_partial_key('a');
      ASSERT_EQ('a', current);
      for (int i = 1; i < 17; ++i) {
        current = n48->next_partial_key(current + 1);
        ASSERT_EQ(test_keys[i], current);
      }
      delete n48;
      for (int i = 0; i < 17; ++i) {
        delete dummy_children[i];
      }
    }
TEST(ArtNode16, grow_to_node_48_preserves_offset_PR_20_prev_partial_key_after_grow) {
    // This test reproduces the bug reported in PR #20:
    // When node_16 grows to node_48, the indexes_ array must use 128 offset

    
      LeafNode<void*>* dummy_children[17];
      for (int i = 0; i < 17; ++i) {
        dummy_children[i] = new LeafNode<void*>(nullptr);
      }
      node_16<void*>* n16 = new node_16<void*>();
      char test_keys[17];
      for (int i = 0; i < 17; ++i) {
        test_keys[i] = 'a' + i; // a, b, c, ..., q
      }
      for (int i = 0; i < 16; ++i) {
        n16->set_child(test_keys[i], dummy_children[i]);
      }
      ASSERT_TRUE(n16->is_full());
      auto* n48 = static_cast<node_48<void*>*>(n16->grow());
      ASSERT_TRUE(n48 != nullptr);
      for (int i = 0; i < 16; ++i) {
        auto** child_ptr = n48->find_child(test_keys[i]);
        ASSERT_TRUE(child_ptr != nullptr);
        ASSERT_TRUE(*child_ptr == dummy_children[i]);
      }
      n48->set_child(test_keys[16], dummy_children[16]);
      for (int i = 0; i < 17; ++i) {
        auto** child_ptr = n48->find_child(test_keys[i]);
        ASSERT_TRUE(child_ptr != nullptr);
        ASSERT_TRUE(*child_ptr == dummy_children[i]);
      }
      // Starting from last key 'q', we should find all keys in reverse order
      char current = n48->prev_partial_key('q');
      ASSERT_EQ('q', current);
      for (int i = 15; i >= 0; --i) {
        current = n48->prev_partial_key(current - 1);
        ASSERT_EQ(test_keys[i], current);
      }
      delete n48;
      for (int i = 0; i < 17; ++i) {
        delete dummy_children[i];
      }
    }
TEST(ArtNode48, monte_carlo) {
    /* set up */
    array<char, 256> partial_keys;
    array<node<void*> *, 256> children;

    for (int i = 0; i < 256; i += 1) {
      /* populate partial_keys with all values in the partial_keys_t domain */
      partial_keys[i] = i - 128;

      /* populate child nodes */
      children[i] = new LeafNode<void*>(nullptr);
    }

    /* rng */
    mt19937 g(0);

    for (int experiment = 0; experiment < 10000; experiment += 1) {
      /* test subject */
      node_48<void*> node;

      /* shuffle in order to make a seemingly random insertion order */
      shuffle(partial_keys.begin(), partial_keys.end(), g);

      for (int i = 0; i < 48; i += 1) {
        ASSERT_FALSE(node.is_full());

        auto partial_key = partial_keys[i];
        auto child = children[i];
        node.set_child(partial_key, child);

        for (int j = 0; j <= i; j += 1) {
          auto p_k = partial_keys[j];
          auto expected_child = children[j];
          auto actual_child_ptr = node.find_child(p_k);
          ASSERT_TRUE(actual_child_ptr != nullptr);
          auto actual_child = *actual_child_ptr;
          ASSERT_EQ(expected_child, actual_child);
        }
      }
      ASSERT_TRUE(node.is_full());
    }

    /* tear down */
    for (int i = 0; i < 256; i += 1) {
      delete children[i];
    }
  }
TEST(ArtNode48, delete_child_delete_child_that_doesn_t_exist_0) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(0) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode48, delete_child_delete_min_1) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(1) == nullptr);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode48, delete_child_delete_inner_2) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(2) == nullptr);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode48, delete_child_delete_child_that_doesn_t_exist_3) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(3) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode48, delete_child_delete_inner_4) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(subject.find_child(4) == nullptr);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode48, delete_child_delete_max_5) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(5) == &n5);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(subject.find_child(5) == nullptr);
    }
TEST(ArtNode48, delete_child_delete_child_that_doesn_t_exist_6) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_48<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(6) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode48, next_partial_key_completely_empty_node) {
    node_48<void*> n;

    
      for (int i = 0; i < 256; ++i) {
        ASSERT_THROW(n.next_partial_key(-128), std::out_of_range);
      }
    }
TEST(ArtNode48, next_partial_key_child_at_128) {
    node_48<void*> n;

    
      n.set_child(-128, nullptr);
      ASSERT_EQ(-128, n.next_partial_key(-128));
      for (int i = 1; i < 256; ++i) {
        ASSERT_THROW(n.next_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode48, next_partial_key_child_at_127) {
    node_48<void*> n;

    
      n.set_child(127, nullptr);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(127, n.next_partial_key(i - 128));
      }
    }
TEST(ArtNode48, next_partial_key_dense_children) {
    node_48<void*> n;

    
      n.set_child(0, nullptr);
      n.set_child(1, nullptr);
      n.set_child(2, nullptr);
      n.set_child(3, nullptr);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(1, n.next_partial_key(1));
      ASSERT_EQ(2, n.next_partial_key(2));
      ASSERT_EQ(3, n.next_partial_key(3));
      ASSERT_THROW(n.next_partial_key(4), std::out_of_range);
    }
TEST(ArtNode48, next_partial_key_sparse_children) {
    node_48<void*> n;

    
      n.set_child(0, nullptr);
      n.set_child(5, nullptr);
      n.set_child(10, nullptr);
      n.set_child(100, nullptr);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(5, n.next_partial_key(1));
      ASSERT_EQ(10, n.next_partial_key(6));
      ASSERT_EQ(100, n.next_partial_key(11));
      ASSERT_THROW(n.next_partial_key(101), std::out_of_range);
    }
TEST(ArtNode48, previous_partial_key_completely_empty_node) {
    node_48<void*> n;

    
      ASSERT_THROW(n.prev_partial_key(127), std::out_of_range);
    }
TEST(ArtNode48, previous_partial_key_child_at_128) {
    node_48<void*> n;

    
      n.set_child(-128, nullptr);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(-128, n.prev_partial_key(i - 128));
      }
    }
TEST(ArtNode48, previous_partial_key_child_at_127) {
    node_48<void*> n;

    
      n.set_child(127, nullptr);
      ASSERT_EQ(127, n.prev_partial_key(127));
      for (int i = 0; i < 255; ++i) {
        ASSERT_THROW(n.prev_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode48, previous_partial_key_dense_children) {
    node_48<void*> n;

    
      n.set_child(1, nullptr);
      n.set_child(2, nullptr);
      n.set_child(3, nullptr);
      n.set_child(4, nullptr);
      ASSERT_EQ(1, n.prev_partial_key(1));
      ASSERT_EQ(2, n.prev_partial_key(2));
      ASSERT_EQ(3, n.prev_partial_key(3));
      ASSERT_EQ(4, n.prev_partial_key(4));
      ASSERT_EQ(4, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode48, previous_partial_key_sparse_children) {
    node_48<void*> n;

    
      n.set_child(1, nullptr);
      n.set_child(5, nullptr);
      n.set_child(10, nullptr);
      n.set_child(100, nullptr);
      ASSERT_EQ(1, n.prev_partial_key(4));
      ASSERT_EQ(5, n.prev_partial_key(9));
      ASSERT_EQ(10, n.prev_partial_key(99));
      ASSERT_EQ(100, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode256, monte_carlo) {
    /* set up */
    array<char, 256> partial_keys;
    array<node<void*> *, 256> children;

    for (int i = 0; i < 256; i += 1) {
      /* populate partial_keys with all values in the char domain */
      partial_keys[i] = i - 128;
      children[i] = new LeafNode<void*>(nullptr);
    }

    /* rng */
    mt19937 g(0);

    for (int experiment = 0; experiment < 100; experiment += 1) {
      /* test subject */
      node_256<void*> node;

      /* shuffle in order to make a seemingly random insertion order */
      shuffle(partial_keys.begin(), partial_keys.end(), g);
      for (int i = 0; i < 256; i += 1) {
        ASSERT_FALSE(node.is_full());

        auto partial_key = partial_keys[i];
        auto child = children[i];
        node.set_child(partial_key, child);

        for (int j = 0; j <= i; j += 1) {
          auto p_k = partial_keys[j];
          auto expected_child = children[j];
          auto actual_child_ptr = node.find_child(p_k);
          ASSERT_TRUE(actual_child_ptr != nullptr);
          auto actual_child = *actual_child_ptr;
          ASSERT_EQ(expected_child, actual_child);
        }
      }
      ASSERT_TRUE(node.is_full());
    }

    for (int i = 0; i < 256; ++i) {
      delete children[i];
    }
  }
TEST(ArtNode256, delete_child_delete_child_that_doesn_t_exist_0) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(0) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode256, delete_child_delete_min_1) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(1) == nullptr);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode256, delete_child_delete_inner_2) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(subject.find_child(2) == nullptr);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode256, delete_child_delete_child_that_doesn_t_exist_3) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(3) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode256, delete_child_delete_inner_4) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(subject.find_child(4) == nullptr);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode256, delete_child_delete_max_5) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(5) == &n5);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(subject.find_child(5) == nullptr);
    }
TEST(ArtNode256, delete_child_delete_child_that_doesn_t_exist_6) {
    LeafNode<void*> n0(nullptr);
    LeafNode<void*> n1(nullptr);
    LeafNode<void*> n2(nullptr);
    LeafNode<void*> n3(nullptr);
    LeafNode<void*> n4(nullptr);
    LeafNode<void*> n5(nullptr);
    LeafNode<void*> n6(nullptr);

    node_256<void*> subject;

    subject.set_child(1, &n1);
    subject.set_child(2, &n2);
    subject.set_child(4, &n4);
    subject.set_child(5, &n5);

    
      ASSERT_TRUE(subject.del_child(6) == nullptr);
      ASSERT_TRUE(*subject.find_child(1) == &n1);
      ASSERT_TRUE(*subject.find_child(2) == &n2);
      ASSERT_TRUE(*subject.find_child(4) == &n4);
      ASSERT_TRUE(*subject.find_child(5) == &n5);
    }
TEST(ArtNode256, next_partial_key_completely_empty_node) {
    node_256<void*> n;

    
      ASSERT_THROW(n.next_partial_key(-128), std::out_of_range);
    }
TEST(ArtNode256, next_partial_key_child_at_128) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      n.set_child(-128, &n0);
      ASSERT_EQ(-128, n.next_partial_key(-128));
      for (int i = 1; i < 256; ++i) {
        ASSERT_THROW(n.next_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode256, next_partial_key_child_at_127) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      n.set_child(127, &n0);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(127, n.next_partial_key(i - 128));
      }
    }
TEST(ArtNode256, next_partial_key_dense_children) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      LeafNode<void*> n1(nullptr);
      LeafNode<void*> n2(nullptr);
      LeafNode<void*> n3(nullptr);
      n.set_child(0, &n0);
      n.set_child(1, &n1);
      n.set_child(2, &n2);
      n.set_child(3, &n3);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(1, n.next_partial_key(1));
      ASSERT_EQ(2, n.next_partial_key(2));
      ASSERT_EQ(3, n.next_partial_key(3));
      ASSERT_THROW(n.next_partial_key(4), std::out_of_range);
    }
TEST(ArtNode256, next_partial_key_sparse_children) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      LeafNode<void*> n1(nullptr);
      LeafNode<void*> n2(nullptr);
      LeafNode<void*> n3(nullptr);
      n.set_child(0, &n0);
      n.set_child(5, &n1);
      n.set_child(10, &n2);
      n.set_child(100, &n3);
      ASSERT_EQ(0, n.next_partial_key(0));
      ASSERT_EQ(5, n.next_partial_key(1));
      ASSERT_EQ(10, n.next_partial_key(6));
      ASSERT_EQ(100, n.next_partial_key(11));
      ASSERT_THROW(n.next_partial_key(101), std::out_of_range);
    }
TEST(ArtNode256, previous_partial_key_completely_empty_node) {
    node_256<void*> n;

    
      for (int i = 0; i < 256; ++i) {
        ASSERT_THROW(n.prev_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode256, previous_partial_key_child_at_128) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      n.set_child(-128, &n0);
      for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(-128, n.prev_partial_key(i - 128));
      }
    }
TEST(ArtNode256, previous_partial_key_child_at_127) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      n.set_child(127, &n0);
      ASSERT_EQ(127, n.prev_partial_key(127));
      for (int i = 0; i < 255; ++i) {
        ASSERT_THROW(n.prev_partial_key(i - 128), std::out_of_range);
      }
    }
TEST(ArtNode256, previous_partial_key_dense_children) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      LeafNode<void*> n1(nullptr);
      LeafNode<void*> n2(nullptr);
      LeafNode<void*> n3(nullptr);
      n.set_child(1, &n0);
      n.set_child(2, &n1);
      n.set_child(3, &n2);
      n.set_child(4, &n3);
      ASSERT_EQ(1, n.prev_partial_key(1));
      ASSERT_EQ(2, n.prev_partial_key(2));
      ASSERT_EQ(3, n.prev_partial_key(3));
      ASSERT_EQ(4, n.prev_partial_key(4));
      ASSERT_EQ(4, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtNode256, previous_partial_key_sparse_children) {
    node_256<void*> n;

    
      LeafNode<void*> n0(nullptr);
      LeafNode<void*> n1(nullptr);
      LeafNode<void*> n2(nullptr);
      LeafNode<void*> n3(nullptr);
      n.set_child(1, &n0);
      n.set_child(5, &n1);
      n.set_child(10, &n2);
      n.set_child(100, &n3);
      ASSERT_EQ(1, n.prev_partial_key(4));
      ASSERT_EQ(5, n.prev_partial_key(9));
      ASSERT_EQ(10, n.prev_partial_key(99));
      ASSERT_EQ(100, n.prev_partial_key(127));
      ASSERT_THROW(n.prev_partial_key(0), std::out_of_range);
    }
TEST(ArtIterator, single_element_iteration_iterate_over_single_element) {
    
      int value = 42;
      Art<int*> m;

      m.set("key1", &value);

      auto it = m.begin();
      auto it_end = m.end();

      // First element should be accessible
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&value, *it);
      ASSERT_EQ("key1", it.key());

      // Increment past the only element
      ++it;

      // Should now be at end
      ASSERT_TRUE(it == it_end);
    }
TEST(ArtIterator, single_element_iteration_iterate_over_single_element_with_for_loop) {
    
      int value = 1;
      Art<int*> m;

      m.set("key1", &value);

      int count = 0;
      for(auto itor = m.begin(); itor != m.end(); ++itor) {
        ASSERT_EQ("key1", itor.key());
        ASSERT_EQ(&value, *itor);
        ++count;
      }

      ASSERT_EQ(1, count);
    }
TEST(ArtIterator, full_lexicographic_traversal_controlled_test) {
    
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 4;
      int int4 = 5;
      int int5 = 5;
      int int6 = 6;

      Art<int*> m;

      m.set("aa", &int0);
      m.set("aaaa", &int1);
      m.set("aaaaaaa", &int2);
      m.set("aaaaaaaaaa", &int3);
      m.set("aaaaaaaba", &int4);
      m.set("aaaabaa", &int5);
      m.set("aaaabaaaaa", &int6);

      /* The above statements construct the following tree:
       *
       *          (aa)
       *   $_____/ |a
       *   /       |
       *  ()->0   (a)
       *   $_____/ |a\____________b
       *   /       |              \
       *  ()->1   (aa)            (aa)
       *   $_____/ |a\___b         |$\____a
       *   /       |     \         |      \
       *  ()->2 (aa$)->3 (a$)->4 ()->5 (aa$)->6
       *
       */

      auto it = m.begin();
      auto it_end = m.end();
      std::string key;
      key.reserve(20);

      // 0
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int0, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 3, "aa"));
      ASSERT_EQ("aa", it.key());

      ++it;
      // 1
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int1, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 5, "aaaa"));
      ASSERT_EQ("aaaa", it.key());

      ++it;
      // 2
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int2, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 8, "aaaaaaa"));
      ASSERT_EQ("aaaaaaa", it.key());

      ++it;
      // 3
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int3, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 11, "aaaaaaaaaa"));
      ASSERT_EQ("aaaaaaaaaa", it.key());

      ++it;
      // 4
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int4, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 10, "aaaaaaaba"));
      ASSERT_EQ("aaaaaaaba", it.key());

      ++it;
      // 5
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int5, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 8, "aaaabaa"));
      ASSERT_EQ("aaaabaa", it.key());

      ++it;
      // 6
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int6, *it);
      it.key(key.begin());
      ASSERT_TRUE(std::equal(key.begin(), key.begin() + 11, "aaaabaaaaa"));
      ASSERT_EQ("aaaabaaaaa", it.key());

      ++it;
      // 7 (overflow)
      ASSERT_TRUE(it == it_end);
    }
TEST(ArtIterator, full_lexicographic_traversal_tree_len) {
    
      int n = 0x10000;
      char key[5];
      int value;
      Art<int*> m;
      for (int i = 0; i < n; ++i) {
        std::snprintf(key, 5, "%04X", i);
        m.set(key, &value);
      }

      auto it = m.begin();
      auto it_end = m.end();
      int actual_n = 0;
      for (; it != it_end; ++it) {
          ++actual_n;
      }
      ASSERT_EQ(n, actual_n);
    }
TEST(ArtIterator, range_lexicographic_traversal_controlled_test) {
    
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 3;
      int int4 = 4;
      int int5 = 5;
      int int6 = 6;

      Art<int*> m;

      m.set("aa", &int0);
      m.set("aaaa", &int1);
      m.set("aaaaaaa", &int2);
      m.set("aaaaaaaaaa", &int3);
      m.set("aaaaaaaba", &int4);
      m.set("aaaabaa", &int5);
      m.set("aaaabaaaaa", &int6);

      /* The above statements construct the following tree:
       *
       *          (aa)
       *   $_____/ |a
       *   /       |
       *  ()->0   (a)
       *   $_____/ |a\____________b
       *   /       |              \
       *  ()->1   (aa)            (aa)
       *   $ ____/ |a\___b         |a\____$
       *    /      |     \         |      \
       *  ()->2 (aa$)->3 (a$)->4 ()->5 (aa$)->6
       *
       */

      // iterator on ["aaaaaaaaaa",3]
      auto it = m.begin("aaaaaaaaaa");

      // iterator on ["aaaabaaaaa",6]
      auto it_end = m.begin("aaaabaaaaa");

      // 3
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int3, *it);

      ++it;
      // 4
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int4, *it);

      ++it;
      // 5
      ASSERT_TRUE(it != it_end);
      ASSERT_EQ(&int5, *it);

      ++it;
      // 6
      ASSERT_TRUE(it == it_end);
    }
TEST(ArtIterator, range_lexicographic_traversal_monte_carlo) {
    
      mt19937_64 rng(0);
      int n_bytes = 4;
      int n = 1 << (n_bytes * 4);
      char keys[n][n_bytes + 1];
      int value;
      Art<int*> m;
      for (int i = 0; i < n; ++i) {
        std::snprintf(keys[i], n_bytes + 1, "%04X", i); // note: change format if you change n_bytes
        m.set(keys[i], &value);
      }
      for (int experiment = 0; experiment < 1000; ++experiment) {
        int start = rng() % n;
        int end;
        do {
          end = rng() % n;
        } while (end < start);
        auto it = m.begin(keys[start]);
        auto it_end = m.begin(keys[end]);
        int actual_n = 0;
        for (; it != it_end; ++it) {
            ++actual_n;
        }
        ASSERT_EQ(end - start, actual_n);
      }
    }

}  // namespace fermat
