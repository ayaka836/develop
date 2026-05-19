cmake_minimum_required(VERSION 3.14.1)

project(integrate_kvcs)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message("CMAKE_EXPORT_COMPILE_COMMANDS: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Build type (default DEBUG)" FORCE)
    message(STATUS "No build type selected, default to DEBUG")
endif()

if (NOT COMPILE_TYPE)
    set(COMPILE_TYPE "Real" CACHE STRING "Compile type (default Real)" FORCE)
    message(STATUS "No compile type selected, default to Real")
endif()

if (CPU_TYPE STREQUAL "X86")
    set(CPU_TYPE x86_64)
else ()	
    set(CPU_TYPE arm_64)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "None")
	set(CMAKE_BUILD_TYPE "DEBUG")
endif()

message("build type ${CMAKE_BUILD_TYPE} config type ${CMAKE_CONFIGURATION_TYPES}")
message("install prefix ${CMAKE_INSTALL_PREFIX}")
message("cpu type ${CPU_TYPE}")

if (${CMAKE_INSTALL_PREFIX} STREQUAL /usr/local)
    # 单组件构建，设置默认安装目录
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/build CACHE STRING "install prefix" FORCE)
    set(KVCS_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}")
else()
    # 全量源码构建，安装目录加上组件名称前缀，以区分不同组件的发布件
    set(KVCS_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/)
endif()

if (NOT CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" CACHE STRING "path for include() and find_package(MODULE)" FORCE)
    message(STATUS "No module path selected, default to ${CMAKE_MODULE_PATH}")
else()
    # 组件内部新增CMAKE_MODULE_PATH，按需添加
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
    message(STATUS "CMAKE_MODULE_PATH in tran is ${CMAKE_MODULE_PATH}")
endif()

if (NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/output" CACHE STRING "path for find_package(CONFIG)" FORCE)
    message(STATUS "No prefix path selected, default to ${CMAKE_PREFIX_PATH}")
else()
    # 组件内部新增CMAKE_PREFIX_PATH，按需添加
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/output")
    message(STATUS "CMAKE_PREFIX_PATH in tran is ${CMAKE_PREFIX_PATH}")
endif()

include(ExternalProject)

# 由于不能给ExternalProject BUILD_COMMAND config.sh传入list参数，这里替换分割符规避
string(REPLACE ";" "::" MODUL_LIST "${CMAKE_MODULE_PATH}")
string(REPLACE ";" "::" PREFIX_LIST "${CMAKE_PREFIX_PATH}")

# 是否二进制比对
if (NOT DEFINED BEP_VERIFY)
    set(BEP_VERIFY "false" CACHE STRING "BEP_VERIFY (default false)" FORCE)
endif()

ExternalProject_Add(kvcs_sdk
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/my_pkg_root
    CMAKE_ARGS  -DPRODUCT_NAME=DORADO
                -DSCENE=KVCS
                -DENABLE_GCOV=False
                -DENABLE_ASAN=True
                -DPRODUCT_TYPE=${PRODUCT_TYPE}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCOMPILE_TYPE=${COMPILE_TYPE}	
                -DPKG_TYPE=${PKG_TYPE}
                -DBEP_VERIFY=${BEP_VERIFY}
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} 				
                $<$<STREQUAL:${CPU_TYPE},x86_64>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain_euler_gcc_x86_64.cmake>
                $<$<STREQUAL:${CPU_TYPE},arm_64>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain_euler_arm_aarch64.cmake>
    BUILD_COMMAND $(MAKE)
    BUILD_ALWAYS TRUE
    LIST_SEPARATOR ::
)

ExternalProject_Add(kvprot_sdk
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kvprot
    INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/my_pkg_root
    CMAKE_ARGS  -DPRODUCT_NAME=DORADO
                -DSCENE=KVPROT
                -DENABLE_GCOV=False
                -DENABLE_ASAN=True
                -DPRODUCT_TYPE=${PRODUCT_TYPE}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCOMPILE_TYPE=${COMPILE_TYPE}
                -DPKG_TYPE=${PKG_TYPE}
                -DBEP_VERIFY=${BEP_VERIFY}
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
                -DHAOTIAN_INCLUDE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/haotian_include
                -DHAOTIAN_SDK_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/kvcs_cmpt_build
                $<$<STREQUAL:${CPU_TYPE},x86_64>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain_euler_gcc_x86_64.cmake>
                $<$<STREQUAL:${CPU_TYPE},arm_64>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain_euler_arm_aarch64.cmake>
    BUILD_COMMAND $(MAKE)
    BUILD_ALWAYS TRUE
    LIST_SEPARATOR ::
)

ExternalProject_Add(kvcs_cmpt
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kvcs_cmpt_build/third/
    CMAKE_ARGS  -DPRODUCT_NAME=DORADO
                -DSCENE=KVCS
                -DENABLE_GCOV=False
                -DENABLE_ASAN=True
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCOMPILE_TYPE=${COMPILE_TYPE}
                -DPKG_TYPE=${PKG_TYPE}
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
                $<$<STREQUAL:${CPU_TYPE},x86_64>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain_euler_gcc_x86_64.cmake>
                $<$<STREQUAL:${CPU_TYPE},arm_64>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain_euler_arm_aarch64.cmake>
    BUILD_COMMAND $(MAKE)
    BUILD_ALWAYS TRUE
    LIST_SEPARATOR ::
)
