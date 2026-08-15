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

#pragma once

//////////////////////////////////////////////////////////////////////////////
// This file implements VectorMultimap. It acts much like std::multimap, except
// its underlying representation is a random access container such as vector.
// These containers are sometimes also known as "sorted vectors."
// vector_maps have an advantage over conventional maps in that their memory
// is contiguous and node-less. The result is that lookups are faster, more
// cache friendly (which potentially more so benefits speed), and the container
// uses less memory. The downside is that inserting new items into the container
// is slower if they are inserted in random order instead of in sorted order.
// This tradeoff is well-worth it for many cases. Note that VectorMultimap allows
// you to use a deque or other random access container which may perform
// better for you than vector.
//
// Note that with vector_set, vector_multiset, vector_map, VectorMultimap
// that the modification of the container potentially invalidates all
// existing iterators into the container, unlike what happens with conventional
// sets and maps.
//
// This type could conceptually use a fermat::array as its underlying container,
// however the current design requires an allocator aware container.
// Consider using a fixed_vector instead.
//////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <functional>
#include <vector>
#include <fermat/maps/internal/sorted_container.h>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <cstddef>

namespace fermat {
    /// multimap_value_compare
    ///
    /// Our adapter for the comparison function in the template parameters.
    ///
    /// todo: deprecate this. shouldn't have all these member functions available (including constructor) and shouldn't be a public type.
    template <typename Key, typename Value, typename Compare>
    class multimap_value_compare : public Compare {
    public:
        explicit multimap_value_compare(const Compare& x)
            : Compare(x) {
        }

        bool operator()(const Value& a, const Value& b) const { return Compare::operator()(a.first, b.first); }

        bool operator()(const Value& a, const Key& b) const { return Compare::operator()(a.first, b); }

        bool operator()(const Key& a, const Value& b) const { return Compare::operator()(a, b.first); }

        bool operator()(const Key& a, const Key& b) const { return Compare::operator()(a, b); }
    }; // multimap_value_compare

    namespace internal {
        template <typename Value, typename Compare>
        struct equal_range_comp {
            const Compare& comp;

            explicit equal_range_comp(const Compare& x)
                : comp(x) {
            }

            template <typename KX>
            bool operator()(const KX& lhs, const Value& rhs) const {
                return comp(lhs, rhs.first);
            }

            template <typename KX>
            bool operator()(const Value& lhs, const KX& rhs) const {
                return comp(lhs.first, rhs);
            }
        }; // equal_range_comp
    }

