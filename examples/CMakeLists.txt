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

set(ROARING_EXAMPLES
        c_example1
        c_readme_example
        cpp_example1
        cpp_example2
        cpp_readme_example
)

foreach (case IN LISTS ROARING_EXAMPLES)
    kmcmake_cc_binary(
            NAME ${case}
            SOURCES
            roaring/${case}.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            LINKS
            fermat::fermat_static
            ${KMCMAKE_DEPS_LINK}
    )
endforeach ()
