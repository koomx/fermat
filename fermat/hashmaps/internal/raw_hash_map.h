// Copyright 2018 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "fermat/base/common.h"

#include <tuple>
#include <type_traits>
#include <utility>

#include <turbo/base/throw_delegate.h>
#include <fermat/base/common_policy_traits.h>
#include <fermat/hashmaps/internal/raw_hash_set.h> // IWYU pragma: export
#include <turbo/macros/config.h>
#include <turbo/memory/container_memory.h>
#include <turbo/meta/type_traits.h>

namespace fermat::container_internal {


        template <class Policy, class... Params>
        class raw_hash_map;

        template <typename Policy, typename Hash, typename Eq, typename Alloc>
        struct InstantiateRawHashMap {
            using type = typename fermat::base_internal::ApplyWithoutDefaultSuffix<
                raw_hash_map,
                fermat::base_internal::TypeList<int, typename Policy::DefaultHash, typename Policy::DefaultEq,
                    typename Policy::DefaultAlloc>,
                fermat::base_internal::TypeList<Policy, Hash, Eq, Alloc>>::type;
        };

        template <class Policy, class... Params>
        class raw_hash_map : public raw_hash_set<Policy, Params...> {
            // P is Policy. It's passed as a template argument to support maps that have
            // incomplete types as values, as in unordered_map<K, IncompleteType>.
            // MappedReference<> may be a non-reference type.
            template <class P>
            using MappedReference = decltype(P::value(
                std::addressof(std::declval<typename raw_hash_map::reference>())));

            // MappedConstReference<> may be a non-reference type.
            template <class P>
            using MappedConstReference = decltype(P::value(
                std::addressof(std::declval<typename raw_hash_map::const_reference>())));

            using Hash = typename raw_hash_map::raw_hash_set::hasher;
            using Eq = typename raw_hash_map::raw_hash_set::key_equal;
            using Alloc = typename raw_hash_map::raw_hash_set::allocator_type;

            template <class K>
            using key_arg =
                typename ::fermat::base_internal::KeyArg<::fermat::base_internal::IsTransparent<Eq>::value && ::fermat::base_internal::IsTransparent<Hash>::value>::
                    template type<K, typename Policy::key_type>;

            // NOTE: The mess here is to shorten the code for the (very repetitive)
            // function overloads, and to allow the lifetime-bound overloads to dispatch
            // to the non-lifetime-bound overloads, to ensure there is a single source of
            // truth for each overload set.
            //
            // Enabled if an assignment from the given type would require the
            // source object to remain alive for the life of the element.
            //
            // TODO(b/402804213): Remove these traits and simplify the overloads whenever
            // we have a better mechanism available to handle lifetime analysis.
            template <class K, bool Value, typename = void>
            using LifetimeBoundK = ::fermat::base_internal::HasValue<
                Value, std::conditional_t<::fermat::base_internal::policy_trait_element_is_owner<Policy>::value, std::false_type, turbo::type_traits_internal::IsLifetimeBoundAssignment<typename Policy::key_type, K>>>;
            template <class V, bool Value, typename = void>
            using LifetimeBoundV = ::fermat::base_internal::HasValue<Value, turbo::type_traits_internal::IsLifetimeBoundAssignment<typename Policy::mapped_type, V>>;
            template <class K, bool KValue, class V, bool VValue, typename... Dummy>
            using LifetimeBoundKV = std::conjunction<LifetimeBoundK<K, KValue, std::void_t<Dummy...>>,
                LifetimeBoundV<V, VValue>>;

        public:
            using key_type = typename Policy::key_type;
            using mapped_type = typename Policy::mapped_type;

            static_assert(!std::is_reference_v<key_type>, "");

            // TODO(b/187807849): Evaluate whether to support reference mapped_type and
            // remove this assertion if/when it is supported.
            static_assert(!std::is_reference_v<mapped_type>, "");

            using iterator = typename raw_hash_map::raw_hash_set::iterator;
            using const_iterator = typename raw_hash_map::raw_hash_set::const_iterator;

            raw_hash_map() { }
            using raw_hash_map::raw_hash_set::raw_hash_set;

