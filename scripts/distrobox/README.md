# distrobox setup

Ensure to have [distrobox](https://github.com/89luca89/distrobox) and [podman](https://podman.io/) or [docker](https://www.docker.com/) installed. For a list of depdencencies, see script [install-ubuntu-packages.sh](./install-ubuntu-packages.sh)

``` sh
# setup container
./distrobox_setup.sh

# enter container with shell
./distrobox_run.sh

# run command in container
./distrobox_run.sh /bin/fish
```

### Compile CUDA applications
Source `./env-gcc-4.4.sh`to access gcc-4.4 and cuda 5 sdk.
