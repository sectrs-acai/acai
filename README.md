# trusted-peripherals

## setup
```sh
./scripts/init.sh
```

## build x86 host
```sh
./buildconf/linux-host/setup.sh init
./buildconf/linux-host/setup.sh build
```

## build aarch64 ns guest
```sh
./buildconf/linux-guest/setup.sh init
./buildconf/linux-guest/setup.sh build
```

## run
```sh
./buildconf/linux-guest/setup.sh run
./buildconf/linux-guest/setup.sh run_fvp
./buildconf/linux-host/setup.sh run
```
`/mnt/host/` mounts to source root of this directory.

## toolchains

### build with x86 toolchain
```sh
# ensure to run /buildconf/linux-host/setup.sh init/build first
source ./scripts/env-x86.sh
cd /your/source/directory
make
```
### build with aarch64 toolchain
```sh
# ensure to run /buildconf/linux-guest/setup.sh init/build first
source ./scripts/aarch64.sh
cd /your/source/directory
make
```

## repositories and branches
```
# x86 host with faulthook escape
branch: trusted-peripherals-kernel/trusted-periph/linux-host

# aarch64 ns linux
branch: trusted-peripherals-kernel/trusted-periph/linux-guest
```
