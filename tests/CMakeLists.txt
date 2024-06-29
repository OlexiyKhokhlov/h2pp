project(h2pp_test)

add_executable(${PROJECT_NAME} test_hpack.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} H2PP::h2pp)

add_test(NAME ${PROJECT_NAME} COMMAND ./${PROJECT_NAME})
