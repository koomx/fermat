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

#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

namespace fermat::art_internal {

    template <class T>
    class Node;
    template <class T>
    class InnerNode;
    template <class T>
    class LeafNode;


    template <class T>
    class InnerNode;

    template <class T>
    class ChildIterator {
    public:
        ChildIterator() = default;
        explicit ChildIterator(InnerNode<T>* n);
        ChildIterator(InnerNode<T>* n, int relative_index);
        ChildIterator(const ChildIterator<T>& other) = default;
        ChildIterator(ChildIterator<T>&& other) noexcept = default;
        ChildIterator<T>& operator=(const ChildIterator<T>& other) = default;
        ChildIterator<T>& operator=(ChildIterator<T>&& other) noexcept = default;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = char;
        using difference_type = int;
        using pointer = const value_type*;
        using reference = char;

        reference operator*() const;
        pointer operator->() const;
        ChildIterator& operator++();
        ChildIterator operator++(int);
        ChildIterator& operator--();
        ChildIterator operator--(int);
        bool operator==(const ChildIterator& rhs) const;
        bool operator!=(const ChildIterator& rhs) const;
        bool operator<(const ChildIterator& rhs) const;
        bool operator>(const ChildIterator& rhs) const;
        bool operator<=(const ChildIterator& rhs) const;
        bool operator>=(const ChildIterator& rhs) const;

        char get_partial_key() const;
        Node<T>* get_child_node() const;

    private:
        InnerNode<T>* node_ = nullptr;
        char cur_partial_key_ = -128;
        int relative_index_ = 0;
    };

    template <class T>
    ChildIterator<T>::ChildIterator(InnerNode<T>* n)
        : ChildIterator<T>(n, 0) { }

    template <class T>
    ChildIterator<T>::ChildIterator(InnerNode<T>* n, int relative_index)
        : node_(n)
        , cur_partial_key_(0)
        , relative_index_(relative_index) {
        if (relative_index_ < 0) {
            /* relative_index is out of bounds, no seek */
            return;
        }

        if (relative_index_ >= node_->n_children()) {
            /* relative_index is out of bounds, no seek */
            return;
        }

        if (relative_index_ == node_->n_children() - 1) {
            cur_partial_key_ = node_->prev_partial_key(127);
            return;
        }

        cur_partial_key_ = node_->next_partial_key(-128);
        for (int i = 0; i < relative_index_; ++i) {
            cur_partial_key_ = node_->next_partial_key(cur_partial_key_ + 1);
        }
    }

    template <class T>
    typename ChildIterator<T>::reference ChildIterator<T>::operator*() const {
        if (relative_index_ < 0 || relative_index_ >= node_->n_children()) {
            throw std::out_of_range("child iterator is out of range");
        }

        return cur_partial_key_;
    }

    template <class T>
    typename ChildIterator<T>::pointer ChildIterator<T>::operator->() const {
        if (relative_index_ < 0 || relative_index_ >= node_->n_children()) {
            throw std::out_of_range("child iterator is out of range");
        }

        return &cur_partial_key_;
    }

    template <class T>
    ChildIterator<T>& ChildIterator<T>::operator++() {
        ++relative_index_;
        if (relative_index_ < 0) {
            return *this;
        } else if (relative_index_ == 0) {
            cur_partial_key_ = node_->next_partial_key(-128);
        } else if (relative_index_ < node_->n_children()) {
            cur_partial_key_ = node_->next_partial_key(cur_partial_key_ + 1);
        }
        return *this;
    }

    template <class T>
    ChildIterator<T> ChildIterator<T>::operator++(int) {
        auto old = *this;
        operator++();
        return old;
    }

    template <class T>
    ChildIterator<T>& ChildIterator<T>::operator--() {
        --relative_index_;
        if (relative_index_ > node_->n_children() - 1) {
            return *this;
        } else if (relative_index_ == node_->n_children() - 1) {
            cur_partial_key_ = node_->prev_partial_key(127);
        } else if (relative_index_ >= 0) {
            cur_partial_key_ = node_->prev_partial_key(cur_partial_key_ - 1);
        }
        return *this;
    }

    template <class T>
    ChildIterator<T> ChildIterator<T>::operator--(int) {
        auto old = *this;
        operator--();
        return old;
    }

