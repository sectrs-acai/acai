# Run experiments

Accelerators:  
  - GPU: Nvidia GeForce GTX 460 SE
  - FPGA: Xilinx Virtex Ultrascale+ VCU118

## Realm VM Boot
Realm VM:

```
# pwd is ./scripts
# XXX: core 3 is reserved for tracing

core=0x4
root=../assets/snapshots

nice -n -20 taskset $core ./$root/lkvm run --realm  --disable-sve -c 1 -m 1000 \
    -k $root/Image-cca -i $root/rootfs.realm.cpio --9p "/,host0" -p "fvp_escape_loop fvp_escape_on ip=off"
```

## Build
- ./scripts/install.sh (see ./doc/ on how to build).

## GPU
- Benchmarks: ./src/gpu_driver/rodinia-bench/cuda/setup.sh
- Pre Benchmark Setup: 
   - Load ACAI Helper Program: ./scripts/fvp/setup_devmem_intercept_realm.sh
   - Load GPU Driver: ./scripts/fvp/setup_fvp_gpu.sh

## FPGA
- Benchmarks: ./src/benchmarking/fpga/setup.sh
- Pre Benchmark Setup:
   - Load ACAI Helper Program: ./scripts/fvp/setup_devmem_intercept_realm.sh
   - Load FPGA Driver: ./scripts/fvp/setup_fvp_fpga.sh


## Gathering Data
- We implement a custom tracer plugin to gather executed instructions in the FVP.
- The setup scripts already instrument the FVP to use this plugin.
- Upon benchmark completion, the tracer plugin creates data dumps in /tmp/arm_*
  on the x86 host system running the FVP.
- The benchmarks are instrumented to use marker instructions to signalize special events.
  - Grep for (CCA_ marcors in the benchmarks)
  - For instance: 
      `#define CCA_MARKER(marker) __asm__ volatile("MOV XZR, " STR(marker))`
  - The MOV instruction is used to store a marker value into the zero register (XZR),
    which is a NOP and is detected by the tracer plugin as event of special interest.
