# Usage Instructions on PCIe bypass for FVP

In order to reproduce benchmark results, you must have a compatible accelerator
connected to your host. In our work, we used the following:

- Nvidia GeForce GTX 460 SE, with Nouveau Kernel Driver 5.10
- Xilinx Virtex Ultrascale+ VCU118 with xdma Kernel driver 

Note that for benchmarking, you can not run the FVP in the provided distrobox
container. The host must run the custom kernel with faulthook patches enabled,
and have access to the accelerators over PCIe. The distrobox container only
simplifies the build process of these artifacts.

- The bypass mechanism is further discussed in
  [/doc/eth_mthesis_cca.pdf](/doc/eth_mthesis_cca.pdf).

## x86 Host 
In order to sandbox escape out of the FVP, the host must run a custom kernel
with the faulthook patches.

Faulthook is a patchset of changes in the page fault code of the kernel. It
enables a manager in userspace to control the FVP's host memory allocations.
This userspace manager acts as an intermediary, facilitating communication
between PCIe devices on the host system, such as the GPU, and software running
on the FVP. This enables the execution of direct memory access (DMA) and memory
mapped input/output (I/O) operations on actual accelerators.


![/doc/assets/faulthook.png](/doc/assets/faulthook.png)

For more information, see the software packages located at:

- Faulthook Kernel tree ./src/linux-host
- Build helpers: ./buildconf/linux-host
- Userspace manager for GPU: ./src/gpu_driver
- Userspace manager for FPGA: ./src/fpga_driver

The provided distrobox setup builds a faulthook kernel with source code in ./src/linux-host.
You may change the kconfig files to add feature support for your hardware.


## Libc allocator hooks
The FVP must be launched with libc allocator hooks. The hooks change memory
alignment of the FVP for DMA and memory mapped I/O. You find the source code in
`./src/fpga_driver/libhook`.

In `./scripts/run_fvp.sh` script, libc allocator functions are replaced with `LD_PRELOAD` directive.
```sh
preload=./assets/fvp/bin/libhook-libc-2.35.so
FVP=$(which FVP_Base_RevC-2xAEMvA)

sudo LD_PRELOAD=$preload $FVP \
   /* ... fvp launch params here */
```

Ensure to run the FVP with root rights. Root is required to pin memory.

## Launch FVP with bypass mechanism

The subsequent steps describe the setup to enable the pcie bypass mechanism on
the FVP.

### 0. Build Software Packages
Build the project as described in [/doc/build_project.md](/doc/build_project.md) 
or [/doc/artifact-evaluation.md](/doc/artfifact-evaluation.md).

### 1. Compile and Boot into Host Kernel

The bypass mechanism is based on a set of kernel patches to introduce a new
special page fault, so called faulthook. The patchset supports Kernel 5.10. The
FVP uses faulthooks to sandbox escape out of its level of abstraction to
interact with accelerators on the host system.

Build the Linux kernel located in `./src/linux-host` and boot it on your
machine. You find a sample kernel config file in
`./buildconf/linux-host/buildroot/kernel_config`. Grep for `custom config` to
find a list of additional kconfig options enabled. In particular, faulthooks are
enabled with the config show below:

```
CONFIG_FAULTHOOK=y
```

### 2. Build and Run FPGA Userspace Manager

![/doc/assets/fpga2.png](/doc/assets/fpga2.png)

#### Compile Userspace Manager
Having the host kernel in place, compile the host userspace utility programs
which interact with the FVP for the PCIe bypass.

You find the FPGA userspace manager in `./src/fpga_driver`.

Build the userspace manager with:
```sh
# in distrobox container:
cd src/fpga_driver/
./build.sh build
```
This builds the userspace manager `fpga_usr_manager`.

You may either run the manager during boot of the hypervisor in Non-Secure
world, or during boot of a realm VM. In the first case, the manager aligns
memory allocations for the hypervisor, whereas in the latter case, it aligns
memory for a realm VM. These alignments allow the userspace manager to access
the FVP's memory of a realm VM or the Non-Secure memory of the hypervisor.

#### Boot FVP
Boot the FVP as described in [/doc/run_realm.md](/doc/run_realm.md).

Kernel arguments to enable PCIe bypass for code running in non-secure world:
```
mem=1900M isolcpus=3 console=ttyAMA0 earlycon=pl011,0x1c090000 root=/dev/vda 
ip=on cpuidle.off=1 ip=off fvp_escape_on cma=256MB"
```

The flag `fvp_escape_on` or `fvp_escape_off` determines if the kernel should wait
until the userspace manager establishes a handshake with memory alignment.

```
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (1/12)
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (2/12)
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (3/12)
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (4/12)
```
During the handshake, the FVP Linux kernel running in Non-Secure 
world marks all its empty pages with their page frame number.
The x86 userspace manager scans the address space of the FVP and 
finds these marked pages in memory. It is subsequently able to identify 
the location of the FVP physical memory in the underlying x86 Linux address space.

