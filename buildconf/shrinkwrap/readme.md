Build tfa, rmm, linux based off ./src/ directory with the shrinkwrap
docker image.

```sh
./setup.sh init
./setup.sh build
./setup.sh run
```

Build kvmtool from ./src/ directory

```sh
./setup.sh kvmtool
```

Build rmm, tfa using local Makefile.
```sh
./setup.sh local
./setup.sh clean
```
