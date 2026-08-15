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

set(CLEANUP_TESTS
        common_policy_traits_test
        container_memory_test
        hash_function_defaults_test
        hash_generator_testing
        hash_policy_testing_test
        hash_policy_traits_test
        hashtable_control_bytes_test
        layout_test
        node_slot_policy_test
        raw_hash_set_allocator_test
        raw_hash_set_resize_impl_test
        raw_hash_set_test
        test_instance_tracker_test
        unordered_map_test
        unordered_set_test
)

foreach (case IN LISTS CLEANUP_TESTS)
    kmcmake_cc_test(
            NAME ${case}
            MODULE container
            SOURCES
            ${case}.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            LINKS
            GTest::gtest
            GTest::gmock
            GTest::gtest_main
            turbo::container_testing_static
            turbo::base_testing_static
            turbo::turbo_static
            ${KMCMAKE_DEPS_LINK}
    )
endforeach ()