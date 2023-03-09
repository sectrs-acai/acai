# trusted-peripherals

## setup
```sh
./scripts/init.sh
```
If git fails with `error: Server does not allow request for
  unadvertised object X` you may run `git submodule sync` to fix the issue.

## build x86 host
```sh
./buildconf/linux-host/setup.sh init
./buildconf/linux-host/setup.sh build
```

## build aarch64 ns guest
```sh
# kernel based on lts 5.10
./buildconf/linux-guest/setup.sh init
./buildconf/linux-guest/setup.sh build

# kernel based on kvm/cca patches
./buildconf/linux-cca-guest/setup.sh init
./buildconf/linux-cca-guest/setup.sh build
```

## run
```sh
./buildconf/linux-guest/setup.sh run         # run on qemu (no tfa stack. lts 5.10)
./buildconf/linux-guest/setup.sh run_fvp     # run on fvp (full stack, lts 5.10)
./buildconf/linux-host/setup.sh run          # run on qemu
./buildconf/linux-cca-guest/setup.sh run_fvp # run on fvp (cca, full stack)
```
- `/mnt/host/` mounts to source root of this directory.
- qemu runs require qemu-system-aarch64 or qemu-system-x86_64 installed.

## develop 

### build with buildroot x86 toolchain
```sh
# ensure to run /buildconf/linux-host/setup.sh init/build first
source ./scripts/env-x86.sh
cd /your/source/directory
make
```

### build with buildroot aarch64 toolchain (lts 5.10 kernel headers)
```sh
# ensure to run /buildconf/linux-guest/setup.sh init/build first
source ./scripts/aarch64-5-10.sh
cd /your/source/directory
make
```

### build with buildroot aarch64 toolchain (6.2 kernel headers)
```sh
# ensure to run /buildconf/linux-cca-guest/setup.sh init/build first
source ./scripts/aarch64.sh
cd /your/source/directory
make
```

### develop on tfa/rmm
```sh
# change ./src/{tfa, rmm} 
./buildconf/shrinkwrap/setup.sh init
./buildconf/shrinkwrap/setup.sh build
./buildconf/shrinkwrap/setup.sh run
```

## misc
- Run the FVP with faulthook and libc hooks: ./doc/fvp_libhook_setup.md
