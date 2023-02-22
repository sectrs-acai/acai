#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set +u

source $SCRIPT_DIR/../env.sh

FVP=$(which FVP_Base_RevC-2xAEMvA)
# FVP=/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/linux-guest/tools/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA
scal=/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/linux-guest/tools/Base_RevC_AEMvA_pkg/plugins/Linux64_GCC-9.3/ScalableVectorExtension.so

    #-C bp.terminal_0.mode=telnet \
    #-C bp.terminal_0.start_telnet=0 \
    #-C bp.terminal_1.mode=raw \
    #-C bp.terminal_1.start_telnet=0 \
    #-C bp.terminal_2.mode=raw \
    #-C bp.terminal_2.start_telnet=0 \
    #-C bp.terminal_3.mode=raw \
    #-C bp.terminal_3.start_telnet=0 \

function run_fvp {
  local bl1=$1
  local fip=$2
  local image=$3
  local rootfs=$4
  local p9_folder=$5
  local mount_tag=host0
  local preload=$6
  set -euo pipefail
  local LOCAL_NET_PORT=8022



  # XXX: libhook is in fvp bin directory along with fvp binary
  # XXX: We need sudo so we can pin fvp memory
	sudo LD_PRELOAD=$preload $FVP \
    --data cluster0.cpu0=${image}@0x84000000 \
        --plugin=$scal \
    --stat \
     -C SVE.ScalableVectorExtension.has_sme=0 \
     -C SVE.ScalableVectorExtension.has_sve2=1 \
    -C bp.dram_metadata.is_enabled=1 \
    -C bp.dram_size=4 \
    -C bp.flashloader0.fname=${fip} \
    -C bp.hostbridge.userNetPorts=8022=22 \
    -C bp.hostbridge.userNetworking=1 \
    -C bp.refcounter.non_arch_start_at_default=1 \
    -C bp.refcounter.use_real_time=0 \
    -C bp.secure_memory=1 \
    -C bp.secureflashloader.fname=${bl1} \
    -C bp.smsc_91c111.enabled=1 \
    -C bp.ve_sysregs.exit_on_shutdown=1 \
    -C bp.virtioblockdevice.image_path=${rootfs} \
    -C bp.vis.disable_visualisation=1 \
    -C cache_state_modelled=0 \
    -C cluster0.NUM_CORES=4 \
    -C cluster0.PA_SIZE=48 \
    -C cluster0.check_memory_attributes=0 \
    -C cluster0.clear_reg_top_eret=2 \
    -C cluster0.ecv_support_level=2 \
    -C cluster0.enhanced_pac2_level=3 \
    -C cluster0.gicv3.cpuintf-mmap-access-level=2 \
    -C cluster0.gicv3.without-DS-support=1 \
    -C cluster0.gicv4.mask-virtual-interrupt=1 \
    -C cluster0.has_16k_granule=1 \
    -C cluster0.has_amu=1 \
    -C cluster0.has_arm_v8-1=1 \
    -C cluster0.has_arm_v8-2=1 \
    -C cluster0.has_arm_v8-3=1 \
    -C cluster0.has_arm_v8-4=1 \
    -C cluster0.has_arm_v8-5=1 \
    -C cluster0.has_arm_v8-6=1 \
    -C cluster0.has_arm_v8-7=1 \
    -C cluster0.has_arm_v9-0=1 \
    -C cluster0.has_arm_v9-1=1 \
    -C cluster0.has_arm_v9-2=1 \
    -C cluster0.has_branch_target_exception=1 \
    -C cluster0.has_brbe=1 \
    -C cluster0.has_large_system_ext=1 \
    -C cluster0.has_large_va=1 \
    -C cluster0.has_rndr=1 \
    -C cluster0.max_32bit_el=0 \
    -C cluster0.memory_tagging_support_level=3 \
    -C cluster0.rme_support_level=2 \
    -C cluster0.stage12_tlb_size=1024 \
    -C cluster1.NUM_CORES=4 \
    -C cluster1.PA_SIZE=48 \
    -C cluster1.check_memory_attributes=0 \
    -C cluster1.clear_reg_top_eret=2 \
    -C cluster1.ecv_support_level=2 \
    -C cluster1.enhanced_pac2_level=3 \
    -C cluster1.gicv3.cpuintf-mmap-access-level=2 \
    -C cluster1.gicv3.without-DS-support=1 \
    -C cluster1.gicv4.mask-virtual-interrupt=1 \
    -C cluster1.has_16k_granule=1 \
    -C cluster1.has_amu=1 \
    -C cluster1.has_arm_v8-1=1 \
    -C cluster1.has_arm_v8-2=1 \
    -C cluster1.has_arm_v8-3=1 \
    -C cluster1.has_arm_v8-4=1 \
    -C cluster1.has_arm_v8-5=1 \
    -C cluster1.has_arm_v8-6=1 \
    -C cluster1.has_arm_v8-7=1 \
    -C cluster1.has_arm_v9-0=1 \
    -C cluster1.has_arm_v9-1=1 \
    -C cluster1.has_arm_v9-2=1 \
    -C cluster1.has_branch_target_exception=1 \
    -C cluster1.has_brbe=1 \
    -C cluster1.has_large_system_ext=1 \
    -C cluster1.has_large_va=1 \
    -C cluster1.has_rndr=1 \
    -C cluster1.max_32bit_el=0 \
    -C cluster1.memory_tagging_support_level=3 \
    -C cluster1.rme_support_level=2 \
    -C cluster1.stage12_tlb_size=1024 \
    -C pci.pci_smmuv3.mmu.SMMU_AIDR=2 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR0=4592187 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR1=6291458 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR3=5908 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR5=4294902901 \
    -C pci.pci_smmuv3.mmu.SMMU_ROOT_IDR0=3 \
    -C pci.pci_smmuv3.mmu.SMMU_ROOT_IIDR=1083 \
    -C pci.pci_smmuv3.mmu.SMMU_S_IDR1=2684354562 \
    -C pci.pci_smmuv3.mmu.SMMU_S_IDR2=0 \
    -C pci.pci_smmuv3.mmu.SMMU_S_IDR3=0 \
    -C pci.pci_smmuv3.mmu.root_register_page_offset=131072 \
    -C pctl.startup=0.0.0.0 \
    -C bp.virtiop9device.root_path=${p9_folder} \
	  -C bp.virtiop9device.mount_tag=${mount_tag} \
  	  -C bp.hostbridge.userNetworking=1 \
	  -C bp.pl011_uart0.out_file=uart0.log \
	  -C bp.pl011_uart1.out_file=uart1.log \
	  -C bp.pl011_uart2.out_file=uart2.log


  echo $?
  echo "return code"

}

function usage {
  echo "usage="
	echo "$0 <bl1 path> <fip path> <kernel image path> <rootfs path> <p9_folder endpoint path> <libc hook preload file path> <value='ssh' if using ssh to connect>"
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

preload=""
if [ -z "$6" ]; then
    echo "using no preload"
else
	preload=$6
  echo "using preload=$preload"
fi

if [ "$7" == "ssh" ]; then
  echo "setting up xauth to use x11 forwarding with sudo"
	set -x
  xauth list | grep unix`echo $DISPLAY | cut -c10-12` > /tmp/xauth
  sudo xauth add `cat /tmp/xauth`
fi

set -x
run_fvp $1 $2 $3 $4 $5 $preload
set +x
