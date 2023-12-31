# ACAI
[![License: MIT](https://img.shields.io/badge/USENIX%20Security%20'24-Accepted%20Paper-blue)](https://www.usenix.org/conference/usenixsecurity24/presentation/sridhara)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![acai-artifact-eval](https://github.com/sectrs-acai/acai/actions/workflows/build-acai.yml/badge.svg)](https://github.com/sectrs-acai/acai/actions/workflows/build-acai.yml)
[![Twitter](https://img.shields.io/twitter/follow/sectrs_ethz)](https://twitter.com/sectrs_ethz)



<img src="https://github.com/sectrs-acai/acai/assets/2311941/60969033-76c2-408e-b07d-f70830f449a8" width="600">


## Abstract 
Trusted execution environments in several existing and upcoming CPUs demonstrate
the success of confidential computing, with the caveat that tenants cannot use
accelerators such as GPUs and FPGAs. Even after hardware changes to enable TEEs
on both sides and software changes to adopt existing code to leverage these
features, it results in redundant data copies and hardware encryption at the
bus-level and on the accelerator thus degrading the performance and defeating
the purpose of using accelerators. In this paper, we reconsider the Arm
Confidential Computing Architecture (CCA) design — an upcoming TEE feature in
Arm v9 — to address this gap. We observe that CCA offers the right abstraction
and mechanisms to allow confidential VMs to use accelerators as a first class
abstraction, while relying on the hardware-based memory protection to preserve
security. We build ACAI, a CCA-based solution, to demonstrate the feasibility of
our approach while addressing several critical security gaps. Our experimental
results on GPU and FPGA show that ACAI can achieve strong security guarantees
while maintaining performance and compatibility.


```TeX
@article{DBLP:journals/corr/abs-2305-15986,
  author       = {Supraja Sridhara and Andrin Bertschi and Benedict Schl{\"{u}}ter and Mark Kuhne and Fabio Aliberti and Shweta Shinde},
  title        = {{ACAI:} Extending Arm Confidential Computing Architecture Protection
                  from CPUs to Accelerators},
  journal      = {CoRR},
  volume       = {abs/2305.15986},
  year         = {2023},
  url          = {https://doi.org/10.48550/arXiv.2305.15986},
  doi          = {10.48550/ARXIV.2305.15986},
  eprinttype    = {arXiv},
  eprint       = {2305.15986},
  timestamp    = {Wed, 07 Jun 2023 14:31:13 +0200},
  biburl       = {https://dblp.org/rec/journals/corr/abs-2305-15986.bib},
  bibsource    = {dblp computer science bibliography, https://dblp.org}
}
```

https://arxiv.org/abs/2305.15986

## Build the Project

Please follow [Artifact Evaluation](./doc/artifact-evaluation.md) for
instructions how to build and run the project. You find more tutorials and
documentation [here](./doc).

## Files and Directories

``` sh
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
