#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
KVPROT_LLT_ROOT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
KVPROT_ROOT_DIR=$(cd "${KVPROT_LLT_ROOT_DIR}/.." && pwd)
TOP_DIR=$(cd "${KVPROT_ROOT_DIR}/.." && pwd)

LLT_EXE_NAME="kvprot_llt"
LLT_EXE_PATH="${KVPROT_LLT_ROOT_DIR}/${LLT_EXE_NAME}"
TEST_LCOV="${KVPROT_LLT_ROOT_DIR}/coverage"
TEST_LCOV_RESULT="${TEST_LCOV}/result_lcov"
G_TESTCASE_OUTPUT="${TEST_LCOV}/test_detail.xml"
LCOV_OUTPUT_PATH="${KVPROT_LLT_ROOT_DIR}/lcov_output"
LCOV_HTML_OUTPUT_PATH="${KVPROT_LLT_ROOT_DIR}/lcov_html_output"
LCOV_SRC_DIR="${KVPROT_ROOT_DIR}/src"

G_RET=0
COVERAGE_FLAG=0
GDB_FLAG=0
G_TEST_INCLUDE="*"
G_TEST_EXCLUDE=""

function usage()
{
    echo "Usage:"
    echo "-r : run llt param."
    echo "     [debug]: run kvprot llt normal;"
    echo "     [gdb]: run kvprot llt with gdb;"
    echo "     [cov]: run kvprot llt and stat coverage."
    echo "-f : gtest filter, for example 'KvprotDtTest.Parse*'."
}

function make_testcase()
{
    cd "${KVPROT_LLT_ROOT_DIR}"
    echo "begin to excute kvprot llt make"
    if [ "${COVERAGE_FLAG}" -eq 1 ]; then
        make clean_kvprot_llt
        make kvprot_llt KVPROT_LLT_COVERAGE=1
    else
        make kvprot_llt
    fi
    cd -
}

function exec_testcase()
{
    cd "${KVPROT_LLT_ROOT_DIR}"

    local ret=0
    if [ ! -f "${LLT_EXE_PATH}" ]; then
        echo "EXECUTE ERROR:${LLT_EXE_PATH} not exist"
        return 1
    fi

    mkdir -p "${TEST_LCOV_RESULT}"
    local g_test_filter="${G_TEST_INCLUDE}-${G_TEST_EXCLUDE}"
    local llt_output="${TEST_LCOV_RESULT}/${LLT_EXE_NAME}.xml"
    local llt_cmd="${LLT_EXE_PATH} --gtest_filter=${g_test_filter} --gtest_output=xml:${llt_output}"

    echo "----------------${llt_cmd}-------------------"
    if [ "${GDB_FLAG}" -eq 1 ]; then
        gdb "${LLT_EXE_PATH}"
    else
        "${LLT_EXE_PATH}" --gtest_filter="${g_test_filter}" --gtest_output="xml:${llt_output}"
    fi

    if [ ! -f "${llt_output}" ]; then
        echo "ERRO:${llt_output} not exist"
        ret=1
    fi

    cd -
    return ${ret}
}

