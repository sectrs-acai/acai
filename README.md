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
### build with x86 toolchain
```sh
source ./scripts/env-x86.sh
cd /your/source/directory && make
```
## build aarch64 ns guest
```sh
./buildconf/linux-guest/setup.sh init
./buildconf/linux-guest/setup.sh build
```

### build with aarch64 toolchain
```sh
source ./scripts/aarch64.sh
cd /your/source/directory && make
```

## run
```sh
./buildconf/linux-guest/setup.sh run
./buildconf/linux-guest/setup.sh run_fvp
./buildconf/linux-host/setup.sh run
```
`/mnt/host/` mounts to source root of this directory.

## repositories and branches
```
# x86 host with faulthook escape
branch: trusted-peripherals-kernel/trusted-periph/linux-host

# aarch64 ns linux
branch: trusted-peripherals-kernel/trusted-periph/linux-guest
```
