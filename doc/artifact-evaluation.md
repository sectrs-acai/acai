#  USENIX Security '24 Artifact ACAI

# Overview
- 1. Build Instructions (20 human minutes + 250 compute minutes)
- 2. Configuration Instructions (5 human minutes)
- 3. Usage Instructions (26 minutes)
- 4. Validate Results, Documentation and Source Code


# Build Instructions (20 human minutes + 250 compute minutes)

- You need an x86 Linux system to build the project.
- The build requires ca. 60 GB of available storage.
- In order to run a minimal example, your host system needs X server running. We recommend [setting up VNC](https://www.digitalocean.com/community/tutorials/how-to-install-and-configure-vnc-on-ubuntu-20-04) to access X remotely.
- Running the minimal example in the simulation software needs ca. 4 GB of ram.
- We used an Ubuntu 20.04 system to build and evaluate our research artifacts.
  However, we capture all build dependencies in an Ubuntu 20.04-based
  docker/distrobox container so you may choose any host Linux distribution to
  build the artifacts, provided it supports Docker.
- You are about to cross compile several linux kernels, root filesystems and software packages. 
  If you are running this in a VM, ensure that the VM has several cores to speed up compilation.
- A GitHub runner continuously builds the ACAI artifacts (steps 0. to 3. in the following build instructions). Throughout USENIX Security '24 artifact evaluation period, this self-hosted runner remains accessible and serves as a resource to solve build-related challenges that
may arise.

### 0. Prerequisites (5 human minutes)
Ensure to have the following two technologies installed:

- Docker: https://www.docker.com/
  ([Install](https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository))
- Distrobox: https://github.com/89luca89/distrobox
  ([Install](https://github.com/89luca89/distrobox#alternative-methods))

Distrobox is a set of shell scripts to simplify the use of a docker container
as a development environment.

We cross compile different toolchains, Linux kernels and root file systems along
with our research artifacts. These are required to run a hypervisor and spawn a
realm VM on the FVP. Apart from Distrobox and Docker, all build dependencies are
configured within the container.

### 1. Download Repository and Submodules (5 human minutes + 30 compute minutes)

```sh
git clone https://github.com/sectrs-acai/acai.git
cd acai
git submodule update --init --recursive

./scripts/init.sh
```

- The scripts access the internet and downloads several git repositories, and assets,
  including toolchains and Arm's FVP.
- Git submodules can be frustrating and their setup hairy, if a clone fails you
  may be better off by deleting the repository and cloning the entire repository
  again.


### 2. Build the Docker Container (5 human minutes + 20 compute minutes)
Build and enter the docker container with following commands.

```sh
# Build the container
./scripts/distrobox/distrobox_setup.sh

# Enter the container with a shell
./scripts/distrobox/distrobox_run.sh
```

- Distrobox/Docker mounts your home directory into the container.
- The scripts access the internet and configures an ubuntu docker machine.
- You may want to launch a better shell in the docker container, e.g. fish for
  better auto complete
- For subsequent runs, you can enter the container directly 
  with the `distrobox_run.sh` script.

### 3. Build Software (5 human minutes + 200 compute minutes)

From within the docker container, build all software packages:
```sh
# inside docker:
./scripts/install.sh
```

- If the build fails, you can delete /output-distrobox/ and run install.sh again
- Expect this to take some time.
- Upon success, you have built all components to run our prototype on the FVP
  simulation software.

# Configuration Instructions (2 human minutes)
- There is no Armv9 hardware available in silicon yet, this is why we use 
  Arm's proprietary simulation software (FVP) to run our prototype.
- The FVP is already downloaded and preconfigured through the previous steps.
- We believe our launch configurations contain sane defaults, if you wish to
  further configure the FVP, you find the launch script in
  `./scripts/run_fvp.sh`.
  - You may change the simulated DRAM size (in GB) with `-C bp.dram_size=<val>`

# Usage Instructions (26 minutes)
- In order to run the Arm FVP Simulation software, you need to grant the docker
  container access to your local X server
- The FVP launches X11 xterm applications to display console output in different
  security domains of the simulated Arm system.
- You need a running X server on your system.  

```sh
# outside of docker container, 
# where $USER is a shell variable referring to your user account.

xhost local:$USER
```

## Minimal Working Example 

### 1. Launch Hypervisor on FVP (2 human minutes + 2 compute minutes)
With X server permissions properly configured, launch the FVP with the command below:

```sh
# inside of docker container
./buildconf/tfa/setup.sh run_linux
```
- We recommend you try to launch the FVP from within the docker container. If
  this fails for some reason, run the above script outside of docker (On Linux
  host machine).
- This spawns three X11 xterm windows and runs our research prototype.
- EL3 and realm-EL2 runs ACAI firmware code
- If you are receiving the error: "xterm: Xt error: Can't open display", ensure that
  you are running the script without a terminal multiplexer such as tmux or
  screen inside the docker container. We pin the FVP pages to DRAM such that
  they are not paged out which requires sudo priviledges.
- If you are receiving the error: "Warning: This program is an suid-root
  program...", and running the script witout a terminal multiplexer does not fix
  the issue, remove sudo invocation from
  [setup.sh](https://github.com/sectrs-acai/acai/blob/0b0ae3daee9ab239071c5831fdfe3c71fd072238/scripts/run_fvp.sh#L27).
  Sudo is only required for the PCIe bypass which we do not need in the minimal
  working example.

In one of the three xterm windows, login with user _root_ and no password. You
are now running a KVM hypervisor in Non-Secure world. The git repository is
mounted in `/mnt/host`.

### 2. Spawn a realm VM with KVM (2 human minutes + 15 compute minutes)

Upon login, multiplex your terminal with screen to get two shells:

```sh
screen
# press enter
screen
```

- You can switch between screen sessions with Ctrl-A.

In one of the sessions, boot a realm VM:

```sh
cd /mnt/host/scripts/
./run_toy_realm.sh
```
- This boots a realm VM on the simulated system
- Use root and no password to login. Boot may take up to 15 mins
- The realm VM mounts the git repository to `/mnt/host/mnt/host`.
- Ignore (9P mounting) errors in the console output, mounting succeeds nonetheless. 

You can kill the realm VM by running the following command in the screen session
of the hypervisor or just restart the FVP (kill the Linux process).

```sh
# Aarch64: 
/mnt/host/assets/snapshots/lkvm stop -a

# or on x86
pkill -f FVP
```

### 3. Delegating realm private Memory to device with ACAI (5 human minutes)
- Without the PCIe escape mechanism and accelerators, you cannot execute our
  benchmark suite on the simulated Arm FVP.
- Instead, this demo demonstrates ACAIs protected mode by delegating realm
  private memory to a device.
- The device is the SMMU Testengine in the FVP, which can act as a dummy PCIe
  device on the system. The Testengine is guarded behind the FVP's SMMU
  component and subject to ACAIs access restrictions, just like other PCIe
  devices on real hardware.

Minimal example:
  - 1. The demo allocates realm VM private memory
  - 2. the Testengine tries to access the memory, the access fails
  - 3. We delegate the memory with ACAI to be accessible by the Testengine
  - 4. The Testengine can access the memory.

In the realm VM, run
```
cd /mnt/host/mnt/host/src/testing/testengine
./run.sh
```

<details>
<summary>Sample Output</summary>

```
[ 2136.078504] [i] sample init
[ 2136.105725] [+] allocating memory
[ 2136.117628] [+] setting up test engine
[ 2136.127967] [+] test engine successfully claimed
[ 2136.140036] [+] try accessing realm private memory from test engine
[ 2300.713126] loop count
[ 2509.661640] HANDLING RME_EXIT RMI_TRIGGER_TESTENGINE
[ 2509.661788] calling rec_trigger_testengine sid: 31, iova_src: 894c1000, iova_dst: 894c0000
[ 2509.661928] copy from 0x894c1000, to 0x894c0000 0x1000 bytes
[ 2509.661977] SMMUv3TestEngine: Waiting for MEMCPY completion for frame: 894c1000
[ 2509.662061] arm-smmu-v3 2b400000.iommu: handle events arm_smmu_evtq_thread
[ 2509.662141] loop count
[ 2509.662156] SMMUv3: Test failed, status ENGINE_ERROR
[ 2509.662453] arm-smmu-v3 2b400000.iommu: SSID valid 0 | StreamID value 0
[ 2509.662594] arm-smmu-v3 2b400000.iommu: EVT_ID Unknown
[ 2509.662713] arm-smmu-v3 2b400000.iommu: event 0x00 received:
[ 2509.662842] arm-smmu-v3 2b400000.iommu:      0x0000000000000000
[ 2509.662971] arm-smmu-v3 2b400000.iommu:      0x0000000000000000
[[ 2509.663100] arm-smmu-v3 2b400000.iommu:     0x0000000000000000
[ 2509.663229] arm-smmu-v3 2b400000.iommu:      0x0000000000000000
 2136.158602] [+] Test engine cannot access realm private memory (expected)
[ 2136.173495] [+] Delegating page (ipa) addr 894c1000 to be accessible by testengine
[ 2136.191725] [+] Delegate success
[ 2136.200939] [+] try accessing realm private memory from test engine (delegated) 
[ 2136.218555] [+] before access:
[ 2136.227757] 0000000000000000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  |
[ 2509.749202] HANDLING RME_EXIT RMI_TRIGGER_TESTENGINE
[ 2509.749297] calling rec_trigger_testengine sid: 31, iova_src: 894c1000, iova_dst: 894c0000
[ 2509.749436] copy from 0x894c1000, to 0x894c0000 0x1000 bytes
[ 2509.749487] SMMUv3TestEngine: Waiting for MEMCPY completion for frame: 894c1000
[ 2136.245914] [+] after access:
[ 2136.254935] 0000000000000000  ba ba ba ba ba ba ba ba ba ba ba ba ba ba ba ba  |����������������|
[ 2136.275824] [+] Successful. Memory delegated and accessible
[ 2136.289684] [+] Delegating page back to realm VM
[ 2136.301891] [+] Done

```
</details>

# Validate Results, Documentation and Source Code
- We cannot provide access to our accelerators used for benchmarking:
  - GPU: Nvidia GeForce GTX 460 SE (Fermi Architecture)
  - FPGA: Xilinx Virtex Ultrascale+ VCU118
- Apart from access to these accelerators, the host system must run a custom
  kernel with a patched pagefault mechanism (./src/linux-host), used for the
  PCIe bypass. The kernel must run bare-metal and must have full access to the
  accelerators over PCIe.
- You find more documentation on the FVP escape mechanism in
  [pcie_bypass.md](/doc/pcie_bypass.md).
- You find more documentation on the benchmark setup in [run_experiments.md](/doc/run_experiments.md).   

## Files and Directories

The repository directory resembles the following structure and source code

```
/ext...................................: External project dependencies 
/buildconf.............................: Scripts and configuration to build artifacts
/scripts...............................: Helper scripts
/output................................: Artifact output
/output-distrobox......................: Artifact output built in container
/src...................................: Sources and source submodules
/scr/tfa...............................: TFA Monitor
/src/rmm...............................: RMM
/src/linux-cca-guest...................: CCA-enabled kernel
/src/fpga_driver.......................: PCIe bypass for FPGA
/src/fpga_driver/fh_host...............: ioctl library for PCIe bypass on x86
/src/fpga_driver/fpga_escape_libhook...: x86 FPGA Userspace Manager
/src/fpga_driver/xdma..................: x86 FPGA Host driver
/src/fpga_driver/xdma_stub.............: aarch64 FPGA guest driver
/src/fpga_driver/libhook...............: FVP memory alignment for DMA/ mmap
/src/fpga_driver/devmem_intercept .....: ACAI aarch64 kernel helper
/src/gpu_driver........................: PCIe bypass for GPU
/src/gpu_driver/gdev-guest.............: aarch64 GPU guest driver
/src/gpu_driver/gdev-host..............: x86 GPU host driver
/src/gpu_driver/gpu_gdev_usr_manager...: x86 GPU Userspace Manager
/src/gpu_driver/rodinia-bench..........: GPU Benchmarks CUDA Driver API
/src/benchmarking/fpga.................: FPGA Benchmarks
/src/linux-host........................: Faulthook host kernel
/src/encrypted-cuda....................: Encryption layer
/src/kvmtool...........................: Virtual Machine Manager
/src/tfa-tests.........................: TFA tests
```

Essential source repositories referenced in the paper:

- `/src/tfa` is a fork of Arm Trusted Firmware with our Firmware changes
- `/src/rmm` is a fork of the Realm Management Monitor with our Firmware changes
- `/src/linux-cca-guest` is a CCA enabled kernel used for the Hypervisor and a
  realm VM
- PCIe bypass mechanisms for FPGA and GPU are implemented in `/src/fpga_driver`
  and `/src/gpu_driver`
- The PCIe bypass mechanism requires a patched host kernel located in
  `/src/linux-host`
- PCIe bypass aware guest drivers for FPGA and GPU are located in
  `/src/fpga_driver/xdma_stub` and `/src/gpu_driver/gdev-guest`
  - These are only required for simulation on the FVP, due to lack of available Armv9 hardware.  
- Benchmarks for GPU are in `/src/gpu_driver/rodinia-bench`
- Benchmarks for FPGA are in `/src/benchmarks/fpga`
- `/src/encrypted-cuda` is an encryption layer which encrypts GPU Cuda Driver API
  calls
- ACAI helper tool is in `/src/fpga_driver/devmem_intercept`

## Overview source-compiled Artifacts 

Upon build success, you find the following artifacts:

Aarch64 Platform:
- Our version of TFA, RMM: 
  * `./assets/snapshots/{bl1.bin|fip.bin}`
- CCA enabled hypervisor+realm kernel: 
  * `./assets/snapshots/Image-cca`
- Rootfile system for non-secure hypervisor: 
  * `./assets/snapshots/rootfs-ns.ext2`
- Rootfile system for realm VM: 
  * `./assets/snapshots/rootfs.realm.cpio`

Aarch64 Realm VM:
- Escape aware FPGA driver: 
  * `./src/fpga_driver/xdma_stub/linux-kernel/xdma/xdma_stub.ko`
- Escape aware GPU driver: 
  * `./src/gpu_driver/gdev-guest/mod/gdev/gdev_stub.ko`
- ACAI-helper kernel module: 
  * `./src/fpga_driver/devmem_intercept/devmem_intercept.ko`

x86 Host:
- Header-only ioctl library for PCIe bypass enabled kernel: 
  * `./src/fpga_driver/fh_host/fh_host_header.h`
- PCIe bypass enabled kernel: 
  * `./output-distrobox/buildroot-linux-host/images/bzImage`
- FVP patch for memory alignment: 
  * `./src/fpga_driver/libhook/libhook-libc-2.31.so`
- Userspace manager for GPU: 
  * `./src/fpga_driver/fpga_escape_libhook/fpga_usr_manager`
- Userspace manager for FPGA: 
  * `./src/gpu_driver/gpu_gdev_usr_manager/gpu_gdev_usr_manager`
- Pt editor host module: 
  * `./ext/pteditor/module/pteditor.ko`
