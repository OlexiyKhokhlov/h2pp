cmake_minimum_required(VERSION 3.29 FATAL_ERROR)
project(h2pp
    VERSION 0.0.1
    DESCRIPTION "HTTP/2 C++20 client library"
    HOMEPAGE_URL "https://github.com/OlexiyKhokhlov/h2pp"
    LANGUAGES CXX
)

option(USE_TEST "Enable/Disable tests building" true)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wc++20-compat")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -flto=auto")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto=auto")
endif()

message(STATUS "System: " ${CMAKE_SYSTEM_NAME} " " ${CMAKE_SYSTEM_VERSION})
message(STATUS "Processor: " ${CMAKE_HOST_SYSTEM_PROCESSOR})
message(STATUS "CMake generates: " ${CMAKE_GENERATOR})
message(STATUS "C++ Standard: " ${CMAKE_CXX_STANDARD})
message(STATUS "Build type:" ${CMAKE_BUILD_TYPE})

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
# Required boost::asio, boost::URL
find_package(Boost 1.84 REQUIRED COMPONENTS url system unit_test_framework CONFIG)

add_subdirectory(lib)
add_subdirectory(examples)

if(USE_TEST AND ${USE_TEST})
    enable_testing()
    add_subdirectory(tests)
endif()

