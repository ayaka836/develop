#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved
# 设置安装路径 并创建
set -e
trap 'echo "grpc_install failed at line $LINENO"; exit 1' ERR

CURRENT_DIR="$(cd "$(dirname "$0")"; pwd)"
ROOT_DIR=${CURRENT_DIR}/../../../..
OPEN_SOURCE_DIR=${ROOT_DIR}/import_include/open_source

GRPC_FILE=grpc-1.60.0
ABSEIL_FILE=abseil-cpp-20230802.1
C_ARES_FILE=c-ares-1.19.1
PROTOBUF_FILE=protobuf-25.1
RE2_FILE=re2-2024-02-01
ZLIB_FILE=zlib-1.2.13

MY_INSTALL_DIR=${OPEN_SOURCE_DIR}/grpc/${GRPC_FILE}

function install_patch() {
    for PATCH_FILE in *.patch; do
        echo "install patch file: ${PATCH_FILE}"
        patch -p1 <  ${PATCH_FILE}
    done
}

function patch_gcc() {
    patch_file=${MY_INSTALL_DIR}/third_party/protobuf/src/google/protobuf/compiler/gcc7_string_view_alloc.cc
    touch ${patch_file}
    echo '#include "absl/strings/string_view.h"' >> ${patch_file}
    echo '#include <memory>' >> ${patch_file}

    echo '// 显式实例化 std::allocator<absl::string_view> 的默认构造/析构' >> ${patch_file}
    echo 'template class std::allocator<absl::string_view>;' >> ${patch_file}

    cmake_file=${MY_INSTALL_DIR}/third_party/protobuf/src/file_lists.cmake
    sed -i '/\${protobuf_SOURCE_DIR}\/src\/google\/protobuf\/compiler\/zip_writer\.cc/a\  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/gcc7_string_view_alloc.cc' ${cmake_file}
}

