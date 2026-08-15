# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

kmcmake_cc_library(
        NAMESPACE ${PROJECT_NAME}
        NAME hashmaps_testing
        SOURCES
        internal/hash_generator_testing.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PLINKS
        GTest::gtest
        GTest::gmock
        fermat::fermat_static
        ${KMCMAKE_DEPS_LINK}
)

set(HASHMAPS_TESTS
        flat_hash_set_test
        flat_hash_map_test
        linked_hash_map_test
        linked_hash_set_test
        node_hash_map_test
        node_hash_set_test
)

foreach (case IN LISTS HASHMAPS_TESTS)
    kmcmake_cc_test(
            NAME ${case}
            MODULE hashmaps
            SOURCES
            ${case}.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            LINKS
            fermat::hashmaps_testing_static
            fermat::base_testing_static
            GTest::gtest
            GTest::gmock
            GTest::gtest_main
            ${KMCMAKE_DEPS_LINK}
    )
endforeach ()

add_subdirectory(internal)
