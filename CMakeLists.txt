cmake_minimum_required(VERSION 3.12)

project(router VERSION 0.1 LANGUAGES CXX)

option(ROUTER_BUILD_EXAMPLES "Enable examples build" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(contrib)

add_library(router INTERFACE)

target_compile_features(router INTERFACE cxx_std_20)

target_include_directories(router INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(router INTERFACE tl::expected)

add_library(elsid::router ALIAS router)

install(TARGETS router EXPORT router_targets
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    RUNTIME DESTINATION bin COMPONENT Runtime
    PUBLIC_HEADER DESTINATION include COMPONENT Development
    BUNDLE DESTINATION bin COMPONENT Runtime
)

install(
    DIRECTORY   include/router
    DESTINATION include
)

install(
    TARGETS     router
    EXPORT      router-targets
    DESTINATION lib
)

install(
    EXPORT      router-targets
    NAMESPACE   elsid::
    DESTINATION lib/cmake/router
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/router/cmake/router_config_version.cmake"
    COMPATIBILITY AnyNewerVersion
)

export(EXPORT router_targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/router/router_targets.cmake"
    NAMESPACE elsid::
)

configure_file(cmake/router_config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/router/cmake/router_config.cmake"
    COPYONLY
)

set(ConfigPackageLocation lib/cmake/router)

install(EXPORT router_targets
    FILE router_targets.cmake
    NAMESPACE elsid::
    DESTINATION ${ConfigPackageLocation}
)

install(FILES cmake/router_config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/router/cmake/router_config_version.cmake"
    DESTINATION ${ConfigPackageLocation}
    COMPONENT Devel
)

if(ROUTER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
