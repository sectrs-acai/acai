# trusted-peripherals

## Build

``` sh
./scripts/init.sh

# build host
./buildconf/linux-host/setup.sh init
./buildconf/linux-host/setup.sh build

# build ns guest
./buildconf/linux-guest/setup.sh init
./buildconf/linux-guest/setup.sh build
```

## Run

```sh
./buildconf/linux-guest/setup.sh run_fvp
```




