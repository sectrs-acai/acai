# Build the Project

This file describes the steps to set up the build environment and build TFA,
kernels, RMM, kvmtool and shared libraries.
  
  * [0. Initialize Project](#0-initialize-project)
  * [1. Build Dependencies, Toolchain and Software Packages](#1-build-dependencies-toolchain-and-software-packages)
  * [2. Build TFA, RMM](#2-build-tfa-rmm)
  * [3. Build kvmtool](#3-build-kvmtool)
  
- Note: Steps [1](#1-build-dependencies-toolchain-and-software-packages),
[2](#2-build-tfa-rmm) and [3](#3-build-kvmtool) are captured in the install
script [/scripts/install.sh](/scripts/install.sh). You may skip these steps if
you used the install script.

- Note: To configure the host system for the PCIe-bypass mechanism, see
  [/doc/pcie-bypass.md](/doc/pcie-bypass.md). The bypass mechanism is required to run
  benchmark on the FVP.
  
## 0. Initialize Project <!-- TOC --><a name="0-initialize-project"></a>

To get started, clone the project and all its submodules by executing the code listed below:

```
git clone git@gitlab.inf.ethz.ch:PRV-SHINDE/projects/proj-2022/rmm/trusted-peripherals.git
cd trusted-peripherals
git submodule update --init --recursive
./scripts/init.sh
```

This may take some time. This downloads all external dependencies and binaries
required to complete the next steps.

For the subsequent steps, we recommend using the provided Ubuntu 20.04 distrobox
container. (requires distrobox and {podman|docker}): You find the distrobox
container in [/scripts/distrobox](/scripts/distrobox).

```
# Setup and run Container
./scripts/distrbox/distrobox_setup.sh

# Enter container with Shell
./scripts/distrbox/distrobox_run.sh

user@trusted-periph-distrobox $    
```

## 1. Build Dependencies, Toolchain and Software Packages <a name="1-build-dependencies-toolchain-and-software-packages"></a>

We use [buildroot](https://buildroot.org/) to capture all build dependencies. With the previous steps executed, buildroot is already downloaded. 

Run the following scripts to build a CCA enabled kernel, as well as the Rootfile
system for both Non-Secure and Realm worlds:

```
# For Non-Secure world
./buildconf/linux-cca-guest/setup.sh init
./buildconf/linux-cca-guest/setup.sh build

# For Realm World
./buildconf/linux-cca-realm/setup.sh init
./buildconf/linux-cca-realm/setup.sh build
```

Upon success, you find the build artifacts in [/assets/snapshots](/assets/snapshots):
- `rootfs.ext2`: Root file system for Non-Secure world
- `rootfs.realm.cpio`: Root file system for Realm world
- `Image-cca`: CCA-enabled kernel for Non-Secure and Realm world 


## 2. Build TFA, RMM <a name="2-build-tfa-rmm"></a>

To build the firmware image package (FIP) with TFA and RMM configured to boot a
Linux kernel, execute the following scripts:

```
./buildconf/tfa/setup.sh init
./buildconf/tfa/setup.sh linux

```

This configuration allows TFA to directly boot a Linux kernel on the FVP without
a dedicated bootloader.

Upon success, you find the build artifacts in /assets/snapshots:
- `fip.bin`: Firmware Image Package
- `bl1.bin`: Secureflashloader bl1 image 


## 3. Build kvmtool <a name="3-build-kvmtool"></a>
We use kvmtool as the VMM to launch a VM. You may use one of the pre-compiled
binaries in the `/assets/` directory or build kvmtool as follows:

```
./buildconf/shrinkwrap/setup.sh init
./buildconf/shrinkwrap/setup.sh kvmtool
```

Upon success, you find the build artifacts in /assets/snapshots:
- `lkvm`: the VMM

Proceed with [/doc/run_realm.md](/doc/run_realm.md) to run the FVP.
