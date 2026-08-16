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

#include <vector>

#include <turbo/base/internal/raw_logging.h>

namespace fermat {
    /// ring_buffer_iterator
    ///
    /// We force this iterator to act like a random access iterator even if
    /// the underlying container doesn't support random access iteration.
    /// Any BidirectionalIterator can be a RandomAccessIterator; it just
    /// might be inefficient in some cases.
    ///
    template <typename T, typename Pointer, typename Reference, typename Container>
    struct ring_buffer_iterator {
    public:
        typedef ring_buffer_iterator<T, Pointer, Reference, Container> this_type;
        typedef T value_type;
        typedef Pointer pointer;
        typedef Reference reference;
        typedef typename Container::size_type size_type;
        typedef typename Container::difference_type difference_type;
        typedef typename Container::iterator container_iterator;
        typedef typename Container::const_iterator container_const_iterator;
        typedef ring_buffer_iterator<T, T*, T&, Container> iterator;
        typedef ring_buffer_iterator<T, const T*, const T&, Container> const_iterator;
        typedef std::random_access_iterator_tag iterator_category;

    public:
        Container* mpContainer;
        container_iterator mContainerIterator;

    public:
        ring_buffer_iterator();

        ring_buffer_iterator(Container* pContainer, const container_iterator& containerIterator);

        ring_buffer_iterator(const iterator& x);

        ring_buffer_iterator& operator=(const iterator& x);

        reference operator*() const;

        pointer operator->() const;

        this_type& operator++();

        this_type operator++(int);

        this_type& operator--();

        this_type operator--(int);

        this_type& operator+=(difference_type n);

        this_type& operator-=(difference_type n);

        this_type operator+(difference_type n) const;

        this_type operator-(difference_type n) const;

    protected:
        void increment(difference_type n, std::input_iterator_tag);

        void increment(difference_type n, std::random_access_iterator_tag);
    }; // struct ring_buffer_iterator

    /// RingBuffer
    ///
    /// Implements a ring buffer via a given container type, which would
    /// typically be a vector or array, though any container which supports
    /// bidirectional iteration would work.
    ///
    /// A ring buffer is a FIFO (first-in, first-out) container which acts
    /// much like a queue. The difference is that a ring buffer is implemented
    /// via chasing pointers around a container and moving the read and write
    /// positions forward (and possibly wrapping around) as the container is
    /// read and written via pop_front and push_back.
    ///
    /// The benefit of a ring buffer is that memory allocations don't occur
    /// and new elements are neither added nor removed from the container.
    /// Elements in the container are simply assigned values in circles around
    /// the container.
    ///
    /// RingBuffer is different from other containers -- including adapter
    /// containers -- in how iteration is done. Iteration of a ring buffer
    /// starts at the current begin position, proceeds to the end of the underlying
    /// container, and continues at the begin of the underlying container until
    /// the ring buffer's current end position. Thus a RingBuffer does
    /// indeed have a begin and an end, though the values of begin and end
    /// chase each other around the container. An empty RingBuffer is one
    /// in which end == begin, and a full RingBuffer is one in which
    /// end + 1 == begin.
    ///
    /// Example of a ring buffer layout, where + indicates queued items:
    ///    ++++++++++--------------------------------+++++++++
    ///              ^                               ^
    ///              end                             begin
    ///
    /// Empty ring buffer:
    ///    ---------------------------------------------------
    ///                              ^
    ///                          begin / end
    ///
    /// Full ring buffer. Note that one item is necessarily unused; it is
    /// analagous to a '\0' at the end of a C string:
    ///    +++++++++++++++++++++++++++++++++++++++++-+++++++++
    ///                                             ^^
    ///                                           end begin
    ///
    /// A push_back operation on a ring buffer assigns the new value to end.
    /// If there is no more space in the buffer, this will result in begin
    /// being overwritten and the begin position being moved foward one position.
    /// The user can use the full() function to detect this condition.
    /// Note that elements in a ring buffer are not created or destroyed as
    /// their are added and removed; they are merely assigned. Only on
    /// container construction and destruction are any elements created and
    /// destroyed.
    ///
    /// The ring buffer can be used in either direction. By this we mean that
    /// you can use push_back to add items and pop_front to remove them; or you can
    /// use push_front to add items and pop_back to remove them. You aren't
    /// limited to these operations; you can push or pop from either side
    /// arbitrarily and you can insert or erase anywhere in the container.
    ///
    /// The ring buffer requires the user to specify a Container type, which
    /// by default is vector. However, any container with bidirectional iterators
    /// will work, such as list, deque, string or any of the fixed_* versions
    /// of these containers, such as fixed_string. Since ring buffer works via copying
    /// elements instead of allocating and freeing nodes, inserting in the middle
    /// of a ring buffer based on list (instead of vector) is no more efficient.
    ///
    /// To use the ring buffer, its container must be resized to the desired
    /// ring buffer size. Changing the size of a ring buffer may cause ring
    /// buffer iterators to invalidate.
    ///
    /// An alternative to using a ring buffer is to use a list with a user-created
    /// node pool and custom allocator. There are various tradeoffs that result from this.
    ///
    /// Example usage:
    ///     RingBuffer< int, list<int> > rb(100);
    ///     rb.push_back(1);
    ///
    /// Example usage:
    ///     // Example of creating an on-screen debug log that shows 16
    ///     // strings at a time and scrolls older strings away.
    ///
    ///     // Create ring buffer of 16 strings.
    ///     RingBuffer< string, vector<string> > debugLogText(16);
    ///
    ///     // Reserve 128 chars for each line. This can make it so that no
    ///     // runtime memory allocations occur.
    ///     for(vector<string>::iterator it = debugLogText.get_container().begin(),
    ///         itEnd = debugLogText.get_container().end(); it != itEnd; ++it)
    ///     {
    ///         (*it).reserve(128);
    ///     }
    ///
    ///     // Add a new string, using push_front() and front() instead of
    ///     // push_front(str) in order to avoid creating a temporary str.
    ///     debugLogText.push_front();
    ///     debugLogText.front() = "Player fired weapon";
    ///
    template <typename T, typename Container = std::vector<T>, typename Allocator = typename Container::allocator_type>
    class RingBuffer {
    public:
        typedef RingBuffer<T, Container, Allocator> this_type;
        typedef Container container_type;
        typedef Allocator allocator_type;

