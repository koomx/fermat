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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fermat/art/internal/iterator.h>
#include <iostream>
#include <iterator>
#include <stdexcept>
#if defined(__i386__) || defined(__amd64__)
#include <emmintrin.h>
#endif

namespace fermat::art_internal {

    template <class T>
    class Node {
    public:
        virtual ~Node() = default;

        Node() = default;
        Node(const Node<T>& other) = default;
        Node(Node<T>&& other) noexcept = default;
        Node<T>& operator=(const Node<T>& other) = default;
        Node<T>& operator=(Node<T>&& other) noexcept = default;

        /**
         * Determines if this Node is a leaf Node, i.e., contains a value.
         * Needed for downcasting a Node<T> instance to a LeafNode<T> or InnerNode<T> instance.
         */
        virtual bool is_leaf() const = 0;

        /**
         * Determines the number of matching bytes between the Node's prefix and the key.
         *
         * Given a Node with prefix: "abbbd", a key "abbbccc",
         * check_prefix returns 4, since byte 4 of the prefix ('d') does not
         * match byte 4 of the key ('c').
         *
         * key:     "abbbccc"
         * prefix:  "abbbd"
         *           ^^^^*
         * index:    01234
         */
        int check_prefix(const char* key, int key_len) const;

        char* prefix_ = nullptr;
        uint16_t prefix_len_ = 0;
    };

    template <class T>
    int Node<T>::check_prefix(const char* key, int /* key_len */) const {
        return std::mismatch(prefix_, prefix_ + prefix_len_, key).second - key;
    }

    template <class T>
    class art;

    template <class T>
    class LeafNode : public Node<T> {
    public:
        explicit LeafNode(T value);
        bool is_leaf() const override;

        T value_;
    };

    template <class T>
    LeafNode<T>::LeafNode(T value)
        : value_(value) { }

    template <class T>
    bool LeafNode<T>::is_leaf() const {
        return true;
    }

    template <class T>
    class InnerNode : public Node<T> {
    public:
        InnerNode() = default;
        InnerNode(const InnerNode<T>& other) = default;
        InnerNode(InnerNode<T>&& other) noexcept = default;
        InnerNode<T>& operator=(const InnerNode<T>& other) = default;
        InnerNode<T>& operator=(InnerNode<T>&& other) noexcept = default;
        virtual ~InnerNode() = default;

        bool is_leaf() const override;

        /**
         * Finds and returns the child node identified by the given partial key.
         *
         * @param partial_key - The partial key associated with the child.
         * @return Child node identified by the given partial key or
         * a null pointer of no child node is associated with the partial key.
         */
        virtual Node<T>** find_child(char partial_key) = 0;

        /**
         * Adds the given node to the node's children.
         * No bounds checking is done.
         * If a child already exists under the given partial key, the child
         * is overwritten without deleting it.
         *
         * @pre Node should not be full.
         * @param partial_key - The partial key associated with the child.
         * @param child - The child node.
         */
        virtual void set_child(char partial_key, Node<T>* child) = 0;

        /**
         * Deletes the child associated with the given partial key.
         *
         * @param partial_key - The partial key associated with the child.
         */
        virtual Node<T>* del_child(char partial_key) = 0;

        /**
         * Creates and returns a new node with bigger children capacity.
         * The current node gets deleted.
         *
         * @return node with bigger capacity
         */
        virtual InnerNode<T>* grow() = 0;

        /**
         * Creates and returns a new node with lesser children capacity.
         * The current node gets deleted.
         *
         * @pre node must be undefull
         * @return node with lesser capacity
         */
        virtual InnerNode<T>* shrink() = 0;

        /**
         * Determines if the node is full, i.e. can carry no more child nodes.
         */
        virtual bool is_full() const = 0;

        /**
         * Determines if the node is underfull, i.e. carries less child nodes than
         * intended.
         */
        virtual bool is_underfull() const = 0;

        virtual int n_children() const = 0;

        virtual char next_partial_key(char partial_key) const = 0;

        virtual char prev_partial_key(char partial_key) const = 0;

