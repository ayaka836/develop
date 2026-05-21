SHELL := /bin/bash

TESTCASE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
KVPROT_ROOT_DIR := $(abspath $(TESTCASE_DIR)/..)
REPO_ROOT_DIR := $(abspath $(KVPROT_ROOT_DIR)/..)

KVPROT_LLT_COVERAGE ?= 0
KVPROT_LLT_BUILD_NAME := kvprot_llt
ifeq ($(KVPROT_LLT_COVERAGE),1)
KVPROT_LLT_BUILD_NAME := kvprot_llt_cov
endif

KVPROT_LLT_BUILD_DIR ?= $(TESTCASE_DIR)/build/$(KVPROT_LLT_BUILD_NAME)
KVPROT_LLT_EXE ?= $(TESTCASE_DIR)/kvprot_llt

GTEST_ROOT_DIR ?= $(TESTCASE_DIR)/third_party/googletest
GTEST_SRC_DIR := $(GTEST_ROOT_DIR)/googletest
GTEST_ALL_SRC := $(GTEST_SRC_DIR)/src/gtest-all.cc
GTEST_HEADER := $(GTEST_SRC_DIR)/include/gtest/gtest.h

HAOTIAN_INCLUDE_ROOT ?= $(REPO_ROOT_DIR)/haotian_include
HAOTIAN_ROOT ?= $(REPO_ROOT_DIR)/haotian
IMPORT_INCLUDE_ROOT ?= $(REPO_ROOT_DIR)/import_include

CXX ?= g++
KVPROT_LLT_CXXFLAGS ?= -std=gnu++14 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-field-initializers -Wno-old-style-cast
KVPROT_LLT_CPPFLAGS ?=
KVPROT_LLT_LDFLAGS ?=
KVPROT_LLT_LDLIBS ?= -pthread
KVPROT_LLT_EXTRA_INCLUDE_DIRS ?=

ifeq ($(KVPROT_LLT_COVERAGE),1)
KVPROT_LLT_CXXFLAGS += --coverage -O0 -g
KVPROT_LLT_LDFLAGS += --coverage
endif

KVPROT_LLT_FLAGS_RSP := $(KVPROT_LLT_BUILD_DIR)/kvprot_llt.cxxflags.rsp
KVPROT_LLT_INCLUDE_MAP_DIR := $(KVPROT_LLT_BUILD_DIR)/include_map
KVPROT_LLT_INCLUDE_MAP_STAMP := $(KVPROT_LLT_INCLUDE_MAP_DIR)/.stamp
KVPROT_LLT_OBJ := $(KVPROT_LLT_BUILD_DIR)/kvprot_dt.o
KVPROT_LLT_GTEST_OBJ := $(KVPROT_LLT_BUILD_DIR)/gtest-all.o

