# Buildtools

We built a custom build system to support the iterative development process
required to prototype confidential device access on Arm CCA. The system is bash
and GNU Make-based and uses the package management system of the Buildroot
project.

### Toolchains
We cross-compile software packages for two architectures, namely AArch64 and
x86_64. This is required because we build software components for the x86 host
and the Fixed Virtual Platform (FVP). The build system is responsible for
downloading and configuring two custom toolchains. We build environment wrappers
that source into a custom build environment, where the appropriate toolchain is
set up and required software dependencies compiled for the target architecture.
This allows us to agree on a reproducible standard development environment
across different machines.

### Root File systems
The build system generates two custom AArch64-based root file systems for both
the Non-secure and realm world. These file systems have all required libraries
bundled and essential software packages such as Python interpreter and its
dependencies installed.

We further configure 9P (Plan 9 Filesystem Protocol) directory sharing with the
underlying host system, eliminating the need for rebuilding the root file
systems when software dependencies change. Additionally, the root file systems
have matching kernel headers installed such that we can compile kernel modules
on the x86 host and run them on the FVP.


### Fixed Virtual Platform
The build system automates the download process of the FVP and patches its
memory allocation such that we can properly bypass the FVP and interact with
accelerators on the x86 host. It further configures the tracer plugins for the
benchmarking evaluation.


### Kernels and Packages
Furthermore, the build system assembles three kernel images, one for the x86
host with the faulthook patchset present and two AArch64 Linux kernels for ARM
CCA in the Non-secure and realm worlds. These kernels can be executed on the FVP
(for AArch64 kernels) or QEMU. We configure QEMU to support kernel debugging.
The build system further handles the download and compilation of TFA, RMM, and
kvmtool, along with their respective library dependencies.

The efforts put into building a system to build the project proved valuable
because they increased developer productivity. Creating a custom build system
became necessary because of all the interdependencies of the required software
packages, different architectures, and development machines.