        /**
         * Iterator on the first child node.
         *
         * @return Iterator on the first child node.
         */
        ChildIterator<T> begin();
        std::reverse_iterator<ChildIterator<T>> rbegin();

        /**
         * Iterator on after the last child node.
         *
         * @return Iterator on after the last child node.
         */
        ChildIterator<T> end();
        std::reverse_iterator<ChildIterator<T>> rend();
    };

    template <class T>
    bool InnerNode<T>::is_leaf() const {
        return false;
    }

    template <class T>
    ChildIterator<T> InnerNode<T>::begin() {
        return ChildIterator<T>(this);
    }

    template <class T>
    std::reverse_iterator<ChildIterator<T>> InnerNode<T>::rbegin() {
        return std::reverse_iterator<ChildIterator<T>>(end());
    }

    template <class T>
    ChildIterator<T> InnerNode<T>::end() {
        return ChildIterator<T>(this, n_children());
    }

    template <class T>
    std::reverse_iterator<ChildIterator<T>> InnerNode<T>::rend() {
        return std::reverse_iterator<ChildIterator<T>>(begin());
    }

    /// node 4

    template <class T>
    class node_0;
    template <class T>
    class node_16;

    template <class T>
    class node_4 : public InnerNode<T> {
        friend class node_0<T>;
        friend class node_16<T>;

    public:
        Node<T>** find_child(char partial_key) override;
        void set_child(char partial_key, Node<T>* child) override;
        Node<T>* del_child(char partial_key) override;
        InnerNode<T>* grow() override;
        InnerNode<T>* shrink() override;
        bool is_full() const override;
        bool is_underfull() const override;

        char next_partial_key(char partial_key) const override;

        char prev_partial_key(char partial_key) const override;

        int n_children() const override;

    private:
        uint8_t n_children_ = 0;
        char keys_[4];
        Node<T>* children_[4];
    };

    template <class T>
    Node<T>** node_4<T>::find_child(char partial_key) {
        for (int i = 0; i < n_children_; ++i) {
            if (keys_[i] == partial_key) {
                return &children_[i];
            }
        }
        return nullptr;
    }

    template <class T>
    void node_4<T>::set_child(char partial_key, Node<T>* child) {
        /* determine index for child */
        int c_i;
        for (c_i = 0; c_i < n_children_ && partial_key >= keys_[c_i]; ++c_i) {
        }
        std::memmove(keys_ + c_i + 1, keys_ + c_i, n_children_ - c_i);
        std::memmove(children_ + c_i + 1, children_ + c_i,
            (n_children_ - c_i) * sizeof(void*));

        keys_[c_i] = partial_key;
        children_[c_i] = child;
        ++n_children_;
    }

    template <class T>
    Node<T>* node_4<T>::del_child(char partial_key) {
        Node<T>* child_to_delete = nullptr;
        for (int i = 0; i < n_children_; ++i) {
            if (child_to_delete == nullptr && partial_key == keys_[i]) {
                child_to_delete = children_[i];
            }
            if (child_to_delete != nullptr) {
                /* move existing sibling to the left */
                keys_[i] = i < n_children_ - 1 ? keys_[i + 1] : 0;
                children_[i] = i < n_children_ - 1 ? children_[i + 1] : nullptr;
            }
        }
        if (child_to_delete != nullptr) {
            --n_children_;
        }
        return child_to_delete;
    }

    template <class T>
    InnerNode<T>* node_4<T>::grow() {
        auto new_node = new node_16<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        new_node->n_children_ = this->n_children_;
        std::copy(this->keys_, this->keys_ + this->n_children_, new_node->keys_);
        std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
        delete this;
        return new_node;
    }

    template <class T>
    InnerNode<T>* node_4<T>::shrink() {
        throw std::runtime_error("node_4 cannot shrink");
    }

    template <class T>
    bool node_4<T>::is_full() const {
        return n_children_ == 4;
    }

    template <class T>
    bool node_4<T>::is_underfull() const {
        return false;
    }

