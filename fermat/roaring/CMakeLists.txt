if(${CMAKE_C_COMPILER_ID} MATCHES "Intel") # icc / icpc
  # prevent shared libraries from depending on Intel provided libraries
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel")
endif()


# we default on a shared library.
if(FERMAT_ROARING_BUILD_STATIC)
  set(FERMAT_ROARING_LIB_TYPE STATIC)
  MESSAGE( STATUS "Building a static library." )
else()
  MESSAGE( STATUS "Building a dynamic library." )
  set(FERMAT_ROARING_LIB_TYPE SHARED)
endif()

MESSAGE( STATUS "FERMAT_ROARING_LIB_TYPE: " ${FERMAT_ROARING_LIB_TYPE})
set(FERMAT_ROARING_SRC
    isadetection.cc
    array_util.cc
    bitset_util.cc
    art/art.cc
    bitset.cc
    containers/array.cc
    containers/bitset.cc
    containers/containers.cc
    containers/convert.cc
    containers/mixed_intersection.cc
    containers/mixed_union.cc
    containers/mixed_equal.cc
    containers/mixed_subset.cc
    containers/mixed_negation.cc
    containers/mixed_xor.cc
    containers/mixed_andnot.cc
    containers/run.cc
    memory.cc
    roaring.cc
    roaring64.cc
    roaring_priority_queue.cc
    roaring_array.cc)

if(FERMAT_ROARING_BUILD_C_AS_CPP)  # more checks and tools, e.g. <type_traits> analysis
  SET_SOURCE_FILES_PROPERTIES(${FERMAT_ROARING_SRC} PROPERTIES LANGUAGE CXX)
endif()

add_library(roaring ${FERMAT_ROARING_LIB_TYPE} ${FERMAT_ROARING_SRC})
if(FERMAT_ROARING_DISABLE_AVX512)
  target_compile_definitions(roaring PUBLIC CFERMAT_ROARING_COMPILER_SUPPORTS_AVX512=0)
endif(FERMAT_ROARING_DISABLE_AVX512)

if(FERMAT_ROARING_DISABLE_AVX)
  target_compile_definitions(roaring PUBLIC FERMAT_ROARING_DISABLE_AVX=1)
endif(FERMAT_ROARING_DISABLE_AVX)

if(FERMAT_ROARING_DISABLE_X64)
  target_compile_definitions(roaring PUBLIC FERMAT_ROARING_DISABLE_X64=1)
endif(FERMAT_ROARING_DISABLE_X64)

if(FERMAT_ROARING_DISABLE_NEON)
  target_compile_definitions(roaring PUBLIC DISABLENEON=1)
endif(FERMAT_ROARING_DISABLE_NEON)

target_link_libraries(roaring PUBLIC "$<BUILD_INTERFACE:roaring-headers>")
target_link_libraries(roaring PUBLIC "$<BUILD_INTERFACE:roaring-headers-cpp>")

target_include_directories(roaring PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#
#install(TARGETS roaring DESTINATION lib)
#
install(TARGETS roaring 
   EXPORT roaring-config
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)

# Set up both the build and install trees as suitable for find_package calls.
# Configuring the build tree as well as the install tree makes it much easier
# to test other libraries against custom builds of CRoaring by pointing
# CMAKE_PREFIX_PATH or equivalent directly at the build directory of CRoaring.
foreach(tree_type BUILD INSTALL)
  if(tree_type STREQUAL "BUILD")
    set(install_location ".")
  else()
    set(install_location "${CMAKE_INSTALL_LIBDIR}/cmake/roaring")
  endif()

  set(build_location "${PROJECT_BINARY_DIR}/${install_location}")
  write_basic_package_version_file(
    "${build_location}/roaring-config-version.cmake"
    VERSION ${FERMAT_ROARING_LIB_VERSION}
    COMPATIBILITY SameMajorVersion)
  configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/config.cmake.in"
                                "${build_location}/roaring-config.cmake"
                                INSTALL_DESTINATION "${install_location}")

  if(tree_type STREQUAL "BUILD")
    export(EXPORT roaring-config
           FILE "${build_location}/roaring-targets.cmake"
           NAMESPACE fermat::roaring::)

  else()
    install(DIRECTORY "${build_location}/" DESTINATION "${install_location}")
    install(EXPORT roaring-config
            DESTINATION "${install_location}"
            FILE "roaring-targets.cmake"
            NAMESPACE fermat::roaring::)
  endif()
endforeach()

if(NOT MSVC)
## We output the library at the root of the current directory where cmake is invoked
## This is handy but Visual Studio will happily ignore us
set_target_properties(roaring PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  VERSION ${FERMAT_ROARING_LIB_VERSION}
  SOVERSION ${FERMAT_ROARING_LIB_SOVERSION})
MESSAGE( STATUS "Library output directory (does not apply to Visual Studio): " ${CMAKE_BINARY_DIR})
endif()

if(MSVC AND (FERMAT_ROARING_LIB_TYPE STREQUAL "SHARED"))
  if (CMAKE_VERSION VERSION_LESS 3.4)
    MESSAGE( STATUS "To build  a Windows DLL using Visual Studio, you may need cmake 3.4 or better." )
  endif()
  MESSAGE( STATUS "Building a Windows DLL using Visual Studio, exporting all symbols automatically." )
 set_target_properties(roaring
    PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif()


if(FERMAT_ROARING_SANITIZE)
  message(STATUS "Enabling address sanitizer")
  target_compile_options(roaring PUBLIC -fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_compile_definitions(roaring PUBLIC ASAN_OPTIONS=detect_leaks=1)
  target_link_libraries(roaring PUBLIC -fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
endif()
if(FERMAT_ROARING_SANITIZE_UNDEFINED)
  message(STATUS "Enabling undefined behavior sanitizer")
  target_compile_options(roaring PUBLIC -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(roaring PUBLIC -fsanitize=undefined)
endif()
if(FERMAT_ROARING_SANITIZE_THREADS)
  message(STATUS "Enabling thread sanitizer")
  target_compile_options(roaring PUBLIC -fsanitize=thread -fno-sanitize-recover=all)
  target_link_libraries(roaring PUBLIC -fsanitize=thread -fno-sanitize-recover=all)
endif()
if(FERMAT_ROARING_SANITIZE_MEMORY)
  if(NOT (CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    message(FATAL_ERROR "FERMAT_ROARING_SANITIZE_MEMORY requires Clang/LLVM (MemorySanitizer). Configure with CC=clang CXX=clang++.")
  endif()
  message(STATUS "Enabling memory sanitizer")
  target_compile_options(roaring PUBLIC -fsanitize=memory -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_libraries(roaring PUBLIC -fsanitize=memory -fno-omit-frame-pointer -fno-sanitize-recover=all)
endif()
