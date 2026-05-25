cmake_minimum_required(VERSION 3.14)

project(kvprot VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Build type (default DEBUG)" FORCE)
endif()

add_compile_options(-Wall -Wswitch-default -fno-common)

set(KVPROT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(KVPROT_SRC_DIR ${KVPROT_ROOT_DIR}/src)

set(HAOTIAN_INCLUDE_ROOT "${KVPROT_ROOT_DIR}/../haotian_include" CACHE PATH "Haotian include root")
set(HAOTIAN_SDK_ROOT "${KVPROT_ROOT_DIR}/../kvcs_cmpt_build" CACHE PATH "Haotian SDK root")
set(IMPORT_INCLUDE_ROOT "${KVPROT_ROOT_DIR}/../import_include" CACHE PATH "Import include root")

set(KVPROT_SOURCES
    ${KVPROT_SRC_DIR}/cmd/kvprot_cmd.c
    ${KVPROT_SRC_DIR}/cmd/kvprot_delete.c
    ${KVPROT_SRC_DIR}/common/kvprot_common.c
    ${KVPROT_SRC_DIR}/init/kvprot_init.c
    ${KVPROT_SRC_DIR}/scat/kvprot_scat.c
)

set(KVPROT_HAOTIAN_INCLUDE_DIRS "")
set(KVPROT_HAOTIAN_INCLUDE_ROOTS
    "${HAOTIAN_INCLUDE_ROOT}/aa_include"
    "${HAOTIAN_INCLUDE_ROOT}/distribute_include"
    "${HAOTIAN_INCLUDE_ROOT}/dataturbo_include"
    "${HAOTIAN_INCLUDE_ROOT}/opensourceUpdate"
    "${IMPORT_INCLUDE_ROOT}/dorado")

function(kvprot_create_include_map out_var duplicate_file)
    set(include_map_dir "${CMAKE_CURRENT_BINARY_DIR}/kvprot_include_map")
    file(REMOVE_RECURSE "${include_map_dir}")
    file(MAKE_DIRECTORY "${include_map_dir}")
    file(WRITE "${duplicate_file}" "")

    set(mapped_headers "")
    foreach(root_dir IN LISTS ARGN)
        if (EXISTS "${root_dir}")
            file(GLOB_RECURSE header_files CONFIGURE_DEPENDS
                "${root_dir}/*.h"
                "${root_dir}/*.hh"
                "${root_dir}/*.hpp"
                "${root_dir}/*.hxx")
            list(SORT header_files)
            foreach(header_file IN LISTS header_files)
                get_filename_component(header_name "${header_file}" NAME)
                set(mapped_header "${include_map_dir}/${header_name}")
                if (NOT header_name IN_LIST mapped_headers)
                    file(CREATE_LINK "${header_file}" "${mapped_header}" SYMBOLIC)
                    list(APPEND mapped_headers "${header_name}")
                else()
                    file(APPEND "${duplicate_file}" "${header_name} -> ${header_file}\n")
                endif()
            endforeach()
        endif()
    endforeach()

    set(${out_var} "${include_map_dir}" PARENT_SCOPE)
endfunction()

foreach(include_dir
        "${HAOTIAN_INCLUDE_ROOT}/aa_include"
        "${HAOTIAN_INCLUDE_ROOT}/aa_include/infrastructure"
        "${HAOTIAN_INCLUDE_ROOT}/aa_include/infrastructure/lvos"
        "${HAOTIAN_INCLUDE_ROOT}/aa_include/infrastructure/open_source"
        "${HAOTIAN_INCLUDE_ROOT}/distribute_include"
        "${HAOTIAN_INCLUDE_ROOT}/dataturbo_include"
        "${HAOTIAN_INCLUDE_ROOT}/opensourceUpdate"
        "${IMPORT_INCLUDE_ROOT}/dorado")
    if (EXISTS "${include_dir}")
        list(APPEND KVPROT_HAOTIAN_INCLUDE_DIRS "${include_dir}")
    endif()
endforeach()

kvprot_create_include_map(
    KVPROT_INCLUDE_MAP_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/kvprot_include_map_duplicates.txt"
    ${KVPROT_HAOTIAN_INCLUDE_ROOTS})

list(REMOVE_DUPLICATES KVPROT_HAOTIAN_INCLUDE_DIRS)

find_package(Threads REQUIRED)

if ("${BEP_VERIFY}" STREQUAL "true")
    set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
    add_compile_definitions(
        __FILE__="redacted"
        __DATE__="redacted"
        __TIMESTAMP__="redacted"
        __TIME__="redacted"
    )
    add_compile_options(
        -Wno-builtin-macro-redefined
    )
    set(CMAKE_RANLIB "echo")
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> rcsD <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> rcsD <TARGET> <LINK_FLAGS> <OBJECTS>")
endif()

message(STATUS "kvprot source dir: ${KVPROT_ROOT_DIR}")
message(STATUS "kvprot haotian include root: ${HAOTIAN_INCLUDE_ROOT}")
message(STATUS "kvprot haotian sdk root: ${HAOTIAN_SDK_ROOT}")
message(STATUS "kvprot import include root: ${IMPORT_INCLUDE_ROOT}")
message(STATUS "kvprot build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "kvprot compile type: ${COMPILE_TYPE}")
message(STATUS "kvprot product type: ${PRODUCT_TYPE}")

add_library(kvprot SHARED ${KVPROT_SOURCES})
add_library(kvprot::kvprot ALIAS kvprot)
set_source_files_properties(${KVPROT_SOURCES} PROPERTIES LANGUAGE CXX)
set_target_properties(kvprot PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(kvprot
    PUBLIC
        $<BUILD_INTERFACE:${KVPROT_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (KVPROT_HAOTIAN_INCLUDE_DIRS)
    target_include_directories(kvprot PRIVATE ${KVPROT_HAOTIAN_INCLUDE_DIRS})
endif()

target_compile_definitions(kvprot PRIVATE
    MY_PID=PID_TGT_MIDDLE
    CPU_PART_NAME_NORMAL=\"IOD_NORMAL\")

target_compile_options(kvprot PRIVATE -idirafter "${KVPROT_INCLUDE_MAP_DIR}")

if (EXISTS "${HAOTIAN_SDK_ROOT}/third")
    target_link_directories(kvprot PRIVATE "${HAOTIAN_SDK_ROOT}/third")
endif()

target_link_libraries(kvprot PRIVATE Threads::Threads)

set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_ARCHIVE_DIR bin/link)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR lib)

include(CMakePackageConfigHelpers)

install(TARGETS kvprot
    EXPORT kvprot-targets
    RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_ARCHIVE_DIR}
)

install(DIRECTORY include/ DESTINATION ${INSTALL_INCLUDE_DIR})

install(EXPORT kvprot-targets
    NAMESPACE kvprot::
    FILE kvprot-targets.cmake
    DESTINATION ${INSTALL_CONFIG_DIR}
)

configure_package_config_file(
    ${KVPROT_ROOT_DIR}/cmake/kvprot-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/kvprot-config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIG_DIR}
    PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LIBRARY_DIR INSTALL_ARCHIVE_DIR INSTALL_RUNTIME_DIR INSTALL_CONFIG_DIR
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
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
