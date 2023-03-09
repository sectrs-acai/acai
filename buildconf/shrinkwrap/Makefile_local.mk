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
LOG_LEVEL=40
jobs=$(shell nproc)
CFLAGS=-Wno-error

# XXX: If you enable debug mode artifacts end up in /debug/ not /release/
DEBUG=1

LOCAL_RMM_ROOT=$(SRC_ROOT)/rmm
LOCAL_TFA_ROOT=$(SRC_ROOT)/tfa
LOCAL_TFA_TESTS_ROOT=$(SRC_ROOT)/tfa-tests
FVP_BIN=$(PROJ_ROOT)/assets/fvp/bin/FVP_Base_RevC-2xAEMvA

export CROSS_COMPILE=$(PROJ_ROOT)/assets/toolchain-arm/aarch64-none-elf/bin/aarch64-none-elf-

# for local build
LOCAL_RMM_BUILD_DIR=$(LOCAL_RMM_ROOT)/tmp/build/rmm
LOCAL_TFA_TEST_BUILD_DIR=$(LOCAL_TFA_TESTS_ROOT)/build

LOCAL_TFTF=$(LOCAL_TFA_TEST_BUILD_DIR)/fvp/debug/tftf.bin
LOCAL_RMM=$(LOCAL_RMM_BUILD_DIR)/rmm.img
LOCAL_FIP=$(LOCAL_TFA_ROOT)/build/fvp/debug/fip.bin
LOCAL_BL1=$(LOCAL_TFA_ROOT)/build/fvp/debug/bl1.bin

local: local-tfa-linux
tests: local-tfa
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
		PLAT=fvp DEBUG=$(DEBUG) LOG_LEVEL=$(LOG_LEVEL) \
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
        DEBUG=$(DEBUG) \
	    LOG_LEVEL=$(LOG_LEVEL) \
        BL33=$(LOCAL_TFTF) \
        all fip
	@echo created $(LOCAL_BL1)
	@echo created $(LOCAL_FIP)
	cp -f $(LOCAL_BL1) $(PROJ_ROOT)/assets/snapshots
	cp -f $(LOCAL_FIP) $(PROJ_ROOT)/assets/snapshots


.PHONY: local-tfa-linux
local-tfa-linux: local-rmm
	cd $(LOCAL_TFA_ROOT) && \
	$(MAKE_BIN) -j$(jobs) PLAT=fvp \
		DEBUG=$(DEBUG) \
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
		PRELOADED_BL33_BASE=0x88000000 \
		all fip

	@echo created $(LOCAL_BL1)
	@echo created $(LOCAL_FIP)
	cp -f $(LOCAL_BL1) $(PROJ_ROOT)/assets/snapshots
	cp -f $(LOCAL_FIP) $(PROJ_ROOT)/assets/snapshots


local-run-fvp:
	$(FVP_BIN) \
	-C bp.secureflashloader.fname=$(LOCAL_BL1)          \
	-C bp.flashloader0.fname=$(LOCAL_FIP)               \
	-C bp.refcounter.non_arch_start_at_default=1                   \
	-C bp.refcounter.use_real_time=0                               \
	-C bp.ve_sysregs.exit_on_shutdown=1                            \
	-C cache_state_modelled=0                                      \
	-C bp.dram_size=2                                              \
	-C bp.secure_memory=1                                          \
	-C pci.pci_smmuv3.mmu.SMMU_ROOT_IDR0=3                         \
	-C pci.pci_smmuv3.mmu.SMMU_ROOT_IIDR=0x43B                     \
	-C pci.pci_smmuv3.mmu.root_register_page_offset=0x20000        \
	-C cluster0.NUM_CORES=4                                        \
	-C cluster0.PA_SIZE=48                                         \
	-C cluster0.ecv_support_level=2                                \
	-C cluster0.gicv3.cpuintf-mmap-access-level=2                  \
	-C cluster0.gicv3.without-DS-support=1                         \
	-C cluster0.gicv4.mask-virtual-interrupt=1                     \
	-C cluster0.has_arm_v8-6=1                                     \
	-C cluster0.has_amu=1                                          \
	-C cluster0.has_branch_target_exception=1                      \
	-C cluster0.rme_support_level=2                                \
	-C cluster0.has_rndr=1                                         \
	-C cluster0.has_v8_7_pmu_extension=2                           \
	-C cluster0.max_32bit_el=-1                                    \
	-C cluster0.stage12_tlb_size=1024                              \
	-C cluster0.check_memory_attributes=0                          \
	-C cluster0.ish_is_osh=1                                       \
	-C cluster0.restriction_on_speculative_execution=2             \
	-C cluster0.restriction_on_speculative_execution_aarch32=2     \
	-C cluster1.NUM_CORES=4                                        \
	-C cluster1.PA_SIZE=48                                         \
	-C cluster1.ecv_support_level=2                                \
	-C cluster1.gicv3.cpuintf-mmap-access-level=2                  \
	-C cluster1.gicv3.without-DS-support=1                         \
	-C cluster1.gicv4.mask-virtual-interrupt=1                     \
	-C cluster1.has_arm_v8-6=1                                     \
	-C cluster1.has_amu=1                                          \
	-C cluster1.has_branch_target_exception=1                      \
	-C cluster1.rme_support_level=2                                \
	-C cluster1.has_rndr=1                                         \
	-C cluster1.has_v8_7_pmu_extension=2                           \
	-C cluster1.max_32bit_el=-1                                    \
	-C cluster1.stage12_tlb_size=1024                              \
	-C cluster1.check_memory_attributes=0                          \
	-C cluster1.ish_is_osh=1                                       \
	-C cluster1.restriction_on_speculative_execution=2             \
	-C cluster1.restriction_on_speculative_execution_aarch32=2     \
	-C pctl.startup=0.0.0.0                                        \
	-C bp.smsc_91c111.enabled=1                                    \
	-C bp.hostbridge.userNetworking=1 \
	-C bp.pl011_uart0.out_file=uart0.log \
	-C bp.pl011_uart1.out_file=uart1.log \
	-C bp.pl011_uart2.out_file=uart2.log


    # --cadi-server
