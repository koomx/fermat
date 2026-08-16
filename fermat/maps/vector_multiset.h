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
// This file implements VectorMultiset. It acts much like std::multiset, except
// its underlying representation is a random access container such as vector.
// These containers are sometimes also known as "sorted vectors."
// vector_sets have an advantage over conventional sets in that their memory
// is contiguous and node-less. The result is that lookups are faster, more
// cache friendly (which potentially more so benefits speed), and the container
// uses less memory. The downside is that inserting new items into the container
// is slower if they are inserted in random order instead of in sorted order.
// This tradeoff is well-worth it for many cases. Note that VectorMultiset allows
// you to use a deque or other random access container which may perform
// better for you than vector.
//
// Note that with vector_set, VectorMultiset, vector_map, vector_multimap
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
    /// VectorMultiset
    ///
    /// Implements a multiset via a random access container such as a vector.
    /// This container is also known as a sorted_vector. We choose to call it
    /// VectorMultiset, as that is a more consistent universally applicable name
    /// for it in this library.
    ///
    /// Note that with vector_set, VectorMultiset, vector_map, vector_multimap
    /// that the modification of the container potentially invalidates all
    /// existing iterators into the container, unlike what happens with conventional
    /// sets and maps.
    ///
    /// This type could conceptually use a fermat::array as its underlying container,
    /// however the current design requires an allocator aware container.
    /// Consider using a fixed_vector instead.
    ///
    /// To consider: std::multiset has the limitation that values in the set cannot
    /// be modified, with the idea that modifying them would change their sort
    /// order. We have the opportunity to make it so that values can be modified
    /// via changing iterators to be non-const, with the downside being that
    /// the container can get screwed up if the user screws up. Alternatively,
    /// we can do what std STL does and require the user to make their stored
    /// classes use 'mutable' as needed. See the C++ standard defect report
    /// #103 (DR 103) for a discussion of this.
    ///
    template <typename Key, typename Compare = std::less<Key>,
        typename RandomAccessContainer = std::vector<Key>>
    class VectorMultiset : protected Compare, public RandomAccessContainer {
    public:
        typedef RandomAccessContainer base_type;
        typedef VectorMultiset<Key, Compare, RandomAccessContainer> this_type;
        typedef Key key_type;
        typedef Key value_type;
        typedef Compare key_compare;
        typedef Compare value_compare;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef typename base_type::iterator iterator;
        // **Currently typedefing from iterator instead of const_iterator due to const issues **: Note that we typedef from const_iterator. This is by design, as sets are sorted and values cannot be modified. To consider: allow values to be modified and thus risk changing their sort values.
        typedef typename base_type::const_iterator const_iterator;
        typedef typename base_type::reverse_iterator reverse_iterator;
        // See notes directly above regarding const_iterator.
        typedef typename base_type::const_reverse_iterator const_reverse_iterator;

        using base_type::begin;
        using base_type::end;

    public:
        // We have an empty ctor and a ctor that takes an allocator instead of one for both
        // because this way our RandomAccessContainer wouldn't be required to have an constructor
        // that takes allocator_type.
        VectorMultiset();

        explicit VectorMultiset(const key_compare& comp);

        VectorMultiset(const this_type& x);

        VectorMultiset(this_type&& x);

        VectorMultiset(std::initializer_list<value_type> ilist, const key_compare& compare = key_compare());

        template <typename InputIterator>
        VectorMultiset(InputIterator first, InputIterator last);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        template <typename InputIterator>
        VectorMultiset(InputIterator first, InputIterator last, const key_compare& compare);

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

        iterator insert(value_type&& value);

        iterator insert(const_iterator position, const value_type& value);

        iterator insert(const_iterator position, value_type&& value);

        void insert(std::initializer_list<value_type> ilist);

        template <typename InputIterator>
        void insert(InputIterator first, InputIterator last);

        template <typename Iter = iterator, typename std::enable_if<!std::is_same_v<Iter, const_iterator>, int>::type = 0>
        iterator erase(iterator position) { return erase(const_iterator(position)); }

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        size_type erase(const key_type& k) { return DoErase(k); }

        reverse_iterator erase(const_reverse_iterator position);

        reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        template <typename KX, typename Cmp = Compare,
            std::enable_if_t<!std::is_convertible_v<KX&&, iterator> && !std::is_convertible_v<KX&&, const_iterator>
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
        std::pair<iterator, iterator> equal_range(const KX& k) { return DoEqualRange(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        std::pair<const_iterator, const_iterator> equal_range(const KX& k) const { return DoEqualRange(k); }

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
        iterator DoFind(const KX& k);

        template <typename KX>
        const_iterator DoFind(const KX& k) const;

        template <typename KX>
        size_type DoCount(const KX& k) const;

        template <typename KX>
        std::pair<iterator, iterator> DoEqualRange(const KX& k);

        template <typename KX>
        std::pair<const_iterator, const_iterator> DoEqualRange(const KX& k) const;

        template <typename KX>
        iterator DoLowerBound(const KX& k);

        template <typename KX>
        const_iterator DoLowerBound(const KX& k) const;

        template <typename KX>
        iterator DoUpperBound(const KX& k);

        template <typename KX>
        const_iterator DoUpperBound(const KX& k) const;

        template <typename KX>
        std::pair<iterator, iterator> DoEqualRangeSmall(const KX& k);

        template <typename KX>
        std::pair<const_iterator, const_iterator> DoEqualRangeSmall(const KX& k) const;
    }; // VectorMultiset

    ///////////////////////////////////////////////////////////////////////
    // VectorMultiset
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>::VectorMultiset()
        : value_compare()
        , base_type() {
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>::VectorMultiset(const key_compare& comp)
        : value_compare(comp)
        , base_type() {
        // Empty
    }

    template <typename K, typename C, typename RAC>
    template <typename InputIterator>
    inline VectorMultiset<K, C, RAC>::VectorMultiset(InputIterator first, InputIterator last)
        : value_compare()
        , base_type() {
        insert(first, last);
    }

    template <typename K, typename C, typename RAC>
    template <typename InputIterator>
    inline VectorMultiset<K, C, RAC>::VectorMultiset(InputIterator first, InputIterator last,
        const key_compare& compare)
        : value_compare(compare)
        , base_type() {
        insert(first, last);
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>::VectorMultiset(const this_type& x)
        : value_compare(x)
        , base_type(x) {
        // Empty
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>::VectorMultiset(this_type&& x)
        // careful to only copy / move the distinct base sub-objects of x:
        : value_compare(static_cast<value_compare&>(x))
        , base_type(std::move(static_cast<base_type&&>(x))) {
        // Empty. Note: x is left with empty contents but its original value_compare instead of the default one.
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>::VectorMultiset(std::initializer_list<value_type> ilist,
        const key_compare& compare)
        : value_compare(compare)
        , base_type() {
        insert(ilist.begin(), ilist.end());
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>&
    VectorMultiset<K, C, RAC>::operator=(const this_type& x) {
        base_type::operator=(x);
        value_compare::operator=(x);
        return *this;
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>&
    VectorMultiset<K, C, RAC>::operator=(this_type&& x) {
        base_type::operator=(std::move(x));
        using std::swap;
        swap(static_cast<value_compare&>(*this), static_cast<value_compare&>(x));
        return *this;
    }

    template <typename K, typename C, typename RAC>
    inline VectorMultiset<K, C, RAC>&
    VectorMultiset<K, C, RAC>::operator=(std::initializer_list<value_type> ilist) {
        base_type::clear();
        insert(ilist.begin(), ilist.end());
        return *this;
    }

    template <typename K, typename C, typename RAC>
    inline void VectorMultiset<K, C, RAC>::swap(this_type& x) {
        base_type::swap(x);
        using std::swap;
        swap(static_cast<value_compare&>(*this), static_cast<value_compare&>(x));
    }

    template <typename K, typename C, typename RAC>
    inline const typename VectorMultiset<K, C, RAC>::key_compare&
    VectorMultiset<K, C, RAC>::key_comp() const {
        return static_cast<const key_compare&>(*this);
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::key_compare&
    VectorMultiset<K, C, RAC>::key_comp() {
        return static_cast<key_compare&>(*this);
    }

    template <typename K, typename C, typename RAC>
    inline const typename VectorMultiset<K, C, RAC>::value_compare&
    VectorMultiset<K, C, RAC>::value_comp() const {
        return static_cast<const value_compare&>(*this);
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::value_compare&
    VectorMultiset<K, C, RAC>::value_comp() {
        return static_cast<value_compare&>(*this);
    }

    template <typename K, typename C, typename RAC>
    template <class... Args>
    typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::emplace(Args&&... args) {
        value_type value(std::forward<Args>(args)...);
        return insert(std::move(value));
    }

    template <typename K, typename C, typename RAC>
    template <class... Args>
    typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::emplace_hint(const_iterator position, Args&&... args) {
        value_type value(std::forward<Args>(args)...);
        return insert(position, std::move(value));
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::insert(const value_type& value) {
        const iterator itUB(upper_bound(value));
        return base_type::insert(itUB, value);
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::insert(value_type&& value) {
        const iterator itUB(upper_bound(value));
        return base_type::insert(itUB, std::move(value));
    }

    template <typename K, typename C, typename RAC>
    inline void VectorMultiset<K, C, RAC>::insert(std::initializer_list<value_type> ilist) {
        insert(ilist.begin(), ilist.end());
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::insert(const_iterator position, const value_type& value) {
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

    template <typename K, typename C, typename RAC>
    typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::insert(const_iterator position, value_type&& value) {
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

    template <typename K, typename C, typename RAC>
    template <typename InputIterator>
    inline void VectorMultiset<K, C, RAC>::insert(InputIterator first, InputIterator last) {
        // To consider: Improve the speed of this by getting the length of the
        //              input range and resizing our container to that size
        //              before doing the insertions. We can't use reserve
        //              because we don't know if we are using a vector or not.
        //              Alternatively, force the user to do the reservation.
        // To consider: When inserting values that come from a container
        //              like this container, use the property that they are
        //              known to be sorted and speed up the inserts here.
        for (; first != last; ++first)
            base_type::insert(upper_bound(*first), *first);
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::erase(const_iterator position) {
        // Note that we return iterator and not void. This allows for more efficient use of
        // the container and is consistent with the C++ language defect report #130 (DR 130)
        return base_type::erase(position);
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::erase(const_iterator first, const_iterator last) {
        return base_type::erase(first, last);
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::reverse_iterator
    VectorMultiset<K, C, RAC>::erase(const_reverse_iterator position) {
        return reverse_iterator(base_type::erase((++position).base()));
    }

    template <typename K, typename C, typename RAC>
    inline typename VectorMultiset<K, C, RAC>::reverse_iterator
    VectorMultiset<K, C, RAC>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        return reverse_iterator(base_type::erase((++last).base(), (++first).base()));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::size_type
    VectorMultiset<K, C, RAC>::DoErase(KX&& k) {
        const std::pair<iterator, iterator> pairIts(equal_range(k));

        if (pairIts.first != pairIts.second)
            base_type::erase(pairIts.first, pairIts.second);

        return (size_type)std::distance(pairIts.first, pairIts.second); // This can result in any value >= 0.
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::DoFind(const KX& k) {
        const std::pair<iterator, iterator> pairIts(equal_range(k));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::const_iterator
    VectorMultiset<K, C, RAC>::DoFind(const KX& k) const {
        const std::pair<const_iterator, const_iterator> pairIts(equal_range(k));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::find_as(const U& u, BinaryPredicate predicate) {
        const std::pair<iterator, iterator> pairIts(std::equal_range(begin(), end(), u, predicate));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline typename VectorMultiset<K, C, RAC>::const_iterator
    VectorMultiset<K, C, RAC>::find_as(const U& u, BinaryPredicate predicate) const {
        const std::pair<const_iterator, const_iterator> pairIts(std::equal_range(begin(), end(), u, predicate));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::size_type
    VectorMultiset<K, C, RAC>::DoCount(const KX& k) const {
        const std::pair<const_iterator, const_iterator> pairIts(equal_range(k));
        return (size_type)std::distance(pairIts.first, pairIts.second);
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::DoLowerBound(const KX& k) {
        return std::lower_bound(begin(), end(), k, static_cast<value_compare&>(*this));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::const_iterator
    VectorMultiset<K, C, RAC>::DoLowerBound(const KX& k) const {
        return std::lower_bound(begin(), end(), k, static_cast<const value_compare&>(*this));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::iterator
    VectorMultiset<K, C, RAC>::DoUpperBound(const KX& k) {
        return std::upper_bound(begin(), end(), k, static_cast<value_compare&>(*this));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMultiset<K, C, RAC>::const_iterator
    VectorMultiset<K, C, RAC>::DoUpperBound(const KX& k) const {
        return std::upper_bound(begin(), end(), k, static_cast<const value_compare&>(*this));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultiset<K, C, RAC>::iterator, typename VectorMultiset<K, C, RAC>::iterator>
    VectorMultiset<K, C, RAC>::DoEqualRange(const KX& k) {
        return std::equal_range(begin(), end(), k, static_cast<value_compare&>(*this));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultiset<K, C, RAC>::const_iterator, typename VectorMultiset<K, C, RAC>::const_iterator>
    VectorMultiset<K, C, RAC>::DoEqualRange(const KX& k) const {
        return std::equal_range(begin(), end(), k, static_cast<const value_compare&>(*this));
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultiset<K, C, RAC>::iterator, typename VectorMultiset<K, C, RAC>::iterator>
    VectorMultiset<K, C, RAC>::DoEqualRangeSmall(const KX& k) {
        const iterator itLower(lower_bound(k));
        iterator itUpper(itLower);

        while ((itUpper != end()) && !value_compare::operator()(k, *itUpper))
            ++itUpper;

        return std::pair<iterator, iterator>(itLower, itUpper);
    }

    template <typename K, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMultiset<K, C, RAC>::const_iterator, typename VectorMultiset<K, C, RAC>::const_iterator>
    VectorMultiset<K, C, RAC>::DoEqualRangeSmall(const KX& k) const {
        const const_iterator itLower(lower_bound(k));
        const_iterator itUpper(itLower);

        while ((itUpper != end()) && !value_compare::operator()(k, *itUpper))
            ++itUpper;

        return std::pair<const_iterator, const_iterator>(itLower, itUpper);
    }

    ///////////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////////

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline bool operator==(const VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        const VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        return (a.size() == b.size()) && std::equal(b.begin(), b.end(), a.begin());
    }

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline bool operator<(const VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        const VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), a.value_comp());
    }

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline bool operator!=(const VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        const VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        return !(a == b);
    }

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline bool operator>(const VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        const VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        return b < a;
    }

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline bool operator<=(const VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        const VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        return !(b < a);
    }

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline bool operator>=(const VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        const VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        return !(a < b);
    }

    template <typename Key, typename Compare, typename RandomAccessContainer>
    inline void swap(VectorMultiset<Key, Compare, RandomAccessContainer>& a,
        VectorMultiset<Key, Compare, RandomAccessContainer>& b) {
        a.swap(b);
    }
} // namespace fermat

