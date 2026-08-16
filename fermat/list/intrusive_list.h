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

///////////////////////////////////////////////////////////////////////////////
// The intrusive list container is similar to a list, with the primary
// different being that intrusive lists allow you to control memory
// allocation.
//
// * Intrusive lists store the nodes directly in the data items. This
//   is done by deriving the object from IntrusiveListNode.
//
// * The container does no memory allocation -- it works entirely with
//   the submitted nodes. This does mean that it is the client's job to 
//   free the nodes in an intrusive list, though.
//
// * Valid node pointers can be converted back to iterators in O(1).
//   This is because objects in the list are also nodes in the list.
//
// * IntrusiveList does not support copy construction or assignment; 
//   the push, pop, and insert operations take ownership of the 
//   passed object.
//
// Usage notes:
//
// * You can use an IntrusiveList directly with the standard nodes
//   if you have some other way of converting the node pointer back
//   to your data pointer.
//
// * Remember that the list destructor doesn't deallocate nodes -- it can't.
//
// * The size is not cached; this makes size() linear time but splice() is
//   constant time. This does mean that you can remove() an element without
//   having to figure out which list it is in, however.
//
// * You can insert a node into multiple intrusive_lists. One way to do so
//   is to (ab)use inheritance:
//
//      struct NodeA : public IntrusiveListNode {};
//      struct NodeB : public IntrusiveListNode {};
//      struct Object : public NodeA, nodeB {};
//
//      IntrusiveList<NodeA> listA;
//      IntrusiveList<NodeB> listB;
//
//      listA.push_back(obj);
//      listB.push_back(obj);
//
// * find() vs. locate()
//   The find(v) algorithm returns an iterator p such that *p == v; IntrusiveList::locate(v) 
//   returns an iterator p such that &*p == &v. IntrusiveList<> doesn't have find() mainly 
//   because list<> doesn't have it either, but there's no reason it couldn't. IntrusiveList
//   uses the name 'find' because:
//      - So as not to confuse the member function with the well-defined free function from algorithm.h.
//      - Because it is not API-compatible with std::find().
//      - Because it simply locates an object within the list based on its node entry and doesn't perform before any value-based searches or comparisons.
//
// Differences between IntrusiveList and std::list:
//
// Issue                            std::list       IntrusiveList
// --------------------------------------------------------------
// Automatic node ctor/dtor         Yes             No
// Can memmove() container          Maybe*          No
// Same item in list twice          Yes(copy/byref) No
// Can store non-copyable items     No              Yes
// size()                           O(1) or O(n)    O(n)
// clear()                          O(n)            O(1)
// erase(range)                     O(n)            O(1)
// splice(range)                    O(1) or O(n)    O(1)
// Convert reference to iterator    No              O(1)
// Remove without container         No              O(1)
// Nodes in mixed allocators        No              Yes
//
// *) Not required by standard but can be done with some STL implementations.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#include <iterator>
#include <algorithm>
#include <turbo/log/logging.h>

namespace fermat {
    template<class T>
    class IntrusiveList;

    /// IntrusiveListNode
	///
	/// By design this must be a POD, as user structs will be inheriting from
	/// it and they may wish to remain POD themselves. However, if the
	///
    struct IntrusiveListNode {
        IntrusiveListNode *mpNext{nullptr};
        IntrusiveListNode *mpPrev{nullptr};

        IntrusiveListNode() = default;

        ~IntrusiveListNode() {
            KCHECK(!mpNext&& !mpPrev) << "~IntrusiveListNode(): List is non-empty.";
        }
    };

    // It's not clear if this really should be needed. An old GCC compatible compiler is generating some crashing optimized code when strict aliasing is enabled, but analysis of it seems to blame the compiler. However, this topic can be tricky.


    /// IntrusiveListIterator
	///
    template<typename T, typename Pointer, typename Reference>
    class IntrusiveListIterator {
    public:
        typedef IntrusiveListIterator<T, Pointer, Reference> this_type;
        typedef IntrusiveListIterator<T, T *, T &> iterator;
        typedef IntrusiveListIterator<T, const T *, const T &> const_iterator;
        typedef T value_type;
        typedef T node_type;
        typedef IntrusiveListNode base_node_type;
        typedef ptrdiff_t difference_type;
        typedef Pointer pointer;
        typedef Reference reference;
        typedef std::bidirectional_iterator_tag iterator_category;

    private:
        base_node_type *_pnode;

    public:
        IntrusiveListIterator();

        // Note: you can also construct an iterator from T* via this, since T should inherit from
        // IntrusiveListNode.
        explicit IntrusiveListIterator(const base_node_type *pNode);

