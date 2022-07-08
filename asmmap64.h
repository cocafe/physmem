#ifndef PHYSRAM_ASMMAP64_H
#define PHYSRAM_ASMMAP64_H

#include <stdint.h>

#define ASMMAP_IOCTL_MAPMEM             0x9C402580
#define ASMMAP_IOCTL_UNMAPMEM           0x9C402584

struct asmmap_ioctl {
        uint64_t phys_addr;
        uint8_t *virt_addr;
        uint32_t length[2];
};

int asmmap64_open(void);
int asmmap64_close(void);
uint8_t *asmmap64_mmap(uint64_t phys_addr, uint32_t length);
int asmmap64_unmmap(uint8_t *virt_addr, uint32_t length);

#endif //PHYSRAM_ASMMAP64_H
