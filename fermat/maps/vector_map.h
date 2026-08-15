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
// This file implements VectorMap. It acts much like std::map, except its
// underlying representation is a random access container such as vector.
// These containers are sometimes also known as "sorted vectors."
// vector_maps have an advantage over conventional maps in that their memory
// is contiguous and node-less. The result is that lookups are faster, more
// cache friendly (which potentially more so benefits speed), and the container
// uses less memory. The downside is that inserting new items into the container
// is slower if they are inserted in random order instead of in sorted order.
// This tradeoff is well-worth it for many cases. Note that VectorMap allows
// you to use a deque or other random access container which may perform
// better for you than vector.
//
// Note that with vector_set, vector_multiset, VectorMap, vector_multimap
// that the modification of the container potentially invalidates all
// existing iterators into the container, unlike what happens with conventional
// sets and maps.
//
// This type could conceptually use a fermat::array as its underlying container,
// however the current design requires an allocator aware container.
// Consider using a fixed_vector instead.
//////////////////////////////////////////////////////////////////////////////
///

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

#include <turbo/base/internal/raw_logging.h>
#include <fermat/maps/internal/sorted_container.h>

namespace fermat {
    /// map_value_compare
    ///
    /// Our adapter for the comparison function in the template parameters.
    ///
    /// todo: deprecate this. shouldn't have all these member functions available (including constructor) and shouldn't be a public type.
    template <typename Key, typename Value, typename Compare>
    class map_value_compare : public Compare {
    public:
        explicit map_value_compare(const Compare& x)
            : Compare(x) {
        }

        bool operator()(const Value& a, const Value& b) const { return Compare::operator()(a.first, b.first); }

        bool operator()(const Value& a, const Key& b) const { return Compare::operator()(a.first, b); }

        bool operator()(const Key& a, const Value& b) const { return Compare::operator()(a, b.first); }

        bool operator()(const Key& a, const Key& b) const { return Compare::operator()(a, b); }
    }; // map_value_compare

    /// VectorMap
    ///
    /// Implements a map via a random access container such as a vector.
    ///
    /// Note that with vector_set, vector_multiset, VectorMap, vector_multimap
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
    /// insertion type varies between map and VectorMap in that the latter doesn't take
    /// const. This also means that a certain amount of automatic safety provided by
    /// the implementation is lost, as the compiler will let the wayward user modify
    /// a key and thus make the container no longer ordered behind its back.
    ///
    template <typename Key, typename T, typename Compare = std::less<Key>,
        typename RandomAccessContainer = std::vector<std::pair<Key, T>>>
    class VectorMap : protected map_value_compare<Key, std::pair<Key, T>, Compare>, public RandomAccessContainer {
    public:
        typedef RandomAccessContainer base_type;
        typedef VectorMap<Key, T, Compare, RandomAccessContainer> this_type;
        typedef Key key_type;
        typedef T mapped_type;
        typedef std::pair<Key, T> value_type;
        typedef Compare key_compare;
        typedef map_value_compare<Key, value_type, Compare> value_compare;
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
        typedef std::pair<iterator, bool> insert_return_type;

        using base_type::begin;
        using base_type::end;

    public:
        VectorMap();

        explicit VectorMap(const key_compare& comp);

        VectorMap(const this_type& x);

        VectorMap(this_type&& x);

        VectorMap(std::initializer_list<value_type> ilist, const key_compare& compare = key_compare());

        template <typename InputIterator>
        VectorMap(InputIterator first, InputIterator last);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        template <typename InputIterator>
        VectorMap(InputIterator first, InputIterator last, const key_compare& compare);

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
        std::pair<iterator, bool> emplace(Args&&... args);

        template <class... Args>
        iterator emplace_hint(const_iterator position, Args&&... args);

        std::pair<iterator, bool> insert(const value_type& value);

        std::pair<iterator, bool> insert(const key_type& otherValue);

