project(h2pp_test)

add_executable(${PROJECT_NAME}_utils test_utils.cpp)
add_executable(${PROJECT_NAME}_hpack
    test_hpack.cpp
    test_hpack_huffman.cpp
    test_hpack_integer.cpp
    test_hpack_string.cpp
)
add_executable(${PROJECT_NAME}_http2 test_http2.cpp)

target_link_libraries(${PROJECT_NAME}_utils PRIVATE Boost::unit_test_framework H2PP::h2pp)
target_link_libraries(${PROJECT_NAME}_hpack PRIVATE Boost::unit_test_framework H2PP::h2pp)
target_link_libraries(${PROJECT_NAME}_http2 PRIVATE Boost::unit_test_framework H2PP::h2pp)

add_test(NAME ${PROJECT_NAME}_utils COMMAND ./${PROJECT_NAME}_utils)
add_test(NAME ${PROJECT_NAME}_hpack COMMAND ./${PROJECT_NAME}_hpack)
add_test(NAME ${PROJECT_NAME}_http2 COMMAND ./${PROJECT_NAME}_http2)
