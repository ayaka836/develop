cmake_minimum_required(VERSION 3.14)

project(kvprot VERSION 1.0.0 LANGUAGES CXX)

add_compile_options(-Wall -Wswitch-default -fno-common)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Build type (default DEBUG)" FORCE)
endif()

set(HAOTIAN_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../haotian_include" CACHE PATH "Haotian include root")
set(HAOTIAN_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../kvcs_cmpt_build" CACHE PATH "Haotian SDK root")

set(KVPROT_HAOTIAN_INCLUDE_DIRS "")
foreach(include_dir
        "${HAOTIAN_INCLUDE_ROOT}/aa_include"
        "${HAOTIAN_INCLUDE_ROOT}/distribute_include"
        "${HAOTIAN_INCLUDE_ROOT}/dataturbo_include")
    if (EXISTS "${include_dir}")
        list(APPEND KVPROT_HAOTIAN_INCLUDE_DIRS "${include_dir}")
    endif()
endforeach()

add_library(kvprot SHARED
    src/kvprot.cpp
)
add_library(kvprot::kvprot ALIAS kvprot)

target_include_directories(kvprot
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (KVPROT_HAOTIAN_INCLUDE_DIRS)
    target_include_directories(kvprot PRIVATE ${KVPROT_HAOTIAN_INCLUDE_DIRS})
endif()

if (EXISTS "${HAOTIAN_SDK_ROOT}/third")
    target_link_directories(kvprot PRIVATE "${HAOTIAN_SDK_ROOT}/third")
endif()

add_executable(kvprot_hello
    examples/hello_kvprot.cpp
)
target_link_libraries(kvprot_hello PRIVATE kvprot)

set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR lib)

include(CMakePackageConfigHelpers)

install(TARGETS kvprot kvprot_hello
    EXPORT kvprot-targets
    RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(DIRECTORY include/ DESTINATION ${INSTALL_INCLUDE_DIR})
install(DIRECTORY examples/ DESTINATION examples/kvprot)
install(EXPORT kvprot-targets NAMESPACE kvprot:: FILE kvprot-targets.cmake DESTINATION ${INSTALL_CONFIG_DIR})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/kvprot-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/kvprot-config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIG_DIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/kvprot-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kvprot-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kvprot-config-version.cmake
    DESTINATION ${INSTALL_CONFIG_DIR}
)
