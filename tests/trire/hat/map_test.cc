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
#include <cstddef>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <fermat/trie/hat/htrie_map.h>
#include <tests/trire/hat/utils.h>

using MapTypes = ::testing::Types<fermat::htrie_map<char, std::int64_t>,
                                  fermat::htrie_map<char, std::string>,
                                  fermat::htrie_map<char, throw_move_test>,
                                  fermat::htrie_map<char, move_only_test>>;

template <typename T>
class HtrieMapTyped : public ::testing::Test {};
TYPED_TEST_SUITE(HtrieMapTyped, MapTypes);

TYPED_TEST(HtrieMapTyped, Insert) {
  using TMap = TypeParam;
  using char_tt = typename TMap::char_type;
  using value_tt = typename TMap::mapped_type;

  const std::size_t nb_values = 1000;
  typename TMap::iterator it;
  bool inserted;

  TMap map(8);

  for (std::size_t i = 0; i < nb_values; i++) {
    std::tie(it, inserted) =
        map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));

    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_EQ(*it, utils::get_value<value_tt>(i));
    EXPECT_TRUE(inserted);
  }
  EXPECT_EQ(map.size(), nb_values);

  for (std::size_t i = 0; i < nb_values; i++) {
    std::tie(it, inserted) = map.insert(utils::get_key<char_tt>(i),
                                        utils::get_value<value_tt>(i + 1));

    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_EQ(*it, utils::get_value<value_tt>(i));
    EXPECT_FALSE(inserted);
  }

  for (std::size_t i = 0; i < nb_values; i++) {
    it = map.find(utils::get_key<char_tt>(i));

    EXPECT_NE(it, map.end());
    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_EQ(*it, utils::get_value<value_tt>(i));
  }

  for (auto it = map.begin(); it != map.end(); ++it) {
    auto it_find = map.find(it.key());

    EXPECT_NE(it_find, map.end());
    EXPECT_EQ(it_find.key(), it.key());
  }
}

TEST(HtrieMap, InsertWithTooLongString) {
  fermat::htrie_map<char, std::int64_t, fermat::ah::str_hash<char>, std::uint8_t>
      map;
  map.burst_threshold(8);

  for (std::size_t i = 0; i < 1000; i++) {
    map.insert(utils::get_key<char>(i), utils::get_value<std::int64_t>(i));
  }

  const std::string long_string(map.max_key_size(), 'a');
  EXPECT_TRUE(
      map.insert(long_string, utils::get_value<std::int64_t>(1000)).second);

  const std::string too_long_string(map.max_key_size() + 1, 'a');
  EXPECT_THROW(
      map.insert(too_long_string, utils::get_value<std::int64_t>(1001)),
      std::length_error);
}

TYPED_TEST(HtrieMapTyped, EraseAll) {
  using TMap = TypeParam;
  const std::size_t nb_values = 1000;
  TMap map = utils::get_filled_map<TMap>(nb_values, 8);

  auto it = map.erase(map.begin(), map.end());
  EXPECT_EQ(it, map.end());
  EXPECT_EQ(map.begin(), map.end());
  EXPECT_TRUE(map.empty());
}

TEST(HtrieMap, RangeErase) {
  using TMap = fermat::htrie_map<char, std::int64_t>;

  const std::size_t nb_values = 1000;
  TMap map = utils::get_filled_map<TMap>(nb_values, 8);

  auto it_first = std::next(map.begin(), 14);
  auto it_last = std::next(map.begin(), 994);

  auto it = map.erase(it_first, it_last);
  EXPECT_EQ(std::distance(it, map.end()), 6);
  EXPECT_EQ(map.size(), 20);
  EXPECT_EQ(std::distance(map.begin(), map.end()), 20);
}

TYPED_TEST(HtrieMapTyped, EraseLoop) {
  using TMap = TypeParam;
  std::size_t nb_values = 1000;
  TMap map = utils::get_filled_map<TMap>(nb_values, 8);
  TMap map2 = utils::get_filled_map<TMap>(nb_values, 8);

  auto it = map.begin();
  auto it2 = map2.begin();
  while (it != map.end()) {
    it = map.erase(it);
    --nb_values;

    EXPECT_EQ(map.count(it2.key()), 0);
    EXPECT_EQ(map.size(), nb_values);
    ++it2;
  }

  EXPECT_TRUE(map.empty());
}