function download_source() {
    # decompress grpc
    cd ${OPEN_SOURCE_DIR}/grpc
    tar -zxvf ${GRPC_FILE}.tar.gz
    cp -rf ./*.patch ${GRPC_FILE}
    cd ${GRPC_FILE}
    install_patch
    # x86 gcc doesn't support -fstack-protector-strong option, so ignore this patch, we use -fstack-protector-all
    patch -R < add-secure-compile-option-in-Makefile.patch
    # solve the cmake error(looking for pthread.h - not found) for x86
    sed -i '25a\set(CMAKE_THREAD_LIBS_INIT "-lpthread")' CMakeLists.txt
    sed -i '26a\set(CMAKE_HAVE_THREADS_LIBRARY 1)' CMakeLists.txt
    sed -i '27a\set(CMAKE_USE_WIN32_THREADS_INIT 0)' CMakeLists.txt
    sed -i '28a\set(CMAKE_USE_PTHREADS_INIT 1)' CMakeLists.txt
    sed -i '29a\set(THREADS_PREFER_PTHREAD_FLAG ON)' CMakeLists.txt

    # decompress abseil-cpp
    echo "decompress abseil-cpp"
    cd ${OPEN_SOURCE_DIR}/abseil-cpp/
    tar -zxvf ${ABSEIL_FILE}.tar.gz
    cp -rf ./*.patch ${ABSEIL_FILE}
    cd ${ABSEIL_FILE}
    install_patch

    # decompress c-ares
    echo "decompress c-ares"
    cd ${OPEN_SOURCE_DIR}/c-ares
    tar -zxvf ${C_ARES_FILE}.tar.gz
    cp -rf ./*.patch ${C_ARES_FILE}
    cd ${C_ARES_FILE}
    install_patch

    # decompress protobuf
    echo "decompress protobuf"
    cd ${OPEN_SOURCE_DIR}/protobuf
    tar -zxvf protobuf-all-25.1.tar.gz
    cp -rf ./*.patch ${PROTOBUF_FILE}
    cd ${PROTOBUF_FILE}
    install_patch

    # decompress re2
    echo "decompress re2"
    cd ${OPEN_SOURCE_DIR}/re2
    tar -zxvf 2024-02-01.tar.gz
    cp -rf ./*.patch ${RE2_FILE}
    cd ${RE2_FILE}
    install_patch
    # Comment Reason: No such file or directory
    # patch -R < 0001-add-secure-compile-option-in-Makefile.patch
    # patch -R < 0002-add-secure-compile-fs-check-in-Makefile.patch

    # decompress zlib
    echo "decompress zlib"
    cd ${OPEN_SOURCE_DIR}/zlib
    tar -Jxvf ${ZLIB_FILE}.tar.xz
    cp -rf ./*.patch ${ZLIB_FILE}
    cd ${ZLIB_FILE}
    install_patch
}

function replace_third_party() {
    cd ${OPEN_SOURCE_DIR}
    rm -rf grpc/${GRPC_FILE}/third_party/cares/cares/*
    rm -rf grpc/${GRPC_FILE}/third_party/protobuf/*
    rm -rf grpc/${GRPC_FILE}/third_party/abseil-cpp/*
    rm -rf grpc/${GRPC_FILE}/third_party/zlib/*

    mkdir -p grpc/${GRPC_FILE}/third_party/opencensus-proto/src

    cp -rf c-ares/${C_ARES_FILE}/* grpc/${GRPC_FILE}/third_party/cares/cares/
    rm -rf c-ares

    cp -rf abseil-cpp/${ABSEIL_FILE}/* grpc/${GRPC_FILE}/third_party/abseil-cpp/
    rm -rf abseil-cpp

    cp -rf protobuf/${PROTOBUF_FILE}/* grpc/${GRPC_FILE}/third_party/protobuf/
    rm -rf protobuf

    cp -rf re2/${RE2_FILE}/* grpc/${GRPC_FILE}/third_party/re2/
    rm -rf re2

    cp -rf zlib/${ZLIB_FILE}/* grpc/${GRPC_FILE}/third_party/zlib/
    rm -rf zlib

    patch_gcc
}

function grpc_compile_and_install() {
    # 编译安装
    cd ${MY_INSTALL_DIR}
    echo "--- compile grpc ---"
    echo "add_compile_options(-fpermissive)" >> ./CMakeLists.txt
    echo "set(CMAKE_CXX_STANDARD_REQUIRED ON)" >> ./CMakeLists.txt
    echo "set(CMAKE_CXX_EXTENSIONS OFF)" >> ./CMakeLists.txt
    echo "add_compile_options(-fstack-protector-strong)" >> ./CMakeLists.txt
    echo "set(CMAKE_C_FLAGS \"-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack \${CMAKE_C_FLAGS}\")" >> ./CMakeLists.txt
    echo "set(CMAKE_CXX_FLAGS \"-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack \${CMAKE_CXX_FLAGS}\")" >> ./CMakeLists.txt
    
    if [ "${BISHENG_COMPILE}" == "yes" ]; then
        sed -i '/^cmake_minimum_required/a\set(CMAKE_C_COMPILER clang)\nset(CMAKE_CXX_COMPILER clang++)\nset(CMAKE_AR llvm-ar)\nset(CMAKE_RANLIB llvm-ranlib)\nset(CMAKE_STRIP llvm-strip)' ./CMakeLists.txt
        echo "set(CMAKE_C_FLAGS \"-D__ARM_ARCH_8A=1 \${CMAKE_C_FLAGS}\")" >> ./CMakeLists.txt
        echo "set(CMAKE_CXX_FLAGS \"-D__ARM_ARCH_8A=1 \${CMAKE_CXX_FLAGS}\")" >> ./CMakeLists.txt
    fi

    rm -rf cmake/build
    mkdir -p cmake/build
    pushd cmake/build

    ARCH=$(uname -m)
    echo "ARCH:" $ARCH
    if [[ "$ARCH" == "armv7l" ]] || [[ "$ARCH" == "aarch64" ]]; then
        echo "current env: arm"
        cmake ../../ -DgRPC_INSTALL=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${MY_INSTALL_DIR} \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_CXX_FLAGS="-fstack-protector-all -D_FORTIFY_SOURCE=2 -O2 -ftrapv -Wl,-z,now -s" \
            -DgRPC_ZLIB_PROVIDER=package \
            -DgRPC_SSL_PROVIDER=package \
            -DOPENSSL_VERSION="1.1.1f" \
            -DOPENSSL_SSL_LIBRARY=${ROOT_DIR}/kvcs_cmpt_build/openssl/lib/libssl.so.1.1 \
            -DOPENSSL_CRYPTO_LIBRARY=${ROOT_DIR}/kvcs_cmpt_build/openssl/lib/libcrypto.so.1.1 \
            -DOPENSSL_INCLUDE_DIR=${ROOT_DIR}/kvcs_cmpt_build/openssl/include/
    else
        echo "current env: x86"
        cmake ../../ -DgRPC_INSTALL=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${MY_INSTALL_DIR} \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_CXX_STANDARD=14 \
            -DCMAKE_CXX_FLAGS="-fstack-protector-all -D_FORTIFY_SOURCE=2 -O2 -ftrapv -Wl,-z,now -s" \
            -DCMAKE_C_COMPILER=/opt/buildtools/gcc8.5.0/bin/gcc \
            -DCMAKE_CXX_COMPILER=/opt/buildtools/gcc8.5.0/bin/g++ \
            -DgRPC_ZLIB_PROVIDER=package \
            -DgRPC_SSL_PROVIDER=package \
            -DOPENSSL_VERSION="1.0.2k" \
            -DOPENSSL_SSL_LIBRARY=/usr/lib64/libssl.so.1.0.2k\
            -DOPENSSL_CRYPTO_LIBRARY=/usr/lib64/libcrypto.so.1.0.2k \
            -DOPENSSL_INCLUDE_DIR=/usr/include/openssl/ \
            -DgRPC_SYSTEMD_FOUND=FALSE \
            -DHAVE_LIBSYSTEMD=FALSE
    fi

    make -j32
    make install
    popd
}

function make_package() {
    mkdir -p ${ROOT_DIR}/build/my_pkg_root/lib/kvcs_open_source/grpc/
    cp -r ${MY_INSTALL_DIR}/lib ${ROOT_DIR}/build/my_pkg_root/lib/kvcs_open_source/grpc
    cp -r ${MY_INSTALL_DIR}/lib64 ${ROOT_DIR}/build/my_pkg_root/lib/kvcs_open_source/grpc
}

function remove_rpath() {
    patchelf --remove-rpath ${ROOT_DIR}/build/my_pkg_root/lib/kvcs_open_source/grpc/lib64/libprotobuf.so
    patchelf --remove-rpath ${ROOT_DIR}/build/my_pkg_root/lib/kvcs_open_source/grpc/lib64/libprotoc.so
    patchelf --remove-rpath ${ROOT_DIR}/build/my_pkg_root/lib/kvcs_open_source/grpc/lib64/libprotobuf-lite.so
}

if [ -d "${MY_INSTALL_DIR}/cmake/build" ];then
    make_package
    echo "Grpc has been bulid."
    exit 0
fi

LD_LIBRARY_PATH_OLD=$(echo "${LD_LIBRARY_PATH}")
if [ -z "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH=${OPEN_SOURCE_DIR}/grpc/${GRPC_FILE}/cmake/build:${OPEN_SOURCE_DIR}/grpc/${GRPC_FILE}/cmake/build/third_party/protobuf
else
    export LD_LIBRARY_PATH=${OPEN_SOURCE_DIR}/grpc/${GRPC_FILE}/cmake/build:${OPEN_SOURCE_DIR}/grpc/${GRPC_FILE}/cmake/build/third_party/protobuf:${LD_LIBRARY_PATH}
fi
download_source
replace_third_party
grpc_compile_and_install
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_OLD}

make_package
remove_rpath
echo "Grpc bulid successed."
