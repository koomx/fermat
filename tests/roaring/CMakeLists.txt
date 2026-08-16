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

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testdata/")
set(BENCHMARK_DATA_DIR "")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)

set(ROARING_TESTS
        add_offset
        array_container_unit
        art_unit
        bitset_container_unit
        cbitset_unit
        container_comparison_unit
        cpp_random_unit
        cpp_roaring64_unit
        cpp_unit
        format_portability_unit
        mixed_container_unit
        refcount_unit
        roaring64_serialization
        roaring64_unit
        robust_deserialization_unit
        run_container_unit
        threads_unit
        toplevel_unit
        util_unit
)

foreach (case IN LISTS ROARING_TESTS)
    kmcmake_cc_test(
            NAME ${case}
            MODULE roaring
            SOURCES
            ${case}.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            DEFINES
            FERMAT_ROARING_EXCEPTIONS=1
            LINKS
            fermat::fermat_static
            GTest::gtest
            GTest::gmock
            GTest::gtest_main
            ${KMCMAKE_DEPS_LINK}
    )
endforeach ()

target_compile_definitions(roaring_refcount_unit PRIVATE
        CFERMAT_ROARING_ATOMIC_IMPL=CFERMAT_ROARING_ATOMIC_IMPL_NONE)