TYPED_TEST(HtrieMapTyped, EraseUnknown) {
  using TMap = TypeParam;
  using char_tt = typename TMap::char_type;

  std::size_t nb_values = 1000;
  TMap map = utils::get_filled_map<TMap>(nb_values, 9);

  EXPECT_EQ(map.erase(utils::get_key<char_tt>(1001)), 0);
  EXPECT_EQ(map.erase(map.cbegin(), map.cbegin()), map.begin());
  EXPECT_EQ(map, utils::get_filled_map<TMap>(nb_values, 8));
}

TYPED_TEST(HtrieMapTyped, InsertEraseInsert) {
  using TMap = TypeParam;
  using char_tt = typename TMap::char_type;
  using value_tt = typename TMap::mapped_type;

  const std::size_t nb_values = 1000;
  typename TMap::iterator it;
  bool inserted;

  TMap map;
  map.burst_threshold(8);

  for (std::size_t i = 0; i < nb_values / 2; i++) {
    std::tie(it, inserted) =
        map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));

    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_EQ(*it, utils::get_value<value_tt>(i));
    EXPECT_TRUE(inserted);
  }
  EXPECT_EQ(map.size(), nb_values / 2);

  for (std::size_t i = 0; i < nb_values / 2; i++) {
    if (i % 2 == 0) {
      EXPECT_EQ(map.erase(utils::get_key<char_tt>(i)), 1);
      EXPECT_EQ(map.find(utils::get_key<char_tt>(i)), map.end());
    }
  }
  EXPECT_EQ(map.size(), nb_values / 4);

  for (std::size_t i = nb_values / 2; i < nb_values; i++) {
    std::tie(it, inserted) =
        map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));

    EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
    EXPECT_EQ(*it, utils::get_value<value_tt>(i));
    EXPECT_TRUE(inserted);
  }
  EXPECT_EQ(map.size(), nb_values - nb_values / 4);

  for (std::size_t i = 0; i < nb_values; i++) {
    it = map.find(utils::get_key<char_tt>(i));

    if (i % 2 == 0 && i < nb_values / 2) {
      EXPECT_EQ(it, map.cend());
    } else {
      EXPECT_EQ(it.key(), (utils::get_key<char_tt>(i)));
      EXPECT_EQ(*it, utils::get_value<value_tt>(i));
    }
  }
}

TEST(HtrieMap, EraseWithEmptyTrieNode) {
  fermat::htrie_map<char, int> map = {
      {"k11", 1}, {"k12", 2}, {"k13", 3}, {"k14", 4}};
  map.burst_threshold(4);
  map.insert("k1", 5);
  map.insert("k", 6);
  map.insert("", 7);

  EXPECT_EQ(map.erase("k11"), 1);
  EXPECT_EQ(map.erase("k12"), 1);
  EXPECT_EQ(map.erase("k13"), 1);
  EXPECT_EQ(map.erase("k14"), 1);
  EXPECT_EQ(std::distance(map.begin(), map.end()), 3);

  EXPECT_EQ(map.erase("k1"), 1);
  EXPECT_EQ(std::distance(map.begin(), map.end()), 2);

  EXPECT_EQ(map.erase("k"), 1);
  EXPECT_EQ(std::distance(map.begin(), map.end()), 1);

  EXPECT_EQ(map.erase(""), 1);
  EXPECT_EQ(std::distance(map.begin(), map.end()), 0);
}

TEST(HtrieMap, Emplace) {
  fermat::htrie_map<char, move_only_test> map;
  map.emplace("test1", 1);
  map.emplace_ks("testIgnore", 4, 3);

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map.at("test1"), move_only_test(1));
  EXPECT_EQ(map.at("test"), move_only_test(3));
}