```
-- launch of FVP
-- Hypervisor boot in Non-Secure world.
[    0.000000] Linux version 6.2.0-rc1 (b@beancd) (aarch64-none-linux-gnu-gcc (Arm GNU Toolchain 12.2.Rel1 (Build arm-12.24))
               12.2.1 20221205, GNU ld (Arm GNU Toolchain 12.2.Rel1 (Build arm-12.24)) 2.39.0.20221210) 
               #1 SMP PREEMPT Sun Jun  4 14:43:04 CEST 2023
[    0.000000] Machine model: FVP Base
[    0.000000] Memory limited to 1900MB
[    0.000000] earlycon: pl11 at MMIO 0x000000001c090000 (options '')
[    0.000000] printk: bootconsole [pl11] enabled
[    0.000000] efi: UEFI not found.
[    0.000000] Reserved memory: created DMA memory pool at 0x0000000018000000, size 8 MiB
[    0.000000] OF: reserved mem: initialized node vram@18000000, compatible id shared-dma-pool
[    0.000000] fvp_escape_status: 1
[    0.000000] fvp escape PFN escape mapping: collecting pages
[    0.000000] total dram range: 80000000-f6c00000
[    0.000000] fvp escape: PFN escape mapping: touched: 475801, untouched: 10599
[    0.000000] fvp escape: magic page pfn=f5c8f
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (1/12)
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (2/12)
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (3/12)
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (4/12)
[    0.000000] fvp escape: waiting for go from host
[    0.000000] fvp escape: waiting for go from host
[    0.000000] Please launch userspace manager to exchange pfn escape mapping (5/12)
[    0.000000] fvp escape: addr mapping success
[    0.000000] fvp escape: reserving 0xf5c8f000+0xbb8000
[    0.000000] NUMA: No NUMA configuration found
[    0.000000] NUMA: Faking a node at [mem 0x0000000080000000-0x00000000f6bfffff]
[    0.000000] NUMA: NODE_DATA [mem 0xf5c7ea00-0xf5c80fff]
[    0.000000] Zone ranges:
[    0.000000]   DMA      [mem 0x0000000080000000-0x00000000f6bfffff]
[    0.000000]   DMA32    empty
[    0.000000]   Normal   empty
[    0.000000] Movable zone start for each node
[    0.000000] Early memory node ranges
...
```

#### Run Userspace Manager

You may use the script `./src/fpga_driver/fpga_escape_libhook/setup_fpga_usr_manager.sh`
to run the userspace manager for FPGA.

It will:
- build and load the kernel module pteditor found in ./ext/pteditor
- build and load the XDMA kernel module for the FPGA
- run the userspace manager

```
-- on x86 host:
-- successful handshake for memory alignment.
+ sudo ./run_fpga_usr_manager.sh
[+]  Using pid 2373493
[+]  opening file /tmp/hooks_mmap_2373493
[+]  from: 7ffcf7a23000, to 7ffff7a23000
[+]  pid: 2373493
[+]  escape page size: bb8000 bytes
[~]  Reading settings file from /tmp/.temp.libhook_settings.txt
[-]  Error opening file: /tmp/.temp.libhook_settings.txt
[-]  restoring context from file failed: -1
[-]  map memory from file failed. Restarting without file: -1
[~]  Searching addresses 0x7ffcf7a23000-0x7ffff7a23000 in pid 2373493...
[~]  Searching addresses 0x7ffcf7a23000-0x7ffff7a23000 in pid 2373493...
[-]  found escape page: 0x7ffd0042f000=1006735
[~]  mapping vaddr 0x7ffd0042f000 into host
[~]  mapping 3000 pages from addrs 7ffd0042f000-7ffd00fe7000@(pid 2373493) to 7ffff7447000-7ffff7fff000@(pid this)
[+]  mapping successful
[+]  guest reserved 0xbb8000 bytes for escape
[+]  init_addr_map with pfn: 0x80010-0xf6846 (485430)
[~]  fvpmem_scan_addresses...
[+]  escape_page: 0x7ffd0042f000
00 CA FE CA FE CA FE CA  8F 5C 0F 00 00 00 00 00  |  .........\......
01 CA FE CA FE CA FE CA  01 00 00 00 00 00 00 00  |  ................
03 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00  |  ................
08 00 00 00 00 00 00 00  00 80 BB 00 00 00 00 00  |  ................
[~]  Writing settings to /tmp/.temp.libhook_settings.txt
[~]  Writing mappings to /tmp/.temp.libhook_mapping.txt
[+]  continuing guest
[~]  verifying pfn-vaddr mappings
[~]  non-contiguous addresses : 0x7ffd0527b000, 0x0
[~]  non-contiguous addresses : 0x0, 0x7ffd0527c000
[~]  non-contiguous addresses : 0x7ffd07278000, 0x7ffcf7da4000
[~]  non-contiguous addresses : 0x7ffcf7db3000, 0x0
[~]  non-contiguous addresses : 0x0, 0x7ffd031e1000
[~]  non-contiguous addresses : 0x7ffd031e4000, 0x7ffd031e0000
[~]  non-contiguous addresses : 0x7ffd031e0000, 0x0
[~]  non-contiguous addresses : 0x0, 0x7ffd07279000
[~]  non-contiguous addresses : 0x7ffd70ac8000, 0x7ffcfa5a0000
[+]  number of unmapped pfn: 9627
[+]  number of mapped pfn: 475802
[~]  verifying escape_page_pfn
[+]  calling weak do_init
[~]  Listening for faulthooks...
```