        std::pair<iterator, bool> insert(key_type&& otherValue);

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
        std::pair<iterator, iterator> equal_range(const KX& k) { return DoEqualRange(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        std::pair<const_iterator, const_iterator> equal_range(const KX& k) const { return DoEqualRange(k); }

        template <typename U, typename BinaryPredicate>
        std::pair<iterator, iterator> equal_range(const U& u, BinaryPredicate predicate);

        template <typename U, typename BinaryPredicate>
        std::pair<const_iterator, const_iterator> equal_range(const U& u, BinaryPredicate) const;

        // Note: VectorMap operator[] returns a reference to the mapped_type, same as map does.
        // But there's an important difference: This reference can be invalidated by -any- changes
        // to the VectorMap that cause it to change capacity. This is unlike map, with which
        // mapped_type references are invalidated only if that mapped_type element itself is removed
        // from the map. This is because vector is array-based and map is node-based. As a result
        // the following code that is safe for map is unsafe for VectorMap for the case that
        // the vMap[100] doesn't already exist in the VectorMap:
        //     vMap[100] = vMap[0]
        mapped_type& operator[](const key_type& k) { return DoGetElement(k); }
        mapped_type& operator[](key_type&& k) { return DoGetElement(std::move(k)); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        mapped_type& operator[](KX&& k) { return DoGetElement(std::forward<KX>(k)); }

        // non-standard! this was originally inherited from vector with incorrect semantics.
        // this is only defined so that we can deprecate it.
        // use `*(map.begin() + index)` if you want to get an element by index.
        reference at(size_type index);

        const_reference at(size_type index) const;

        // after the deprecation period the above should be replaced with:
        // mapped_type& at(const key_type& k) { return at_key(k); }
        // const mapped_type& at(const key_type& k) const { return at_key(k); }

        // aka. the standard's at() member function.
        mapped_type& at_key(const key_type& k) { return DoAtKey(k); }
        const mapped_type& at_key(const key_type& k) const { return DoAtKey(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        mapped_type& at_key(const KX& k) { return DoAtKey(k); }

        template <typename KX, typename Cmp = Compare, std::enable_if_t<fermat::vectors_internal::is_transparent_comparison_v<Cmp>, bool> = true>
        const mapped_type& at_key(const KX& k) const { return DoAtKey(k); }

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
        mapped_type& DoGetElement(KX&& k);

        template <typename KX>
        mapped_type& DoAtKey(const KX& k);

        template <typename KX>
        const mapped_type& DoAtKey(const KX& k) const;
    }; // VectorMap

    ///////////////////////////////////////////////////////////////////////
    // VectorMap
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>::VectorMap()
        : value_compare(C())
        , base_type() {
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>::VectorMap(const key_compare& comp)
        : value_compare(comp)
        , base_type() {
        // Empty
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>::VectorMap(const this_type& x)
        : value_compare(x)
        , base_type(x) {
        // Empty
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>::VectorMap(this_type&& x)
        // careful to only copy / move the distinct base sub-objects of x:
        : value_compare(static_cast<value_compare&>(x))
        , base_type(std::move(static_cast<base_type&&>(x))) {
        // Empty. Note: x is left with empty contents but its original value_compare instead of the default one.
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>::VectorMap(std::initializer_list<value_type> ilist, const key_compare& compare)
        : value_compare(compare)
        , base_type() {
        insert(ilist.begin(), ilist.end());
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename InputIterator>
    inline VectorMap<K, T, C, RAC>::VectorMap(InputIterator first, InputIterator last)
        : value_compare(key_compare())
        , base_type() {
        insert(first, last);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename InputIterator>
    inline VectorMap<K, T, C, RAC>::VectorMap(InputIterator first, InputIterator last, const key_compare& compare)
        : value_compare(compare)
        , base_type() {
        insert(first, last);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>&
    VectorMap<K, T, C, RAC>::operator=(const this_type& x) {
        base_type::operator=(x);
        value_compare::operator=(x);
        return *this;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>&
    VectorMap<K, T, C, RAC>::operator=(this_type&& x) {
        base_type::operator=(std::move(static_cast<base_type&&>(x)));
        using std::swap;
        swap(static_cast<value_compare&>(*this), static_cast<value_compare&>(x));
        return *this;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline VectorMap<K, T, C, RAC>&
    VectorMap<K, T, C, RAC>::operator=(std::initializer_list<value_type> ilist) {
        base_type::clear();
        insert(ilist.begin(), ilist.end());
        return *this;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline void VectorMap<K, T, C, RAC>::swap(this_type& x) {
        base_type::swap(x);
        using std::swap;
        swap(static_cast<value_compare&>(*this), static_cast<value_compare&>(x));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline const typename VectorMap<K, T, C, RAC>::key_compare&
    VectorMap<K, T, C, RAC>::key_comp() const {
        return static_cast<const key_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::key_compare&
    VectorMap<K, T, C, RAC>::key_comp() {
        return static_cast<key_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline const typename VectorMap<K, T, C, RAC>::value_compare&
    VectorMap<K, T, C, RAC>::value_comp() const {
        return static_cast<const value_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::value_compare&
    VectorMap<K, T, C, RAC>::value_comp() {
        return static_cast<value_compare&>(*this);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <class... Args>
    inline std::pair<typename VectorMap<K, T, C, RAC>::iterator, bool>
    VectorMap<K, T, C, RAC>::emplace(Args&&... args) {
        value_type value(std::forward<Args>(args)...);
        return insert(std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <class... Args>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::emplace_hint(const_iterator position, Args&&... args) {
        value_type value(std::forward<Args>(args)...);
        return insert(position, std::move(value));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline std::pair<typename VectorMap<K, T, C, RAC>::iterator, bool>
    VectorMap<K, T, C, RAC>::insert(const value_type& value) {
        const iterator itLB(lower_bound(value.first));

        if ((itLB != end()) && !value_compare::operator()(value, *itLB))
            return std::pair<iterator, bool>(itLB, false);

        return std::pair<iterator, bool>(base_type::insert(itLB, value), true);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline std::pair<typename VectorMap<K, T, C, RAC>::iterator, bool>
    VectorMap<K, T, C, RAC>::insert(const key_type& otherValue) {
        value_type value(otherValue, mapped_type { });
        const iterator itLB(lower_bound(value.first));

        if ((itLB != end()) && !value_compare::operator()(value, *itLB))
            return std::pair<iterator, bool>(itLB, false);

        return std::pair<iterator, bool>(base_type::insert(itLB, std::move(value)), true);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline std::pair<typename VectorMap<K, T, C, RAC>::iterator, bool>
    VectorMap<K, T, C, RAC>::insert(key_type&& otherValue) {
        value_type value(otherValue, mapped_type { });
        const iterator itLB(lower_bound(value.first));

        if ((itLB != end()) && !value_compare::operator()(value, *itLB))
            return std::pair<iterator, bool>(itLB, false);

        return std::pair<iterator, bool>(base_type::insert(itLB, std::move(value)), true);
    }

    template <typename K, typename T, typename C, typename RAC>
    typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::insert(const_iterator position, const value_type& value) {
        // We assume that the user knows what he is doing and has supplied us with
        // a position that is right where value should be inserted (put in front of).
        // We do a test to see if the position is correct. If so then we insert,
        // if not then we ignore the input position.

        if ((position == end()) || value_compare::operator()(value, *position))
        // If the element at position is greater than value...
        {
            if ((position == begin()) || value_compare::operator()(*(position - 1), value))
                // If the element before position is less than value...
                return base_type::insert(position, value);
        }

        // In this case we either have an incorrect position or value is already present.
        // We fall back to the regular insert function. An optimization would be to detect
        // that the element is already present, but that's only useful if the user supplied
        // a good position but a present element.
        const std::pair<typename VectorMap<K, T, C, RAC>::iterator, bool> result = insert(value);

        return result.first;
    }

    template <typename K, typename T, typename C, typename RAC>
    typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::insert(const_iterator position, value_type&& value) {
        if ((position == end()) || value_compare::operator()(value, *position))
        // If the element at position is greater than value...
        {
            if ((position == begin()) || value_compare::operator()(*(position - 1), value))
                // If the element before position is less than value...
                return base_type::insert(position, std::move(value));
        }

        const std::pair<typename VectorMap<K, T, C, RAC>::iterator, bool> result = insert(std::move(value));

        return result.first;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline void VectorMap<K, T, C, RAC>::insert(std::initializer_list<value_type> ilist) {
        insert(ilist.begin(), ilist.end());
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename InputIterator>
    inline void VectorMap<K, T, C, RAC>::insert(InputIterator first, InputIterator last) {
        // To consider: Improve the speed of this by getting the length of the
        //              input range and resizing our container to that size
        //              before doing the insertions. We can't use reserve
        //              because we don't know if we are using a vector or not.
        //              Alternatively, force the user to do the reservation.
        // To consider: When inserting values that come from a container
        //              like this container, use the property that they are
        //              known to be sorted and speed up the inserts here.
        for (; first != last; ++first)
            insert(*first);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::erase(const_iterator position) {
        // Note that we return iterator and not void. This allows for more efficient use of
        // the container and is consistent with the C++ language defect report #130 (DR 130)
        return base_type::erase(position);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::erase(const_iterator first, const_iterator last) {
        return base_type::erase(first, last);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::size_type
    VectorMap<K, T, C, RAC>::DoErase(KX&& k) {
        const iterator it(find(std::forward<KX>(k)));

        if (it != end()) // If it exists...
        {
            erase(it);
            return 1;
        }
        return 0;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::reverse_iterator
    VectorMap<K, T, C, RAC>::erase(const_reverse_iterator position) {
        return reverse_iterator(base_type::erase((++position).base()));
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::reverse_iterator
    VectorMap<K, T, C, RAC>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        return reverse_iterator(base_type::erase((++last).base(), (++first).base()));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::DoFind(const KX& k) {
        const std::pair<iterator, iterator> pairIts(equal_range(k));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::const_iterator
    VectorMap<K, T, C, RAC>::DoFind(const KX& k) const {
        const std::pair<const_iterator, const_iterator> pairIts(equal_range(k));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::find_as(const U& u, BinaryPredicate predicate) {
        const std::pair<iterator, iterator> pairIts(equal_range(u, predicate));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline typename VectorMap<K, T, C, RAC>::const_iterator
    VectorMap<K, T, C, RAC>::find_as(const U& u, BinaryPredicate predicate) const {
        const std::pair<const_iterator, const_iterator> pairIts(equal_range(u, predicate));
        return (pairIts.first != pairIts.second) ? pairIts.first : end();
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::size_type
    VectorMap<K, T, C, RAC>::DoCount(const KX& k) const {
        const const_iterator it(find(k));
        return (it != end()) ? (size_type)1 : (size_type)0;
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::DoLowerBound(const KX& k) {
        auto comp = [this](const value_type& value, const KX& key) {
            return static_cast<const key_compare&>(*this)(value.first, key);
        };
        return std::lower_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::const_iterator
    VectorMap<K, T, C, RAC>::DoLowerBound(const KX& k) const {
        auto comp = [this](const value_type& value, const KX& key) {
            return static_cast<const key_compare&>(*this)(value.first, key);
        };
        return std::lower_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::iterator
    VectorMap<K, T, C, RAC>::DoUpperBound(const KX& k) {
        auto comp = [this](const KX& key, const value_type& value) {
            return static_cast<const key_compare&>(*this)(key, value.first);
        };
        return std::upper_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::const_iterator
    VectorMap<K, T, C, RAC>::DoUpperBound(const KX& k) const {
        auto comp = [this](const KX& key, const value_type& value) {
            return static_cast<const key_compare&>(*this)(key, value.first);
        };
        return std::upper_bound(begin(), end(), k, comp);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMap<K, T, C, RAC>::iterator, typename VectorMap<K, T, C, RAC>::iterator>
    VectorMap<K, T, C, RAC>::DoEqualRange(const KX& k) {
        // The resulting range will either be empty or have one element,
        // so instead of doing two tree searches (one for lower_bound and
        // one for upper_bound), we do just lower_bound and see if the
        // result is a range of size zero or one.
        const iterator itLower(lower_bound(k));

        if ((itLower == end()) || static_cast<key_compare&>(*this)(k, itLower->first))
            // If at the end or if (k is < itLower)...
            return std::pair<iterator, iterator>(itLower, itLower);

        iterator itUpper(itLower);
        return std::pair<iterator, iterator>(itLower, ++itUpper);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline std::pair<typename VectorMap<K, T, C, RAC>::const_iterator, typename VectorMap<K, T, C, RAC>::const_iterator>
    VectorMap<K, T, C, RAC>::DoEqualRange(const KX& k) const {
        // The resulting range will either be empty or have one element,
        // so instead of doing two tree searches (one for lower_bound and
        // one for upper_bound), we do just lower_bound and see if the
        // result is a range of size zero or one.
        const const_iterator itLower(lower_bound(k));

        if ((itLower == end()) || static_cast<const key_compare&>(*this)(k, itLower->first))
            // If at the end or if (k is < itLower)...
            return std::pair<const_iterator, const_iterator>(itLower, itLower);

        const_iterator itUpper(itLower);
        return std::pair<const_iterator, const_iterator>(itLower, ++itUpper);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline std::pair<typename VectorMap<K, T, C, RAC>::iterator, typename VectorMap<K, T, C, RAC>::iterator>
    VectorMap<K, T, C, RAC>::equal_range(const U& u, BinaryPredicate predicate) {
        // The resulting range will either be empty or have one element,
        // so instead of doing two tree searches (one for lower_bound and
        // one for upper_bound), we do just lower_bound and see if the
        // result is a range of size zero or one.
        map_value_compare<U, value_type, BinaryPredicate> predicate_cmp(predicate);

        const iterator itLower(std::lower_bound(begin(), end(), u, predicate_cmp));

        if ((itLower == end()) || predicate_cmp(u, *itLower)) // If at the end or if (k is < itLower)...
            return std::pair<iterator, iterator>(itLower, itLower);

        iterator itUpper(itLower);
        return std::pair<iterator, iterator>(itLower, ++itUpper);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename U, typename BinaryPredicate>
    inline std::pair<typename VectorMap<K, T, C, RAC>::const_iterator, typename VectorMap<K, T, C, RAC>::const_iterator>
    VectorMap<K, T, C, RAC>::equal_range(const U& u, BinaryPredicate predicate) const {
        // The resulting range will either be empty or have one element,
        // so instead of doing two tree searches (one for lower_bound and
        // one for upper_bound), we do just lower_bound and see if the
        // result is a range of size zero or one.
        map_value_compare<U, value_type, BinaryPredicate> predicate_cmp(predicate);

        const const_iterator itLower(std::lower_bound(begin(), end(), u, predicate_cmp));

        if ((itLower == end()) || predicate_cmp(u, *itLower)) // If at the end or if (k is < itLower)...
            return std::pair<const_iterator, const_iterator>(itLower, itLower);

        const_iterator itUpper(itLower);
        return std::pair<const_iterator, const_iterator>(itLower, ++itUpper);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::mapped_type&
    VectorMap<K, T, C, RAC>::DoGetElement(KX&& k) {
        iterator itLB(lower_bound(k));

        if ((itLB == end()) || key_comp()(k, (*itLB).first))
            itLB = insert(itLB, value_type(std::forward<KX>(k), mapped_type()));
        return (*itLB).second;
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::reference
    VectorMap<K, T, C, RAC>::at(size_type index) {
        return *(begin() + index);
    }

    template <typename K, typename T, typename C, typename RAC>
    inline typename VectorMap<K, T, C, RAC>::const_reference
    VectorMap<K, T, C, RAC>::at(size_type index) const {
        return *(begin() + index);
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline typename VectorMap<K, T, C, RAC>::mapped_type&
    VectorMap<K, T, C, RAC>::DoAtKey(const KX& k) {
        // use the use const version of ::at to remove duplication
        return const_cast<mapped_type&>(const_cast<VectorMap<K, T, C, RAC> const*>(this)->at_key(k));
    }

    template <typename K, typename T, typename C, typename RAC>
    template <typename KX>
    inline const typename VectorMap<K, T, C, RAC>::mapped_type&
    VectorMap<K, T, C, RAC>::DoAtKey(const KX& k) const {
        const_iterator itLB(lower_bound(k));

        TURBO_RAW_CHECK((itLB != end()) && !key_comp()(k, itLB->first),
            "VectorMap::at key does not exist");

        return itLB->second;
    }

    ///////////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline bool operator==(const VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        const VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        return (a.size() == b.size()) && std::equal(b.begin(), b.end(), a.begin());
    }

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline bool operator<(const VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        const VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), a.value_comp());
    }

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline bool operator!=(const VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        const VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        return !(a == b);
    }

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline bool operator>(const VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        const VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        return b < a;
    }

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline bool operator<=(const VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        const VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        return !(b < a);
    }

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline bool operator>=(const VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        const VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        return !(a < b);
    }

    template <typename Key, typename T, typename Compare, typename RandomAccessContainer>
    inline void swap(VectorMap<Key, T, Compare, RandomAccessContainer>& a,
        VectorMap<Key, T, Compare, RandomAccessContainer>& b) {
        a.swap(b);
    }
} // namespace fermat