TEST(HtrieMap, EqualPrefixRange) {
  std::set<std::string> sequence_set;
  for (std::size_t i = 1; i <= 1000; i = i * 10) {
    for (std::size_t j = 2 * i; j < 3 * i; j++) {
      sequence_set.insert("Key " + std::to_string(j));
    }
  }

  fermat::htrie_map<char, int> map;
  map.burst_threshold(7);

  for (int i = 0; i < 4000; i++) {
    map.insert("Key " + std::to_string(i), i);
  }

  auto range = map.equal_prefix_range("Key 2");
  EXPECT_EQ(std::distance(range.first, range.second), 1111);

  std::set<std::string> set;
  for (auto it = range.first; it != range.second; ++it) {
    set.insert(it.key());
  }
  EXPECT_EQ(set.size(), 1111);
  EXPECT_EQ(set, sequence_set);

  range = map.equal_prefix_range("");
  EXPECT_EQ(std::distance(range.first, range.second), 4000);

  range = map.equal_prefix_range("Key 1000");
  EXPECT_EQ(std::distance(range.first, range.second), 1);
  EXPECT_EQ(range.first.key(), "Key 1000");

  range = map.equal_prefix_range("aKey 1000");
  EXPECT_EQ(std::distance(range.first, range.second), 0);

  range = map.equal_prefix_range("Key 30000");
  EXPECT_EQ(std::distance(range.first, range.second), 0);

  range = map.equal_prefix_range("Unknown");
  EXPECT_EQ(std::distance(range.first, range.second), 0);

  range = map.equal_prefix_range("KE");
  EXPECT_EQ(std::distance(range.first, range.second), 0);
}

TEST(HtrieMap, EqualPrefixRangeEmpty) {
  fermat::htrie_map<char, int> map;

  auto range = map.equal_prefix_range("");
  EXPECT_EQ(std::distance(range.first, range.second), 0);

  range = map.equal_prefix_range("A");
  EXPECT_EQ(std::distance(range.first, range.second), 0);

  range = map.equal_prefix_range("Aa");
  EXPECT_EQ(std::distance(range.first, range.second), 0);
}

TEST(HtrieMap, LongestPrefix) {
  fermat::htrie_map<char, int> map(4);
  map = {{"a", 1},      {"aa", 1},      {"aaa", 1},   {"aaaaa", 1},
         {"aaaaaa", 1}, {"aaaaaaa", 1}, {"ab", 1},    {"abcde", 1},
         {"abcdf", 1},  {"abcdg", 1},   {"abcdh", 1}, {"babc", 1}};

  EXPECT_EQ(map.longest_prefix("a").key(), "a");
  EXPECT_EQ(map.longest_prefix("aa").key(), "aa");
  EXPECT_EQ(map.longest_prefix("aaa").key(), "aaa");
  EXPECT_EQ(map.longest_prefix("aaaa").key(), "aaa");
  EXPECT_EQ(map.longest_prefix("ab").key(), "ab");
  EXPECT_EQ(map.longest_prefix("abc").key(), "ab");
  EXPECT_EQ(map.longest_prefix("abcd").key(), "ab");
  EXPECT_EQ(map.longest_prefix("abcdz").key(), "ab");
  EXPECT_EQ(map.longest_prefix("abcde").key(), "abcde");
  EXPECT_EQ(map.longest_prefix("abcdef").key(), "abcde");
  EXPECT_EQ(map.longest_prefix("abcdefg").key(), "abcde");
  EXPECT_EQ(map.longest_prefix("dabc"), map.end());
  EXPECT_EQ(map.longest_prefix("b"), map.end());
  EXPECT_EQ(map.longest_prefix("bab"), map.end());
  EXPECT_EQ(map.longest_prefix("babd"), map.end());
  EXPECT_EQ(map.longest_prefix(""), map.end());

  map.insert("", 1);
  EXPECT_EQ(map.longest_prefix("dabc").key(), "");
  EXPECT_EQ(map.longest_prefix("").key(), "");
}