function collect_testcase_output()
{
    cd "${KVPROT_LLT_ROOT_DIR}"
    echo "--------------G_TESTCASE_OUTPUT:${G_TESTCASE_OUTPUT}-------------"
    echo "--------------TEST_LCOV:${TEST_LCOV}-------------"

    local xml_count
    xml_count=$(find "${TEST_LCOV}" -type f -name "*.xml" | wc -l)
    echo "--------------xml_count:${xml_count}-------------"

    if [ "${xml_count}" -gt 1 ] && [ -f "${KVPROT_LLT_ROOT_DIR}/gtest_collect_output.py" ]; then
        python "${KVPROT_LLT_ROOT_DIR}/gtest_collect_output.py" "${TEST_LCOV_RESULT}" "${G_TESTCASE_OUTPUT}"
    elif [ "${xml_count}" -eq 1 ]; then
        cp -af "${TEST_LCOV_RESULT}"/*.xml "${G_TESTCASE_OUTPUT}"
    elif [ "${xml_count}" -gt 1 ]; then
        local first_xml
        first_xml=$(find "${TEST_LCOV_RESULT}" -type f -name "*.xml" | sort | head -n 1)
        cp -af "${first_xml}" "${G_TESTCASE_OUTPUT}"
    else
        echo "ERROR:Report is null"
        G_RET=1
    fi

    if [ -f "${G_TESTCASE_OUTPUT}" ]; then
        sed -i -r '/^[ ]*$/d' "${G_TESTCASE_OUTPUT}"
        cp -af "${G_TESTCASE_OUTPUT}" "${KVPROT_LLT_ROOT_DIR}/test_detail.xml"
    fi
    cd -
}

function stat_coverage()
{
    local enable_branch="--rc lcov_branch_coverage=1"
    local last_path
    last_path=$(pwd)

    if ! command -v lcov >/dev/null 2>&1 || ! command -v genhtml >/dev/null 2>&1; then
        echo "ERROR: lcov/genhtml not found"
        return 1
    fi

    echo "gen output info"
    cd "${TEST_LCOV}"
    rm -rf "${TEST_LCOV}"/*.info

    lcov -d "${KVPROT_LLT_ROOT_DIR}/build/kvprot_llt_cov" \
        -b "${TOP_DIR}" \
        -c -o "${TEST_LCOV}/${LLT_EXE_NAME}.info" ${enable_branch}

    lcov -e "${TEST_LCOV}/${LLT_EXE_NAME}.info" \
        "*/kvprot/src/*" \
        "${LCOV_SRC_DIR}/*" \
        -o "${TEST_LCOV}/output.info" ${enable_branch}

    lcov -r "${TEST_LCOV}/output.info" "*.h" "*/third_party/*" "*/testcase/*" \
        -o "${TEST_LCOV}/output.filtered.info" ${enable_branch}
    mv "${TEST_LCOV}/output.filtered.info" "${TEST_LCOV}/output.info"

    echo "start genhtml"
    genhtml --branch-coverage --demangle-cpp "${TEST_LCOV}/output.info" -o "${TEST_LCOV}"
    echo "end genhtml"

    cd "${last_path}"
}

if [ "$#" -gt 0 ] && [[ "$1" != -* ]]; then
    case "$1" in
        cov|debug|gdb)
            set -- -r "$1" "${@:2}"
            ;;
        *)
            set -- -f "$1" "${@:2}"
            ;;
    esac
fi

while getopts r:f:h opts; do
    case ${opts} in
        r)
            if [ "${OPTARG}" = "debug" ]; then
                :
            elif [ "${OPTARG}" = "gdb" ]; then
                GDB_FLAG=1
            elif [ "${OPTARG}" = "cov" ]; then
                COVERAGE_FLAG=1
            else
                echo "Invalid para of -r, default execute llt case"
            fi
            ;;
        f)
            G_TEST_INCLUDE="${OPTARG}"
            echo "G_TEST_INCLUDE: ${G_TEST_INCLUDE}"
            ;;
        h)
            usage
            exit 0
            ;;
        ?)
            echo "invalid para"
            exit 1
            ;;
    esac
done

mkdir -p "${TEST_LCOV}" "${TEST_LCOV_RESULT}" "${LCOV_OUTPUT_PATH}" "${LCOV_HTML_OUTPUT_PATH}"

start=$(date +%s)

echo -e "\n Compile kvprot llt code >>>>>> \n"
make_testcase

if [ "${COVERAGE_FLAG}" -eq 1 ]; then
    find "${KVPROT_LLT_ROOT_DIR}/build/kvprot_llt_cov" -name "*.gcda" -delete
fi

echo -e "\n Execute kvprot llt test case >>>>>> \n"
if ! exec_testcase; then
    echo "Execute kvprot llt case failed"
    G_RET=1
    exit ${G_RET}
fi

if [ "${COVERAGE_FLAG}" -eq 1 ]; then
    collect_testcase_output
    stat_coverage || G_RET=1
fi

end=$(date +%s)
time=$((end - start))
echo "use time: ${time} second"
echo "test ok"
exit ${G_RET}
