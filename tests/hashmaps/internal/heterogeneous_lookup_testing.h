// Copyright 2025 The Abseil Authors.
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

#include <cstddef>
#include <ostream>

#include <tests/base/internal/test_instance_tracker.h>
#include <gmock/gmock.h>
#include <turbo/macros/config.h>

namespace fermat {

    namespace container_internal {

        // An expensive class that is convertible to CheapType to demonstrate
        // heterogeneous lookups.
        class ExpensiveType : public fermat::test_internal::CopyableMovableInstance {
        public:
            explicit ExpensiveType(int value)
                : fermat::test_internal::CopyableMovableInstance(value) { }

            friend std::ostream& operator<<(std::ostream& os, const ExpensiveType& a) {
                return os << a.value();
            }
        };

        class CheapType {
        public:
            explicit CheapType(const int value)
                : value_(value) { }

            explicit operator ExpensiveType() const { return ExpensiveType(value_); }

            int value() const { return value_; }

        private:
            int value_;
        };

        struct HeterogeneousHash {
            using is_transparent = void;
            size_t operator()(const ExpensiveType& a) const { return a.value(); }
            size_t operator()(const CheapType& a) const { return a.value(); }
        };

        struct HeterogeneousEqual {
            using is_transparent = void;
            bool operator()(const ExpensiveType& a, const ExpensiveType& b) const {
                return a.value() == b.value();
            }
            bool operator()(const ExpensiveType& a, const CheapType& b) const {
                return a.value() == b.value();
            }
            bool operator()(const CheapType& a, const ExpensiveType& b) const {
                return a.value() == b.value();
            }
        };

        MATCHER_P(HasExpensiveValue, n, "") {
            return ::testing::ExplainMatchResult(n, arg.value(), result_listener);
        }

    } // namespace container_internal

} // namespace tests