TEST(HtrieMap, ForEachPrefixOf) {
  using map_type = fermat::htrie_map<char, int>;
  using path_type = std::vector<std::string>;

  map_type map(4);
  map = {{"a", 1},      {"aa", 1},      {"aaa", 1},   {"aaaaa", 1},
         {"aaaaaa", 1}, {"aaaaaaa", 1}, {"ab", 1},    {"abcde", 1},
         {"abcdf", 1},  {"abcdg", 1},   {"abcdh", 1}, {"babc", 1}};

  std::vector<std::pair<const char*, path_type>> test_vectors = {
      {"a", {"a"}},
      {"aa", {"a", "aa"}},
      {"aaa", {"a", "aa", "aaa"}},
      {"aaaa", {"a", "aa", "aaa"}},
      {"ab", {"a", "ab"}},
      {"abc", {"a", "ab"}},
      {"abcd", {"a", "ab"}},
      {"abcdz", {"a", "ab"}},
      {"abcde", {"a", "ab", "abcde"}},
      {"abcdef", {"a", "ab", "abcde"}},
      {"abcdefg", {"a", "ab", "abcde"}},
      {"dabc", {}},
      {"b", {}},
      {"bab", {}},
      {"babd", {}},
      {"", {}},
  };

  for (const auto& v : test_vectors) {
    path_type p;
    const path_type& expected = v.second;
    auto visitor = [&p](map_type::const_iterator it) { p.push_back(it.key()); };
    map.for_each_prefix_of(v.first, visitor);
    EXPECT_EQ(p, expected) << "...for test vector input '" << v.first << "'";
  }
}

TEST(HtrieMap, ErasePrefix) {
  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(10000, 200);

  auto check_nb_equal_prefix = [&map](const std::string& key,
                                      std::ptrdiff_t nb_values) {
    auto range = map.equal_prefix_range(key);
    return std::distance(range.first, range.second) == nb_values;
  };

  EXPECT_TRUE(check_nb_equal_prefix("Key 1", 1111));
  EXPECT_EQ(map.erase_prefix("Key 1"), 1111);
  EXPECT_EQ(map.size(), 8889);
  EXPECT_TRUE(check_nb_equal_prefix("Key 1", 0));

  EXPECT_TRUE(check_nb_equal_prefix("Key 22", 111));
  EXPECT_EQ(map.erase_prefix("Key 22"), 111);
  EXPECT_EQ(map.size(), 8778);
  EXPECT_TRUE(check_nb_equal_prefix("Key 2", 1000));

  EXPECT_TRUE(check_nb_equal_prefix("Key 333", 11));
  EXPECT_EQ(map.erase_prefix("Key 333"), 11);
  EXPECT_EQ(map.size(), 8767);
  EXPECT_TRUE(check_nb_equal_prefix("Key 3", 1100));

  EXPECT_TRUE(check_nb_equal_prefix("Key 4444", 1));
  EXPECT_EQ(map.erase_prefix("Key 4444"), 1);
  EXPECT_EQ(map.size(), 8766);
  EXPECT_TRUE(check_nb_equal_prefix("Key 4", 1110));

  EXPECT_TRUE(check_nb_equal_prefix("Key 55555", 0));
  EXPECT_EQ(map.erase_prefix("Key 55555"), 0);
  EXPECT_EQ(map.size(), 8766);
  EXPECT_TRUE(check_nb_equal_prefix("Key 5", 1111));

  for (auto it = map.begin(); it != map.end(); ++it) {
    EXPECT_EQ(it.key().find("Key 1"), std::string::npos);
    EXPECT_EQ(it.key().find("Key 22"), std::string::npos);
    EXPECT_EQ(it.key().find("Key 333"), std::string::npos);
    EXPECT_EQ(it.key().find("Key 4444"), std::string::npos);
  }

  EXPECT_EQ(std::distance(map.begin(), map.end()), map.size());
}

TEST(HtrieMap, ErasePrefix2) {
  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(10000, 100);

  auto check_nb_equal_prefix = [&map](const std::string& key,
                                      std::ptrdiff_t nb_values) {
    auto range = map.equal_prefix_range(key);
    return std::distance(range.first, range.second) == nb_values;
  };

  for (size_t i = 0; i < 10; i++) {
    EXPECT_EQ(map.erase_prefix("Key 2" + std::to_string(i)), 111);
    EXPECT_TRUE(check_nb_equal_prefix("Key 2", 1111 - (i + 1) * 111));
  }
  EXPECT_EQ(map.erase_prefix("Key 2"), 1);
  EXPECT_EQ(map.size(), 8889);
  EXPECT_TRUE(check_nb_equal_prefix("Key 2", 0));

  EXPECT_EQ(std::distance(map.begin(), map.end()), map.size());
}