        typedef typename Container::value_type value_type;
        typedef typename Container::reference reference;
        typedef typename Container::const_reference const_reference;
        typedef typename Container::size_type size_type;
        typedef typename Container::difference_type difference_type;
        typedef typename Container::iterator container_iterator;
        typedef typename Container::const_iterator container_const_iterator;
        typedef ring_buffer_iterator<T, T*, T&, Container> iterator;
        typedef ring_buffer_iterator<T, const T*, const T&, Container> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    public:
        // We declare public so that global comparison operators can be implemented without adding an inline level and without tripping up GCC 2.x friend declaration failures. GCC (through at least v4.0) is poor at inlining and performance wins over correctness.
        Container c;
        // We follow the naming convention established for stack, queue, priority_queue and name this 'c'. This variable must always have a size of at least 1, as even an empty RingBuffer has an unused terminating element.

    protected:
        container_iterator _begin; // We keep track of where our begin and end are by using Container iterators.
        container_iterator _end;
        size_type _size;

    public:
        // There currently isn't a RingBuffer constructor that specifies an initial size, unlike other containers.
        explicit RingBuffer(size_type cap = 0); // Construct with an initial capacity (but size of 0).
        explicit RingBuffer(size_type cap, const allocator_type& allocator);

        explicit RingBuffer(const Container& x);

        explicit RingBuffer(const allocator_type& allocator);

        RingBuffer(const this_type& x);

        RingBuffer(this_type&& x) noexcept;

        RingBuffer(this_type&& x, const allocator_type& allocator);

        RingBuffer(std::initializer_list<value_type> ilist, const allocator_type& allocator = allocator_type { });

        // This function sets the capacity to be equal to the size of the initializer list.

        // No destructor necessary. Default will do.

        this_type& operator=(const this_type& x);

        this_type& operator=(std::initializer_list<value_type> ilist);

        this_type& operator=(this_type&& x) noexcept;

        void assign(std::initializer_list<value_type> ilist);

        template <typename InputIterator>
        void assign(InputIterator first, InputIterator last);

        void swap(this_type& x) noexcept;

        iterator begin() noexcept;

        const_iterator begin() const noexcept;

        const_iterator cbegin() const noexcept;

        iterator end() noexcept;

        const_iterator end() const noexcept;

        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;

        const_reverse_iterator rbegin() const noexcept;

        const_reverse_iterator crbegin() const noexcept;

        reverse_iterator rend() noexcept;

        const_reverse_iterator rend() const noexcept;

        const_reverse_iterator crend() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool full() const noexcept;

        size_type size() const noexcept;

        size_type capacity() const noexcept;

        void resize(size_type n);

        void set_capacity(size_type n);

        // Sets the capacity to the given value, including values less than the current capacity. Adjusts the size downward if n < size, by throwing out the oldest elements in the buffer.
        void reserve(size_type n);

        // Reserve a given capacity. Doesn't decrease the capacity; it only increases it (for compatibility with other containers' behavior).

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        void push_back(const value_type& value);

        reference push_back();

        void push_front(const value_type& value);

        reference push_front();

        void pop_back();

        void pop_front();

        reference operator[](size_type n);

        const_reference operator[](size_type n) const;

        // To consider:
        // size_type read(value_type* pDestination, size_type nCount);
        // size_type read(iterator** pPosition1, iterator** pPosition2, size_type& nCount1, size_type& nCount2);

        /* To do:
            template <class... Args>
            reference emplace_front(Args&&... args);

            template <class... Args>
            reference emplace_back(Args&&... args);

            template <class... Args>
            iterator emplace(const_iterator position, Args&&... args);
        */

        iterator insert(const_iterator position, const value_type& value);

