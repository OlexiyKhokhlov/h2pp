cmake_minimum_required(VERSION 3.29 FATAL_ERROR)
project(client)

if(NOT TARGET H2PP::h2pp)
    # If the case of out of h2pp tree building
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED True)

    find_package(H2pp REQUIRED)
    find_package(Threads REQUIRED)
    find_package(OpenSSL REQUIRED)
    # Required boost::asio, boost::URL
    find_package(Boost 1.84 REQUIRED COMPONENTS url system CONFIG)
endif()

set(SOURCES
  client.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE H2PP::h2pp)
