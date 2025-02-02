cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER "clang-18")
set(CMAKE_CXX_COMPILER "clang++-18")

project(Woeful)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-error=unused-parameter -std=c++20")
if(PCAP STREQUAL "OH_YEAH")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPCAP")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
if(DOUBLE_DEBUG STREQUAL "OH_YEAH")
  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG2")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto -funroll-loops")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# cpack shit
set(CPACK_DEBIAN_PACKAGE_DEPENDS zlib1g)
set(CPACK_PACKAGE_CONTACT foxmoss@mediaology.com)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER FoxMoss)
set(CPACK_PACKAGE_NAME "woeful")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")

include(FetchContent)

FetchContent_Declare(
    expected 
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
)
FetchContent_MakeAvailable(expected)


FetchContent_Declare(
    tinyxml2 
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG master 
)

FetchContent_MakeAvailable(tinyxml2)

include_directories(${expected_SOURCE_DIR}/include/)

add_subdirectory(src)
add_subdirectory(tests)