        void insert(const_iterator position, size_type n, const value_type& value);

        void insert(const_iterator position, std::initializer_list<value_type> ilist);

        template <typename InputIterator>
        void insert(const_iterator position, InputIterator first, InputIterator last);

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        reverse_iterator erase(const_reverse_iterator position);

        reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        void clear();

        container_type& get_container();

        const container_type& get_container() const;

    protected:
        // size_type DoGetSize(std::input_iterator_tag) const;
        // size_type DoGetSize(std::random_access_iterator_tag) const;
    }; // class RingBuffer

    ///////////////////////////////////////////////////////////////////////
    // ring_buffer_iterator
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Pointer, typename Reference, typename Container>
    ring_buffer_iterator<T, Pointer, Reference, Container>::ring_buffer_iterator()
        : mpContainer(NULL)
        , mContainerIterator() {
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    ring_buffer_iterator<T, Pointer, Reference, Container>::ring_buffer_iterator(
        Container* pContainer, const container_iterator& containerIterator)
        : mpContainer(pContainer)
        , mContainerIterator(containerIterator) {
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    ring_buffer_iterator<T, Pointer, Reference, Container>::ring_buffer_iterator(const iterator& x)
        : mpContainer(x.mpContainer)
        , mContainerIterator(x.mContainerIterator) {
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    ring_buffer_iterator<T, Pointer, Reference, Container>&
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator=(const iterator& x) {
        mpContainer = x.mpContainer;
        mContainerIterator = x.mContainerIterator;
        return *this;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::reference
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator*() const {
        return *mContainerIterator;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::pointer
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator->() const {
        return &*mContainerIterator;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type&
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator++() {
        if (KUMO_UNLIKELY(++mContainerIterator == mpContainer->end()))
            mContainerIterator = mpContainer->begin();
        return *this;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator++(int) {
        const this_type temp(*this);
        if (KUMO_UNLIKELY(++mContainerIterator == mpContainer->end()))
            mContainerIterator = mpContainer->begin();
        return temp;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type&
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator--() {
        if (KUMO_UNLIKELY(mContainerIterator == mpContainer->begin()))
            mContainerIterator = mpContainer->end();
        --mContainerIterator;
        return *this;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator--(int) {
        const this_type temp(*this);
        if (KUMO_UNLIKELY(mContainerIterator == mpContainer->begin()))
            mContainerIterator = mpContainer->end();
        --mContainerIterator;
        return temp;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type&
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator+=(difference_type n) {
        typedef typename std::iterator_traits<container_iterator>::iterator_category IC;
        increment(n, IC());
        return *this;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type&
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator-=(difference_type n) {
        typedef typename std::iterator_traits<container_iterator>::iterator_category IC;
        increment(-n, IC());
        return *this;
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator+(difference_type n) const {
        return this_type(*this).operator+=(n);
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    typename ring_buffer_iterator<T, Pointer, Reference, Container>::this_type
    ring_buffer_iterator<T, Pointer, Reference, Container>::operator-(difference_type n) const {
        return this_type(*this).operator+=(-n);
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    void ring_buffer_iterator<T, Pointer, Reference, Container>::increment(difference_type n, std::input_iterator_tag) {
        // n cannot be negative, as input iterators don't support reverse iteration.
        while (n-- > 0)
            operator++();
    }

    template <typename T, typename Pointer, typename Reference, typename Container>
    void ring_buffer_iterator<T, Pointer, Reference, Container>::increment(
        difference_type n, std::random_access_iterator_tag) {
        // We make the assumption here that the user is incrementing from a valid
        // starting position to a valid ending position. Thus *this + n yields a
        // valid iterator, including if n happens to be a negative value.

        if (n >= 0) {
            const difference_type d = mpContainer->end() - mContainerIterator;

            if (n < d)
                mContainerIterator += n;
            else
                mContainerIterator = mpContainer->begin() + (n - d);
        } else {
            // Recall that n and d here will be negative and so the logic here works as intended.
            const difference_type d = mpContainer->begin() - mContainerIterator;

            if (n >= d)
                mContainerIterator += n;
            else
                mContainerIterator = mpContainer->end() + (n - d);
        }
    }

    // Random access iterators must support operator + and operator -.
    // You can only add an integer to an iterator, and you cannot add two iterators.
    template <typename T, typename Pointer, typename Reference, typename Container>
    inline ring_buffer_iterator<T, Pointer, Reference, Container>
    operator+(ptrdiff_t n, const ring_buffer_iterator<T, Pointer, Reference, Container>& x) {
        return x + n; // Implement (n + x) in terms of (x + n).
    }

    // You can only add an integer to an iterator, but you can subtract two iterators.
    template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, typename Container>
    inline typename ring_buffer_iterator<T, PointerA, ReferenceA, Container>::difference_type
    operator-(const ring_buffer_iterator<T, PointerA, ReferenceA, Container>& a,
        const ring_buffer_iterator<T, PointerB, ReferenceB, Container>& b) {
        typedef typename ring_buffer_iterator<T, PointerA, ReferenceA, Container>::difference_type difference_type;

        // To do: If container_iterator is a random access iterator, then do a simple calculation.
        // Otherwise, we have little choice but to iterate from a to b and count as we go.
        // See the RingBuffer::size function for an implementation of this.

        // Iteration implementation:
        difference_type d = 0;

        for (ring_buffer_iterator<T, PointerA, ReferenceA, Container> temp(b); temp != a; ++temp)
            ++d;

        return d;
    }

    // The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
    // Thus we provide additional template paremeters here to support this. The defect report does not
    // require us to support comparisons between reverse_iterators and const_reverse_iterators.
    template <typename T, typename PointerA, typename ReferenceA, typename ContainerA,
        typename PointerB, typename ReferenceB, typename ContainerB>
    inline bool operator==(const ring_buffer_iterator<T, PointerA, ReferenceA, ContainerA>& a,
        const ring_buffer_iterator<T, PointerB, ReferenceB, ContainerB>& b) {
        // Perhaps we should compare the container pointer as well.
        // However, for valid iterators this shouldn't be necessary.
        return a.mContainerIterator == b.mContainerIterator;
    }

    template <typename T, typename PointerA, typename ReferenceA, typename ContainerA,
        typename PointerB, typename ReferenceB, typename ContainerB>
    inline bool operator!=(const ring_buffer_iterator<T, PointerA, ReferenceA, ContainerA>& a,
        const ring_buffer_iterator<T, PointerB, ReferenceB, ContainerB>& b) {
        // Perhaps we should compare the container pointer as well.
        // However, for valid iterators this shouldn't be necessary.
        return !(a.mContainerIterator == b.mContainerIterator);
    }

    // We provide a version of operator!= for the case where the iterators are of the
    // same type. This helps prevent ambiguity errors in the presence of rel_ops.
    template <typename T, typename Pointer, typename Reference, typename Container>
    inline bool operator!=(const ring_buffer_iterator<T, Pointer, Reference, Container>& a,
        const ring_buffer_iterator<T, Pointer, Reference, Container>& b) {
        return !(a.mContainerIterator == b.mContainerIterator);
    }

    ///////////////////////////////////////////////////////////////////////
    // RingBuffer
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(size_type cap)
        : c() // Default construction with default allocator for the container.
    {
        // To do: This code needs to be amended to deal with possible exceptions
        // that could occur during the resize call below.

        // We add one because the element at _end is necessarily unused.
        c.resize(cap + 1);
        // Possibly we could construct 'c' with size, but c may not have such a ctor, though we rely on it having a resize function.
        _begin = c.begin();
        _end = _begin;
        _size = 0;
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(size_type cap, const allocator_type& allocator)
        : c(allocator) {
        // To do: This code needs to be amended to deal with possible exceptions
        // that could occur during the resize call below.

        // We add one because the element at _end is necessarily unused.
        c.resize(cap + 1);
        // Possibly we could construct 'c' with size, but c may not have such a ctor, though we rely on it having a resize function.
        _begin = c.begin();
        _end = _begin;
        _size = 0;
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(const Container& x)
        : c(x)
    // This copies elements from x, but unless the user is doing some tricks, the only thing that matters is that c.size() == x.size().
    {
        // To do: This code needs to be amended to deal with possible exceptions
        // that could occur during the resize call below.
        if (c.empty())
            c.resize(1);
        _begin = c.begin();
        _end = _begin;
        _size = 0;
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(const allocator_type& allocator)
        : c(allocator) {
        // To do: This code needs to be amended to deal with possible exceptions
        // that could occur during the resize call below.

        // We add one because the element at _end is necessarily unused.
        c.resize(1);
        // Possibly we could construct 'c' with size, but c may not have such a ctor, though we rely on it having a resize function.
        _begin = c.begin();
        _end = _begin;
        _size = 0;
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(const this_type& x)
        : c(x.c) {
        _begin = c.begin();
        _end = _begin;
        _size = x._size;

        std::advance(_begin, std::distance(const_cast<this_type&>(x).c.begin(), x._begin));
        // We can do a simple distance algorithm here, as there will be no wraparound.
        std::advance(_end, std::distance(const_cast<this_type&>(x).c.begin(), x._end));
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(this_type&& x) noexcept
        : c() // Default construction with default allocator for the container.
    {
        c.resize(1);
        // Possibly we could construct 'c' with size, but c may not have such a ctor, though we rely on it having a resize function.
        _begin = c.begin();
        _end = _begin;
        _size = 0;

        swap(x);
        // We are leaving x in an unusual state by swapping default-initialized members with it, as it won't be usable and can be only destructible.
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(this_type&& x, const allocator_type& allocator)
        : c(allocator) {
        c.resize(1);
        // Possibly we could construct 'c' with size, but c may not have such a ctor, though we rely on it having a resize function.
        _begin = c.begin();
        _end = _begin;
        _size = 0;

        if (c.get_allocator() == x.c.get_allocator())
            swap(x);
        // We are leaving x in an unusual state by swapping default-initialized members with it, as it won't be usable and can be only destructible.
        else
            operator=(x);
    }

    template <typename T, typename Container, typename Allocator>
    RingBuffer<T, Container, Allocator>::RingBuffer(std::initializer_list<value_type> ilist,
        const allocator_type& allocator)
        : c(allocator) {
        c.resize(ilist.size() + 1);
        _begin = c.begin();
        _end = _begin;
        _size = 0;

        assign(ilist.begin(), ilist.end());
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::this_type&
    RingBuffer<T, Container, Allocator>::operator=(const this_type& x) {
        if (&x != this) {
            c = x.c;

            _begin = c.begin();
            _end = _begin;
            _size = x._size;

            std::advance(_begin, std::distance(const_cast<this_type&>(x).c.begin(), x._begin));
            // We can do a simple distance algorithm here, as there will be no wraparound.
            std::advance(_end, std::distance(const_cast<this_type&>(x).c.begin(), x._end));
        }

        return *this;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::this_type&
    RingBuffer<T, Container, Allocator>::operator=(this_type&& x) noexcept {
        swap(x);
        return *this;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::this_type&
    RingBuffer<T, Container, Allocator>::operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template <typename T, typename Container, typename Allocator>
    template <typename InputIterator>
    void RingBuffer<T, Container, Allocator>::assign(InputIterator first, InputIterator last) {
        // To consider: We can make specializations of this for pointer-based
        // iterators to PODs and turn the action into a memcpy.
        clear();

        for (; first != last; ++first)
            push_back(*first);
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::swap(this_type& x) noexcept {
        if (&x != this) {
            const difference_type dBegin = std::distance(c.begin(), _begin);
            // We can do a simple distance algorithm here, as there will be no wraparound.
            const difference_type dEnd = std::distance(c.begin(), _end);

            const difference_type dxBegin = std::distance(x.c.begin(), x._begin);
            const difference_type dxEnd = std::distance(x.c.begin(), x._end);

            std::swap(c, x.c);
            std::swap(_size, x._size);

            _begin = c.begin();
            std::advance(_begin, dxBegin); // We can do a simple advance algorithm here, as there will be no wraparound.

            _end = c.begin();
            std::advance(_end, dxEnd);

            x._begin = x.c.begin();
            std::advance(x._begin, dBegin);

            x._end = x.c.begin();
            std::advance(x._end, dEnd);
        }
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::iterator
    RingBuffer<T, Container, Allocator>::begin() noexcept {
        return iterator(&c, _begin);
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_iterator
    RingBuffer<T, Container, Allocator>::begin() const noexcept {
        return const_iterator(const_cast<Container*>(&c), _begin);
        // We trust that the const_iterator will respect const-ness.
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_iterator
    RingBuffer<T, Container, Allocator>::cbegin() const noexcept {
        return const_iterator(const_cast<Container*>(&c), _begin);
        // We trust that the const_iterator will respect const-ness.
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::iterator
    RingBuffer<T, Container, Allocator>::end() noexcept {
        return iterator(&c, _end);
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_iterator
    RingBuffer<T, Container, Allocator>::end() const noexcept {
        return const_iterator(const_cast<Container*>(&c), _end);
        // We trust that the const_iterator will respect const-ness.
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_iterator
    RingBuffer<T, Container, Allocator>::cend() const noexcept {
        return const_iterator(const_cast<Container*>(&c), _end);
        // We trust that the const_iterator will respect const-ness.
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reverse_iterator
    RingBuffer<T, Container, Allocator>::rbegin() noexcept {
        return reverse_iterator(iterator(&c, _end));
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reverse_iterator
    RingBuffer<T, Container, Allocator>::rbegin() const noexcept {
        return const_reverse_iterator(const_iterator(const_cast<Container*>(&c), _end));
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reverse_iterator
    RingBuffer<T, Container, Allocator>::crbegin() const noexcept {
        return const_reverse_iterator(const_iterator(const_cast<Container*>(&c), _end));
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reverse_iterator
    RingBuffer<T, Container, Allocator>::rend() noexcept {
        return reverse_iterator(iterator(&c, _begin));
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reverse_iterator
    RingBuffer<T, Container, Allocator>::rend() const noexcept {
        return const_reverse_iterator(const_iterator(const_cast<Container*>(&c), _begin));
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reverse_iterator
    RingBuffer<T, Container, Allocator>::crend() const noexcept {
        return const_reverse_iterator(const_iterator(const_cast<Container*>(&c), _begin));
    }

    template <typename T, typename Container, typename Allocator>
    bool RingBuffer<T, Container, Allocator>::empty() const noexcept {
        return _begin == _end;
    }

    template <typename T, typename Container, typename Allocator>
    bool RingBuffer<T, Container, Allocator>::full() const noexcept {
        // Implementation that relies on c.size() being a fast operation:
        // return _size == (c.size() - 1); // (c.size() - 1) == capacity(); we are attempting to reduce function calls.

        // Version that has constant speed guarantees, but is still pretty fast.
        const_iterator afterEnd(end());
        ++afterEnd;
        return afterEnd.mContainerIterator == _begin;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::size_type
    RingBuffer<T, Container, Allocator>::size() const noexcept {
        return _size;

        // Alternatives:
        // return std::distance(begin(), end());
        // return end() - begin(); // This is more direct than using distance().
        // typedef typename std::iterator_traits<container_iterator>::iterator_category IC;
        // return DoGetSize(IC()); // This is more direct than using iterator math.
    }

    /*
    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::size_type
    RingBuffer<T, Container, Allocator>::DoGetSize(std::input_iterator_tag) const
    {
        // We could alternatively just use std::distance() here, but we happen to
        // know that such code would boil down to what we have here, and we might
        // as well remove function calls where possible.
        difference_type d = 0;

        for(const_iterator temp(begin()), tempEnd(end()); temp != tempEnd; ++temp)
            ++d;

        return (size_type)d;
    }
    */

    /*
    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::size_type
    RingBuffer<T, Container, Allocator>::DoGetSize(std::random_access_iterator_tag) const
    {
        // A simpler but less efficient implementation fo this function would be:
        //     return std::distance(_begin, _end);
        //
        // The calculation of distance here takes advantage of the fact that random
        // access iterators' distances can be calculated by simple pointer calculation.
        // Thus the code below boils down to a few subtractions when using a vector,
        // string, or array as the Container type.
        //
        const difference_type dBegin = std::distance(const_cast<Container&>(c).begin(), _begin); // const_cast here solves a little compiler
        const difference_type dEnd   = std::distance(const_cast<Container&>(c).begin(), _end);   // argument matching problem.

        if(dEnd >= dBegin)
            return dEnd - dBegin;

        return c.size() - (dBegin - dEnd);
    }
    */

    namespace Internal {
        ///////////////////////////////////////////////////////////////
        // has_overflow_allocator
        //
        // returns true_type when the specified container type is an
        // std::fixed_*  container and therefore has an overflow
        // allocator type.
        //
        template <typename T, typename = void>
        struct has_overflow_allocator : std::false_type {
        };

        template <typename T>
        struct has_overflow_allocator<T, std::void_t<decltype(std::declval<T>().get_overflow_allocator())>> : std::true_type {
        };

        ///////////////////////////////////////////////////////////////
        // GetFixedContainerCtorAllocator
        //
        // std::fixed_* containers are only constructible via their
        // overflow allocator type. This helper select the appropriate
        // allocator from the specified container.
        //
        template <typename Container, bool UseOverflowAllocator = has_overflow_allocator<Container>()()>
        struct GetFixedContainerCtorAllocator {
            auto& operator()(Container& c) { return c.get_overflow_allocator(); }
        };

        template <typename Container>
        struct GetFixedContainerCtorAllocator<Container, false> {
            auto& operator()(Container& c) { return c.get_allocator(); }
        };
    } // namespace Internal

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::resize(size_type n) {
        // Note that if n > size(), we just move the end position out to
        // the begin + n, with the data being the old end and the new end
        // being stale values from the past. This is by design, as the concept
        // of arbitrarily resizing a ring buffer like this is currently deemed
        // to be vague in what it intends to do. We can only assume that the
        // user knows what he is doing and will deal with the stale values.
        TURBO_RAW_CHECK(c.size() >= 1, "RingBuffer capacity must be >= 1");
        const size_type cap = (c.size() - 1);

        _size = n;

        if (n > cap) {
            // If we need to grow in capacity...
            // Given that a growing operation will always result in memory allocation,
            // we currently implement this function via the usage of a temp container.
            // This makes for a simple implementation, but in some cases it is less
            // efficient. In particular, if the container is a node-based container like
            // a (linked) list, this function would be faster if we simply added nodes
            // to ourself. We would do this by inserting the nodes to be after end()
            // and adjusting the begin() position if it was after end().

            // To do: This code needs to be amended to deal with possible exceptions
            // that could occur during the resize call below.

            Container cTemp(c);
            cTemp.resize(n + 1);
            std::copy(begin(), end(), cTemp.begin());
            std::swap(c, cTemp);

            _begin = c.begin();
            _end = _begin;
            std::advance(_end, n);
            // We can do a simple advance algorithm on this because we know that _end will not wrap around.
        } else {
            // We could do a check here for n != size(), but that would be costly and people don't usually resize things to their same size.
            _end = _begin;

            // std::advance(_end, n); // We *cannot* use this because there may be wraparound involved.

            // To consider: Possibly we should implement some more detailed logic to optimize the code here.
            // We'd need to do different behaviour dending on whether the container iterator type is a
            // random access iterator or otherwise.

            while (n--) {
                if (KUMO_UNLIKELY(++_end == c.end()))
                    _end = c.begin();
            }
        }
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::size_type
    RingBuffer<T, Container, Allocator>::capacity() const noexcept {
        TURBO_RAW_CHECK(c.size() >= 1, "RingBuffer capacity must be >= 1");
        // This is required because even an empty RingBuffer has one unused termination element, somewhat like a \0 at the end of a C string.

        return (c.size() - 1); // Need to subtract one because the position at _end is unused.
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::set_capacity(size_type n) {
        const size_type capacity = (c.size() - 1);

        if (n != capacity) {
            // If we need to change capacity...
            Container cTemp(c);
            cTemp.resize(n + 1);

            iterator itCopyBegin = begin();

            if (n < _size) // If we are shrinking the capacity, to less than our size...
            {
                std::advance(itCopyBegin, _size - n);
                _size = n;
            }

            std::copy(itCopyBegin, end(), cTemp.begin());
            // The begin-end range may in fact be larger than n, in which case values will be overwritten.
            std::swap(c, cTemp);

            _begin = c.begin();
            _end = _begin;
            std::advance(_end, _size);
            // We can do a simple advance algorithm on this because we know that _end will not wrap around.
        }
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::reserve(size_type n) {
        // We follow the pattern of vector and only do something if n > capacity.
        TURBO_RAW_CHECK(c.size() >= 1, "RingBuffer capacity must be >= 1");

        if (n > (c.size() - 1)) {
            // If we need to grow in capacity... // (c.size() - 1) == capacity(); we are attempting to reduce function calls.
            Container cTemp(c);
            cTemp.resize(n + 1);
            std::copy(begin(), end(), cTemp.begin());
            std::swap(c, cTemp);

            _begin = c.begin();
            _end = _begin;
            std::advance(_end, _size);
            // We can do a simple advance algorithm on this because we know that _end will not wrap around.
        }
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reference
    RingBuffer<T, Container, Allocator>::front() {
        return *_begin;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reference
    RingBuffer<T, Container, Allocator>::front() const {
        return *_begin;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reference
    RingBuffer<T, Container, Allocator>::back() {
        // return *(end() - 1); // Can't use this because not all iterators support operator-.

        iterator temp(end()); // To do: Find a way to construct this temporary in the return statement.
        return *(--temp); // We can do it by making all our containers' iterators support operator-.
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reference
    RingBuffer<T, Container, Allocator>::back() const {
        // return *(end() - 1); // Can't use this because not all iterators support operator-.

        const_iterator temp(end()); // To do: Find a way to construct this temporary in the return statement.
        return *(--temp); // We can do it by making all our containers' iterators support operator-.
    }

    /// A push_back operation on a ring buffer assigns the new value to end.
    /// If there is no more space in the buffer, this will result in begin
    /// being overwritten and the begin position being moved foward one position.
    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::push_back(const value_type& value) {
        *_end = value;

        if (++_end == c.end())
            _end = c.begin();

        if (_end == _begin) {
            if (++_begin == c.end())
                _begin = c.begin();
        } else
            ++_size;
    }

    /// A push_back operation on a ring buffer assigns the new value to end.
    /// If there is no more space in the buffer, this will result in begin
    /// being overwritten and the begin position being moved foward one position.
    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reference
    RingBuffer<T, Container, Allocator>::push_back() {
        // We don't do the following assignment, as the value at _end is already constructed;
        // it is merely possibly not default-constructed. However, the spirit of push_back
        // is that the user intends to do an assignment or data modification after the
        // push_back call. The user can always execute *back() = value_type() if he wants.
        //*_end = value_type();

        if (++_end == c.end())
            _end = c.begin();

        if (_end == _begin) {
            if (++_begin == c.end())
                _begin = c.begin();
        } else
            ++_size;

        return back();
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::pop_back() {
        TURBO_RAW_CHECK(_end != _begin, "RingBuffer pop on empty"); // We assume that size() > 0 and thus that there is something to pop.

        if (KUMO_UNLIKELY(_end == c.begin()))
            _end = c.end();
        --_end;
        --_size;
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::push_front(const value_type& value) {
        if (KUMO_UNLIKELY(_begin == c.begin()))
            _begin = c.end();

        if (--_begin == _end) {
            if (KUMO_UNLIKELY(_end == c.begin()))
                _end = c.end();
            --_end;
        } else
            ++_size;

        *_begin = value;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reference
    RingBuffer<T, Container, Allocator>::push_front() {
        if (KUMO_UNLIKELY(_begin == c.begin()))
            _begin = c.end();

        if (--_begin == _end) {
            if (KUMO_UNLIKELY(_end == c.begin()))
                _end = c.end();
            --_end;
        } else
            ++_size;

        // See comments above in push_back for why we don't execute this:
        // *_begin = value_type();

        return *_begin; // Same as return front();
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::pop_front() {
        TURBO_RAW_CHECK(_begin != _end, "RingBuffer pop on empty"); // We assume that _end > _begin and thus that there is something to pop.

        if (++_begin == c.end())
            _begin = c.begin();
        --_size;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reference
    RingBuffer<T, Container, Allocator>::operator[](size_type n) {
        // return *(begin() + n); // Can't use this because not all iterators support operator+.

        // This should compile to code that is nearly as efficient as that above.
        // The primary difference is the possible generation of a temporary in this case.
        iterator temp(begin());
        std::advance(temp, n);
        return *(temp.mContainerIterator);
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::const_reference
    RingBuffer<T, Container, Allocator>::operator[](size_type n) const {
        // return *(begin() + n); // Can't use this because not all iterators support operator+.

        // This should compile to code that is nearly as efficient as that above.
        // The primary difference is the possible generation of a temporary in this case.
        const_iterator temp(begin());
        std::advance(temp, n);
        return *(temp.mContainerIterator);
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::iterator
    RingBuffer<T, Container, Allocator>::insert(const_iterator position, const value_type& value) {
        // To consider: It would be faster if we could tell that position was in the first
        // half of the container and instead of moving things after the position back,
        // we could move things before the position forward.

        iterator afterEnd(end());
        iterator beforeEnd(afterEnd);

        ++afterEnd;

        if (afterEnd.mContainerIterator == _begin) // If we are at full capacity...
            --beforeEnd;
        else
            push_back();

        iterator itPosition(position.mpContainer, position.mContainerIterator);
        // We merely copy from const_iterator to iterator.
        std::copy_backward(itPosition, beforeEnd, end());
        *itPosition = value;

        return itPosition;
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::insert(const_iterator position, size_type n, const value_type& value) {
        // To do: This can be improved with a smarter version. However,
        // this is a little tricky because we need to deal with the case
        // whereby n is greater than the size of the container itself.
        while (n--)
            insert(position, value);
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container,
        Allocator>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        insert(position, ilist.begin(), ilist.end());
    }

    template <typename T, typename Container, typename Allocator>
    template <typename InputIterator>
    void RingBuffer<T, Container,
        Allocator>::insert(const_iterator position, InputIterator first, InputIterator last) {
        // To do: This can possibly be improved with a smarter version.
        // However, this can be tricky if distance(first, last) is greater
        // than the size of the container itself.
        for (; first != last; ++first, ++position)
            insert(position, *first);
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::iterator
    RingBuffer<T, Container, Allocator>::erase(const_iterator position) {
        iterator itPosition(position.mpContainer, position.mContainerIterator);
        // We merely copy from const_iterator to iterator.
        iterator iNext(itPosition);

        std::copy(++iNext, end(), itPosition);
        pop_back();

        return itPosition;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::iterator
    RingBuffer<T, Container, Allocator>::erase(const_iterator first, const_iterator last) {
        iterator itFirst(first.mpContainer, first.mContainerIterator);
        // We merely copy from const_iterator to iterator.
        iterator itLast(last.mpContainer, last.mContainerIterator);

        typename iterator::difference_type d = std::distance(itFirst, itLast);

        std::copy(itLast, end(), itFirst);

        while (d--) // To do: improve this implementation.
            pop_back();

        return itFirst;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reverse_iterator
    RingBuffer<T, Container, Allocator>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::reverse_iterator
    RingBuffer<T, Container, Allocator>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }

    template <typename T, typename Container, typename Allocator>
    void RingBuffer<T, Container, Allocator>::clear() {
        // Don't clear the container; we use its valid data for our elements.
        _begin = c.begin();
        _end = c.begin();
        _size = 0;
    }

    template <typename T, typename Container, typename Allocator>
    typename RingBuffer<T, Container, Allocator>::container_type&
    RingBuffer<T, Container, Allocator>::get_container() {
        return c;
    }

    template <typename T, typename Container, typename Allocator>
    const typename RingBuffer<T, Container, Allocator>::container_type&
    RingBuffer<T, Container, Allocator>::get_container() const {
        return c;
    }

    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Container, typename Allocator>
    inline bool operator==(const RingBuffer<T, Container, Allocator>& a,
        const RingBuffer<T, Container, Allocator>& b) {
        return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
    }

    template <typename T, typename Container, typename Allocator>
    inline bool operator<(const RingBuffer<T, Container, Allocator>& a,
        const RingBuffer<T, Container, Allocator>& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template <typename T, typename Container, typename Allocator>
    inline bool operator!=(const RingBuffer<T, Container, Allocator>& a,
        const RingBuffer<T, Container, Allocator>& b) {
        return !(a == b);
    }

    template <typename T, typename Container, typename Allocator>
    inline bool operator>(const RingBuffer<T, Container, Allocator>& a,
        const RingBuffer<T, Container, Allocator>& b) {
        return (b < a);
    }

    template <typename T, typename Container, typename Allocator>
    inline bool operator<=(const RingBuffer<T, Container, Allocator>& a,
        const RingBuffer<T, Container, Allocator>& b) {
        return !(b < a);
    }

    template <typename T, typename Container, typename Allocator>
    inline bool operator>=(const RingBuffer<T, Container, Allocator>& a,
        const RingBuffer<T, Container, Allocator>& b) {
        return !(a < b);
    }

    template <typename T, typename Container, typename Allocator>
    inline void swap(RingBuffer<T, Container, Allocator>& a, RingBuffer<T, Container, Allocator>& b) noexcept {
        a.swap(b);
    }
} // namespace fermat

