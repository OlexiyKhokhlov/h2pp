CMAKE_MINIMUM_REQUIRED (VERSION 3.29)

set(HPACK_SOURCES
    hpack/bitstream.cpp
    hpack/bitstream.h
    hpack/constants.h
    hpack/decoder.cpp
    hpack/decoder.h
    hpack/dynamic_table.cpp
    hpack/dynamic_table.h
    hpack/encoder.cpp
    hpack/encoder.h
    hpack/encoder_stream.h
    hpack/encoder_stream.cpp
    hpack/header_field.h
    hpack/header_field.cpp
    hpack/hpack_table.h
    hpack/indexed_dynamic_table.cpp
    hpack/indexed_dynamic_table.h
    hpack/integer.cpp
    hpack/integer.h
    hpack/huffman.cpp
    hpack/huffman.h
    hpack/static_table.cpp
    hpack/static_table.h
    hpack/string.cpp
    hpack/string.h
)

set(SOURCES
    base_client.cpp
    base_client.h
    connection.h
    client_session.h
    method.h
    method.cpp
    dummy_window.h
    dummy_window.cpp
    error.cpp
    error.h
    frame.cpp
    frame.h
    frame_builder.h
    frame_builder.cpp
    protocol.h
    request.cpp
    request.h
    response.cpp
    response.h
    settings_manager.cpp
    settings_manager.h
    stream.cpp
    stream.h
    stream_registry.cpp
    stream_registry.h
)

set(UTILS_SOURCES
    utils/buffer.h
    utils/endianess.h
    utils/streambuf.cpp
    utils/streambuf.h
    utils/utils.h
)

set(PUBLIC_HEADERS
    # utils/buffer.h
    # utils/endianess.h
    # hpack/header_field.h
    method.h
    request.h
    response.h
    connection.h
    base_client.h
    client_session.h
)

add_library(${PROJECT_NAME} STATIC)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(${PROJECT_NAME} PRIVATE
    ${UTILS_SOURCES}
    ${HPACK_SOURCES}
    ${SOURCES}
)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY PUBLIC_HEADER ${PUBLIC_HEADERS}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/h2pp>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
	Threads::Threads 
        Boost::url
        OpenSSL::SSL
        OpenSSL::Crypto
)

add_library(H2PP::h2pp ALIAS ${PROJECT_NAME})

### Install
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION  ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/h2pp
)

# Install a few headers manually since install TARGETS ignores nested dirs
install(FILES
    utils/buffer.h
    utils/endianess.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/h2pp/utils
)
install(FILES
    hpack/header_field.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/h2pp/hpack
)

install(EXPORT ${PROJECT_NAME}
        FILE H2ppTargets.cmake
        NAMESPACE H2PP::
        DESTINATION  ${CMAKE_INSTALL_LIBDIR}/cmake/h2pp
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/H2ppConfig.cmake"
  INSTALL_DESTINATION  ${CMAKE_INSTALL_LIBDIR}/cmake/h2pp
)

write_basic_package_version_file(
    H2ppConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY ExactVersion
)

install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/H2ppConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/H2ppConfigVersion.cmake"
        DESTINATION  ${CMAKE_INSTALL_LIBDIR}/cmake/h2pp
)


