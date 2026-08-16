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
#include <fermat/art/internal/iterator.h>
#include <fermat/art/internal/node.h>
#include <iostream>
#include <stack>

namespace fermat {

    template <class T>
    class Art {
    public:
        using iterator = art_internal::ArtIterator<T>;
        using const_iterator = art_internal::ArtIterator<T>;
        using value_type = T;
        using node_type = art_internal::Node<T>;
    private:
        using child_iterator = art_internal::ChildIterator<T>;
        using inner_node = art_internal::InnerNode<T>;
        using leaf_node = art_internal::LeafNode<T>;
    public:
        ~Art();

        /**
         * Finds the value associated with the given key.
         *
         * @param key - The key to find.
         * @return the value associated with the key or a default constructed value.
         */
        T get(const char* key) const;

        /**
         * Associates the given key with the given value.
         * If another value is already associated with the given key,
         * since the method consumer is the resource owner.
         *
         * @param key - The key to associate with the value.
         * @param value - The value to be associated with the key.
         * @return a nullptr if no other value is associated with they or the
         * previously associated value.
         */
        T set(const char* key, T value);

        /**
         * Deletes the given key and returns it's associated value.
         * The associated value is returned,
         * since the method consumer is the resource owner.
         * If no value is associated with the given key, nullptr is returned.
         *
         * @param key - The key to delete.
         * @return the values assciated with they key or a nullptr otherwise.
         */
        T del(const char* key);

        /**
         * Forward iterator that traverses the tree in lexicographic order.
         */
        iterator begin();

        /**
         * Forward iterator that traverses the tree in lexicographic order starting
         * from the provided key.
         */
        iterator begin(const char* key);

        /**
         * Iterator to the end of the lexicographic order.
         */
        iterator end();

    private:
        node_type* root_ = nullptr;
    };

    template <class T>
    Art<T>::~Art() {
        if (root_ == nullptr) {
            return;
        }
        std::stack<node_type*> node_stack;
        node_stack.push(root_);
        node_type* cur;
        inner_node* cur_inner;
        child_iterator it, it_end;
        while (!node_stack.empty()) {
            cur = node_stack.top();
            node_stack.pop();
            if (!cur->is_leaf()) {
                cur_inner = static_cast<inner_node*>(cur);
                for (it = cur_inner->begin(), it_end = cur_inner->end(); it != it_end; ++it) {
                    node_stack.push(*cur_inner->find_child(*it));
                }
            }
            if (cur->prefix_ != nullptr) {
                delete[] cur->prefix_;
            }
            delete cur;
        }
    }

    template <class T>
    T Art<T>::get(const char* key) const {
        node_type*cur = root_, **child;
        int depth = 0, key_len = std::strlen(key) + 1;
        while (cur != nullptr) {
            if (cur->prefix_len_ != cur->check_prefix(key + depth, key_len - depth)) {
                /* prefix mismatch */
                return T { };
            }
            if (cur->prefix_len_ == key_len - depth) {
                /* exact match */
                return cur->is_leaf() ? static_cast<leaf_node*>(cur)->value_ : T { };
            }
            child = static_cast<inner_node*>(cur)->find_child(key[depth + cur->prefix_len_]);
            depth += (cur->prefix_len_ + 1);
            cur = child != nullptr ? *child : nullptr;
        }
        return T { };
    }

