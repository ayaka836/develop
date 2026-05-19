cmake_minimum_required(VERSION 3.14)

project(kvcs)

add_compile_options(-Wall -Wswitch-default -fno-common)
add_definitions("-Wall -g")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_EXTENSIONS ON)

set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CTRL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OCEANSTOROS_INC_DIR ${BASE_DIR}/import_include/dorado)
set(HAOTIAN_INC_DIR ${BASE_DIR}/haotian_include/aa_include)
set(DISTRIBUTE_INC_DIR ${BASE_DIR}/haotian_include/distribute_include)
set(FUSION_INC_DIR ${BASE_DIR}/import_include/fusion)
set(EXPORT_INC_DIR ${BASE_DIR}/export_include)

include(${BASE_DIR}/cmake/intf_lib.cmake)
include(${BASE_DIR}/cmake/intf_kvcs_lib.cmake)

if (${BEP_VERIFY} STREQUAL "true")
    set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1) 
    set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
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
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> rcsD <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> rcsD <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> rcsD <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> rcsD <TARGET> <LINK_FLAGS> <OBJECTS>")
endif()

message("cmake source dir is " ${CMAKE_SOURCE_DIR}) 
message("project source dir is " ${PROJECT_SOURCE_DIR})
message("cmake binary dir is " ${CMAKE_BINARY_DIR})
message("base dir is " ${BASE_DIR})
message("ctrl dir is " ${CTRL_DIR})
message("os include dir is " ${OCEANSTOROS_INC_DIR})
message("haotian include dir is " ${HAOTIAN_INC_DIR})
message("fusion include dir is " ${FUSION_INC_DIR})
message("product type is " ${PRODUCT_TYPE})
message(STATUS "cpu type " ${CPU_TYPE})
message(STATUS "build type " ${CMAKE_BUILD_TYPE})
message(STATUS "complie type " ${COMPILE_TYPE})
message(STATUS "product type " ${PRODUCT_TYPE})

add_subdirectory(common)
add_subdirectory(include)
add_subdirectory(core)
add_subdirectory(ctrl_flow)
add_subdirectory(kvcs_control)
add_subdirectory(store_mgt)
add_subdirectory(kvcs_biz)
add_subdirectory(kvcs_perf)

set(KVCS_LIBS
    kvcs
)

message("strip type " ${CMAKE_C_FLAGS})

add_custom_target(KVCS_TARGETS 
	DEPENDS ${KVCS_LIBS}
)

set(INSTALL_TARGETS ${KVCS_LIBS})
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_ARCHIVE_DIR bin/link)
set(INSTALL_OBJECT_DIR  lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR  lib)

include(CMakePackageConfigHelpers)
install(TARGETS 
    ${INSTALL_TARGETS} 
    EXPORT kvcs-targets 
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
	LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
	ARCHIVE DESTINATION ${INSTALL_ARCHIVE_DIR}
)

install(EXPORT kvcs-targets NAMESPACE kvcs:: FILE kvcs-targets.cmake DESTINATION ${INSTALL_CONFIG_DIR})
configure_package_config_file(${BASE_DIR}/cmake/kvcs-config.cmake.in 
    ${CMAKE_CURRENT_BINARY_DIR}/kvcs-config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIG_DIR}
    PATH_VARS INSTALL_BASE_DIR INSTALL_INCLUDE_DIR INSTALL_LIBRARY_DIR INSTALL_ARCHIVE_DIR INSTALL_OBJECT_DIR INSTALL_RUNTIME_DIR INSTALL_CONFIG_DIR
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
)

# 安装包配置文件    
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kvcs-config.cmake DESTINATION ${INSTALL_CONFIG_DIR})
install(FILES  	
		DESTINATION conf
)

install(PROGRAMS  #脚本
		DESTINATION script
)
