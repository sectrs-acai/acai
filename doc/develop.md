# Develop

### Files and Folders

This repository configures and builds Linux kernels and rootfs for Aarch64 and
x86. It further builds TFA, TF-RMM, kvmtool along with all projects required to
demonstrate trusted device assignment on Arm CCA.

``` sh
./src..........: Sources and source submodules
./ext..........: External submodules
./buildconf....: Scripts and configuration to build artifacts
./scripts......: Helper scripts
./output.......: Artifact output
```

To run the system end to end, we require a [patched x86 host kernel](/src/linux-host) for the
FVP, as well as Arm CCA-enabled kernels and rootfile systems for Aarch64 in
Realm and Non-Secure worlds.

We capture all build dependencies in buildroot projects. The following
environments configure the environments for either x86 builds, or Aarch64.

- [Build environment for x86 host](#build-x86)
- [Build environment for Aarch64 Non-Secure World](#build-aarch64-ns)
- [Build environment for Aarch64 Realm World](#build-aarch64-realm)

After init and build these environments, you may source into the [buildroot provided SDK
scripts](https://buildroot.org/downloads/manual/manual.html#_advanced_usage) to
access appropriate cross compilers and shared libraries, i.e. `source ./scripts/aarch64.sh`.

### Build environment for x86 host <a name="build-x86"></a>
```sh
./buildconf/linux-host/setup.sh init
./buildconf/linux-host/setup.sh build

# Run host kernel in qemu
./buildconf/linux-host/setup.sh run
```

### Build environment for Aarch64 Non-Secure World <a name="build-aarch64-ns"></a>
```sh
# kernel based on kvm/cca patches
./buildconf/linux-cca-guest/setup.sh init
./buildconf/linux-cca-guest/setup.sh build

# Run armcca ns world kernel on fvp
./buildconf/linux-cca-guest/setup.sh run_fvp 
```

The realm VM is spawned with kvmtool from within Non-Secure world on the FVP.  
So in order to launch a realm, ensure to build dependencies for Aarch64 Realm
World as well.

### Build environment for Aarch64 Realm World <a name="build-aarch64-realm"></a>
```sh
# kernel based on kvm/cca patches
./buildconf/linux-cca-realm/setup.sh init
./buildconf/linux-cca-realm/setup.sh build
```

## Develop 

#### Build with buildroot x86 toolchain
To access the bundled x86 toolchain, you may source the script below.

```sh
# ensure to run ./buildconf/linux-host/setup.sh init/build first
source ./scripts/env-x86.sh
cd /your/source/directory
make
```

#### Build with buildroot Aarch64 toolchain (6.2 kernel headers)
To access the bundled Aarch64 toolchain for ArmCCA, you may source the script below.

```sh
# ensure to run ./buildconf/linux-cca-guest/setup.sh init/build first
source ./scripts/aarch64.sh
cd /your/source/directory
make
```

#### Develop on tfa/rmm/tests

```sh
# ensure to run ./buildconf/linux-cca-guest/setup.sh init/build first
# change ./src/{tfa, rmm, tfa-tests} 
./buildconf/tfa/setup.sh {clean|tests|run_tests|linux|run_linux}
```

### Misc
- run the FVP with faulthook and libc hooks: /doc/fvp_libhook_setup.md
- in the rootfs, `/mnt/host/` mounts to source root of this git repository.
- qemu runs require qemu-system-aarch64 or qemu-system-x86_64 installed.


### Troubleshooting
- Git fails with `error: Server does not allow request for unadvertised object X

  - Try to run `git submodule sync` to fix the issue.

- 9p directory sharing fails in the FVP during kernel boot with 9pnet_virtio: no channels available for device 
  - ensure to configure 9p in the device tree
  ```
  # fvp-base-psci-common.dtsi

   virtio_p9@140000 {
       compatible = "virtio,mmio";
       reg = <0x0 0x1c140000 0x0 0x1000>;
       interrupts = <0x0 0x2b 0x4>;
  };

  ```
- no 9p directory mounted
  - mount 9p directory in realm: `mount -t 9p -o trans=virtio,version=9p2000.L host0 /mnt` 
  where host0 is the tag specified during fvp launch and /mnt the target mount location


