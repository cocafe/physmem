#include <stdint.h>

#include <windows.h>
#include <winternl.h>
#include <winerror.h>

#include "asmmap64.h"
#include "logging.h"

static HANDLE asmmap64_drv = INVALID_HANDLE_VALUE;

int asmmap64_open(void)
{
        HANDLE drv;

        if (asmmap64_drv != INVALID_HANDLE_VALUE)
                return -EEXIST;

        drv = CreateFile(L"\\\\.\\ASMMAP64",
                         GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE,
                         0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, 0);

        if (drv == INVALID_HANDLE_VALUE)
                return -ENOENT;

        asmmap64_drv = drv;

        return 0;
}

int asmmap64_close(void)
{
        if (asmmap64_drv == INVALID_HANDLE_VALUE)
                return -ENOENT;

        CloseHandle(asmmap64_drv);
        asmmap64_drv = INVALID_HANDLE_VALUE;

        return 0;
}

uint8_t *asmmap64_mmap(uint64_t phys_addr, uint32_t length)
{
        struct asmmap_ioctl ioctl = {
                .phys_addr = phys_addr,
                .virt_addr = NULL,
                .length = { length, length },
        };
        DWORD bytes_returned;

        if (asmmap64_drv == INVALID_HANDLE_VALUE)
                return NULL;

        if (0 == DeviceIoControl(asmmap64_drv,
                                 ASMMAP_IOCTL_MAPMEM,
                                 &ioctl,
                                 sizeof(ioctl),
                                 &ioctl,
                                 sizeof(ioctl),
                                 &bytes_returned,
                                 0)) {
                pr_err("DeviceIoControl() failed, err = %ld\n", GetLastError());
                return NULL;
        }

        return ioctl.virt_addr;
}

int asmmap64_unmmap(uint8_t *virt_addr, uint32_t length)
{
        struct asmmap_ioctl ioctl = {
                .phys_addr = 0,
                .virt_addr = virt_addr,
                .length = { length, length },
        };
        DWORD bytes_returned;

        if (!virt_addr)
                return -EINVAL;

        if (asmmap64_drv == INVALID_HANDLE_VALUE)
                return -ENODEV;

        if (0 == DeviceIoControl(asmmap64_drv,
                                 ASMMAP_IOCTL_UNMAPMEM,
                                 &ioctl,
                                 sizeof(ioctl),
                                 &ioctl,
                                 sizeof(ioctl),
                                 &bytes_returned,
                                 0)) {
                pr_err("DeviceIoControl() failed, err = %ld\n", GetLastError());
                return -EIO;
        }

        return 0;
}
