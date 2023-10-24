#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

source $SCRIPT_DIR/../env.sh
FVP=$ASSETS_DIR/fvp/bin/FVP_Base_RevC-2xAEMvA

#
# XXX: Running the tracer plugin (--benchmark)
# requires two FVP plugins (ACAI GenericTrace.so and Arm ToggleMTIPlugin.so)
#

function run_fvp {
  set -x
  local bl1=$1
  local fip=$2
  local image=$3
  local rootfs=$4
  local p9_folder=$5
  local mount_tag=host0
  local preload=$6
  local plugins=$7
  local LOCAL_NET_PORT=8022

  # XXX: libhook is in fvp bin directory along with fvp binary
  # XXX: We need sudo so we can pin fvp memory
  sudo LD_PRELOAD=$preload nice -n -20 $FVP \
    --data cluster0.cpu0=${image}@0x84000000 \
    --stat \
    -C bp.dram_metadata.is_enabled=1 \
    -C bp.dram_size=3 \
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
    -C pci.pci_smmuv3.mmu.SMMU_IDR0=0x46123b \
    -C pci.pci_smmuv3.mmu.SMMU_IDR1=0x600002 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR3=0x1714 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR5=0xffff0475 \
    -C pci.pci_smmuv3.mmu.SMMU_ROOT_IDR0=3 \
    -C pci.pci_smmuv3.mmu.SMMU_ROOT_IIDR=0x43b \
    -C pci.pci_smmuv3.mmu.SMMU_S_IDR1=0xa0000002 \
    -C pci.pci_smmuv3.mmu.SMMU_S_IDR2=0 \
    -C pci.pci_smmuv3.mmu.SMMU_S_IDR3=0 \
    -C pci.pci_smmuv3.mmu.root_register_page_offset=0x20000 \
    -C pci.pci_smmuv3.mmu.SMMU_IDR0=0x80fe6bf                      \
    -C pci.pci_smmuv3.mmu.SMMU_IDR1=0xe739d08  \
    -C pctl.startup=0.0.0.0 \
    -C bp.virtiop9device.root_path=${p9_folder} \
    -C bp.virtiop9device.mount_tag=${mount_tag} \
    -C bp.hostbridge.userNetworking=1 \
    -C bp.pl011_uart0.out_file=uart0.log \
    -C bp.pl011_uart1.out_file=uart1.log \
    -C bp.pl011_uart2.out_file=uart2.log $plugins

  echo $?
  echo "return code"

}

function generate_plugin_cmds {
  local plugin_dir=$ASSETS_DIR/fvp/benchmarking
  local trace_plugin=$plugin_dir/GenericTrace.so
  local toggle_plugin=$plugin_dir/ToggleMTIPlugin.so

  # local trace_src="FVP_Base_RevC_2xAEMvA.cluster0.cpu3.INST"
  local trace_src="INST"
  local trace_cmds="-C TRACE.GenericTrace.trace-sources=$trace_src"
  local toggle_cmds="-C TRACE.ToggleMTIPlugin.use_hlt=1 -C TRACE.ToggleMTIPlugin.hlt_imm16=0x1337 -C TRACE.ToggleMTIPlugin.disable_mti_from_start=1"

  echo "--plugin=$trace_plugin $trace_cmds --plugin=$toggle_plugin $toggle_cmds"
}

function usage {
  echo "usage: "
  echo "$0 \\"
  echo "  --bl1=<bl1 path> \\ "
  echo "  --fip=<fip path> \\ "
  echo "  --kernel=<kernel image path> \\"
  echo "  --rootfs=<rootfs path> \\ "
  echo "  --p9=<p9_folder endpoint path> \\"
  echo "  [--hook=<libc hook preload file path> ] \\"
  echo "  [--ssh ] \\ "
  echo "  [--benchmark ]"
  exit
}

bl1=""
fip=""
kernel=""
rootfs=""
p9=""
hook=""
ssh=0
benchmark=0

while [ $# -gt 0 ]; do
  case "$1" in
    --bl1=*)
      bl1="${1#*=}"
      echo "using bl1=${bl1}"
      ;;
    --fip=*)
      fip="${1#*=}"
      echo "using fip=${fip}"
      ;;
    --kernel=*)
      kernel="${1#*=}"
      echo "using kernel=${kernel}"
      ;;
    --rootfs=*)
      rootfs="${1#*=}"
      echo "using rootfs=${rootfs}"
      ;;
    --p9=*)
      p9="${1#*=}"
      echo "using p9=${p9}"
      ;;
    --hook=*)
      hook="${1#*=}"
      echo "using libc hook=${hook}"
      ;;
    --ssh*)
      ssh=1
      echo "using ssh"
      ;;
    --benchmark*)
      benchmark=1
      echo "using benchmarking hooks"
      ;;
    --no-benchmark*)
      benchmark=0
      echo "using no benchmarking hooks"
      ;;
  esac
  shift
done

if [ -z "$bl1" ] || [ -z $fip ] || [ -z $kernel ] || [ -z $p9 ]; then
  usage
fi

echo "using configuration: "
echo "bl1=$bl1"
echo "fip=$fip"
echo "kernel=$kernel"
echo "rootfs=$rootfs"
echo "p9=$p9"
echo "hook=$hook"
echo "ssh=$ssh"
echo "benchmark=$benchmark"


if [ "$ssh" == "1" ]; then
  echo "setting up xauth to use x11 forwarding with sudo"
	set -x
  xauth list | grep unix`echo $DISPLAY | cut -c10-12` > /tmp/xauth
  sudo xauth add `cat /tmp/xauth`
fi

benchmark_cmds=""
if [ "$benchmark" == "1" ]; then
  benchmark_cmds=$(generate_plugin_cmds)
  echo "using plugins=$benchmark_cmds"
fi

set -x
run_fvp $bl1 $fip $kernel $rootfs $p9 $hook "$benchmark_cmds"
set +x