    template <class T>
    bool ChildIterator<T>::operator==(const ChildIterator<T>& rhs) const {
        return node_ == rhs.node_ && relative_index_ == rhs.relative_index_;
    }

    template <class T>
    bool ChildIterator<T>::operator<(const ChildIterator<T>& rhs) const {
        return node_ == rhs.node_ && relative_index_ < rhs.relative_index_;
    }

    template <class T>
    bool ChildIterator<T>::operator!=(const ChildIterator<T>& rhs) const {
        return !((*this) == rhs);
    }

    template <class T>
    bool ChildIterator<T>::operator>=(const ChildIterator<T>& rhs) const {
        return !((*this) < rhs);
    }

    template <class T>
    bool ChildIterator<T>::operator<=(const ChildIterator<T>& rhs) const {
        return (rhs >= (*this));
    }

    template <class T>
    bool ChildIterator<T>::operator>(const ChildIterator<T>& rhs) const {
        return (rhs < (*this));
    }

    template <class T>
    char ChildIterator<T>::get_partial_key() const {
        return cur_partial_key_;
    }

    template <class T>
    Node<T>* ChildIterator<T>::get_child_node() const {
        assert(0 <= relative_index_ && relative_index_ < node_->n_children());
        return *node_->find_child(cur_partial_key_);
    }


    template <class T>
    class ArtIterator {
    public:
        struct step {
            Node<T>* child_node_; // no ownership
            int depth_;
            char* key_; // ownership
            ChildIterator<T> child_it_;
            ChildIterator<T> child_it_end_;

            step();
            step(int depth, ChildIterator<T> c_it, ChildIterator<T> c_it_end);
            step(Node<T>* node, int depth, const char* key, ChildIterator<T> c_it, ChildIterator<T> c_it_end);
            step(const step& other);
            step(step&& other);
            ~step();

            step& operator=(const step& other);
            step& operator=(step&& other) noexcept;

            step& operator++();
            step operator++(int);
        };

        ArtIterator();
        explicit ArtIterator(Node<T>* root, std::vector<step> traversal_stack);

        static ArtIterator<T> min(Node<T>* root);
        static ArtIterator<T> greater_equal(Node<T>* root, const char* key);

        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = int;
        using pointer = value_type*;
        /* using reference = const value_type &; */

        /* reference operator*(); */
        value_type operator*();
        pointer operator->();
        ArtIterator<T>& operator++();
        ArtIterator<T> operator++(int);
        bool operator==(const ArtIterator<T>& rhs) const;
        bool operator!=(const ArtIterator<T>& rhs) const;

        template <class OutputIt>
        void key(OutputIt key) const;
        int get_key_len() const;
        const std::string key() const;

    private:
        step& get_step();
        const step& get_step() const;
        Node<T>* get_node() const;
        const char* get_key() const;
        int get_depth() const;

        void seek_leaf();

        Node<T>* root_;
        std::vector<step> traversal_stack_;
    };

    template <class T>
    ArtIterator<T>::step::step()
        : step(nullptr, 0, nullptr, { }, { }) { }

    template <class T>
    ArtIterator<T>::step::step(int depth, ChildIterator<T> c_it, ChildIterator<T> c_it_end)
        : child_node_(c_it != c_it_end ? c_it.get_child_node() : nullptr)
        , depth_(depth)
        , key_(depth ? new char[depth] : nullptr)
        , child_it_(c_it)
        , child_it_end_(c_it_end) { }

    template <class T>
    ArtIterator<T>::step::step(Node<T>* node, int depth, const char* key, ChildIterator<T> c_it, ChildIterator<T> c_it_end)
        : child_node_(node)
        , depth_(depth)
        , key_(depth ? new char[depth] : nullptr)
        , child_it_(c_it)
        , child_it_end_(c_it_end) {
        std::copy_n(key, depth, key_);
    }

    template <class T>
    typename ArtIterator<T>::step& ArtIterator<T>::step::operator++() {
        assert(child_it_ != child_it_end_);
        ++child_it_;
        child_node_ = child_it_ != child_it_end_
            ? child_it_.get_child_node()
            : nullptr;
        if (depth_ > 0) {
            key_[depth_ - 1] = child_it_ != child_it_end_
                ? child_it_.get_partial_key()
                : '\0';
        }
        return *this;
    }

    template <class T>
    typename ArtIterator<T>::step ArtIterator<T>::step::operator++(int) {
        auto old = *this;
        operator++();
        return old;
    }