#### Build Aaarch64 Stub driver
With the Non-Secure kernel booted, compile the FPGA driver for Aarch64 to use in the FVP.
```
# -- in distrobox container on x86 host
source ./scripts/env-aarch64.sh

# build driver
cd ./src/fpga_driver/xdma_stub/linux-kernel/xdma
make

# build tests
cd ./src/fpga_driver/xdma_stub/linux-kernel/tools
make
```
Due to 9P directory sharing, the built artifacts appear in the Non-Secure kernel mounted in `/mnt/host/`.

#### Run Aaarch64 Stub driver

In the FVP Non-Secure world kernel, load the stub XDMA driver with the script
`/mnt/host/scripts/fvp/setup_fvp_fpga.sh`.

```
# pwd
/mnt/host/scripts/fvp
# ./setup_fvp_fpga.sh 
+ cd /mnt/host/scripts/fvp/../..//src/fpga_driver/xdma_tests
+ ./test_guest_load_driver.sh
/mnt/host/src/fpga_driver/xdma_tests
hi
interrupt_selection 4.
Loading driver...insmod xdma.ko poll_mode=1 ...
[23947.795017] xdma_stub: loading out-of-tree module taints kernel.
[23947.800707] xdma_stub: unknown parameter 'poll_mode' ignored
[23947.801923] xdma_stub:pci_stub_init: pci_register_driver
[23947.807434] xdma_stub:faulthook_init: faulthook page: ffff000075c8f000+bb8000, pfn=f5c8f, phyaddr=f5c8f000 
[23947.807600] xdma_stub:faulthook_init: ffff000075c8f000=0
[23947.842705] xdma_stub:faulthook_init: Skipping mapping verify

The Kernel module installed correctly and the xmda devices were recognized.
DONE
# ls -al /dev/xdma0_*
crw-------    1 root     root      511,  36 Jul 20 21:02 /dev/xdma0_c2h_0
crw-------    1 root     root      511,  37 Jul 20 21:02 /dev/xdma0_c2h_1
crw-------    1 root     root      511,  38 Jul 20 21:02 /dev/xdma0_c2h_2
crw-------    1 root     root      511,  39 Jul 20 21:02 /dev/xdma0_c2h_3
crw-------    1 root     root      511,   1 Jul 20 21:02 /dev/xdma0_control
crw-------    1 root     root      511,  32 Jul 20 21:02 /dev/xdma0_h2c_0
crw-------    1 root     root      511,  33 Jul 20 21:02 /dev/xdma0_h2c_1
crw-------    1 root     root      511,  34 Jul 20 21:02 /dev/xdma0_h2c_2
crw-------    1 root     root      511,  35 Jul 20 21:02 /dev/xdma0_h2c_3
crw-------    1 root     root      511,   0 Jul 20 21:02 /dev/xdma0_user
crw-------    1 root     root      511,   2 Jul 20 21:02 /dev/xdma0_xvc
# 
```
Subsequently, you find the bypass aware fpga driver in /dev/xdma*.

#### Run a realm VM
To access the FPGA device in a realm VM, launch the userspace manager during
boot of a realm VM (and not during hypervisor boot). It will then memory align
the FVP's memory limited for the realm VM.

See ./doc/run_realm.md on how to spawn a realm VM.

### 3. Build and Run GPU Userspace Manager
The steps required to interact with the GPU driver are similar to the FPGA.

You find the source code in `./src/gpu_driver/`.

- Userspace Manager: ./src/gpu_driver/gpu_gdev_usr_manager
- x86 Run script: ./src/gpu_driver/gpu_gdev_usr_manager/gpu_setup_host.sh
- Aarch64 stub driver: ./src/gpu_driver/gdev-guest/build_gdev_stub_arch64.sh
- x86 host driver: ./src/gpu_driver/gdev-host/

![/doc/assets/fpga2.png](/doc/assets/gdev.png)