    template <class T>
    T Art<T>::set(const char* key, T value) {
        int key_len = std::strlen(key) + 1, depth = 0, prefix_match_len;
        if (root_ == nullptr) {
            root_ = new leaf_node(value);
            root_->prefix_ = new char[key_len];
            std::copy(key, key + key_len, root_->prefix_);
            root_->prefix_len_ = key_len;
            return T { };
        }

        node_type**cur = &root_, **child;
        inner_node** cur_inner;
        char child_partial_key;
        bool is_prefix_match;

        while (true) {
            /* number of bytes of the current node's prefix that match the key */
            prefix_match_len = (**cur).check_prefix(key + depth, key_len - depth);

            /* true if the current node's prefix matches with a part of the key */
            is_prefix_match = (std::min<int>((**cur).prefix_len_, key_len - depth)) == prefix_match_len;

            if (is_prefix_match && (**cur).prefix_len_ == key_len - depth) {
                /* exact match:
                 * => "replace"
                 * => replace value of current node.
                 * => return old value to caller to handle.
                 *        _                             _
                 *        |                             |
                 *       (aa)                          (aa)
                 *    a /    \ b     +[aaaaa,v3]    a /    \ b
                 *     /      \      ==========>     /      \
                 * *(aa)->v1  ()->v2             *(aa)->v3  ()->v2
                 *
                 */

                /* cur must be a leaf */
                auto cur_leaf = static_cast<leaf_node*>(*cur);
                T old_value = cur_leaf->value_;
                cur_leaf->value_ = value;
                return old_value;
            }

            if (!is_prefix_match) {
                /* prefix mismatch:
                 * => new parent node with common prefix and no associated value.
                 * => new node with value to insert.
                 * => current and new node become children of new parent node.
                 *
                 *        |                        |
                 *      *(aa)                    +(a)->Ø
                 *    a /    \ b     +[ab,v3]  a /   \ b
                 *     /      \      =======>   /     \
                 *  (aa)->v1  ()->v2          *()->Ø +()->v3
                 *                          a /   \ b
                 *                           /     \
                 *                        (aa)->v1 ()->v2
                 *                        /|\      /|\
                 */

                auto new_parent = new art_internal::node_4<T>();
                new_parent->prefix_ = new char[prefix_match_len];
                std::copy((**cur).prefix_, (**cur).prefix_ + prefix_match_len,
                    new_parent->prefix_);
                new_parent->prefix_len_ = prefix_match_len;
                new_parent->set_child((**cur).prefix_[prefix_match_len], *cur);

                // TODO(rafaelkallis): shrink?
                /* memmove((**cur).prefix_, (**cur).prefix_ + prefix_match_len + 1, */
                /*         (**cur).prefix_len_ - prefix_match_len - 1); */
                /* (**cur).prefix_len_ -= prefix_match_len + 1; */

                auto old_prefix = (**cur).prefix_;
                auto old_prefix_len = (**cur).prefix_len_;
                (**cur).prefix_ = new char[old_prefix_len - prefix_match_len - 1];
                (**cur).prefix_len_ = old_prefix_len - prefix_match_len - 1;
                std::copy(old_prefix + prefix_match_len + 1, old_prefix + old_prefix_len,
                    (**cur).prefix_);
                delete[] old_prefix;

                auto new_node = new leaf_node(value);
                new_node->prefix_ = new char[key_len - depth - prefix_match_len - 1];
                std::copy(key + depth + prefix_match_len + 1, key + key_len,
                    new_node->prefix_);
                new_node->prefix_len_ = key_len - depth - prefix_match_len - 1;
                new_parent->set_child(key[depth + prefix_match_len], new_node);

                *cur = new_parent;
                return T { };
            }

            /* must be inner node */
            cur_inner = reinterpret_cast<inner_node**>(cur);
            child_partial_key = key[depth + (**cur).prefix_len_];
            child = (**cur_inner).find_child(child_partial_key);

            if (child == nullptr) {
                /*
                 * no child associated with the next partial key.
                 * => create new node with value to insert.
                 * => new node becomes current node's child.
                 *
                 *      *(aa)->Ø              *(aa)->Ø
                 *    a /        +[aab,v2]  a /    \ b
                 *     /         ========>   /      \
                 *   (a)->v1               (a)->v1 +()->v2
                 */

                if ((**cur_inner).is_full()) {
                    *cur_inner = (**cur_inner).grow();
                }

                auto new_node = new leaf_node(value);
                new_node->prefix_ = new char[key_len - depth - (**cur).prefix_len_ - 1];
                std::copy(key + depth + (**cur).prefix_len_ + 1, key + key_len,
                    new_node->prefix_);
                new_node->prefix_len_ = key_len - depth - (**cur).prefix_len_ - 1;
                (**cur_inner).set_child(child_partial_key, new_node);
                return T { };
            }

            /* propagate down and repeat:
             *
             *     *(aa)->Ø                   (aa)->Ø
             *   a /    \ b    +[aaba,v3]  a /    \ b     repeat
             *    /      \     =========>   /      \     ========>  ...
             *  (a)->v1  ()->v2           (a)->v1 *()->v2
             */

            depth += (**cur).prefix_len_ + 1;
            cur = child;
        }
    }

