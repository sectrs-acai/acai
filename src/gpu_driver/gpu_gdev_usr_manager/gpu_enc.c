#include "gdev_api.h"
#include "gpu_enc.h"

inline uint64_t enc__gmalloc(Ghandle h, uint64_t size)
{
    return gmalloc(h, size);
}

inline uint64_t enc__gfree(Ghandle h, uint64_t addr)
{
    return gfree(h, addr);
}

inline void *enc__gmalloc_dma(Ghandle h, uint64_t size)
{
    return gmalloc_dma(h, size);
}

inline uint64_t enc__gfree_dma(Ghandle h, void *buf)
{
    return gfree_dma(h, buf);
}

inline int enc__gmemcpy_to_device(Ghandle h, uint64_t dst_addr, const void *src_buf, uint64_t size)
{
    return gmemcpy_to_device(h, dst_addr, src_buf, size);
}

inline int enc__gmemcpy_from_device(Ghandle h, void *dst_buf, uint64_t src_addr, uint64_t size)
{
    return gmemcpy_from_device(h, dst_buf, src_addr, size);
}

inline int enc__glaunch(Ghandle h, struct gdev_kernel *kernel, uint32_t *id)
{
    return glaunch(h, kernel, id);
}