            // The last two template parameters ensure that both arguments are rvalues
            // (lvalue arguments are handled by the overloads below). This is necessary
            // for supporting bitfield arguments.
            //
            //   union { int n : 1; };
            //   flat_hash_map<int, int> m;
            //   m.insert_or_assign(n, n);
            //
            // TODO(b/402804213): Remove these macros whenever we have a better mechanism
            // available to handle lifetime analysis.
#define FERMAT_INTERNAL_X(Func, Callee, KQual, VQual, KValue, VValue, Tail, ...)    \
    template <                                                                     \
        typename K = key_type, class V = mapped_type,                              \
        TURBO_INTERNAL_IF_##KValue##_NOR_##VValue(                                 \
            int = (::fermat::base_internal::EnableIf<LifetimeBoundKV<K, KValue, V, VValue,                  \
                    ::fermat::base_internal::IfRRef<int KQual>::AddPtr<K>,                                  \
                    ::fermat::base_internal::IfRRef<int VQual>::AddPtr<V>>>()),                             \
            TURBO_INTERNAL_SINGLE_ARG(                                             \
                int&...,                                                           \
                decltype(::fermat::base_internal::EnableIf<LifetimeBoundKV<K, KValue, V, VValue>>()) = 0))> \
    decltype(auto) Func(                                                           \
        __VA_ARGS__ key_arg<K> KQual k TURBO_INTERNAL_IF_##KValue(                 \
            KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS),                             \
        V VQual v TURBO_INTERNAL_IF_##VValue(                                      \
            KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS))                             \
        KUMO_ATTRIBUTE_LIFETIME_BOUND {                                            \
        return TURBO_INTERNAL_IF_##KValue##_OR_##VValue(                           \
            (this->template Func<K, V, 0>), Callee)(                               \
            std::forward<decltype(k)>(k), std::forward<decltype(v)>(v)) Tail;      \
    }                                                                              \
    static_assert(true, "This is to force a semicolon.")

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&,
                false, false, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&,
                false, true, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&, true,
                false, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&, true,
                true, TURBO_INTERNAL_SINGLE_ARG());

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, false,
                false, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, false,
                true, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, true,
                false, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, true,
                true, TURBO_INTERNAL_SINGLE_ARG());

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, false,
                false, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, false,
                true, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, true,
                false, TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, true,
                true, TURBO_INTERNAL_SINGLE_ARG());

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, false, false,
                TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, false, true,
                TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, true, false,
                TURBO_INTERNAL_SINGLE_ARG());
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, true, true,
                TURBO_INTERNAL_SINGLE_ARG());

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&,
                false, false, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&,
                false, true, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&, true,
                false, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, const&, true,
                true, .first, const_iterator FERMAT_INTERNAL_COMMA);

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, false,
                false, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, false,
                true, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, true,
                false, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, const&, &&, true,
                true, .first, const_iterator FERMAT_INTERNAL_COMMA);

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, false,
                false, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, false,
                true, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, true,
                false, .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, const&, true,
                true, .first, const_iterator FERMAT_INTERNAL_COMMA);

            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, false, false,
                .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, false, true,
                .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, true, false,
                .first, const_iterator FERMAT_INTERNAL_COMMA);
            FERMAT_INTERNAL_X(insert_or_assign, insert_or_assign_impl, &&, &&, true, true,
                .first, const_iterator FERMAT_INTERNAL_COMMA);