TEST(HtrieMap, ErasePrefixAll1) {
  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(1000, 8);
  EXPECT_EQ(map.size(), 1000);
  EXPECT_EQ(map.erase_prefix(""), 1000);
  EXPECT_EQ(map.size(), 0);

  auto range = map.equal_prefix_range("");
  EXPECT_EQ(std::distance(range.first, range.second), 0);
}

TEST(HtrieMap, ErasePrefixAll2) {
  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(1000, 8);
  EXPECT_EQ(map.size(), 1000);
  EXPECT_EQ(map.erase_prefix("Ke"), 1000);
  EXPECT_EQ(map.size(), 0);

  auto range = map.equal_prefix_range("Ke");
  EXPECT_EQ(std::distance(range.first, range.second), 0);
}

TEST(HtrieMap, ErasePrefixAllHashNode) {
  fermat::htrie_map<char, std::int64_t> map;
  map = utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(16, 16);

  EXPECT_EQ(map.size(), 16);
  EXPECT_EQ(map.erase_prefix("Ke"), 16);
  EXPECT_EQ(map.size(), 0);

  auto range = map.equal_prefix_range("Ke");
  EXPECT_EQ(std::distance(range.first, range.second), 0);
}

TEST(HtrieMap, ErasePrefixNone) {
  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(1000, 8);
  EXPECT_EQ(map.erase_prefix("Kea"), 0);
  EXPECT_EQ(map.size(), 1000);
}

TEST(HtrieMap, ErasePrefixEmptyMap) {
  fermat::htrie_map<char, std::int64_t> map;
  EXPECT_EQ(map.erase_prefix("Kea"), 0);
  EXPECT_EQ(map.erase_prefix(""), 0);
}

TEST(HtrieMap, Compare) {
  fermat::htrie_map<char, std::int64_t> map = {
      {"test1", 10}, {"test2", 20}, {"test3", 30}};
  fermat::htrie_map<char, std::int64_t> map2 = {
      {"test3", 30}, {"test2", 20}, {"test1", 10}};
  fermat::htrie_map<char, std::int64_t> map3 = {
      {"test1", 10}, {"test2", 20}, {"test3", -1}};
  fermat::htrie_map<char, std::int64_t> map4 = {{"test3", 30}, {"test2", 20}};

  EXPECT_EQ(map, map);
  EXPECT_EQ(map2, map2);
  EXPECT_EQ(map3, map3);
  EXPECT_EQ(map4, map4);

  EXPECT_EQ(map, map2);
  EXPECT_NE(map, map3);
  EXPECT_NE(map, map4);
  EXPECT_NE(map2, map3);
  EXPECT_NE(map2, map4);
  EXPECT_NE(map3, map4);
}

TEST(HtrieMap, Clear) {
  fermat::htrie_map<char, std::int64_t> map = {{"test1", 10}, {"test2", 20}};

  map.clear();
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.begin(), map.end());
  EXPECT_EQ(map.cbegin(), map.cend());
}

TEST(HtrieMap, AssignOperator) {
  fermat::htrie_map<char, std::int64_t> map = {{"test1", 10}, {"test2", 20}};
  EXPECT_EQ(map.size(), 2);

  map = {{"test3", 30}};
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at("test3"), 30);
}

TEST(HtrieMap, CopyOperator) {
  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(1000, 8);
  fermat::htrie_map<char, std::int64_t> map2 = map;
  fermat::htrie_map<char, std::int64_t> map3;
  map3 = map;

  EXPECT_EQ(map, map2);
  EXPECT_EQ(map, map3);
}

TEST(HtrieMap, MoveOperator) {
  const std::size_t nb_elements = 1000;
  const fermat::htrie_map<char, std::int64_t> init_map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(nb_elements, 8);

  fermat::htrie_map<char, std::int64_t> map =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(nb_elements, 8);
  fermat::htrie_map<char, std::int64_t> map2 =
      utils::get_filled_map<fermat::htrie_map<char, std::int64_t>>(1, 8);
  map2 = std::move(map);

  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.begin(), map.end());
  EXPECT_EQ(map2.size(), nb_elements);
  EXPECT_EQ(map2, init_map);

  fermat::htrie_map<char, std::int64_t> map3;
  map3 = std::move(map2);

  EXPECT_TRUE(map2.empty());
  EXPECT_EQ(map2.begin(), map2.end());
  EXPECT_EQ(map3.size(), nb_elements);
  EXPECT_EQ(map3, init_map);

  map2 = {{"test1", 10}};
  EXPECT_EQ(map2, (fermat::htrie_map<char, std::int64_t>{{"test1", 10}}));
}

