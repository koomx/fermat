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

kmcmake_cc_benchmark(
        NAME all_benchmark
        MODULE roaring
        SOURCES
        benchmark.cpp
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        DEFINES
        BENCHMARK_DATA_DIR="${PROJECT_SOURCE_DIR}/refs/CRoaring-master/benchmarks/realdata"
        INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        LINKS
        fermat::fermat_static
        ${KMCMAKE_DEPS_LINK}
)