    template <class T>
    char node_4<T>::next_partial_key(char partial_key) const {
        for (int i = 0; i < n_children_; ++i) {
            if (keys_[i] >= partial_key) {
                return keys_[i];
            }
        }
        /* return 0; */
        throw std::out_of_range("provided partial key does not have a successor");
    }

    template <class T>
    char node_4<T>::prev_partial_key(char partial_key) const {
        for (int i = n_children_ - 1; i >= 0; --i) {
            if (keys_[i] <= partial_key) {
                return keys_[i];
            }
        }
        /* return 255; */
        throw std::out_of_range("provided partial key does not have a predecessor");
    }

    template <class T>
    int node_4<T>::n_children() const {
        return this->n_children_;
    }

    /// node 16

    template <class T>
    class node_4;
    template <class T>
    class node_48;

    template <class T>
    class node_16 : public InnerNode<T> {
        friend class node_4<T>;
        friend class node_48<T>;

    public:
        Node<T>** find_child(char partial_key) override;
        void set_child(char partial_key, Node<T>* child) override;
        Node<T>* del_child(char partial_key) override;
        InnerNode<T>* grow() override;
        InnerNode<T>* shrink() override;
        bool is_full() const override;
        bool is_underfull() const override;

        char next_partial_key(char partial_key) const override;

        char prev_partial_key(char partial_key) const override;

        int n_children() const override;

    private:
        uint8_t n_children_ = 0;
        char keys_[16];
        Node<T>* children_[16];
    };

    template <class T>
    Node<T>** node_16<T>::find_child(char partial_key) {
#if defined(__i386__) || defined(__amd64__)
        int bitfield = _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_set1_epi8(partial_key),
                           _mm_loadu_si128((__m128i*)keys_)))
            & ((1 << n_children_) - 1);
        return (bool)bitfield ? &children_[__builtin_ctz(bitfield)] : nullptr;
#else
        int lo, mid, hi;
        lo = 0;
        hi = n_children_;
        while (lo < hi) {
            mid = (lo + hi) / 2;
            if (partial_key < keys_[mid]) {
                hi = mid;
            } else if (partial_key > keys_[mid]) {
                lo = mid + 1;
            } else {
                return &children_[mid];
            }
        }
        return nullptr;