KVPROT_LLT_TEST_INPUTS := \
    $(TESTCASE_DIR)/kvprot_dt.cpp \
    $(wildcard $(TESTCASE_DIR)/support/*.cpp) \
    $(wildcard $(TESTCASE_DIR)/support/*.h) \
    $(wildcard $(TESTCASE_DIR)/cases/*.cpp) \
    $(wildcard $(KVPROT_ROOT_DIR)/src/*/*.c) \
    $(wildcard $(KVPROT_ROOT_DIR)/include/*.h)

KVPROT_LLT_LOCAL_INCLUDE_DIRS := \
    $(TESTCASE_DIR)/support \
    $(TESTCASE_DIR)/cases \
    $(KVPROT_ROOT_DIR)/include \
    $(GTEST_SRC_DIR)/include \
    $(GTEST_SRC_DIR)

KVPROT_LLT_EXTERNAL_INCLUDE_ROOTS := \
    $(HAOTIAN_INCLUDE_ROOT)/aa_include \
    $(HAOTIAN_INCLUDE_ROOT)/aa_include/infrastructure \
    $(HAOTIAN_INCLUDE_ROOT)/aa_include/infrastructure/open_source \
    $(HAOTIAN_INCLUDE_ROOT)/distribute_include \
    $(HAOTIAN_INCLUDE_ROOT)/dataturbo_include \
    $(HAOTIAN_INCLUDE_ROOT)/opensourceUpdate \
    $(HAOTIAN_ROOT)/untar_haotian/aa_include \
    $(HAOTIAN_ROOT)/untar_haotian/aa_include/infrastructure \
    $(HAOTIAN_ROOT)/untar_haotian/aa_include/infrastructure/open_source \
    $(HAOTIAN_ROOT)/untar_haotian/distribute_include \
    $(HAOTIAN_ROOT)/untar_haotian/dataturbo_include \
    $(HAOTIAN_ROOT)/untar_haotian/opensourceUpdate \
    $(IMPORT_INCLUDE_ROOT)/dorado \
    $(KVPROT_LLT_EXTRA_INCLUDE_DIRS)

.PHONY: kvprot_llt kvprot_llt_cov run_kvprot_llt run_kvprot_llt_cov clean clean_kvprot_llt kvprot_llt_clean check_kvprot_gtest

kvprot_llt: check_kvprot_gtest $(KVPROT_LLT_EXE)

kvprot_llt_cov:
	$(MAKE) KVPROT_LLT_COVERAGE=1 kvprot_llt

run_kvprot_llt: kvprot_llt
	@mkdir -p "$(TESTCASE_DIR)/coverage/result_lcov"
	"$(KVPROT_LLT_EXE)" --gtest_output="xml:$(TESTCASE_DIR)/coverage/result_lcov/kvprot_llt.xml"

run_kvprot_llt_cov:
	$(MAKE) KVPROT_LLT_COVERAGE=1 run_kvprot_llt

check_kvprot_gtest:
	@test -f "$(GTEST_ALL_SRC)" || { \
		echo "GoogleTest source not found: $(GTEST_ALL_SRC)"; \
		echo "Please clone a complete googletest tree under $(GTEST_ROOT_DIR)."; \
		exit 1; \
	}
	@test -f "$(GTEST_HEADER)" || { \
		echo "GoogleTest header not found: $(GTEST_HEADER)"; \
		echo "The vendored googletest tree is incomplete. Re-clone v1.16.0 or ensure gtest/gtest.h is committed."; \
		exit 1; \
	}

$(KVPROT_LLT_BUILD_DIR):
	@mkdir -p "$@"

$(KVPROT_LLT_INCLUDE_MAP_STAMP): $(THIS_MAKEFILE) | $(KVPROT_LLT_BUILD_DIR)
	@rm -rf "$(KVPROT_LLT_INCLUDE_MAP_DIR)"
	@mkdir -p "$(KVPROT_LLT_INCLUDE_MAP_DIR)"
	@: > "$(KVPROT_LLT_BUILD_DIR)/include_map_duplicates.txt"
	@for root in $(KVPROT_LLT_EXTERNAL_INCLUDE_ROOTS); do \
		if [ -d "$$root" ]; then \
			find "$$root" -type f \( -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) -print | sort; \
		fi; \
	done | while IFS= read -r header; do \
		name=$$(basename "$$header"); \
		dst="$(KVPROT_LLT_INCLUDE_MAP_DIR)/$$name"; \
		if [ ! -e "$$dst" ]; then \
			ln -s "$$header" "$$dst"; \
		else \
			printf '%s -> %s\n' "$$name" "$$header" >> "$(KVPROT_LLT_BUILD_DIR)/include_map_duplicates.txt"; \
		fi; \
	done
	@touch "$@"

$(KVPROT_LLT_FLAGS_RSP): $(THIS_MAKEFILE) $(KVPROT_LLT_INCLUDE_MAP_STAMP) | $(KVPROT_LLT_BUILD_DIR)
	@: > "$@"
	@for dir in $(KVPROT_LLT_LOCAL_INCLUDE_DIRS); do \
		if [ -d "$$dir" ]; then printf '%s\n' "-I$$dir" >> "$@"; fi; \
	done
	@for dir in $(KVPROT_LLT_EXTERNAL_INCLUDE_ROOTS); do \
		if [ -d "$$dir" ]; then printf '%s\n' "-idirafter $$dir" >> "$@"; fi; \
	done
	@if [ -d "$(KVPROT_LLT_INCLUDE_MAP_DIR)" ]; then printf '%s\n' "-idirafter $(KVPROT_LLT_INCLUDE_MAP_DIR)" >> "$@"; fi
	@for flag in $(KVPROT_LLT_CPPFLAGS) $(KVPROT_LLT_CXXFLAGS); do \
		printf '%s\n' "$$flag" >> "$@"; \
	done

$(KVPROT_LLT_OBJ): $(KVPROT_LLT_TEST_INPUTS) $(KVPROT_LLT_FLAGS_RSP)
	$(CXX) @"$(KVPROT_LLT_FLAGS_RSP)" -c "$(TESTCASE_DIR)/kvprot_dt.cpp" -o "$@"

$(KVPROT_LLT_GTEST_OBJ): $(GTEST_ALL_SRC) $(KVPROT_LLT_FLAGS_RSP)
	$(CXX) @"$(KVPROT_LLT_FLAGS_RSP)" -c "$(GTEST_ALL_SRC)" -o "$@"

$(KVPROT_LLT_EXE): $(KVPROT_LLT_OBJ) $(KVPROT_LLT_GTEST_OBJ)
	$(CXX) $(KVPROT_LLT_LDFLAGS) $^ $(KVPROT_LLT_LDLIBS) -o "$@"

clean: clean_kvprot_llt

kvprot_llt_clean: clean_kvprot_llt

clean_kvprot_llt:
	$(RM) -r "$(TESTCASE_DIR)/build/kvprot_llt" "$(TESTCASE_DIR)/build/kvprot_llt_cov" "$(KVPROT_LLT_BUILD_DIR)" "$(KVPROT_LLT_EXE)"
