#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
set -e

cd $(dirname $0)
cur_dir=$(pwd)
cd -

build_py=$cur_dir/../frame/build.py
$build_py --config=kvprot_cmake_sdk.xml $@
exit $?
