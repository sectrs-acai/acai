/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#ifndef _XDMA_IOCALLS_POSIX_H_
#define _XDMA_IOCALLS_POSIX_H_

#include <linux/ioctl.h>

/* Use 'x' as magic number */
#define XDMA_IOC_MAGIC    'x'
/* XL OpenCL X->58(ASCII), L->6C(ASCII), O->0 C->C L->6C(ASCII); */
#define XDMA_XCL_MAGIC 0X586C0C6C

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G and S atomically
 * H means "sHift": switch T and Q atomically
 *
 * _IO(type,nr)		    no arguments
 * _IOR(type,nr,datatype)   read data from driver
 * _IOW(type,nr.datatype)   write data to driver
 * _IORW(type,nr,datatype)  read/write data
 *
 * _IOC_DIR(nr)		    returns direction
 * _IOC_TYPE(nr)	    returns magic
 * _IOC_NR(nr)		    returns number
 * _IOC_SIZE(nr)	    returns size
 */

enum XDMA_IOC_TYPES {
    XDMA_IOC_NOP,
    XDMA_IOC_INFO,
    XDMA_IOC_OFFLINE,
    XDMA_IOC_ONLINE,
    XDMA_IOC_MAX,

    // faulthook
    XDMA_IOC_MMAP,
    XDMA_IOC_UNMMAP,
};

struct xdma_ioc_base {
    unsigned int magic;
    unsigned int command;
};

struct xdma_ioc_info {
    struct xdma_ioc_base base;
    unsigned short vendor;
    unsigned short device;
    unsigned short subsystem_vendor;
    unsigned short subsystem_device;
    unsigned int dma_engine_version;
    unsigned int driver_version;
    unsigned long long feature_id;
    unsigned short domain;
    unsigned char bus;
    unsigned char dev;
    unsigned char func;
};


struct xdma_ioc_faulthook_mmap {
    struct xdma_ioc_base base;
    pid_t pid;
    unsigned long vm_pgoff;
    unsigned long vm_flags;
    unsigned long vm_page_prot;
    unsigned long addr_size;
    unsigned long *addr;

};

struct xdma_ioc_faulthook_unmmap {
    pid_t pid;
    unsigned long addr_size;
    unsigned long *addr;

};

/* IOCTL codes */
#define XDMA_IOCINFO        _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_INFO, \
                    struct xdma_ioc_info)

#define XDMA_IOCMMAP        _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_MMAP, \
                    struct xdma_ioc_faulthook_mmap)

#define XDMA_IOCUNMMAP        _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_UNMMAP, \
                    struct xdma_ioc_faulthook_unmmap)

#define XDMA_IOCOFFLINE        _IO(XDMA_IOC_MAGIC, XDMA_IOC_OFFLINE)
#define XDMA_IOCONLINE        _IO(XDMA_IOC_MAGIC, XDMA_IOC_ONLINE)

#define IOCTL_XDMA_ADDRMODE_SET    _IOW('q', 4, int)
#define IOCTL_XDMA_ADDRMODE_GET    _IOR('q', 5, int)
#define IOCTL_XDMA_ALIGN_GET    _IOR('q', 6, int)

#endif /* _XDMA_IOCALLS_POSIX_H_ */
