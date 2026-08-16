/**
 * MIT License
 *
 * Copyright (c) 2017 Thibaut Goetghebuer-Planchon <tessil@gmx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

#include <fermat/trie/hat/htrie_set.h>
#include <tests/trire/hat/utils.h>

using SetTypes = ::testing::Types<fermat::htrie_set<char>>;

template <typename T>
class HtrieSetTyped : public ::testing::Test {};
TYPED_TEST_SUITE(HtrieSetTyped, SetTypes);

TYPED_TEST(HtrieSetTyped, Insert) {
  using TMap = TypeParam;
  using char_tt = typename TMap::char_type;

  const std::size_t nb_values = 100000;
  TMap set;
  typename TMap::iterator it;
  bool inserted;

  for (std::size_t i = 0; i < nb_values; i++) {
    std::tie(it, inserted) = set.insert(utils::get_key<char_tt>(i));

    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_TRUE(inserted);
  }
  EXPECT_EQ(set.size(), nb_values);

  for (std::size_t i = 0; i < nb_values; i++) {
    std::tie(it, inserted) = set.insert(utils::get_key<char_tt>(i));

    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_FALSE(inserted);
  }

  for (std::size_t i = 0; i < nb_values; i++) {
    it = set.find(utils::get_key<char_tt>(i));

    EXPECT_NE(it, set.end());
    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
  }

  for (auto it = set.begin(); it != set.end(); ++it) {
    auto it_find = set.find(it.key());

    EXPECT_NE(it_find, set.end());
    EXPECT_EQ(it_find.key(), it.key());
  }
}

TEST(HtrieSet, AssignOperator) {
  fermat::htrie_set<char> set = {"test1", "test2"};
  EXPECT_EQ(set.size(), 2);

  set = {"test3"};
  EXPECT_EQ(set.size(), 1);
  EXPECT_EQ(set.count("test3"), 1);
}

TEST(HtrieSet, CopyOperator) {
  fermat::htrie_set<char> set = {"test1", "test2", "test3", "test4"};
  fermat::htrie_set<char> set2 = set;
  fermat::htrie_set<char> set3;
  set3 = set;

  EXPECT_EQ(set, set2);
  EXPECT_EQ(set, set3);
}

TEST(HtrieSet, MoveOperator) {
  fermat::htrie_set<char> set = {"test1", "test2"};
  fermat::htrie_set<char> set2 = std::move(set);

  EXPECT_TRUE(set.empty());
  EXPECT_EQ(set.begin(), set.end());
  EXPECT_EQ(set2.size(), 2);
  EXPECT_EQ(set2, (fermat::htrie_set<char>{"test1", "test2"}));

  fermat::htrie_set<char> set3;
  set3 = std::move(set2);

  EXPECT_TRUE(set2.empty());
  EXPECT_EQ(set2.begin(), set2.end());
  EXPECT_EQ(set3.size(), 2);
  EXPECT_EQ(set3, (fermat::htrie_set<char>{"test1", "test2"}));

  set2 = {"test1"};
  EXPECT_EQ(set2, (fermat::htrie_set<char>{"test1"}));
}

TEST(HtrieSet, SerializeDeserialize) {
  const std::size_t nb_values = 1000;

  fermat::htrie_set<char> set(0);

  set.insert("");
  for (std::size_t i = 1; i < nb_values + 40; i++) {
    set.insert(utils::get_key<char>(i));
  }

  for (std::size_t i = nb_values; i < nb_values + 40; i++) {
    set.erase(utils::get_key<char>(i));
  }
  EXPECT_EQ(set.size(), nb_values);

  serializer serial;
  set.serialize(serial);

  deserializer dserial(serial.str());
  auto set_deserialized = decltype(set)::deserialize(dserial, true);
  EXPECT_EQ(set_deserialized, set);

  deserializer dserial2(serial.str());
  set_deserialized = decltype(set)::deserialize(dserial2, false);
  EXPECT_EQ(set_deserialized, set);
}
