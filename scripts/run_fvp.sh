#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $(git rev-parse --show-toplevel)/env.sh

FVP=FVP_Base_RevC-2xAEMvA

function run_fvp {
	local bl1=$1
        local fip=$2
	local image=$3
	local rootfs=$4
	local p9_folder=$5
        local mount_tag=host0

	$FVP \
		-C bp.secureflashloader.fname=${bl1}          \
		-C bp.flashloader0.fname=${fip}               \
		-C bp.refcounter.non_arch_start_at_default=1                   \
		-C bp.refcounter.use_real_time=0                               \
		-C bp.ve_sysregs.exit_on_shutdown=1                            \
		-C cache_state_modelled=1                                      \
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
		-C bp.smsc_91c111.enabled=1 \
		--data cluster0.cpu0=${image}@0x84000000         \
		-C cache_state_modelled=0 \
		-C bp.virtiop9device.root_path=${p9_folder} \
		-C bp.virtiop9device.mount_tag=${mount_tag} \
		-C bp.virtioblockdevice.image_path=${rootfs} \
		-C bp.hostbridge.userNetworking=1 \
		-C bp.pl011_uart0.out_file=uart0.log \
		-C bp.pl011_uart1.out_file=uart1.log \
		-C bp.pl011_uart2.out_file=uart2.log

}

function usage {
	echo "$0 bl1 fip image rootfs p9_folder"
        exit
}


if [ -z "$1" ]; then
    usage
fi
if [ -z "$2" ]; then
    usage
fi
if [ -z "$3" ]; then
    usage
fi
if [ -z "$4" ]; then
    usage
fi
if [ -z "$5" ]; then
    usage
fi

set -x
run_fvp $1 $2 $3 $4 $5
set +x