#undef FERMAT_INTERNAL_X

            // All `try_emplace()` overloads make the same guarantees regarding rvalue
            // arguments as `std::unordered_map::try_emplace()`, namely that these
            // functions will not move from rvalue arguments if insertions do not happen.
            template <
                class K = key_type, int = ::fermat::base_internal::EnableIf<LifetimeBoundK<K, false, K*>>(),
                class... Args,
                std::enable_if_t<!std::is_convertible_v<K, const_iterator>, int> = 0>
            std::pair<iterator, bool> try_emplace(key_arg<K>&& k, Args&&... args)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return try_emplace_impl(std::forward<key_arg<K>>(k),
                    std::forward<Args>(args)...);
            }

            template <
                class K = key_type, class... Args,
                ::fermat::base_internal::EnableIf<LifetimeBoundK<K, true, K*>> = 0,
                std::enable_if_t<!std::is_convertible_v<K, const_iterator>, int> = 0>
            std::pair<iterator, bool> try_emplace(
                key_arg<K>&& k KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS,
                Args&&... args) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return this->template try_emplace<K, 0>(std::forward<key_arg<K>>(k),
                    std::forward<Args>(args)...);
            }

            template <
                class K = key_type, int = ::fermat::base_internal::EnableIf<LifetimeBoundK<K, false>>(),
                class... Args,
                std::enable_if_t<!std::is_convertible_v<K, const_iterator>, int> = 0>
            std::pair<iterator, bool> try_emplace(const key_arg<K>& k, Args&&... args)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return try_emplace_impl(k, std::forward<Args>(args)...);
            }
            template <
                class K = key_type, class... Args, ::fermat::base_internal::EnableIf<LifetimeBoundK<K, true>> = 0,
                std::enable_if_t<!std::is_convertible_v<K, const_iterator>, int> = 0>
            std::pair<iterator, bool> try_emplace(
                const key_arg<K>& k KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS,
                Args&&... args) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return this->template try_emplace<K, 0>(k, std::forward<Args>(args)...);
            }

            template <class K = key_type, int = ::fermat::base_internal::EnableIf<LifetimeBoundK<K, false, K*>>(),
                class... Args>
            iterator try_emplace(const_iterator, key_arg<K>&& k,
                Args&&... args) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return try_emplace(std::forward<key_arg<K>>(k), std::forward<Args>(args)...)
                    .first;
            }
            template <class K = key_type, class... Args,
                ::fermat::base_internal::EnableIf<LifetimeBoundK<K, true, K*>> = 0>
            iterator try_emplace(const_iterator hint,
                key_arg<K>&& k KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS,
                Args&&... args) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return this->template try_emplace<K, 0>(hint, std::forward<key_arg<K>>(k),
                    std::forward<Args>(args)...);
            }

            template <class K = key_type, int = ::fermat::base_internal::EnableIf<LifetimeBoundK<K, false>>(),
                class... Args>
            iterator try_emplace(const_iterator, const key_arg<K>& k,
                Args&&... args) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return try_emplace(k, std::forward<Args>(args)...).first;
            }
            template <class K = key_type, class... Args,
                ::fermat::base_internal::EnableIf<LifetimeBoundK<K, true>> = 0>
            iterator try_emplace(const_iterator hint,
                const key_arg<K>& k
                    KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS,
                Args&&... args) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return this->template try_emplace<K, 0>(hint, k,
                    std::forward<Args>(args)...);
            }

            template <class K = key_type, class P = Policy>
            MappedReference<P> at(const key_arg<K>& key) KUMO_ATTRIBUTE_LIFETIME_BOUND {
                auto it = this->find(key);
                if (it == this->end()) {
                    turbo::ThrowStdOutOfRange("turbo::container_internal::raw_hash_map<>::at");
                }
                return Policy::value(&*it);
            }

            template <class K = key_type, class P = Policy>
            MappedConstReference<P> at(const key_arg<K>& key) const
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                auto it = this->find(key);
                if (it == this->end()) {
                    turbo::ThrowStdOutOfRange("turbo::container_internal::raw_hash_map<>::at");
                }
                return Policy::value(&*it);
            }

            template <class K = key_type, class P = Policy,
                int = ::fermat::base_internal::EnableIf<LifetimeBoundK<K, false, K*>>()>
            MappedReference<P> operator[](key_arg<K>&& key)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                // It is safe to use unchecked_deref here because try_emplace
                // will always return an iterator pointing to a valid item in the table,
                // since it inserts if nothing is found for the given key.
                return Policy::value(&this->unchecked_deref(
                    try_emplace(std::forward<key_arg<K>>(key)).first));
            }
            template <class K = key_type, class P = Policy, int&...,
                ::fermat::base_internal::EnableIf<LifetimeBoundK<K, true, K*>> = 0>
            MappedReference<P> operator[](
                key_arg<K>&& key KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return this->template operator[]<K, P, 0>(std::forward<key_arg<K>>(key));
            }

            template <class K = key_type, class P = Policy,
                int = ::fermat::base_internal::EnableIf<LifetimeBoundK<K, false>>()>
            MappedReference<P> operator[](const key_arg<K>& key)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                // It is safe to use unchecked_deref here because try_emplace
                // will always return an iterator pointing to a valid item in the table,
                // since it inserts if nothing is found for the given key.
                return Policy::value(&this->unchecked_deref(try_emplace(key).first));
            }
            template <class K = key_type, class P = Policy, int&...,
                ::fermat::base_internal::EnableIf<LifetimeBoundK<K, true>> = 0>
            MappedReference<P> operator[](
                const key_arg<K>& key KUMO_INTERNAL_ATTRIBUTE_CAPTURED_BY_THIS)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                return this->template operator[]<K, P, 0>(key);
            }

        private:
            static_assert(
                std::is_same_v<
                    typename InstantiateRawHashMap<Policy, Hash, Eq, Alloc>::type,
                    raw_hash_map>,
                "Redundant template parameters were passed. Use InstantiateRawHashMap<> "
                "instead");

            template <class K, class V>
            std::pair<iterator, bool> insert_or_assign_impl(K&& k, V&& v)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                auto res = this->find_or_prepare_insert(k);
                if (res.second) {
                    this->emplace_at(res.first, std::forward<K>(k), std::forward<V>(v));
                } else {
                    Policy::value(&Policy::element(res.first)) = std::forward<V>(v);
                }
                return { this->non_iterable_iterator_at_slot(res.first), res.second };
            }

            template <class K = key_type, class... Args>
            std::pair<iterator, bool> try_emplace_impl(K&& k, Args&&... args)
                KUMO_ATTRIBUTE_LIFETIME_BOUND {
                auto res = this->find_or_prepare_insert(k);
                if (res.second) {
                    this->emplace_at(res.first, std::piecewise_construct,
                        std::forward_as_tuple(std::forward<K>(k)),
                        std::forward_as_tuple(std::forward<Args>(args)...));
                }
                return { this->non_iterable_iterator_at_slot(res.first), res.second };
            }
        };

} // namespace fermat::container_internal

