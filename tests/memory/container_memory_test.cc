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

#include <fermat/memory/container_memory.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string_view>
#include <tests/base/internal/test_instance_tracker.h>
#include <turbo/base/no_destructor.h>
#include <turbo/macros/config.h>
#include <turbo/meta/type_traits.h>

namespace fermat::base_internal {

    namespace {

        using ::fermat::test_internal::CopyableMovableInstance;
        using ::fermat::test_internal::InstanceTracker;
        using ::testing::_;
        using ::testing::ElementsAre;
        using ::testing::Gt;
        using ::testing::Pair;

#if KUMO_HAVE_BUILTIN(__builtin_infer_alloc_token)
        TEST(Memory, AlignedTypeAllocToken) {
#if defined(__wasm__)
            GTEST_SKIP() << "Fails on wasm due to lack of heap partitioning support.";
#endif
            EXPECT_GT(__builtin_infer_alloc_token(sizeof(fermat::memory::AlignedType<alignof(void*)>)),
                __builtin_infer_alloc_token(sizeof(int)));
        }
#endif

        TEST(Memory, AlignmentLargerThanBase) {
            std::allocator<int8_t> alloc;
            void* mem = fermat::memory::good_allocate<2>(&alloc, 3);
            EXPECT_EQ(0, reinterpret_cast<uintptr_t>(mem) % 2);
            memcpy(mem, "abc", 3);
            fermat::memory::good_deallocate<2>(&alloc, mem, 3);
        }

        TEST(Memory, AlignmentSmallerThanBase) {
            std::allocator<int64_t> alloc;
            void* mem = fermat::memory::good_allocate<2>(&alloc, 3);
            EXPECT_EQ(0, reinterpret_cast<uintptr_t>(mem) % 2);
            memcpy(mem, "abc", 3);
            fermat::memory::good_deallocate<2>(&alloc, mem, 3);
        }

        std::map<std::type_index, int>& AllocationMap() {
            static turbo::NoDestructor<std::map<std::type_index, int>> map;
            return *map;
        }

        template <typename T>
        struct TypeCountingAllocator {
            TypeCountingAllocator() = default;
            template <typename U>
            TypeCountingAllocator(const TypeCountingAllocator<U>&) { } // NOLINT

            using value_type = T;

            T* allocate(size_t n, const void* = nullptr) {
                AllocationMap()[typeid(T)] += n;
                return std::allocator<T>().allocate(n);
            }
            void deallocate(T* p, std::size_t n) {
                AllocationMap()[typeid(T)] -= n;
                return std::allocator<T>().deallocate(p, n);
            }
        };

        TEST(Memory, AllocateDeallocateMatchType) {
            TypeCountingAllocator<int> alloc;
            void* mem = fermat::memory::good_allocate<1>(&alloc, 1);
            // Verify that it was allocated
            EXPECT_THAT(AllocationMap(), ElementsAre(Pair(_, Gt(0))));
            fermat::memory::good_deallocate<1>(&alloc, mem, 1);
            // Verify that the deallocation matched.
            EXPECT_THAT(AllocationMap(), ElementsAre(Pair(_, 0)));
        }

        class Fixture : public ::testing::Test {
            using Alloc = std::allocator<std::string>;

        public:
            Fixture() { ptr_ = std::allocator_traits<Alloc>::allocate(*alloc(), 1); }
            ~Fixture() override {
                std::allocator_traits<Alloc>::destroy(*alloc(), ptr_);
                std::allocator_traits<Alloc>::deallocate(*alloc(), ptr_, 1);
            }
            std::string* ptr() { return ptr_; }
            Alloc* alloc() { return &alloc_; }

        private:
            Alloc alloc_;
            std::string* ptr_;
        };

        TEST_F(Fixture, ConstructNoArgs) {
            fermat::memory::construct_from_tuple(alloc(), ptr(), std::forward_as_tuple());
            EXPECT_EQ(*ptr(), "");
        }

        TEST_F(Fixture, ConstructOneArg) {
            fermat::memory::construct_from_tuple(alloc(), ptr(), std::forward_as_tuple("abcde"));
            EXPECT_EQ(*ptr(), "abcde");
        }

        TEST_F(Fixture, ConstructTwoArg) {
            fermat::memory::construct_from_tuple(alloc(), ptr(), std::forward_as_tuple(5, 'a'));
            EXPECT_EQ(*ptr(), "aaaaa");
        }

        TEST(pair_args, NoArgs) {
            EXPECT_THAT(fermat::memory::pair_args(),
                Pair(std::forward_as_tuple(), std::forward_as_tuple()));
        }

        TEST(pair_args, TwoArgs) {
            EXPECT_EQ(
                std::make_pair(std::forward_as_tuple(1), std::forward_as_tuple('A')),
                fermat::memory::pair_args(1, 'A'));
        }

        TEST(pair_args, Pair) {
            EXPECT_EQ(
                std::make_pair(std::forward_as_tuple(1), std::forward_as_tuple('A')),
                fermat::memory::pair_args(std::make_pair(1, 'A')));
        }

        TEST(pair_args, Piecewise) {
            EXPECT_EQ(
                std::make_pair(std::forward_as_tuple(1), std::forward_as_tuple('A')),
                fermat::memory::pair_args(std::piecewise_construct, std::forward_as_tuple(1),
                    std::forward_as_tuple('A')));
        }

        TEST(with_constructed, Simple) {
            EXPECT_EQ(1, fermat::memory::with_constructed<std::string_view>(std::make_tuple(std::string("a")), [](std::string_view str) { return str.size(); }));
        }

        template <class F, class Arg>
        decltype(fermat::memory::decompose_value(std::declval<F>(), std::declval<Arg>()))
        DecomposeValueImpl(int, F&& f, Arg&& arg) {
            return fermat::memory::decompose_value(std::forward<F>(f), std::forward<Arg>(arg));
        }

