#ifndef FPGA_CCA_BENCHMARK_H_
#define FPGA_CCA_BENCHMARK_H_

#define STR(s) #s
#define CCA_MARKER(marker) __asm__ volatile("MOV XZR, " STR(marker))

/* file operation call from userspace */
#define CCA_MARKER_DRIVER_FOP CCA_MARKER(0x90);

#define CCA_MARKER_DMA_PAGE_READ(pages_nr) \
for(unsigned long _i = 0; _i < pages_nr; _i ++) { \
CCA_MARKER(0x91); \
}

#define CCA_MARKER_DMA_PAGE_WRITE(pages_nr) \
for(unsigned long _i = 0; _i < pages_nr; _i ++) { \
CCA_MARKER(0x92); \
}

#define CCA_MARKER_MMAP_PAGE(pages_nr) \
for(unsigned long _i = 0; _i < pages_nr; _i ++) { \
CCA_MARKER(0x93); \
}

#define CCA_MARKER_PIN_USER_PAGES_READ(pages_nr) \
for(unsigned long _i = 0; _i < pages_nr; _i ++) { \
CCA_MARKER(0x94); \
}

#define CCA_MARKER_PIN_USER_PAGES_WRITE(pages_nr) \
for(unsigned long _i = 0; _i < pages_nr; _i ++) { \
CCA_MARKER(0x95); \
}

#endif