        // Note: this isn't always a copy constructor, iterator is not always equal to this_type
        IntrusiveListIterator(const iterator &x);

        // Note: this isn't always a copy assignment operator, iterator is not always equal to this_type
        IntrusiveListIterator &operator=(const iterator &x);

        // Calling these on the end() of a list invokes undefined behavior.
        reference operator*() const;

        pointer operator->() const;

        // Returns a pointer to the fully typed node (the same as operator->) this is useful when
        // iterating a list to destroy all the nodes, calling this on the end() of a list results in
        // undefined behavior.
        pointer nodePtr() const;

        IntrusiveListIterator &operator++();

        IntrusiveListIterator &operator--();

        IntrusiveListIterator operator++(int);

        IntrusiveListIterator operator--(int);

        // The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
        // Thus we provide additional template paremeters here to support this. The defect report does not
        // require us to support comparisons between reverse_iterators and const_reverse_iterators.
        template<class PointerB, class ReferenceB>
        bool operator==(const IntrusiveListIterator<T, PointerB, ReferenceB> &other) const {
            return _pnode == other._pnode;
        }

        template<typename PointerB, typename ReferenceB>
        inline bool operator!=(const IntrusiveListIterator<T, PointerB, ReferenceB> &other) const {
            return _pnode != other._pnode;
        }

        // We provide a version of operator!= for the case where the iterators are of the
        // same type. This helps prevent ambiguity errors in the presence of rel_ops.
        inline bool operator!=(const IntrusiveListIterator other) const { return _pnode != other._pnode; }

    private:
        // for the "copy" constructor, which uses non-const iterator even in the
        // const_iterator case.  Also, some of the internal member functions in
        // IntrusiveList<T> want to use _pnode.
        friend const_iterator;
        friend IntrusiveList<T>;

        // for the comparison operators.
        template<class U, class Pointer1, class Reference1>
        friend class IntrusiveListIterator;
    }; // class IntrusiveListIterator


    /// IntrusiveListBase
	///
    class IntrusiveListBase {
    public:
        typedef size_t size_type; // See config.h for the definition of this, which defaults to size_t.
        typedef ptrdiff_t difference_type;

    protected:
        IntrusiveListNode mAnchor; ///< Sentinel node (end). All data nodes are linked in a ring from this node.

    public:
        IntrusiveListBase();

        ~IntrusiveListBase();

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] size_t size() const noexcept; ///< Returns the number of elements in the list; O(n).
        void clear() noexcept; ///< Clears the list; O(1). No deallocation occurs.
        void pop_front();

        ///< Removes an element from the front of the list; O(1). The element must exist, but is not deallocated.
        void pop_back();

        ///< Removes an element from the back of the list; O(1). The element must exist, but is not deallocated.
        KUMO_DLL void reverse() noexcept; ///< Reverses a list so that front and back are swapped; O(n).

        KUMO_DLL bool validate() const;

