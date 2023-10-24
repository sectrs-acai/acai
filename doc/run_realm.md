# Run a realm VM

- Ensure to have the project built successfully before you continue (either [/doc/build_project.md](/doc/build_project.md) or [/doc/artifact-evaluation.md](/doc/artifact-evaluation.md)).
- Without the PCIe bypass mechanism, you are still able to launch a realm VM, however, you cannot access accelerators such as GPU or FPGA and hence cannot run our experiments ([doc/pcie_bypass.md](doc/pcie_bypass.md)).

## 1. Boot the FVP into Non-Secure World <a name="4-boot-the-fvp-into-non-secure-world"></a>
To run the FVP, we use the helper script `./scripts/run_fvp.sh` which configures
the FVP to run a CCA-enabled kernel.

The following script launches the FVP with build artifacts found in the snapshot
directory `./assets/snapshots`.

```
./buildconf/tfa/setup.sh run_linux

# this will execute run_fvp.sh as follows:
#     run_fvp.sh \
#        --bl1=<./assets/snapshots/bl1.bin> \
#        --fip=<./assets/snapshots/fip.bin> \
#        --kernel=<./assets/snapshots/Image-cca> \
#        --rootfs=<./assets/snapshots/rootfs-ns-ext2> \
#        --p9=<./> \
#        [--hook=<./path to libc allocator (see pci bypass)>]
```
- If you grant root access, the FVP can pin its pages such that it is compatible
  with the pcie-bypass mechanism.

- You may run this setup without a patched host kernel (faulthook patch set). In this case the FVP can not interact with devices on the x86 host machine. 

## 2. Navigation in Non-Secure World <a name="5-navigation-in-non-secure-world"></a>

**Root Shell.** Login with user root without password to obtain a root shell.

**Binaries.** We bundle a bunch of binaries into the root file system of the
Non-Secure world kernel. The binaries are configured in the build configuration:

```
./buildconf/linux-cca-guest/buildroot/buildroot_config_fragment_aarch64
```

The file is the fragment of buildroot configuration changes compared to the base
configuration found in `./ext/buildroot/configs/qemu_aarch64_virt_defconfig`

**Directory Sharing.** We configure 9P directory sharing such that the project
source directory is mounted in Non-Secure world in `/mnt/host`. Within a realm
VM, we mount the directory into `/mnt/mnt/host.`


## 3. Boot a realm VM <a name="6-boot-a-realm-vm"></a>
To finally boot a realm VM from Non-Secure world, employ the boot script located
in:

```
/mnt/host/scripts/run_realm.sh
```

The script uses the kernel image and rootfs located in `./assets/snapshots` to
boot a realm VM.

We use kvmtools as the VMM. If you configured the PCIe-bypass mechanism, you may
set bootflag `fvp_esccape_on` such that the realm VM's memory is mapped and can
be accessed from within the FVP's host process. If you do not wish to enable the
bypass mechanism, setting `fvp_escape_off` can decrease boot time.

Depending on the realm VM's memory size (default 1 GB), and host system, booting
may take ca. 30 minutes.

- To switch between realm VM and Non-Secure kernel, launch the above script in a
  screen session.
- Change the Ram size in the run script to speed up VM boot (e.g. 256 MB instead)