    /// VectorMultimap
    ///
    /// Implements a multimap via a random access container such as a vector.
    ///
    /// Note that with vector_set, vector_multiset, vector_map, VectorMultimap
    /// that the modification of the container potentially invalidates all
    /// existing iterators into the container, unlike what happens with conventional
    /// sets and maps.
    ///
    /// This type could conceptually use a fermat::array as its underlying container,
    /// however the current design requires an allocator aware container.
    /// Consider using a fixed_vector instead.
    ///
    /// Note that we set the value_type to be pair<Key, T> and not pair<const Key, T>.
    /// This means that the underlying container (e.g vector) is a container of pair<Key, T>.
    /// Our vector and deque implementations are optimized to assign values in-place and
    /// using a vector of pair<const Key, T> (note the const) would make it hard to use
    /// our existing vector implementation without a lot of headaches. As a result,
    /// at least for the time being we do away with the const. This means that the
    /// insertion type varies between map and vector_map in that the latter doesn't take
    /// const. This also means that a certain amount of automatic safety provided by
    /// the implementation is lost, as the compiler will let the wayward user modify
    /// a key and thus make the container no longer ordered behind its back.
    ///
    template <typename Key, typename T, typename Compare = std::less<Key>,
        typename RandomAccessContainer = std::vector<std::pair<Key, T>>>
    class VectorMultimap : protected multimap_value_compare<Key, std::pair<Key, T>, Compare>,
                           public RandomAccessContainer {
    public:
        typedef RandomAccessContainer base_type;
        typedef VectorMultimap<Key, T, Compare, RandomAccessContainer> this_type;
        typedef Key key_type;
        typedef T mapped_type;
        typedef std::pair<Key, T> value_type;
        typedef Compare key_compare;
        typedef multimap_value_compare<Key, value_type, Compare> value_compare;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef typename base_type::iterator iterator;
        typedef typename base_type::const_iterator const_iterator;
        typedef typename base_type::reverse_iterator reverse_iterator;
        typedef typename base_type::const_reverse_iterator const_reverse_iterator;

        using base_type::begin;
        using base_type::end;

    public:
        // We have an empty ctor and a ctor that takes an allocator instead of one for both
        // because this way our RandomAccessContainer wouldn't be required to have an constructor
        // that takes allocator_type.
        VectorMultimap();

        explicit VectorMultimap(const key_compare& comp);

        VectorMultimap(const this_type& x);

        VectorMultimap(this_type&& x);

        VectorMultimap(std::initializer_list<value_type> ilist, const key_compare& compare = key_compare());

        template <typename InputIterator>
        VectorMultimap(InputIterator first, InputIterator last);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        template <typename InputIterator>
        VectorMultimap(InputIterator first, InputIterator last, const key_compare& compare);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        this_type& operator=(const this_type& x);

        this_type& operator=(std::initializer_list<value_type> ilist);

        this_type& operator=(this_type&& x);

        void swap(this_type& x);

        const key_compare& key_comp() const;

        key_compare& key_comp();

        const value_compare& value_comp() const;

        value_compare& value_comp();

        // Inherited from base class:
        //
        //     void            set_allocator(const allocator_type& allocator);
        //
        //     iterator       begin();
        //     const_iterator begin() const;
        //     const_iterator cbegin() const;
        //
        //     iterator       end();
        //     const_iterator end() const;
        //     const_iterator cend() const;
        //
        //     reverse_iterator       rbegin();
        //     const_reverse_iterator rbegin() const;
        //     const_reverse_iterator crbegin() const;
        //
        //     reverse_iterator       rend();
        //     const_reverse_iterator rend() const;
        //     const_reverse_iterator crend() const;
        //
        //     size_type size() const;
        //     bool      empty() const;
        //     void      clear();

        template <class... Args>
        iterator emplace(Args&&... args);

        template <class... Args>
        iterator emplace_hint(const_iterator position, Args&&... args);

        iterator insert(const value_type& value);

        template <typename P, typename = std::enable_if_t<std::is_constructible_v<value_type, P&&>>>
        iterator insert(P&& otherValue);

        iterator insert(const key_type& otherValue);

        iterator insert(key_type&& otherValue);

        iterator insert(const_iterator position, const value_type& value);

        iterator insert(const_iterator position, value_type&& value);

        void insert(std::initializer_list<value_type> ilist);

        template <typename InputIterator>
        void insert(InputIterator first, InputIterator last);

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        size_type erase(const key_type& k) { return DoErase(k); }

        reverse_iterator erase(const_reverse_iterator position);

        reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        template <typename KX, typename Cmp = Compare,
            std::enable_if_t<!(std::is_convertible_v<KX&&, iterator> || std::is_convertible_v<KX&&, const_iterator>)
                    && fermat::vectors_internal::is_transparent_comparison_v<Cmp>,
                bool> = true>
        size_type erase(KX&& k) { return DoErase(std::forward<KX>(k)); }

        iterator find(const key_type& k) { return DoFind(k); }
        const_iterator find(const key_type& k) const { return DoFind(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        iterator find(const KX& k) { return DoFind(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        const_iterator find(const KX& k) const { return DoFind(k); }

        template <typename U, typename BinaryPredicate>
        iterator find_as(const U& u, BinaryPredicate predicate);

        template <typename U, typename BinaryPredicate>
        const_iterator find_as(const U& u, BinaryPredicate predicate) const;

        bool contains(const key_type& key) const { return DoFind(key) != end(); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        bool contains(const KX& key) const { return DoFind(key) != end(); }

        size_type count(const key_type& k) const { return DoCount(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        size_type count(const KX& k) const { return DoCount(k); }

        iterator lower_bound(const key_type& k) { return DoLowerBound(k); }
        const_iterator lower_bound(const key_type& k) const { return DoLowerBound(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        iterator lower_bound(const KX& k) { return DoLowerBound(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        const_iterator lower_bound(const KX& k) const { return DoLowerBound(k); }

        iterator upper_bound(const key_type& k) { return DoUpperBound(k); }
        const_iterator upper_bound(const key_type& k) const { return DoUpperBound(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        iterator upper_bound(const KX& k) { return DoUpperBound(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        const_iterator upper_bound(const KX& k) const { return DoUpperBound(k); }

        std::pair<iterator, iterator> equal_range(const key_type& k) { return DoEqualRange(k); }
        std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const { return DoEqualRange(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        std::pair<iterator, iterator> equal_range(const KX& key) { return DoEqualRange(key); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        std::pair<const_iterator, const_iterator> equal_range(const KX& key) const { return DoEqualRange(key); }

        /// equal_range_small
        /// This is a special version of equal_range which is optimized for the
        /// case of there being few or no duplicated keys in the tree.
        std::pair<iterator, iterator> equal_range_small(const key_type& k) { return DoEqualRangeSmall(k); }

        std::pair<const_iterator, const_iterator> equal_range_small(const key_type& k) const {
            return DoEqualRangeSmall(k);
        }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        std::pair<iterator, iterator> equal_range_small(const KX& k) { return DoEqualRangeSmall(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        std::pair<const_iterator, const_iterator> equal_range_small(const KX& k) const { return DoEqualRangeSmall(k); }

        // Functions which are disallowed due to being unsafe.
        void push_back(const value_type& value) = delete;

        reference push_back() = delete;

        void* push_back_uninitialized() = delete;

        template <class... Args>
        reference emplace_back(Args&&...) = delete;

        // NOTE(rparolin): It is undefined behaviour if user code fails to ensure the container
        // invariants are respected by performing an explicit call to 'sort' before any other
        // operations on the container are performed that do not clear the elements.
        //
        // 'push_back_unsorted' and 'emplace_back_unsorted' do not satisfy container invariants
        // for being sorted. We provide these overloads explicitly labelled as '_unsorted' as an
        // optimization opportunity when batch inserting elements so users can defer the cost of
        // sorting the container once when all elements are contained. This was done to clarify
        // the intent of code by leaving a trace that a manual call to sort is required.
        //
        template <typename... Args>
        decltype(auto) push_back_unsorted(Args&&... args) { return base_type::push_back(std::forward<Args>(args)...); }

        template <typename... Args>
        decltype(auto) emplace_back_unsorted(Args&&... args) {
            return base_type::emplace_back(std::forward<Args>(args)...);
        }

    private:
        template <typename KX>
        size_type DoErase(KX&& k);

        template <typename KX>
        iterator DoFind(const KX& key);

        template <typename KX>
        const_iterator DoFind(const KX& key) const;

        template <typename KX>
        size_type DoCount(const KX& k) const;

        template <typename KX>
        std::pair<iterator, iterator> DoEqualRange(const KX& key);

        template <typename KX>
        std::pair<const_iterator, const_iterator> DoEqualRange(const KX& key) const;

        template <typename KX>
        iterator DoLowerBound(const KX& k);

        template <typename KX>
        const_iterator DoLowerBound(const KX& k) const;

        template <typename KX>
        iterator DoUpperBound(const KX& k);

        template <typename KX>
        const_iterator DoUpperBound(const KX& k) const;

        template <typename KX>
        std::pair<iterator, iterator> DoEqualRangeSmall(const KX& key);

        template <typename KX>
        std::pair<const_iterator, const_iterator> DoEqualRangeSmall(const KX& key) const;
    }; // VectorMultimap

    ///////////////////////////////////////////////////////////////////////
    // VectorMultimap
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap()
        : value_compare(C())
        , base_type() {
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap(const key_compare& comp)
        : value_compare(comp)
        , base_type() {
        // Empty
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap(const this_type& x)
        : value_compare(x)
        , base_type(x) {
        // Empty
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap(this_type&& x)
        // careful to only copy / move the distinct base sub-objects of x:
        : value_compare(static_cast<value_compare&>(x))
        , base_type(std::move(static_cast<base_type&&>(x))) {
        // Empty. Note: x is left with empty contents but its original value_compare instead of the default one.
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap(std::initializer_list<value_type> ilist,
        const key_compare& compare)
        : value_compare(compare)
        , base_type() {
        insert(ilist.begin(), ilist.end());
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename InputIterator>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap(InputIterator first, InputIterator last)
        : value_compare(key_compare())
        , base_type() {
        insert(first, last);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename InputIterator>
    inline VectorMultimap<K, T, C, RAC>::VectorMultimap(InputIterator first, InputIterator last,
        const key_compare& compare)
        : value_compare(compare)
        , base_type() {
        insert(first, last);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::this_type&
    VectorMultimap<K, T, C, RAC>::operator=(const this_type& x) {
        base_type::operator=(x);
        value_compare::operator=(x);
        return *this;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::this_type&
    VectorMultimap<K, T, C, RAC>::operator=(this_type&& x) {
        base_type::operator=(std::move(x));
        using std::swap;
        swap(static_cast<value_compare&>(*this), static_cast<value_compare&>(x));
        return *this;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::this_type&
    VectorMultimap<K, T, C, RAC>::operator=(std::initializer_list<value_type> ilist) {
        base_type::clear();
        insert(ilist.begin(), ilist.end());
        return *this;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline void VectorMultimap<K, T, C, RAC>::swap(this_type& x) {
        base_type::swap(x);
        using std::swap;
        swap(static_cast<value_compare&>(*this), static_cast<value_compare&>(x));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline const typename VectorMultimap<K, T, C, RAC>::key_compare&
    VectorMultimap<K, T, C, RAC>::key_comp() const {
        return static_cast<const key_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::key_compare&
    VectorMultimap<K, T, C, RAC>::key_comp() {
        return static_cast<key_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline const typename VectorMultimap<K, T, C, RAC>::value_compare&
    VectorMultimap<K, T, C, RAC>::value_comp() const {
        return static_cast<const value_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::value_compare&
    VectorMultimap<K, T, C, RAC>::value_comp() {
        return static_cast<value_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <class... Args>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::emplace(Args&&... args) {
        value_type value(std::forward<Args>(args)...);
        return insert(std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <class... Args>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::emplace_hint(const_iterator position, Args&&... args) {
        value_type value(std::forward<Args>(args)...);
        return insert(position, std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::insert(const value_type& value) {
        const iterator itUB(upper_bound(value.first));
        return base_type::insert(itUB, value);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename P, typename>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::insert(P&& otherValue) {
        value_type value(std::forward<P>(otherValue));
        const iterator itUB(upper_bound(value.first));
        return base_type::insert(itUB, std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::insert(const key_type& otherValue) {
        value_type value(otherValue, mapped_type { });
        const iterator itUB(upper_bound(value.first));
        return base_type::insert(itUB, std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::insert(key_type&& otherValue) {
        value_type value(otherValue, mapped_type { });
        const iterator itUB(upper_bound(value.first));
        return base_type::insert(itUB, std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::insert(const_iterator position, const value_type& value) {
        // We assume that the user knows what he is doing and has supplied us with
        // a position that is right where value should be inserted (put in front of).
        // We do a test to see if the position is correct. If so then we insert,
        // if not then we ignore the input position. However,

        if ((position == end()) || !value_compare::operator()(*position, value))
        // If value is <= the element at position...
        {
            if ((position == begin()) || !value_compare::operator()(value, *(position - 1)))
                // If value is >= the element before position...
                return base_type::insert(position, value);
        }

        // In this case we have an incorrect position. We fall back to the regular insert function.
        return insert(value);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::insert(const_iterator position, value_type&& value) {
        if ((position == end()) || !value_compare::operator()(*position, value))
        // If value is <= the element at position...
        {
            if ((position == begin()) || !value_compare::operator()(value, *(position - 1)))
                // If value is >= the element before position...
                return base_type::insert(position, std::move(value));
        }

        // In this case we have an incorrect position. We fall back to the regular insert function.
        return insert(std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline void VectorMultimap<K, T, C, RAC>::insert(std::initializer_list<value_type> ilist) {
        insert(ilist.begin(), ilist.end());
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename InputIterator>
    inline void VectorMultimap<K, T, C, RAC>::insert(InputIterator first, InputIterator last) {
        // To consider: Improve the speed of this by getting the length of the
        //              input range and resizing our container to that size
        //              before doing the insertions. We can't use reserve
        //              because we don't know if we are using a vector or not.
        //              Alternatively, force the user to do the reservation.
        // To consider: When inserting values that come from a container
        //              like this container, use the property that they are
        //              known to be sorted and speed up the inserts here.
        for (; first != last; ++first)
            base_type::insert(upper_bound((*first).first), *first);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::erase(const_iterator position) {
        // Note that we return iterator and not void. This allows for more efficient use of
        // the container and is consistent with the C++ language defect report #130 (DR 130)
        return base_type::erase(position);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::erase(const_iterator first, const_iterator last) {
        return base_type::erase(first, last);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::size_type
    VectorMultimap<K, T, C, RAC>::DoErase(KX&& k) {
        const std::pair<iterator, iterator> pairIts(equal_range(k));

        if (pairIts.first != pairIts.second)
            base_type::erase(pairIts.first, pairIts.second);

        return (size_type)std::distance(pairIts.first, pairIts.second); // This can result in any value >= 0.
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::reverse_iterator
    VectorMultimap<K, T, C, RAC>::erase(const_reverse_iterator position) {
        return reverse_iterator(base_type::erase((++position).base()));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMultimap<K, T, C, RAC>::reverse_iterator
    VectorMultimap<K, T, C, RAC>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        return reverse_iterator(base_type::erase((++last).base(), (++first).base()));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::DoFind(const KX& k) {
        const std::pair<iterator, iterator> pairIts(equal_range(k));

        if (pairIts.first != pairIts.second)
            return pairIts.first;
        return end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::const_iterator
    VectorMultimap<K, T, C, RAC>::DoFind(const KX& k) const {
        const std::pair<const_iterator, const_iterator> pairIts(equal_range(k));

        if (pairIts.first != pairIts.second)
            return pairIts.first;
        return end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline typename VectorMultimap<K, T, C, RAC>::const_iterator
    VectorMultimap<K, T, C, RAC>::find_as(const U& u, BinaryPredicate predicate) const {
        const std::pair<const_iterator, const_iterator> pairIts(
            std::equal_range(begin(), end(), u, internal::equal_range_comp<value_type, BinaryPredicate>(predicate)));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::find_as(const U& u, BinaryPredicate predicate) {
        const std::pair<iterator, iterator> pairIts(
            std::equal_range(begin(), end(), u, internal::equal_range_comp<value_type, BinaryPredicate>(predicate)));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::size_type
    VectorMultimap<K, T, C, RAC>::DoCount(const KX& k) const {
        const std::pair<const_iterator, const_iterator> pairIts(equal_range(k));
        return (size_type)std::distance(pairIts.first, pairIts.second);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::DoLowerBound(const KX& k) {
        auto comp = [this](const value_type& value, const KX& key) {
            return static_cast<const key_compare&>(*this)(value.first, key);
        };
        return std::lower_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::const_iterator
    VectorMultimap<K, T, C, RAC>::DoLowerBound(const KX& k) const {
        auto comp = [this](const value_type& value, const KX& key) {
            return static_cast<const key_compare&>(*this)(value.first, key);
        };
        return std::lower_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::iterator
    VectorMultimap<K, T, C, RAC>::DoUpperBound(const KX& k) {
        auto comp = [this](const KX& key, const value_type& value) {
            return static_cast<const key_compare&>(*this)(key, value.first);
        };
        return std::upper_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultimap<K, T, C, RAC>::const_iterator
    VectorMultimap<K, T, C, RAC>::DoUpperBound(const KX& k) const {
        auto comp = [this](const KX& key, const value_type& value) {
            return static_cast<const key_compare&>(*this)(key, value.first);
        };
        return std::upper_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultimap<K, T, C, RAC>::iterator, typename VectorMultimap<K, T, C, RAC>::iterator>
    VectorMultimap<K, T, C, RAC>::DoEqualRange(const KX& k) {
        return std::equal_range(begin(), end(), k, internal::equal_range_comp<value_type, key_compare>(*this));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultimap<K, T, C, RAC>::const_iterator, typename VectorMultimap<K, T, C, RAC>::const_iterator>
    VectorMultimap<K, T, C, RAC>::DoEqualRange(const KX& k) const {
        return std::equal_range(begin(), end(), k, internal::equal_range_comp<value_type, key_compare>(*this));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultimap<K, T, C, RAC>::iterator, typename VectorMultimap<K, T, C, RAC>::iterator>
    VectorMultimap<K, T, C, RAC>::DoEqualRangeSmall(const KX& k) {
        const iterator itLower(lower_bound(k));
        iterator itUpper(itLower);

        while ((itUpper != end()) && !static_cast<key_compare&>(*this)(k, itUpper->first))
            ++itUpper;

        return std::pair<iterator, iterator>(itLower, itUpper);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultimap<K, T, C, RAC>::const_iterator, typename VectorMultimap<K, T, C, RAC>::const_iterator>
    VectorMultimap<K, T, C, RAC>::DoEqualRangeSmall(const KX& k) const {
        const const_iterator itLower(lower_bound(k));
        const_iterator itUpper(itLower);

        while ((itUpper != end()) && !static_cast<const key_compare&>(*this)(k, itUpper->first))
            ++itUpper;

        return std::pair<const_iterator, const_iterator>(itLower, itUpper);
    }

    ///////////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////////

    template <typename K, typename T, typename C, typename RAC>
    inline bool operator==(const VectorMultimap<K, T, C, RAC>& a,
        const VectorMultimap<K, T, C, RAC>& b) {
        return (a.size() == b.size()) && std::equal(b.begin(), b.end(), a.begin());
    }

    template <typename K, typename T, typename C, typename RAC>
    inline bool operator<(const VectorMultimap<K, T, C, RAC>& a,
        const VectorMultimap<K, T, C, RAC>& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), a.value_comp());
    }

    template <typename K, typename T, typename C, typename RAC>
    inline bool operator!=(const VectorMultimap<K, T, C, RAC>& a,
        const VectorMultimap<K, T, C, RAC>& b) {
        return !(a == b);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline bool operator>(const VectorMultimap<K, T, C, RAC>& a,
        const VectorMultimap<K, T, C, RAC>& b) {
        return b < a;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline bool operator<=(const VectorMultimap<K, T, C, RAC>& a,
        const VectorMultimap<K, T, C, RAC>& b) {
        return !(b < a);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline bool operator>=(const VectorMultimap<K, T, C, RAC>& a,
        const VectorMultimap<K, T, C, RAC>& b) {
        return !(a < b);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline void swap(VectorMultimap<K, T, C, RAC>& a,
        VectorMultimap<K, T, C, RAC>& b) {
        a.swap(b);
    }
} // namespace fermat