        template <class F, class Arg>
        const char* DecomposeValueImpl(char, F&& f, Arg&& arg) {
            return "not decomposable";
        }

        template <class F, class Arg>
        decltype(DecomposeValueImpl(0, std::declval<F>(), std::declval<Arg>()))
        TryDecomposeValue(F&& f, Arg&& arg) {
            return DecomposeValueImpl(0, std::forward<F>(f), std::forward<Arg>(arg));
        }

        TEST(decompose_value, Decomposable) {
            auto f = [](const int& x, int&& y) { // NOLINT
                EXPECT_EQ(&x, &y);
                EXPECT_EQ(42, x);
                return 'A';
            };
            EXPECT_EQ('A', TryDecomposeValue(f, 42));
        }

        TEST(decompose_value, NotDecomposable) {
            auto f = [](void*) {
                ADD_FAILURE() << "Must not be called";
                return 'A';
            };
            EXPECT_STREQ("not decomposable", TryDecomposeValue(f, 42));
        }

        template <class F, class... Args>
        decltype(fermat::memory::decompose_pair(std::declval<F>(), std::declval<Args>()...))
        decompose_pair_impl(int, F&& f, Args&&... args) {
            return fermat::memory::decompose_pair(std::forward<F>(f), std::forward<Args>(args)...);
        }

        template <class F, class... Args>
        const char* decompose_pair_impl(char, F&& f, Args&&... args) {
            return "not decomposable";
        }

        template <class F, class... Args>
        decltype(decompose_pair_impl(0, std::declval<F>(), std::declval<Args>()...))
        TryDecomposePair(F&& f, Args&&... args) {
            return decompose_pair_impl(0, std::forward<F>(f), std::forward<Args>(args)...);
        }

        TEST(decompose_pair, Decomposable) {
            auto f = [](const int& x, // NOLINT
                         std::piecewise_construct_t, std::tuple<int&&> k,
                         std::tuple<double>&& v) {
                EXPECT_EQ(&x, &std::get<0>(k));
                EXPECT_EQ(42, x);
                EXPECT_EQ(0.5, std::get<0>(v));
                return 'A';
            };
            EXPECT_EQ('A', TryDecomposePair(f, 42, 0.5));
            EXPECT_EQ('A', TryDecomposePair(f, std::make_pair(42, 0.5)));
            EXPECT_EQ('A', TryDecomposePair(f, std::piecewise_construct, std::make_tuple(42), std::make_tuple(0.5)));
        }

        TEST(decompose_pair, NotDecomposable) {
            auto f = [](...) {
                ADD_FAILURE() << "Must not be called";
                return 'A';
            };
            EXPECT_STREQ("not decomposable", TryDecomposePair(f));
            EXPECT_STREQ("not decomposable",
                TryDecomposePair(f, std::piecewise_construct, std::make_tuple(),
                    std::make_tuple(0.5)));
        }

        TEST(MapSlotPolicy, ConstKeyAndValue) {
            using slot_policy = fermat::memory::MapSlotPolicy<const CopyableMovableInstance,
                const CopyableMovableInstance>;
            using slot_type = typename slot_policy::slot_type;

            union Slots {
                Slots() { }
                ~Slots() { }
                slot_type slots[100];
            } slots;

            std::allocator<
                std::pair<const CopyableMovableInstance, const CopyableMovableInstance>>
                alloc;
            InstanceTracker tracker;
            slot_policy::construct(&alloc, &slots.slots[0], CopyableMovableInstance(1),
                CopyableMovableInstance(1));
            for (int i = 0; i < 99; ++i) {
                slot_policy::transfer(&alloc, &slots.slots[i + 1], &slots.slots[i]);
            }
            slot_policy::destroy(&alloc, &slots.slots[99]);

            EXPECT_EQ(tracker.copies(), 0);
        }

        TEST(MapSlotPolicy, TransferReturnsTrue) {
            {
                using slot_policy = fermat::memory::MapSlotPolicy<int, float>;
                EXPECT_TRUE(
                    (std::is_same_v<decltype(slot_policy::transfer<std::allocator<char>>(
                                        nullptr, nullptr, nullptr)),
                        std::true_type>));
            }
            {
                struct NonRelocatable {
                    NonRelocatable() = default;
                    NonRelocatable(NonRelocatable&&) { }
                    NonRelocatable& operator=(NonRelocatable&&) { return *this; }
                    void* self = nullptr;
                };

                EXPECT_FALSE(turbo::is_trivially_relocatable<NonRelocatable>::value);
                using slot_policy = fermat::memory::MapSlotPolicy<int, NonRelocatable>;
                EXPECT_TRUE(
                    (std::is_same_v<decltype(slot_policy::transfer<std::allocator<char>>(
                                        nullptr, nullptr, nullptr)),
                        std::false_type>));
            }
        }

        TEST(MapSlotPolicy, DestroyReturnsTrue) {
            {
                using slot_policy = fermat::memory::MapSlotPolicy<int, float>;
                EXPECT_TRUE(
                    (std::is_same_v<decltype(slot_policy::destroy<std::allocator<char>>(
                                        nullptr, nullptr)),
                        std::true_type>));
            }
            {
                EXPECT_FALSE(std::is_trivially_destructible_v<std::unique_ptr<int>>);
                using slot_policy = fermat::memory::MapSlotPolicy<int, std::unique_ptr<int>>;
                EXPECT_TRUE(
                    (std::is_same_v<decltype(slot_policy::destroy<std::allocator<char>>(
                                        nullptr, nullptr)),
                        std::false_type>));
            }
        }

    } // namespace

} // namespace fermat::base_internal
