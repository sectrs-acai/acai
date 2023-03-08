# Faulthook Fixed Virtual Platforms

## x86 Host 
In order to sandbox escape out of the FVP, the host must run a custom kernel
with the faulthook patches.

- Faulthook Kernel tree ./src/linux-host
- Build helpers: ./buildconfi/linux-host

## Libc allocator hooks
The FVP must be launched with libc allocator hooks.
See ./src/libhook

```sh
preload=./assets/fvp/bin/libhook-libc-2.35.so
FVP=$(which FVP_Base_RevC-2xAEMvA)

sudo LD_PRELOAD=$preload $FVP \
   /* ... fvp launch params here */
```

Ensure to run the FVP with root righs. Root is required to pin memory.

## Tools
- The host must run the patched host kernel driver (e.g. xdma)
- The host must run pteditor kernel module (./ext/pteditor)
- The host must run the userspace manager
- The host must run the FVP