    template <class T>
    ArtIterator<T>::step::step(const ArtIterator<T>::step& other)
        : step(other.child_node_, other.depth_, other.key_, other.child_it_, other.child_it_end_) { }

    template <class T>
    ArtIterator<T>::step::step(ArtIterator<T>::step&& other)
        : child_node_(other.child_node_)
        , depth_(other.depth_)
        , key_(other.key_)
        , child_it_(other.child_it_)
        , child_it_end_(other.child_it_end_) {
        other.child_node_ = nullptr;
        other.depth_ = 0;
        other.key_ = nullptr;
        other.child_it_ = { };
        other.child_it_end_ = { };
    }

    template <class T>
    ArtIterator<T>::step::~step() {
        delete[] key_;
    }

    template <class T>
    typename ArtIterator<T>::step& ArtIterator<T>::step::operator=(const ArtIterator<T>::step& other) {
        if (this != &other) {
            Node<T>* node = other.child_node_;
            int depth = other.depth_;
            char* key = depth ? new char[depth] : nullptr;
            std::copy_n(other.key_, other.depth_, key);
            ChildIterator<T> c_it = other.child_it_;
            ChildIterator<T> c_it_end = other.child_it_end_;

            child_node_ = node;
            depth_ = depth;
            delete[] key_;
            key_ = key;
            child_it_ = c_it;
            child_it_end_ = c_it_end;
        }
        return *this;
    }

    template <class T>
    typename ArtIterator<T>::step& ArtIterator<T>::step::operator=(ArtIterator<T>::step&& other) noexcept {
        if (this != &other) {
            child_node_ = other.child_node_;
            other.child_node_ = nullptr;

            depth_ = other.depth_;
            other.depth_ = 0;

            delete[] key_;
            key_ = other.key_;
            other.key_ = nullptr;

            child_it_ = other.child_it_;
            other.child_it_ = { };

            child_it_end_ = other.child_it_end_;
            other.child_it_end_ = { };
        }
        return *this;
    }

    template <class T>
    ArtIterator<T>::ArtIterator() { }

    template <class T>
    ArtIterator<T>::ArtIterator(Node<T>* root, std::vector<step> traversal_stack)
        : root_(root)
        , traversal_stack_(traversal_stack) {
        seek_leaf();
    }

    template <class T>
    ArtIterator<T> ArtIterator<T>::min(Node<T>* root) {
        return ArtIterator<T>::greater_equal(root, "");
    }

    template <class T>
    ArtIterator<T> ArtIterator<T>::greater_equal(Node<T>* root, const char* key) {
        assert(root != nullptr);

        int key_len = std::strlen(key);
        std::vector<ArtIterator<T>::step> traversal_stack;

        // sentinel child iterator for root
        traversal_stack.push_back({ root, 0, nullptr, { nullptr, -2 }, { nullptr, -1 } });

        while (true) {
            ArtIterator<T>::step& cur_step = traversal_stack.back();
            Node<T>* cur_node = cur_step.child_node_;
            int cur_depth = cur_step.depth_;

            int prefix_match_len = std::min<int>(cur_node->check_prefix(key + cur_depth, key_len - cur_depth), key_len - cur_depth);
            // if search key "equals" the prefix
            if (key_len == cur_depth + prefix_match_len) {
                return ArtIterator<T>(root, traversal_stack);
            }
            // if search key is "greater than" the prefix
            if (prefix_match_len < cur_node->prefix_len_ && key[cur_depth + prefix_match_len] > cur_node->prefix_[prefix_match_len]) {
                ++cur_step;
                return ArtIterator<T>(root, traversal_stack);
            }
            if (cur_node->is_leaf()) {
                continue;
            }
            // seek subtree where search key is "lesser than or equal" the subtree partial key
            InnerNode<T>* cur_inner_node = static_cast<InnerNode<T>*>(cur_node);
            ChildIterator<T> c_it = cur_inner_node->begin();
            ChildIterator<T> c_it_end = cur_inner_node->end();
            // TODO more efficient with specialized node search method?
            for (; c_it != c_it_end; ++c_it) {
                if (key[cur_depth + cur_node->prefix_len_] <= c_it.get_partial_key()) {
                    break;
                }
            }
            int depth = cur_depth + cur_node->prefix_len_ + 1;
            ArtIterator<T>::step child(depth, c_it, c_it_end);
            /* compute child key: cur_key + cur_node->prefix_ + child_partial_key */
            std::copy_n(cur_step.key_, cur_depth, child.key_);
            std::copy_n(cur_node->prefix_, cur_node->prefix_len_, child.key_ + cur_depth);
            child.key_[cur_depth + cur_node->prefix_len_] = c_it.get_partial_key();
            traversal_stack.push_back(child);
        }
    }

