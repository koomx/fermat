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
        NAME base_testing
        SOURCES
        internal/test_instance_tracker.cc
        #internal/exception_safety_testing.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PLINKS
        GTest::gtest
        GTest::gmock
        ${KMCMAKE_DEPS_LINK}
)

set(ALGO_TESTS
        common_policy_traits_test
        layout_test
)


foreach (case IN LISTS ALGO_TESTS)
    kmcmake_cc_test(
            NAME ${case}
            MODULE base
            SOURCES
            ${case}.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            LINKS
            GTest::gtest
            GTest::gmock
            GTest::gtest_main
            ${KMCMAKE_DEPS_LINK}
    )
endforeach ()

add_subdirectory(internal)