    template <class T>
    T Art<T>::del(const char* key) {
        int depth = 0, key_len = std::strlen(key) + 1;

        if (root_ == nullptr) {
            return T { };
        }

        /* pointer to parent, current and child node */
        node_type** cur = &root_;
        inner_node** par = nullptr;

        /* partial key of current and child node */
        char cur_partial_key = 0;

        while (cur != nullptr) {
            if ((**cur).prefix_len_ != (**cur).check_prefix(key + depth, key_len - depth)) {
                /* prefix mismatch => key doesn't exist */

                return T { };
            }

            if (key_len == depth + (**cur).prefix_len_) {
                /* exact match */
                if (!(**cur).is_leaf()) {
                    return T { };
                }
                auto value = static_cast<leaf_node*>(*cur)->value_;
                auto n_siblings = par != nullptr ? (**par).n_children() - 1 : 0;

                if (n_siblings == 0) {
                    /*
                     * => must be root node
                     * => delete root node
                     *
                     *     |                 |
                     *    (aa)->v1          (aa)->v1
                     *     | a     -[aaaaa]
                     *     |       =======>
                     *   *(aa)->v2
                     */

                    if ((**cur).prefix_ != nullptr) {
                        delete[] (**cur).prefix_;
                    }
                    delete (*cur);
                    *cur = nullptr;

                } else if (n_siblings == 1) {
                    /* => delete leaf node
                     * => replace parent with sibling
                     *
                     *        |a                         |a
                     *        |                          |
                     *       (aa)        -"aaaaabaa"     |
                     *    a /    \ b     ==========>    /
                     *     /      \                    /
                     *  (aa)->v1 *()->v2             (aaaaa)->v1
                     *  /|\                            /|\
                     */

                    /* find sibling */
                    auto sibling_partial_key = (**par).next_partial_key(0);
                    if (sibling_partial_key == cur_partial_key) {
                        sibling_partial_key = (**par).next_partial_key(cur_partial_key + 1);
                    }
                    auto sibling = *(**par).find_child(sibling_partial_key);

                    auto old_prefix = sibling->prefix_;
                    auto old_prefix_len = sibling->prefix_len_;

                    sibling->prefix_ = new char[(**par).prefix_len_ + 1 + old_prefix_len];
                    sibling->prefix_len_ = (**par).prefix_len_ + 1 + old_prefix_len;
                    std::copy((**par).prefix_, (**par).prefix_ + (**par).prefix_len_,
                        sibling->prefix_);
                    sibling->prefix_[(**par).prefix_len_] = sibling_partial_key;
                    std::copy(old_prefix, old_prefix + old_prefix_len,
                        sibling->prefix_ + (**par).prefix_len_ + 1);
                    if (old_prefix != nullptr) {
                        delete[] old_prefix;
                    }
                    if ((**cur).prefix_ != nullptr) {
                        delete[] (**cur).prefix_;
                    }
                    delete (*cur);
                    if ((**par).prefix_ != nullptr) {
                        delete[] (**par).prefix_;
                    }
                    delete (*par);

                    /* this looks crazy, but I know what I'm doing */
                    *par = static_cast<inner_node*>(sibling);

                } else /* if (n_siblings > 1) */ {
                    /* => delete leaf node
                     *
                     *        |a                         |a
                     *        |                          |
                     *       (aa)        -"aaaaabaa"    (aa)
                     *    a / |  \ b     ==========> a / |
                     *     /  |   \                   /  |
                     *           *()->v1
                     */

                    if ((**cur).prefix_ != nullptr) {
                        delete[] (**cur).prefix_;
                    }
                    delete (*cur);
                    (**par).del_child(cur_partial_key);
                    if ((**par).is_underfull()) {
                        *par = (**par).shrink();
                    }
                }

                return value;
            }

            /* propagate down and repeat */
            cur_partial_key = key[depth + (**cur).prefix_len_];
            depth += (**cur).prefix_len_ + 1;
            par = reinterpret_cast<inner_node**>(cur);
            cur = (**par).find_child(cur_partial_key);
        }
        return T { };
    }

    template <class T>
    Art<T>::iterator Art<T>::begin() {
        return iterator::min(this->root_);
    }

    template <class T>
    Art<T>::iterator Art<T>::begin(const char* key) {
        return iterator::greater_equal(this->root_, key);
    }

    template <class T>
    Art<T>::iterator Art<T>::end() {
        return iterator();
    }

} // namespace fermat
