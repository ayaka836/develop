#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2010-2018. All rights reserved.
set -e

cd $(dirname $0)
cur_dir=$(pwd)
cd -

build_py=$cur_dir/../frame/build.py
$build_py --config=kvcs_cmake_cmpt.xml $@
exit $?