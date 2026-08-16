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

# Default exported target (install(EXPORT) requires at least one).
# Teaching demos (foo / xxd / binaries) live in CMakeLists.examples.txt
# and are installed only when generating with -DKMCMAKE_GEN_EXAMPLES=ON.

kmcmake_cc_library(
        NAMESPACE ${PROJECT_NAME}
        NAME fermat
        SOURCES
        hashmaps/internal/raw_hash_set.cc
        roaring/isadetection.cc
        roaring/array_util.cc
        roaring/bitset_util.cc
        roaring/art/art.cc
        roaring/bitset.cc
        roaring/containers/array.cc
        roaring/containers/bitset.cc
        roaring/containers/containers.cc
        roaring/containers/convert.cc
        roaring/containers/mixed_intersection.cc
        roaring/containers/mixed_union.cc
        roaring/containers/mixed_equal.cc
        roaring/containers/mixed_subset.cc
        roaring/containers/mixed_negation.cc
        roaring/containers/mixed_xor.cc
        roaring/containers/mixed_andnot.cc
        roaring/containers/run.cc
        roaring/memory.cc
        roaring/roaring.cc
        roaring/roaring64.cc
        roaring/roaring_priority_queue.cc
        roaring/roaring_array.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PLINKS
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)

kmcmake_cc_interface(
        NAMESPACE ${PROJECT_NAME}
        NAME skills
        HEADERS
        skills.h
        version.h
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PUBLIC
)