#endif
    }

    template <class T>
    void node_16<T>::set_child(char partial_key, Node<T>* child) {
        /* determine index for child */
        int child_i = 0;
        for (int i = this->n_children_ - 1;; --i) {
            if (i >= 0 && partial_key < this->keys_[i]) {
                /* move existing sibling to the right */
                this->keys_[i + 1] = this->keys_[i];
                this->children_[i + 1] = this->children_[i];
            } else {
                child_i = i + 1;
                break;
            }
        }

        this->keys_[child_i] = partial_key;
        this->children_[child_i] = child;
        ++n_children_;
    }

    template <class T>
    Node<T>* node_16<T>::del_child(char partial_key) {
        Node<T>* child_to_delete = nullptr;
        for (int i = 0; i < n_children_; ++i) {
            if (child_to_delete == nullptr && partial_key == keys_[i]) {
                child_to_delete = children_[i];
            }
            if (child_to_delete != nullptr) {
                /* move existing sibling to the left */
                keys_[i] = i < n_children_ - 1 ? keys_[i + 1] : 0;
                children_[i] = i < n_children_ - 1 ? children_[i + 1] : nullptr;
            }
        }
        if (child_to_delete != nullptr) {
            --n_children_;
        }
        return child_to_delete;
    }

    template <class T>
    InnerNode<T>* node_16<T>::grow() {
        auto new_node = new node_48<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        new_node->n_children_ = this->n_children_;
        std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
        for (int i = 0; i < n_children_; ++i) {
            new_node->indexes_[128 + (uint8_t)this->keys_[i]] = i;
        }
        delete this;
        return new_node;
    }

    template <class T>
    InnerNode<T>* node_16<T>::shrink() {
        auto new_node = new node_4<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        new_node->n_children_ = this->n_children_;
        std::copy(this->keys_, this->keys_ + this->n_children_, new_node->keys_);
        std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
        delete this;
        return new_node;
    }

    template <class T>
    bool node_16<T>::is_full() const {
        return n_children_ == 16;
    }

    template <class T>
    bool node_16<T>::is_underfull() const {
        return n_children_ == 4;
    }

    template <class T>
    char node_16<T>::next_partial_key(char partial_key) const {
        for (int i = 0; i < n_children_; ++i) {
            if (keys_[i] >= partial_key) {
                return keys_[i];
            }
        }
        throw std::out_of_range("provided partial key does not have a successor");
    }

    template <class T>
    char node_16<T>::prev_partial_key(char partial_key) const {
        for (int i = n_children_ - 1; i >= 0; --i) {
            if (keys_[i] <= partial_key) {
                return keys_[i];
            }
        }
        throw std::out_of_range("provided partial key does not have a predecessor");
    }

    template <class T>
    int node_16<T>::n_children() const {
        return n_children_;
    }

    /// node 48

    template <class T>
    class node_16;
    template <class T>
    class node_256;

    template <class T>
    class node_48 : public InnerNode<T> {
        friend class node_16<T>;
        friend class node_256<T>;

    public:
        node_48();

        Node<T>** find_child(char partial_key) override;
        void set_child(char partial_key, Node<T>* child) override;
        Node<T>* del_child(char partial_key) override;
        InnerNode<T>* grow() override;
        InnerNode<T>* shrink() override;
        bool is_full() const override;
        bool is_underfull() const override;

        char next_partial_key(char partial_key) const override;
        char prev_partial_key(char partial_key) const override;

        int n_children() const override;

    private:
        static const char EMPTY;

        uint8_t n_children_ = 0;
        char indexes_[256];
        Node<T>* children_[48];
    };

    template <class T>
    node_48<T>::node_48() {
        std::fill(this->indexes_, this->indexes_ + 256, node_48::EMPTY);
        std::fill(this->children_, this->children_ + 48, nullptr);
    }

    template <class T>
    Node<T>** node_48<T>::find_child(char partial_key) {
        // TODO(rafaelkallis): direct lookup instead of temp save?
        uint8_t index = indexes_[128 + partial_key];
        return node_48::EMPTY != index ? &children_[index] : nullptr;
    }

    template <class T>
    void node_48<T>::set_child(char partial_key, Node<T>* child) {

        // TODO(rafaelkallis): pick random starting entry in order to increase
        // performance? i.e. for (int i = random([0,48)); i != (i-1) % 48; i = (i+1) %
        // 48){}

        /* find empty child entry */
        for (int i = 0; i < 48; ++i) {
            if (children_[i] == nullptr) {
                indexes_[128 + partial_key] = (uint8_t)i;
                children_[i] = child;
                break;
            }
        }
        ++n_children_;
    }

    template <class T>
    Node<T>* node_48<T>::del_child(char partial_key) {
        Node<T>* child_to_delete = nullptr;
        unsigned char index = indexes_[128 + partial_key];
        if (index != node_48::EMPTY) {
            child_to_delete = children_[index];
            indexes_[128 + partial_key] = node_48::EMPTY;
            children_[index] = nullptr;
            --n_children_;
        }
        return child_to_delete;
    }

    template <class T>
    InnerNode<T>* node_48<T>::grow() {
        auto new_node = new node_256<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        uint8_t index;
        for (int partial_key = -128; partial_key < 127; ++partial_key) {
            index = indexes_[128 + partial_key];
            if (index != node_48::EMPTY) {
                new_node->set_child(partial_key, children_[index]);
            }
        }
        delete this;
        return new_node;
    }

    template <class T>
    InnerNode<T>* node_48<T>::shrink() {
        auto new_node = new node_16<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        uint8_t index;
        for (int partial_key = -128; partial_key < 127; ++partial_key) {
            index = indexes_[128 + partial_key];
            if (index != node_48::EMPTY) {
                new_node->set_child(partial_key, children_[index]);
            }
        }
        delete this;
        return new_node;
    }

    template <class T>
    bool node_48<T>::is_full() const {
        return n_children_ == 48;
    }

    template <class T>
    bool node_48<T>::is_underfull() const {
        return n_children_ == 16;
    }

    template <class T>
    const char node_48<T>::EMPTY = 48;

    template <class T>
    char node_48<T>::next_partial_key(char partial_key) const {
        while (true) {
            if (indexes_[128 + partial_key] != node_48<T>::EMPTY) {
                return partial_key;
            }
            if (partial_key == 127) {
                throw std::out_of_range("provided partial key does not have a successor");
            }
            ++partial_key;
        }
    }

    template <class T>
    char node_48<T>::prev_partial_key(char partial_key) const {
        while (true) {
            if (indexes_[128 + partial_key] != node_48<T>::EMPTY) {
                return partial_key;
            }
            if (partial_key == -128) {
                throw std::out_of_range(
                    "provided partial key does not have a predecessor");
            }
            --partial_key;
        }
    }

    template <class T>
    int node_48<T>::n_children() const {
        return n_children_;
    }

    /// node 256

    template <class T>
    class node_48;

    template <class T>
    class node_256 : public InnerNode<T> {
        friend class node_48<T>;

    public:
        node_256();

        Node<T>** find_child(char partial_key) override;
        void set_child(char partial_key, Node<T>* child) override;
        Node<T>* del_child(char partial_key) override;
        InnerNode<T>* grow() override;
        InnerNode<T>* shrink() override;
        bool is_full() const override;
        bool is_underfull() const override;

        char next_partial_key(char partial_key) const override;

        char prev_partial_key(char partial_key) const override;

        int n_children() const override;

    private:
        uint16_t n_children_ = 0;
        std::array<Node<T>*, 256> children_;
    };

    template <class T>
    node_256<T>::node_256() {
        children_.fill(nullptr);
    }

    template <class T>
    Node<T>** node_256<T>::find_child(char partial_key) {
        return children_[128 + partial_key] != nullptr ? &children_[128 + partial_key]
                                                       : nullptr;
    }

    template <class T>
    void node_256<T>::set_child(char partial_key, Node<T>* child) {
        children_[128 + partial_key] = child;
        ++n_children_;
    }

    template <class T>
    Node<T>* node_256<T>::del_child(char partial_key) {
        Node<T>* child_to_delete = children_[128 + partial_key];
        if (child_to_delete != nullptr) {
            children_[128 + partial_key] = nullptr;
            --n_children_;
        }
        return child_to_delete;
    }

    template <class T>
    InnerNode<T>* node_256<T>::grow() {
        throw std::runtime_error("node_256 cannot grow");
    }

    template <class T>
    InnerNode<T>* node_256<T>::shrink() {
        auto new_node = new node_48<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        for (int partial_key = 0; partial_key < 256; ++partial_key) {
            if (children_[128 + partial_key] != nullptr) {
                new_node->set_child(partial_key, children_[128 + partial_key]);
            }
        }
        delete this;
        return new_node;
    }

    template <class T>
    bool node_256<T>::is_full() const {
        return n_children_ == 256;
    }

    template <class T>
    bool node_256<T>::is_underfull() const {
        return n_children_ == 48;
    }

    template <class T>
    char node_256<T>::next_partial_key(char partial_key) const {
        while (true) {
            if (children_[128 + partial_key] != nullptr) {
                return partial_key;
            }
            if (partial_key == 127) {
                throw std::out_of_range("provided partial key does not have a successor");
            }
            ++partial_key;
        }
    }

    template <class T>
    char node_256<T>::prev_partial_key(char partial_key) const {
        while (true) {
            if (children_[128 + partial_key] != nullptr) {
                return partial_key;
            }
            if (partial_key == -128) {
                throw std::out_of_range(
                    "provided partial key does not have a predecessor");
            }
            --partial_key;
        }
    }

    template <class T>
    int node_256<T>::n_children() const {
        return n_children_;
    }

} // namespace fermat::art_internal