    template <class T>
    typename ArtIterator<T>::value_type ArtIterator<T>::operator*() {
        assert(get_node()->is_leaf());
        return static_cast<LeafNode<T>*>(get_node())->value_;
    }

    template <class T>
    typename ArtIterator<T>::pointer ArtIterator<T>::operator->() {
        assert(get_node()->is_leaf());
        return &static_cast<LeafNode<T>*>(get_node())->value_;
    }

    template <class T>
    ArtIterator<T>& ArtIterator<T>::operator++() {
        assert(get_node()->is_leaf());
        ++get_step();
        seek_leaf();
        return *this;
    }

    template <class T>
    ArtIterator<T> ArtIterator<T>::operator++(int) {
        auto old = *this;
        operator++();
        return old;
    }

    template <class T>
    bool ArtIterator<T>::operator==(const ArtIterator<T>& rhs) const {
        if (traversal_stack_.empty() && rhs.traversal_stack_.empty()) {
            /* both are empty */
            return true;
        }
        if (traversal_stack_.empty() || rhs.traversal_stack_.empty()) {
            /* one is empty */
            return false;
        }
        return get_node() == rhs.get_node();
    }

    template <class T>
    bool ArtIterator<T>::operator!=(const ArtIterator<T>& rhs) const {
        return !(*this == rhs);
    }

    template <class T>
    template <class OutputIt>
    void ArtIterator<T>::key(OutputIt key) const {
        std::copy_n(get_key(), get_depth(), key);
        std::copy_n(get_node()->prefix_, get_node()->prefix_len_, key + get_depth());
    }

    template <class T>
    int ArtIterator<T>::get_key_len() const {
        return get_depth() + get_node()->prefix_len_;
    }

    template <class T>
    const std::string ArtIterator<T>::key() const {
        std::string str(get_depth() + get_node()->prefix_len_ - 1, 0);
        key(str.begin());
        return str;
    }

    template <class T>
    void ArtIterator<T>::seek_leaf() {
        /* traverse up until a node on the right is found or stack gets empty */
        for (; get_step().child_it_ == get_step().child_it_end_; ++get_step()) {
            traversal_stack_.pop_back();
            if (traversal_stack_.empty()) {
                return;
            }
            if (get_step().child_node_ == root_) { // root guard
                traversal_stack_.pop_back();
                assert(traversal_stack_.empty());
                return;
            }
        }

        /* find leftmost leaf node */
        while (!get_node()->is_leaf()) {
            InnerNode<T>* cur_inner_node = static_cast<InnerNode<T>*>(get_node());
            int depth = get_depth() + get_node()->prefix_len_ + 1;
            ChildIterator<T> c_it = cur_inner_node->begin();
            ChildIterator<T> c_it_end = cur_inner_node->end();
            ArtIterator<T>::step child(depth, c_it, c_it_end);
            /* compute child key: cur_key + cur_node->prefix_ + child_partial_key */
            std::copy_n(get_key(), get_depth(), child.key_);
            std::copy_n(get_node()->prefix_, get_node()->prefix_len_, child.key_ + get_depth());
            child.key_[get_depth() + get_node()->prefix_len_] = c_it.get_partial_key();
            traversal_stack_.push_back(child);
        }
    }

    template <class T>
    Node<T>* ArtIterator<T>::get_node() const {
        return get_step().child_node_;
    }

    template <class T>
    int ArtIterator<T>::get_depth() const {
        return get_step().depth_;
    }

    template <class T>
    const char* ArtIterator<T>::get_key() const {
        return get_step().key_;
    }

    template <class T>
    typename ArtIterator<T>::step& ArtIterator<T>::get_step() {
        assert(!traversal_stack_.empty());
        return traversal_stack_.back();
    }

    template <class T>
    const typename ArtIterator<T>::step& ArtIterator<T>::get_step() const {
        assert(!traversal_stack_.empty());
        return traversal_stack_.back();
    }

} // fermat::art_internal