TEST(HtrieMap, At) {
  fermat::htrie_map<char, std::int64_t> map = {{"test1", 10}, {"test2", 20}};
  map.insert("test4", 40);

  EXPECT_EQ(map.at("test1"), 10);
  EXPECT_EQ(map.at("test2"), 20);
  EXPECT_THROW(map.at("test3"), std::out_of_range);
  EXPECT_EQ(map.at("test4"), 40);

  const fermat::htrie_map<char, std::int64_t> map_const = {
      {"test1", 10}, {"test2", 20}, {"test4", 40}};

  EXPECT_EQ(map_const.at("test1"), 10);
  EXPECT_EQ(map_const.at("test2"), 20);
  EXPECT_THROW(map_const.at("test3"), std::out_of_range);
  EXPECT_EQ(map_const.at("test4"), 40);
}

TEST(HtrieMap, EqualRange) {
  fermat::htrie_map<char, std::int64_t> map = {{"test1", 10}, {"test2", 20}};

  auto it_pair = map.equal_range("test1");
  ASSERT_EQ(std::distance(it_pair.first, it_pair.second), 1);
  EXPECT_EQ(it_pair.first.value(), 10);

  it_pair = map.equal_range("");
  EXPECT_EQ(it_pair.first, it_pair.second);
  EXPECT_EQ(it_pair.first, map.end());
}

TEST(HtrieMap, AccessOperator) {
  fermat::htrie_map<char, std::int64_t> map = {{"test1", 10}, {"test2", 20}};

  EXPECT_EQ(map["test1"], 10);
  EXPECT_EQ(map["test2"], 20);
  EXPECT_EQ(map["test3"], std::int64_t());

  map["test3"] = 30;
  EXPECT_EQ(map["test3"], 30);

  EXPECT_EQ(map.size(), 3);
}

TEST(HtrieMap, ShrinkToFit) {
  using TMap = fermat::htrie_map<char, std::int64_t>;
  using char_tt = typename TMap::char_type;
  using value_tt = typename TMap::mapped_type;

  const std::size_t nb_elements = 4000;
  const std::size_t burst_threshold = 7;

  TMap map;
  TMap map2;

  map.burst_threshold(burst_threshold);
  map2.burst_threshold(burst_threshold);

  for (std::size_t i = 0; i < nb_elements / 2; i++) {
    map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
    map2.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
  }

  EXPECT_EQ(map, map2);
  map2.shrink_to_fit();
  EXPECT_EQ(map, map2);

  for (std::size_t i = nb_elements / 2; i < nb_elements; i++) {
    map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
    map2.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
  }

  EXPECT_EQ(map, map2);
  map2.shrink_to_fit();
  EXPECT_EQ(map, map2);
}

TEST(HtrieMap, Swap) {
  fermat::htrie_map<char, std::int64_t> map = {{"test1", 10}, {"test2", 20}};
  fermat::htrie_map<char, std::int64_t> map2 = {
      {"test3", 30}, {"test4", 40}, {"test5", 50}};

  using std::swap;
  swap(map, map2);

  EXPECT_EQ(map, (fermat::htrie_map<char, std::int64_t>{
                     {"test3", 30}, {"test4", 40}, {"test5", 50}}));
  EXPECT_EQ(map2, (fermat::htrie_map<char, std::int64_t>{{"test1", 10},
                                                      {"test2", 20}}));
}

TEST(HtrieMap, SerializeDeserializeEmptyMap) {
  const fermat::htrie_map<char, move_only_test> empty_map;

  serializer serial;
  empty_map.serialize(serial);

  deserializer dserial(serial.str());
  auto empty_map_deserialized = decltype(empty_map)::deserialize(dserial, true);
  EXPECT_EQ(empty_map_deserialized, empty_map);

  deserializer dserial2(serial.str());
  empty_map_deserialized = decltype(empty_map)::deserialize(dserial2, false);
  EXPECT_EQ(empty_map_deserialized, empty_map);
}

