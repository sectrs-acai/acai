#pragma once
/*
 * if GPU__GDEV_BUS_ENC is set,
 * we use wrapper functions which do gpu encryption
 * on x86 side. This should account for a bus-encryption style
 * encryption approach, as an alternative to pure software encryption
 * within the FVP. If unset we forward call to gdev.
 */
#ifdef GPU__GDEV_BUS_ENC
#define gpu__gopen enc__gopen
#define gpu__gclose enc__gclose
#define gpu__gmalloc enc__gmalloc
#define gpu__gfree enc__gfree
#define gpu__gmalloc_dma enc__gmalloc_dma
#define gpu__gfree_dma enc__gfree_dma
#define gpu__gmemcpy_to_device enc__gmemcpy_to_device
#define gpu__gmemcpy_from_device enc__gmemcpy_from_device
#define gpu__glaunch enc__glaunch

uint64_t enc__gmalloc(Ghandle h, uint64_t size);
uint64_t enc__gfree(Ghandle h, uint64_t addr);
void *enc__gmalloc_dma(Ghandle h, uint64_t size);
uint64_t enc__gfree_dma(Ghandle h, void *buf);
int enc__gmemcpy_to_device(Ghandle h, uint64_t dst_addr, const void *src_buf, uint64_t size);
int enc__gmemcpy_from_device(Ghandle h, void *dst_buf, uint64_t src_addr, uint64_t size);
int enc__glaunch(Ghandle h, struct gdev_kernel *kernel, uint32_t *id);
Ghandle enc__gopen(int minor);
int enc__gclose(Ghandle h);

#else
#define gpu__gopen gopen
#define gpu__gclose gclose
#define gpu__gmalloc gmalloc
#define gpu__gfree gfree
#define gpu__gmalloc_dma gmalloc_dma
#define gpu__gfree_dma gfree_dma
#define gpu__gmemcpy_to_device gmemcpy_to_device
#define gpu__gmemcpy_from_device gmemcpy_from_device
#define gpu__glaunch glaunch
#endif

#define gpu__gunmap gunmap
#define gpu__gmemcpy_to_device_async gmemcpy_to_device_async
#define gpu__gmemcpy_user_to_device gmemcpy_user_to_device
#define gpu__gmemcpy_user_to_device_async gmemcpy_user_to_device_async
#define gpu__gmemcpy_from_device_async gmemcpy_from_device_async
#define gpu__gmemcpy_user_from_device gmemcpy_user_from_device
#define gpu__gmemcpy_user_from_device_async gmemcpy_user_from_device_async
#define gpu__gmemcpy gmemcpy
#define gpu__gmemcpy_async gmemcpy_async
#define gpu__gsync gsync
#define gpu__gbarrier gbarrier
#define gpu__gquery gquery
#define gpu__gtune gtune
#define gpu__gshmget gshmget
#define gpu__gshmat gshmat
#define gpu__gshmdt gshmdt
#define gpu__gshmctl gshmctl
#define gpu__gref gref
#define gpu__gunref gunref
#define gpu__gphysget gphysget
#define gpu__gvirtget gvirtget