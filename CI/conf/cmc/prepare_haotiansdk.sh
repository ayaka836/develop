#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
set -e

#解压第三方库头文件
# AA-KVCache/CI/conf/cmc/
cur_dir=$(cd "$(dirname "$0")"; pwd)
# ***//AA-KVCache/CI/conf/cmc

top_dir=$(cd "$(dirname "$0")"; cd ../../..; pwd)
# ***//AA-KVCache

AA_KVCache_SDK_PATH=${top_dir}/kvcs_cmpt_build
# ***//AA-KVCache/kvcs_cmpt_build

HAOTIAN_PATH=${top_dir}/haotian
# ***//AA-KVCache/haotian

HAOTIAN_UNTARPATH=${HAOTIAN_PATH}/untar_haotian
# ***//AA-KVCache/haotian/untar_haotian

HAOTIAN_INCLUDE_PATH=${top_dir}/haotian_include
# ***//AA-KVCache/haotian_include


function log() {
    mkdir -p ${top_dir}/logs
    echo "[$(date +%Y-%m-%d\ %H:%M:%S)]$1" | tee -a ${top_dir}/logs/build.log
}

function downloadDependency() {
    rm -rf $HAOTIAN_PATH
    mkdir -p $HAOTIAN_PATH
    artget pull -d ${top_dir}/CI/conf/cmc/Dependency_AA-KVCache_Third.xml \
     -ap ./ \
     -sp ${top_dir}/CI/build/conf/setting.xml \
     -p "{'PRODUCT_TYPE':\"${PRODUCT_TYPE}\",'PKG_TYPE_NAME':\"${PKG_TYPE_NAME}\"}"
     
    if [ $? -ne 0 ]; then
        log "[error] artget pull Dependency_AA-KVCache_Third.xml failed."
        exit 1
    fi

}

function untarHaotianPackage()
{
    cd ${HAOTIAN_PATH}
    # 存在解压目录，先删除
    rm -rf ${HAOTIAN_UNTARPATH}
    mkdir -p ${HAOTIAN_UNTARPATH}

    if ls *.tgz 1> /dev/null 2>&1; then
        haotian_sdk_name=$(ls *.tgz | head -n 1)
        # 解压 .tgz 包到下一级目录
        tar -zxf ${haotian_sdk_name} -C ${HAOTIAN_UNTARPATH}
    else
        log "[error] file ${haotian_sdk_name} not find haotian tgz in ${HAOTIAN_PATH}"
        exit 1
    fi
    cd -
}

function copyHaotianIncludeDir()
{
    cd ${HAOTIAN_UNTARPATH}
    rm -fr ${HAOTIAN_INCLUDE_PATH}
    mkdir -p ${HAOTIAN_INCLUDE_PATH}
    cp -rf aa_include ${HAOTIAN_INCLUDE_PATH}
    cp -rf dataturbo_include ${HAOTIAN_INCLUDE_PATH}
    cp -rf distribute_include ${HAOTIAN_INCLUDE_PATH}
    cp -rf opensourceUpdate ${HAOTIAN_INCLUDE_PATH}
    cd -
}

function untarAndCopyHaotianLib()
{
    cd ${HAOTIAN_UNTARPATH}
    # 定义目标目录为 ${AA_KVCache_SDK_PATH}/third
    target_dir="${AA_KVCache_SDK_PATH}/third"
    # 确保目标目录存在（-p 表示如果父目录不存在也会创建，且目录已存在时不报错）
    mkdir -p ${target_dir}
    
    if ls *.tgz 1> /dev/null 2>&1; then
        # 遍历每个.tgz文件
        tgz_files=$(ls *.tgz 2>/dev/null)
        for file in ${tgz_files}; do
            log "[info] untaring ${file} to ${target_dir}"
            # 直接解压到 third 目录（-C 指定解压路径）
            tar -zxf ${file} -C ${target_dir}
        done
    else
        log "[error] Could not find haotian sdk .tgz file in ${HAOTIAN_UNTARPATH}"
        exit 1
    fi
    cd -
}

function prepareOpenSsl() {
    # 定义目标目录为 ${AA_KVCache_SDK_PATH}/openssl
    target_dir="${AA_KVCache_SDK_PATH}/openssl"
    # 确保目标目录存在（-p 表示如果父目录不存在也会创建，且目录已存在时不报错）
    mkdir -p ${target_dir}
    mkdir -p ${target_dir}/include
    mkdir -p ${target_dir}/lib
    cp -r /usr/include/openssl/* ${target_dir}/include
    cp -P /usr/lib64/libcrypto.so* ${target_dir}/lib
    cp -P /usr/lib64/libssl.so* ${target_dir}/lib
}

log "[info] begin prepare openssl"
prepareOpenSsl

# haotian 侧直接取sdk 不执行此脚本
if [[ ${PLATFORM_TYPE} == "haotian" ]] || [[ ${PLATFORM_TYPE} == "full_build" ]];
then
    log "[info] PLATFORM_TYPE=${PLATFORM_TYPE}, skip prepare haotiansdk."
    exit 0
fi

log "[info] ============================start prepare_haotiansdk.============================"
if [ "${LOCAL_SDK_FLAG}" == "true" ];then
    log "[info] LOCAL_SDK_FLAG=true, need download haotiansdk dependencies."
    downloadDependency
else
    log "[info] LOCAL_SDK_FLAG=false, skip download haotiansdk dependencies."
fi

log "[info] begin untar haotian package"
untarHaotianPackage
log "[info] begin copy haotian include dir"
copyHaotianIncludeDir
log "[info] begin untar haotian sub-package"
untarAndCopyHaotianLib

log "[info] ============================finish prepare haotian sdk.============================"
cd -
exit $?