TEST(HtrieMap, SerializeDeserializeMap) {
  const std::size_t nb_values = 1000;

  fermat::htrie_map<char, move_only_test> map(7);

  map.insert("", utils::get_value<move_only_test>(0));
  for (std::size_t i = 1; i < nb_values + 40; i++) {
    map.insert(utils::get_key<char>(i), utils::get_value<move_only_test>(i));
  }

  for (std::size_t i = nb_values; i < nb_values + 40; i++) {
    map.erase(utils::get_key<char>(i));
  }
  EXPECT_EQ(map.size(), nb_values);

  serializer serial;
  map.serialize(serial);

  deserializer dserial(serial.str());
  auto map_deserialized = decltype(map)::deserialize(dserial, true);
  EXPECT_EQ(map, map_deserialized);

  deserializer dserial2(serial.str());
  map_deserialized = decltype(map)::deserialize(dserial2, false);
  EXPECT_EQ(map_deserialized, map);
}

TEST(HtrieMap, SerializeDeserializeWithDifferentHash) {
  struct str_hash {
    std::size_t operator()(const char* key, std::size_t key_size) const {
      return fermat::ah::str_hash<char>()(key, key_size) + 123;
    }
  };

  const std::size_t nb_values = 1000;

  fermat::htrie_map<char, move_only_test> map(7);

  map.insert("", utils::get_value<move_only_test>(0));
  for (std::size_t i = 1; i < nb_values + 40; i++) {
    map.insert(utils::get_key<char>(i), utils::get_value<move_only_test>(i));
  }

  for (std::size_t i = nb_values; i < nb_values + 40; i++) {
    map.erase(utils::get_key<char>(i));
  }
  EXPECT_EQ(map.size(), nb_values);

  serializer serial;
  map.serialize(serial);

  deserializer dserial(serial.str());
  auto map_deserialized =
      fermat::htrie_map<char, move_only_test, str_hash>::deserialize(dserial);

  EXPECT_EQ(map.size(), map_deserialized.size());
  for (auto it = map.cbegin(); it != map.cend(); ++it) {
    const auto it_element_rhs = map_deserialized.find(it.key());
    EXPECT_TRUE(it_element_rhs != map_deserialized.cend() &&
                it.value() == it_element_rhs.value());
  }
}

TEST(HtrieMap, SerializeDeserializeMapNoBurst) {
  const std::size_t nb_values = 100;

  fermat::htrie_map<char, move_only_test> map(nb_values + 1);

  map.insert("", utils::get_value<move_only_test>(0));
  for (std::size_t i = 1; i < nb_values; i++) {
    map.insert(utils::get_key<char>(i), utils::get_value<move_only_test>(i));
  }

  EXPECT_EQ(map.size(), nb_values);

  serializer serial;
  map.serialize(serial);

  deserializer dserial(serial.str());
  auto map_deserialized = decltype(map)::deserialize(dserial, true);
  EXPECT_EQ(map, map_deserialized);

  deserializer dserial2(serial.str());
  map_deserialized = decltype(map)::deserialize(dserial2, false);
  EXPECT_EQ(map_deserialized, map);
}

TEST(HtrieMap, EmptyMap) {
  fermat::htrie_map<char, int> map;

  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.empty());

  EXPECT_EQ(map.begin(), map.end());
  EXPECT_EQ(map.begin(), map.cend());
  EXPECT_EQ(map.cbegin(), map.cend());

  EXPECT_EQ(map.find(""), map.end());
  EXPECT_EQ(map.find("test"), map.end());

  EXPECT_EQ(map.count(""), 0);
  EXPECT_EQ(map.count("test"), 0);

  EXPECT_THROW(map.at(""), std::out_of_range);
  EXPECT_THROW(map.at("test"), std::out_of_range);

  auto range = map.equal_range("test");
  EXPECT_EQ(range.first, range.second);

  auto range_prefix = map.equal_prefix_range("test");
  EXPECT_EQ(range_prefix.first, range_prefix.second);

  EXPECT_EQ(map.longest_prefix("test"), map.end());

  EXPECT_EQ(map.erase("test"), 0);
  EXPECT_EQ(map.erase(map.begin(), map.end()), map.end());

  EXPECT_EQ(map.erase_prefix("test"), 0);

  EXPECT_EQ(map["new value"], int{});
}
