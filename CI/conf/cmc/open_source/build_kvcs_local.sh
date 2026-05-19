#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2024-2024. All rights reserved
set -e
cd $(dirname $0)
CUR_DIR=$(pwd)
ROOT_DIR=${CUR_DIR}/../../../..
WORKSPACE_DIR=${ROOT_DIR}/..

function log() {
    mkdir -p ${ROOT_DIR}/logs
    echo "[$(date +%Y-%m-%d\ %H:%M:%S)]$1" | tee -a ${ROOT_DIR}/logs/build.log
}

function downloadOpensource()
{
    if [ -d ${ROOT_DIR}/import_include/open_source/grpc ];then
        log "[info] open source code is ready."
        return 0
    fi
    TEMP_PATH=${ROOT_DIR}/temp_OpenSourceDown
    rm -rf ${TEMP_PATH} && mkdir -p ${TEMP_PATH}
    cd ${TEMP_PATH}
    git init .
    git config user.email "manifest_AA-KVCache_OpenSourceDown"
    git config user.name "manifest_AA-KVCache_OpenSourceDown"
    cp -rf ${ROOT_DIR}/CI/conf/cmc/manifest_AA-KVCache_OpenSourceDown.xml ./
    git add manifest_AA-KVCache_OpenSourceDown.xml
    git commit -m init
    git mm init -u ${TEMP_PATH} -m manifest_AA-KVCache_OpenSourceDown.xml
    git mm sync --depth 1 -j8
    cp -fr AA-KVCache/import_include ${ROOT_DIR}
    rm -rf ${TEMP_PATH}
}

log "[info] ============================begin build kvcs local.============================"
log "[info] begin download opensource"
downloadOpensource
log "[info] ============================finish build kvcs local.============================"
