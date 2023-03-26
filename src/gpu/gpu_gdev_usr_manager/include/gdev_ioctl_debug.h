//
// Created by b on 3/26/23.
//

#ifndef MOD_STUB_GDEV_FH_KERNEL_FH_GDEV_DEBUG_H_
#define MOD_STUB_GDEV_FH_KERNEL_FH_GDEV_DEBUG_H_
#include "gdev_ioctl_def.h"
#define PRIu64 "lx"
#define PRIu32 "lx"

#ifdef __KERNEL__
#define do_print printk
#else
#define do_print printf
#endif


static void debug_gdev_ioctl_handle(struct gdev_ioctl_handle handle) {
    do_print("gdev_ioctl_handle { handle = %" PRIu64 " }\n", handle.handle);
}

static void debug_gdev_ioctl_mem(struct gdev_ioctl_mem mem) {
    do_print("gdev_ioctl_mem { addr = %" PRIu64 ", size = %" PRIu64 " }\n", mem.addr, mem.size);
}

static void debug_gdev_ioctl_dma(struct gdev_ioctl_dma dma) {
    do_print("gdev_ioctl_dma { src_buf = %p, dst_buf = %p, src_addr = %" PRIu64 ", dst_addr = %" PRIu64 ", size = %" PRIu64 ", id = %p }\n",
            dma.src_buf, dma.dst_buf, dma.src_addr, dma.dst_addr, dma.size, dma.id);
}

static void debug_gdev_ioctl_launch(struct gdev_ioctl_launch launch) {
    do_print("gdev_ioctl_launch { kernel = %p, id = %p }\n", launch.kernel, launch.id);
}

static void debug_gdev_ioctl_sync(struct gdev_ioctl_sync sync) {
    do_print("gdev_ioctl_sync { id = %" PRIu32 ", timeout = %p }\n", sync.id, sync.timeout);
}

static void debug_gdev_ioctl_query(struct gdev_ioctl_query query) {
    do_print("gdev_ioctl_query { type = %" PRIu32 ", result = %" PRIu64 " }\n", query.type, query.result);
}

static void debug_gdev_ioctl_tune(struct gdev_ioctl_tune tune) {
    do_print("gdev_ioctl_tune { type = %" PRIu32 ", value = %" PRIu32 " }\n", tune.type, tune.value);
}

static void debug_gdev_ioctl_shm(struct gdev_ioctl_shm shm) {
    do_print("gdev_ioctl_shm { key = %d, id = %d, flags = %d, cmd = %d, addr = %" PRIu64 ", size = %" PRIu64 ", buf = %p }\n",
            shm.key, shm.id, shm.flags, shm.cmd, shm.addr, shm.size, shm.buf);
}

static void debug_gdev_ioctl_map(struct gdev_ioctl_map map) {
    do_print("gdev_ioctl_map { addr = %" PRIu64 ", buf = %" PRIu64 ", size = %" PRIu64 " }\n", map.addr, map.buf, map.size);
}

static void debug_gdev_ioctl_ref(struct gdev_ioctl_ref ref) {
    do_print("gdev_ioctl_ref { addr = %" PRIu64 ", size = %" PRIu64 ", handle_slave = %" PRIu64 ", addr_slave = %" PRIu64 " }\n",
            ref.addr, ref.size, ref.handle_slave, ref.addr_slave);
}

static void debug_gdev_ioctl_unref(struct gdev_ioctl_unref unref) {
    do_print("gdev_ioctl_unref { addr = %" PRIu64 " }\n", unref.addr);
}

static void debug_gdev_ioctl_phys(struct gdev_ioctl_phys phys) {
    do_print("gdev_ioctl_phys { addr = %" PRIu64 ", phys = %" PRIu64 " }\n", phys.addr, phys.phys);
}

static void debug_gdev_ioctl_virt(struct gdev_ioctl_virt virt) {
    do_print("gdev_ioctl_virt { addr = %" PRIu64 ", virt = %" PRIu64 " }\n", virt.addr, virt.virt);
}

static const char* debug_ioctl_cmd_name(int cmd)
{
    switch (cmd) {
        case GDEV_IOCTL_GET_HANDLE:
            return "GDEV_IOCTL_GET_HANDLE";
        case GDEV_IOCTL_GMALLOC:
            return "GDEV_IOCTL_GMALLOC";
        case GDEV_IOCTL_GFREE:
            return "GDEV_IOCTL_GFREE";
        case GDEV_IOCTL_GMALLOC_DMA:
            return "GDEV_IOCTL_GMALLOC_DMA";
        case GDEV_IOCTL_GFREE_DMA:
            return "GDEV_IOCTL_GFREE_DMA";
        case GDEV_IOCTL_GMAP:
            return "GDEV_IOCTL_GMAP";
        case GDEV_IOCTL_GUNMAP:
            return "GDEV_IOCTL_GUNMAP";
        case GDEV_IOCTL_GMEMCPY_TO_DEVICE:
            return "GDEV_IOCTL_GMEMCPY_TO_DEVICE";
        case GDEV_IOCTL_GMEMCPY_TO_DEVICE_ASYNC:
            return "GDEV_IOCTL_GMEMCPY_TO_DEVICE_ASYNC";
        case GDEV_IOCTL_GMEMCPY_FROM_DEVICE:
            return "GDEV_IOCTL_GMEMCPY_FROM_DEVICE";
        case GDEV_IOCTL_GMEMCPY_FROM_DEVICE_ASYNC:
            return "GDEV_IOCTL_GMEMCPY_FROM_DEVICE_ASYNC";
        case GDEV_IOCTL_GMEMCPY:
            return "GDEV_IOCTL_GMEMCPY";
        case GDEV_IOCTL_GMEMCPY_ASYNC:
            return "GDEV_IOCTL_GMEMCPY_ASYNC";
        case GDEV_IOCTL_GLAUNCH:
            return "GDEV_IOCTL_GLAUNCH";
        case GDEV_IOCTL_GSYNC:
            return "GDEV_IOCTL_GSYNC";
        case GDEV_IOCTL_GBARRIER:
            return "GDEV_IOCTL_GBARRIER";
        case GDEV_IOCTL_GQUERY:
            return "GDEV_IOCTL_GQUERY";
        case GDEV_IOCTL_GTUNE:
            return "GDEV_IOCTL_GTUNE";
        case GDEV_IOCTL_GSHMGET:
            return "GDEV_IOCTL_GSHMGET";
        case GDEV_IOCTL_GSHMAT:
            return "GDEV_IOCTL_GSHMAT";
        case GDEV_IOCTL_GSHMDT:
            return "GDEV_IOCTL_GSHMDT";
        case GDEV_IOCTL_GSHMCTL:
            return "GDEV_IOCTL_GSHMCTL";
        case GDEV_IOCTL_GREF:
            return "GDEV_IOCTL_GREF";
        case GDEV_IOCTL_GUNREF:
            return "GDEV_IOCTL_GUNREF";
        case GDEV_IOCTL_GPHYSGET:
            return "GDEV_IOCTL_GPHYSGET";
        case GDEV_IOCTL_GVIRTGET:
            return "GDEV_IOCTL_GVIRTGET";
        default:
            return "UNKNOWN_IOCTL_COMMAND";
    }
}

#endif //MOD_STUB_GDEV_FH_KERNEL_FH_GDEV_DEBUG_H_