        ///< Scans a list for linkage inconsistencies; O(n) time, O(1) space. Returns false if errors are detected, such as loops or branching.
    }; // class IntrusiveListBase


    /// IntrusiveList
	///
	/// Example usage:
	///    struct IntNode : public fermat::IntrusiveListNode {
	///        int mX;
	///        IntNode(int x) : mX(x) { }
	///    };
	///
	///    IntNode nodeA(0);
	///    IntNode nodeB(1);
	///
	///    IntrusiveList<IntNode> intList;
	///    intList.push_back(nodeA);
	///    intList.push_back(nodeB);
	///    intList.remove(nodeA);
	///
    template<typename T = IntrusiveListNode>
    class IntrusiveList : public IntrusiveListBase {
    public:
        typedef IntrusiveList<T> this_type;
        typedef IntrusiveListBase base_type;
        typedef T node_type;
        typedef T value_type;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef IntrusiveListIterator<T, T *, T &> iterator;
        typedef IntrusiveListIterator<T, const T *, const T &> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    public:
        IntrusiveList(); ///< Creates an empty list.
        IntrusiveList(const this_type &x); ///< Creates an empty list; ignores the argument.
        //IntrusiveList(std::initializer_list<value_type> ilist); To consider: Is this feasible, given how initializer_list works by creating a temporary array? Even if it is feasible, is it a good idea?

        this_type &operator=(const this_type &x); ///< Clears the list; ignores the argument.
        void swap(this_type &) noexcept; ///< Swaps the contents of two intrusive lists; O(1).

        iterator begin() noexcept; ///< Returns an iterator pointing to the first element in the list.
        const_iterator begin() const noexcept; ///< Returns a const_iterator pointing to the first element in the list.
        const_iterator cbegin() const noexcept; ///< Returns a const_iterator pointing to the first element in the list.

        iterator end() noexcept; ///< Returns an iterator pointing one-after the last element in the list.
        const_iterator end() const noexcept;

        ///< Returns a const_iterator pointing one-after the last element in the list.
        const_iterator cend() const noexcept;

        ///< Returns a const_iterator pointing one-after the last element in the list.

        reverse_iterator rbegin() noexcept;

        ///< Returns a reverse_iterator pointing at the end of the list (start of the reverse sequence).
        const_reverse_iterator rbegin() const noexcept;

        ///< Returns a const_reverse_iterator pointing at the end of the list (start of the reverse sequence).
        const_reverse_iterator crbegin() const noexcept;

        ///< Returns a const_reverse_iterator pointing at the end of the list (start of the reverse sequence).

        reverse_iterator rend() noexcept;

        ///< Returns a reverse_iterator pointing at the start of the list (end of the reverse sequence).
        const_reverse_iterator rend() const noexcept;

        ///< Returns a const_reverse_iterator pointing at the start of the list (end of the reverse sequence).
        const_reverse_iterator crend() const noexcept;

        ///< Returns a const_reverse_iterator pointing at the start of the list (end of the reverse sequence).

        reference front(); ///< Returns a reference to the first element. The list must be non-empty.
        const_reference front() const; ///< Returns a const reference to the first element. The list must be non-empty.
        reference back(); ///< Returns a reference to the last element. The list must be non-empty.
        const_reference back() const; ///< Returns a const reference to the last element. The list must be non-empty.

        void push_front(value_type &x);

        ///< Adds an element to the front of the list; O(1). The element is not copied. The element must not be in any other list.
        void push_back(value_type &x);

        ///< Adds an element to the back of the list; O(1). The element is not copied. The element must not be in any other list.

        bool contains(const value_type &x) const;

        ///< Returns true if the given element is in the list; O(n). Equivalent to (locate(x) != end()).

        iterator locate(value_type &x);

        ///< Converts a reference to an object in the list back to an iterator, or returns end() if it is not part of the list. O(n)
        const_iterator locate(const value_type &x) const;

        ///< Converts a const reference to an object in the list back to a const iterator, or returns end() if it is not part of the list. O(n)

        iterator insert(const_iterator pos, value_type &x);

        ///< Inserts an element before the element pointed to by the iterator. O(1)
        iterator erase(const_iterator pos); ///< Erases the element pointed to by the iterator. O(1)
        [[nodiscard]] iterator erase(const_iterator first, const_iterator last);

        ///< Erases elements within the iterator range [pos, last). O(1)

        reverse_iterator erase(const_reverse_iterator pos);

        [[nodiscard]] reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        static void remove(value_type &value);

        ///< Erases an element from a list; O(1). Note that this is static so you don't need to know which list the element, although it must be in some list.

        void splice(const_iterator first, value_type &x);

        ///< Moves the given element into this list before the element pointed to by pos; O(1).
				///< Required: x must be in some list or have first/next pointers that point it itself.

        void splice(const_iterator pos, IntrusiveList &x);

        ///< Moves the contents of a list into this list before the element pointed to by pos; O(1).
				///< Required: &x != this (same as std::list).

        void splice(const_iterator pos, IntrusiveList &x, const_iterator i);

        ///< Moves the given element pointed to i within the list x into the current list before
				///< the element pointed to by pos; O(1).

        void splice(const_iterator pos, IntrusiveList &x, const_iterator first, const_iterator last);

        ///< Moves the range of elements [first, last) from list x into the current list before
				///< the element pointed to by pos; O(1).
				///< Required: pos must not be in [first, last). (same as std::list).

    public:
        // Sorting functionality
        // This is independent of the global sort algorithms, as lists are
        // linked nodes and can be sorted more efficiently by moving nodes
        // around in ways that global sort algorithms aren't privy to.

        void merge(this_type &x);

        template<typename Compare>
        void merge(this_type &x, Compare compare);

        void unique();

        template<typename BinaryPredicate>
        void unique(BinaryPredicate);

        void sort();

        template<typename Compare>
        void sort(Compare compare);
    }; // IntrusiveList


    ///////////////////////////////////////////////////////////////////////
    // IntrusiveListNode
    ///////////////////////////////////////////////////////////////////////

    // Moved to be inline within the class because the may-alias attribute is
    // triggering what appears to be a bug in GCC that effectively requires
    // may-alias structs to implement inline member functions within the class
    // declaration. We don't have a .cpp file for
    //     inline IntrusiveListNode::IntrusiveListNode()
    //     {
    //         mpNext = mpPrev = nullptr;
    //     }
    //
    //     inline IntrusiveListNode::~IntrusiveListNode()
    //     {
    //         #if FERMAT_ASSERT_ENABLED
    //             if(mpNext || mpPrev)
    //                 FERMAT_FAIL_MSG("~IntrusiveListNode(): List is non-empty.");
    //         #endif
    //     }


    ///////////////////////////////////////////////////////////////////////
    // IntrusiveListIterator
    ///////////////////////////////////////////////////////////////////////

    template<typename T, typename Pointer, typename Reference>
    inline IntrusiveListIterator<T, Pointer, Reference>::IntrusiveListIterator() {
        _pnode = nullptr;
    }


    template<typename T, typename Pointer, typename Reference>
    inline IntrusiveListIterator<T, Pointer, Reference>::IntrusiveListIterator(const base_node_type *pNode)
        : _pnode(const_cast<base_node_type *>(pNode)) {
        // Empty
    }


    template<typename T, typename Pointer, typename Reference>
    inline IntrusiveListIterator<T, Pointer, Reference>::IntrusiveListIterator(const iterator &x)
        : _pnode(x._pnode) {
        // Empty
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::this_type &
    IntrusiveListIterator<T, Pointer, Reference>::operator=(const iterator &x) {
        _pnode = x._pnode;
        return *this;
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::reference
    IntrusiveListIterator<T, Pointer, Reference>::operator*() const {
        return *static_cast<pointer>(_pnode);
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::pointer
    IntrusiveListIterator<T, Pointer, Reference>::operator->() const {
        return static_cast<pointer>(_pnode);
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::pointer
    IntrusiveListIterator<T, Pointer, Reference>::nodePtr() const {
        return static_cast<pointer>(_pnode);
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::this_type &
    IntrusiveListIterator<T, Pointer, Reference>::operator++() {
        _pnode = _pnode->mpNext;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::this_type
    IntrusiveListIterator<T, Pointer, Reference>::operator++(int) {
        IntrusiveListIterator it(*this);
        _pnode = _pnode->mpNext;
        return it;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::this_type &
    IntrusiveListIterator<T, Pointer, Reference>::operator--() {
        _pnode = _pnode->mpPrev;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename IntrusiveListIterator<T, Pointer, Reference>::this_type
    IntrusiveListIterator<T, Pointer, Reference>::operator--(int) {
        IntrusiveListIterator it(*this);
        _pnode = _pnode->mpPrev;
        return it;
    }

    ///////////////////////////////////////////////////////////////////////
    // IntrusiveListBase
    ///////////////////////////////////////////////////////////////////////

    inline IntrusiveListBase::IntrusiveListBase() {
        mAnchor.mpNext = mAnchor.mpPrev = &mAnchor;
    }

    inline IntrusiveListBase::~IntrusiveListBase() {
        clear();
        mAnchor.mpNext = mAnchor.mpPrev = nullptr;
    }


    inline bool IntrusiveListBase::empty() const noexcept {
        return mAnchor.mpPrev == &mAnchor;
    }


    inline IntrusiveListBase::size_type IntrusiveListBase::size() const noexcept {
        const IntrusiveListNode *p = &mAnchor;
        size_type n = (size_type) -1;

        do {
            ++n;
            p = p->mpNext;
        } while (p != &mAnchor);

        return n;
    }


    inline void IntrusiveListBase::clear() noexcept {
        // Need to clear out all the next/prev pointers in the elements;
        // this makes this operation O(n) instead of O(1).
        IntrusiveListNode *pNode = mAnchor.mpNext;

        while (pNode != &mAnchor) {
            IntrusiveListNode *const pNextNode = pNode->mpNext;
            pNode->mpNext = pNode->mpPrev = nullptr;
            pNode = pNextNode;
        }

        mAnchor.mpNext = mAnchor.mpPrev = &mAnchor;
    }


    inline void IntrusiveListBase::pop_front() {
        IntrusiveListNode *const pNode = mAnchor.mpNext;
        mAnchor.mpNext->mpNext->mpPrev = &mAnchor;
        mAnchor.mpNext = mAnchor.mpNext->mpNext;

        if (pNode != &mAnchor)
            pNode->mpNext = pNode->mpPrev = nullptr;
    }


    inline void IntrusiveListBase::pop_back() {
        IntrusiveListNode *const pNode = mAnchor.mpPrev;
        mAnchor.mpPrev->mpPrev->mpNext = &mAnchor;
        mAnchor.mpPrev = mAnchor.mpPrev->mpPrev;

        if (pNode != &mAnchor)
            pNode->mpNext = pNode->mpPrev = nullptr;
    }


    inline void IntrusiveListBase::reverse() noexcept {
        IntrusiveListNode *pNode = &mAnchor;
        do {
            IntrusiveListNode *const pTemp = pNode->mpNext;
            pNode->mpNext = pNode->mpPrev;
            pNode->mpPrev = pTemp;
            pNode = pNode->mpPrev;
        } while (pNode != &mAnchor);
    }


    inline bool IntrusiveListBase::validate() const {
        const IntrusiveListNode *p = &mAnchor;
        const IntrusiveListNode *q = p;

        // We do two tests below:
        //
        // 1) Prev and next pointers are symmetric. We check (p->next->prev == p)
        //    for each node, which is enough to verify all links.
        //
        // 2) Loop check. We bump the q pointer at one-half rate compared to the
        //    p pointer; (p == q) if and only if we are at the start (which we
        //    don't check) or if there is a loop somewhere in the list.

        do {
            // validate node (even phase)
            if (p->mpNext->mpPrev != p)
                return false; // broken linkage detected

            // bump only fast pointer
            p = p->mpNext;
            if (p == &mAnchor)
                break;

            if (p == q)
                return false; // loop detected

            // validate node (odd phase)
            if (p->mpNext->mpPrev != p)
                return false; // broken linkage detected

            // bump both pointers
            p = p->mpNext;
            q = q->mpNext;

            if (p == q)
                return false; // loop detected
        } while (p != &mAnchor);

        return true;
    }


    ///////////////////////////////////////////////////////////////////////
    // IntrusiveList
    ///////////////////////////////////////////////////////////////////////

    template<typename T>
    inline IntrusiveList<T>::IntrusiveList() {
    }


    template<typename T>
    inline IntrusiveList<T>::IntrusiveList(const this_type & /*x*/)
        : IntrusiveListBase() {
        // We intentionally ignore argument x.
        // To consider: Shouldn't this function simply not exist? Is there a useful purpose for having this function?
        // There should be a comment here about it, though my first guess is that this exists to quell VC++ level 4/-Wall compiler warnings.
    }


    template<typename T>
    inline typename IntrusiveList<T>::this_type &IntrusiveList<T>::operator=(const this_type & /*x*/) {
        // We intentionally ignore argument x.
        // See notes above in the copy constructor about questioning the existence of this function.
        return *this;
    }


    template<typename T>
    inline typename IntrusiveList<T>::iterator IntrusiveList<T>::begin() noexcept {
        return iterator(mAnchor.mpNext);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_iterator IntrusiveList<T>::begin() const noexcept {
        return const_iterator(mAnchor.mpNext);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_iterator IntrusiveList<T>::cbegin() const noexcept {
        return const_iterator(mAnchor.mpNext);
    }


    template<typename T>
    inline typename IntrusiveList<T>::iterator IntrusiveList<T>::end() noexcept {
        return iterator(&mAnchor);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_iterator IntrusiveList<T>::end() const noexcept {
        return const_iterator(&mAnchor);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_iterator IntrusiveList<T>::cend() const noexcept {
        return const_iterator(&mAnchor);
    }


    template<typename T>
    inline typename IntrusiveList<T>::reverse_iterator IntrusiveList<T>::rbegin() noexcept {
        return reverse_iterator(iterator(&mAnchor));
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_reverse_iterator IntrusiveList<T>::rbegin() const noexcept {
        return const_reverse_iterator(const_iterator(&mAnchor));
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_reverse_iterator IntrusiveList<T>::crbegin() const noexcept {
        return const_reverse_iterator(const_iterator(&mAnchor));
    }


    template<typename T>
    inline typename IntrusiveList<T>::reverse_iterator IntrusiveList<T>::rend() noexcept {
        return reverse_iterator(iterator(mAnchor.mpNext));
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_reverse_iterator IntrusiveList<T>::rend() const noexcept {
        return const_reverse_iterator(const_iterator(mAnchor.mpNext));
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_reverse_iterator IntrusiveList<T>::crend() const noexcept {
        return const_reverse_iterator(const_iterator(mAnchor.mpNext));
    }


    template<typename T>
    inline typename IntrusiveList<T>::reference IntrusiveList<T>::front() {
        KCHECK(mAnchor.mpNext != &mAnchor) << "IntrusiveList::front(): empty list.";

        return *static_cast<T *>(mAnchor.mpNext);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_reference IntrusiveList<T>::front() const {
        KCHECK(mAnchor.mpNext != &mAnchor) << "IntrusiveList::front(): empty list.";

        return *static_cast<const T *>(mAnchor.mpNext);
    }


    template<typename T>
    inline typename IntrusiveList<T>::reference IntrusiveList<T>::back() {
        KCHECK(mAnchor.mpNext != &mAnchor) << "IntrusiveList::back(): empty list.";
        return *static_cast<T *>(mAnchor.mpPrev);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_reference IntrusiveList<T>::back() const {
        KCHECK(mAnchor.mpNext != &mAnchor) << "IntrusiveList::back(): empty list.";
        return *static_cast<const T *>(mAnchor.mpPrev);
    }


    template<typename T>
    inline void IntrusiveList<T>::push_front(value_type &x) {
        KCHECK(!x.mpNext && !x.mpPrev) << "IntrusiveList::push_front(): element already on a list.";

        x.mpNext = mAnchor.mpNext;
        x.mpPrev = &mAnchor;
        mAnchor.mpNext = &x;
        x.mpNext->mpPrev = &x;
    }


    template<typename T>
    inline void IntrusiveList<T>::push_back(value_type &x) {
        KCHECK(!x.mpNext && !x.mpPrev) << "IntrusiveList::push_front(): element already on a list.";

        x.mpPrev = mAnchor.mpPrev;
        x.mpNext = &mAnchor;
        mAnchor.mpPrev = &x;
        x.mpPrev->mpNext = &x;
    }


    template<typename T>
    inline bool IntrusiveList<T>::contains(const value_type &x) const {
        for (const IntrusiveListNode *p = mAnchor.mpNext; p != &mAnchor; p = p->mpNext) {
            if (p == &x)
                return true;
        }

        return false;
    }


    template<typename T>
    inline typename IntrusiveList<T>::iterator IntrusiveList<T>::locate(value_type &x) {
        for (IntrusiveListNode *p = (T *) mAnchor.mpNext; p != &mAnchor; p = p->mpNext) {
            if (p == &x)
                return iterator(p);
        }

        return iterator(&mAnchor);
    }


    template<typename T>
    inline typename IntrusiveList<T>::const_iterator IntrusiveList<T>::locate(const value_type &x) const {
        for (const IntrusiveListNode *p = mAnchor.mpNext; p != &mAnchor; p = p->mpNext) {
            if (p == &x)
                return const_iterator(p);
        }

        return const_iterator(&mAnchor);
    }


    template<typename T>
    inline typename IntrusiveList<T>::iterator IntrusiveList<T>::insert(const_iterator pos, value_type &x) {
        KCHECK(!x.mpNext && !x.mpPrev) << "IntrusiveList::push_front(): element already on a list.";

        IntrusiveListNode &next = *pos._pnode;
        IntrusiveListNode &prev = *next.mpPrev;

        prev.mpNext = next.mpPrev = &x;
        x.mpPrev = &prev;
        x.mpNext = &next;

        return iterator(&x);
    }


    template<typename T>
    inline typename IntrusiveList<T>::iterator
    IntrusiveList<T>::erase(const_iterator pos) {
        IntrusiveListNode &prev = *pos._pnode->mpPrev;
        IntrusiveListNode &next = *pos._pnode->mpNext;
        prev.mpNext = &next;
        next.mpPrev = &prev;
        iterator ii(pos._pnode);
        ii._pnode->mpPrev = ii._pnode->mpNext = nullptr;
        return iterator(&next);
    }


    template<typename T>
    inline typename IntrusiveList<T>::iterator
    IntrusiveList<T>::erase(const_iterator first, const_iterator last) {
        IntrusiveListNode &prev = *(first._pnode->mpPrev);
        IntrusiveListNode &next = *last._pnode;

        // need to clear out all the next/prev pointers in the elements;
        // this makes this operation O(n) instead of O(1), sadly, although
        // it's technically amortized O(1) since you could count yourself
        // as paying this cost with each insert.
        IntrusiveListNode *pCur = first._pnode;

        while (pCur != &next) {
            IntrusiveListNode *const pCurNext = pCur->mpNext;
            pCur->mpPrev = pCur->mpNext = nullptr;
            pCur = pCurNext;
        }
        prev.mpNext = &next;
        next.mpPrev = &prev;

        return iterator(last._pnode);
    }


    template<typename T>
    inline typename IntrusiveList<T>::reverse_iterator
    IntrusiveList<T>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T>
    inline typename IntrusiveList<T>::reverse_iterator
    IntrusiveList<T>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }


    template<typename T>
    void IntrusiveList<T>::swap(IntrusiveList &x) noexcept {
        // swap anchors
        IntrusiveListNode temp(mAnchor);
        mAnchor = x.mAnchor;
        x.mAnchor = temp;

        // Fixup node pointers into the anchor, since the addresses of
        // the anchors must stay the same with each list.
        if (mAnchor.mpNext == &x.mAnchor)
            mAnchor.mpNext = mAnchor.mpPrev = &mAnchor;
        else
            mAnchor.mpNext->mpPrev = mAnchor.mpPrev->mpNext = &mAnchor;

        if (x.mAnchor.mpNext == &mAnchor)
            x.mAnchor.mpNext = x.mAnchor.mpPrev = &x.mAnchor;
        else
            x.mAnchor.mpNext->mpPrev = x.mAnchor.mpPrev->mpNext = &x.mAnchor;

        temp.mpPrev = temp.mpNext = nullptr;
    }


    template<typename T>
    void IntrusiveList<T>::splice(const_iterator pos, value_type &value) {
        // Note that splice(pos, x, pos) and splice(pos+1, x, pos)
        // are valid and need to be handled correctly.

        if (pos._pnode != &value) {
            // Unlink item from old list.
            IntrusiveListNode &oldNext = *value.mpNext;
            IntrusiveListNode &oldPrev = *value.mpPrev;
            oldNext.mpPrev = &oldPrev;
            oldPrev.mpNext = &oldNext;

            // Relink item into new list.
            IntrusiveListNode &newNext = *pos._pnode;
            IntrusiveListNode &newPrev = *newNext.mpPrev;

            newPrev.mpNext = &value;
            newNext.mpPrev = &value;
            value.mpPrev = &newPrev;
            value.mpNext = &newNext;
        }
    }


    template<typename T>
    void IntrusiveList<T>::splice(const_iterator pos, IntrusiveList &x) {
        // Note: &x == this is prohibited, so self-insertion is not a problem.
        if (x.mAnchor.mpNext != &x.mAnchor) // If the list 'x' isn't empty...
        {
            IntrusiveListNode &next = *pos._pnode;
            IntrusiveListNode &prev = *next.mpPrev;
            IntrusiveListNode &insertPrev = *x.mAnchor.mpNext;
            IntrusiveListNode &insertNext = *x.mAnchor.mpPrev;

            prev.mpNext = &insertPrev;
            insertPrev.mpPrev = &prev;
            insertNext.mpNext = &next;
            next.mpPrev = &insertNext;
            x.mAnchor.mpPrev = x.mAnchor.mpNext = &x.mAnchor;
        }
    }


    template<typename T>
    void IntrusiveList<T>::splice(const_iterator pos, IntrusiveList & /*x*/, const_iterator i) {
        // Note: &x == this is prohibited, so self-insertion is not a problem.

        // Note that splice(pos, x, pos) and splice(pos + 1, x, pos)
        // are valid and need to be handled correctly.

        // We don't need to check if the source list is empty, because
        // this function expects a valid iterator from the source list,
        // and thus the list cannot be empty in such a situation.

        iterator ii(i._pnode); // Make a temporary non-const version.

        if (pos != ii) {
            // Unlink item from old list.
            IntrusiveListNode &oldNext = *ii._pnode->mpNext;
            IntrusiveListNode &oldPrev = *ii._pnode->mpPrev;
            oldNext.mpPrev = &oldPrev;
            oldPrev.mpNext = &oldNext;

            // Relink item into new list.
            IntrusiveListNode &newNext = *pos._pnode;
            IntrusiveListNode &newPrev = *newNext.mpPrev;

            newPrev.mpNext = ii._pnode;
            newNext.mpPrev = ii._pnode;
            ii._pnode->mpPrev = &newPrev;
            ii._pnode->mpNext = &newNext;
        }
    }


    template<typename T>
    void IntrusiveList<T>::splice(const_iterator pos, IntrusiveList & /*x*/, const_iterator first,
                                  const_iterator last) {
        // Note: &x == this is prohibited, so self-insertion is not a problem.
        if (first != last) {
            IntrusiveListNode &insertPrev = *first._pnode;
            IntrusiveListNode &insertNext = *last._pnode->mpPrev;

            // remove from old list
            insertNext.mpNext->mpPrev = insertPrev.mpPrev;
            insertPrev.mpPrev->mpNext = insertNext.mpNext;

            // insert into this list
            IntrusiveListNode &next = *pos._pnode;
            IntrusiveListNode &prev = *next.mpPrev;

            prev.mpNext = &insertPrev;
            insertPrev.mpPrev = &prev;
            insertNext.mpNext = &next;
            next.mpPrev = &insertNext;
        }
    }


    template<typename T>
    inline void IntrusiveList<T>::remove(value_type &value) {
        IntrusiveListNode &prev = *value.mpPrev;
        IntrusiveListNode &next = *value.mpNext;
        prev.mpNext = &next;
        next.mpPrev = &prev;
        value.mpPrev = value.mpNext = nullptr;
    }


    template<typename T>
    void IntrusiveList<T>::merge(this_type &x) {
        if (this != &x) {
            iterator first(begin());
            iterator firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while ((first != last) && (firstX != lastX)) {
                if (*firstX < *first) {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                } else
                    ++first;
            }

            if (firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }


    template<typename T>
    template<typename Compare>
    void IntrusiveList<T>::merge(this_type &x, Compare compare) {
        if (this != &x) {
            iterator first(begin());
            iterator firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while ((first != last) && (firstX != lastX)) {
                if (compare(*firstX, *first)) {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                } else
                    ++first;
            }

            if (firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }


    template<typename T>
    void IntrusiveList<T>::unique() {
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (*first == *next)
                    erase(next);
                else
                    first = next;
                next = first;
            }
        }
    }


    template<typename T>
    template<typename BinaryPredicate>
    void IntrusiveList<T>::unique(BinaryPredicate predicate) {
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (predicate(*first, *next))
                    erase(next);
                else
                    first = next;
                next = first;
            }
        }
    }


    template<typename T>
    void IntrusiveList<T>::sort() {
        // We implement the algorithm employed by Chris Caulfield whereby we use recursive
        // function calls to sort the list. The sorting of a very large list may fail due to stack overflow
        // if the stack is exhausted. The limit depends on the platform and the avaialble stack space.

        // Easier-to-understand version of the 'if' statement:
        // iterator i(begin());
        // if((i != end()) && (++i != end())) // If the size is >= 2 (without calling the more expensive size() function)...

        // Faster, more inlinable version of the 'if' statement:
        if ((mAnchor.mpNext != &mAnchor) && (mAnchor.mpNext != mAnchor.mpPrev)) {
            // Split the array into 2 roughly equal halves.
            this_type leftList; // This should cause no memory allocation.
            this_type rightList;


            iterator mid(begin());
            std::advance(mid, size() / 2);


            // Move the left half of this into leftList and the right half into rightList.
            leftList.splice(leftList.begin(), *this, begin(), mid);
            rightList.splice(rightList.begin(), *this);

            // Sort the sub-lists.
            leftList.sort();
            rightList.sort();

            // Merge the two halves into this list.
            splice(begin(), leftList);
            merge(rightList);
        }
    }


    template<typename T>
    template<typename Compare>
    void IntrusiveList<T>::sort(Compare compare) {
        // We implement the algorithm employed by Chris Caulfield whereby we use recursive
        // function calls to sort the list. The sorting of a very large list may fail due to stack overflow
        // if the stack is exhausted. The limit depends on the platform and the avaialble stack space.

        // Easier-to-understand version of the 'if' statement:
        // iterator i(begin());
        // if((i != end()) && (++i != end())) // If the size is >= 2 (without calling the more expensive size() function)...

        // Faster, more inlinable version of the 'if' statement:
        if ((mAnchor.mpNext != &mAnchor) && (mAnchor.mpNext != mAnchor.mpPrev)) {
            // Split the array into 2 roughly equal halves.
            this_type leftList; // This should cause no memory allocation.
            this_type rightList;


            iterator mid(begin());
            std::advance(mid, size() / 2);


            // Move the left half of this into leftList and the right half into rightList.
            leftList.splice(leftList.begin(), *this, begin(), mid);
            rightList.splice(rightList.begin(), *this);

            // Sort the sub-lists.
            leftList.sort(compare);
            rightList.sort(compare);

            // Merge the two halves into this list.
            splice(begin(), leftList);
            merge(rightList, compare);
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T>
    bool operator==(const IntrusiveList<T> &a, const IntrusiveList<T> &b) {
        // If we store an mSize member for IntrusiveList, we want to take advantage of it here.
        typename IntrusiveList<T>::const_iterator ia = a.begin();
        typename IntrusiveList<T>::const_iterator ib = b.begin();
        typename IntrusiveList<T>::const_iterator enda = a.end();
        typename IntrusiveList<T>::const_iterator endb = b.end();

        while ((ia != enda) && (ib != endb) && (*ia == *ib)) {
            ++ia;
            ++ib;
        }
        return (ia == enda) && (ib == endb);
    }

    template<typename T>
    bool operator!=(const IntrusiveList<T> &a, const IntrusiveList<T> &b) {
        return !(a == b);
    }

    template<typename T>
    bool operator<(const IntrusiveList<T> &a, const IntrusiveList<T> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template<typename T>
    bool operator>(const IntrusiveList<T> &a, const IntrusiveList<T> &b) {
        return b < a;
    }

    template<typename T>
    bool operator<=(const IntrusiveList<T> &a, const IntrusiveList<T> &b) {
        return !(b < a);
    }

    template<typename T>
    bool operator>=(const IntrusiveList<T> &a, const IntrusiveList<T> &b) {
        return !(a < b);
    }

    template<typename T>
    void swap(IntrusiveList<T> &a, IntrusiveList<T> &b) noexcept {
        a.swap(b);
    }
} // namespace fermat
