#
# Makefile to build and deploy sources outside of yocto build context
# (former Makefile_local.mk in yocto project)
#
PWD=$(shell pwd)
SCRIPT_DIR=$(shell cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_ROOT=$(SCRIPT_DIR)/../..
SRC_ROOT=$(PROJ_ROOT)/src

SCRIPTS_DIR=$(SCRIPT_DIR)/scripts
SHELL := /bin/bash

MAKE_BIN=make
LOG_LEVEL=10
jobs=$(shell nproc)
CFLAGS=-Wno-error

LOCAL_RMM_ROOT=$(SRC_ROOT)/rmm
LOCAL_TFA_ROOT=$(SRC_ROOT)/tfa
LOCAL_TFA_TESTS_ROOT=$(SRC_ROOT)/tfa-tests

export CROSS_COMPILE=$(PROJ_ROOT)/assets/toolchain-arm/aarch64-none-elf/bin/aarch64-none-elf-

# for local build
LOCAL_RMM_BUILD_DIR=$(LOCAL_RMM_ROOT)/tmp/build/rmm
LOCAL_TFA_TEST_BUILD_DIR=$(LOCAL_TFA_TESTS_ROOT)/build

LOCAL_TFTF=$(LOCAL_TFA_TEST_BUILD_DIR)/fvp/release/tftf.bin
LOCAL_RMM=$(LOCAL_RMM_BUILD_DIR)/rmm.img
LOCAL_FIP=$(LOCAL_TFA_ROOT)/build/fvp/release/fip.bin
LOCAL_BL1=$(LOCAL_TFA_ROOT)/build/fvp/release/bl1.bin

local: local-tfa-linux
clean: local-clean

.PHONY:  local-clean
local-clean:
	cd $(LOCAL_TFA_TESTS_ROOT) && make clean
	cd $(LOCAL_TFA_ROOT) && make clean
	rm -r $(LOCAL_RMM_BUILD_DIR) || true
	rm -r $(LOCAL_TFA_TEST_BUILD_DIR) || true
	rm -r $(LOCAL_TFA_ROOT)/build || true

.PHONY: local-rmm
local-rmm:
	cd $(LOCAL_RMM_ROOT) && \
	cmake  -DRMM_CONFIG=fvp_defcfg \
			-S $(LOCAL_RMM_ROOT) \
			-B $(LOCAL_RMM_BUILD_DIR) \
			-DLOG_LEVEL=$(LOG_LEVEL)
	cd $(LOCAL_RMM_ROOT) && \
	cmake  --build $(LOCAL_RMM_BUILD_DIR)


.PHONY: local-tfa-tests
local-tfa-tests:
	cd $(LOCAL_TFA_TESTS_ROOT) && \
    $(MAKE_BIN) -j$(jobs) \
		PLAT=fvp DEBUG=0 LOG_LEVEL=$(LOG_LEVEL) \
		USE_NVM=0 SHELL_COLOR=1 CFLAGS=-Wno-error \
		pack_realm all \
		TESTS=realm-payload

.PHONY: local-tfa
local-tfa: local-rmm local-tfa-tests
	cd $(LOCAL_TFA_ROOT) && \
	$(MAKE_BIN) -j$(jobs) PLAT=fvp \
        ENABLE_RME=1 \
		RMM=$(LOCAL_RMM) \
        FVP_HW_CONFIG_DTS=fdts/fvp-base-gicv3-psci-1t.dts \
        DEBUG=0 \
	    LOG_LEVEL=$(LOG_LEVEL) \
        BL33=$(LOCAL_TFTF) \
        all fip


.PHONY: local-tfa-linux
local-tfa-linux: local-rmm
	cd $(LOCAL_TFA_ROOT) && \
	$(MAKE_BIN) -j$(jobs) PLAT=fvp \
		DEBUG=0 \
		LOG_LEVEL=$(LOG_LEVEL) \
		ARM_DISABLE_TRUSTED_WDOG=1 \
		FVP_HW_CONFIG_DTS=fdts/fvp-base-gicv3-psci-1t.dts \
		ARM_ARCH_MINOR=5 \
		ENABLE_SVE_FOR_NS=1 \
		ENABLE_SVE_FOR_SWD=1 \
		CTX_INCLUDE_PAUTH_REGS=1 \
		BRANCH_PROTECTION=1 \
		CTX_INCLUDE_MTE_REGS=1 \
		ENABLE_FEAT_HCX=0 \
		CTX_INCLUDE_AARCH32_REGS=0 \
		ENABLE_SME_FOR_NS=0 \
		ENABLE_SME_FOR_SWD=0 \
		ENABLE_RME=1 \
		RMM=$(LOCAL_RMM) \
		CTX_INCLUDE_EL2_REGS=1 \
		ARM_LINUX_KERNEL_AS_BL33=1 \
		PRELOADED_BL33_BASE=0x84000000 \
		all fip

	@echo created $(LOCAL_BL1)
	@echo created $(LOCAL_FIP)
	cp -f $(LOCAL_BL1) $(PROJ_ROOT)/assets/snapshots
	cp -f $(LOCAL_FIP) $(PROJ_ROOT)/assets/snapshots
