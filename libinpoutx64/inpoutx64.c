#include <stdint.h>
#include <errno.h>

#include <windows.h>
#include <processthreadsapi.h>

#include "inpoutx64.h"

//Functions exported from DLL.
//For easy inclusion is user projects.
//Original InpOut32 function support
void	_stdcall (*Out32)(short PortAddress, short data);
short	_stdcall (*Inp32)(short PortAddress);

//My extra functions for making life easy
BOOL	_stdcall (*IsInpOutDriverOpen)();  //Returns TRUE if the InpOut driver was opened successfully
BOOL	_stdcall (*IsXP64Bit)();			//Returns TRUE if the OS is 64bit (x64) Windows.

//DLLPortIO function support
UCHAR   _stdcall (*DlPortReadPortUchar) (USHORT port);
void    _stdcall (*DlPortWritePortUchar)(USHORT port, UCHAR Value);

USHORT  _stdcall (*DlPortReadPortUshort) (USHORT port);
void    _stdcall (*DlPortWritePortUshort)(USHORT port, USHORT Value);

ULONG	_stdcall (*DlPortReadPortUlong)(ULONG port);
void	_stdcall (*DlPortWritePortUlong)(ULONG port, ULONG Value);

//WinIO function support (Untested and probably does NOT work - esp. on x64!)
PBYTE	_stdcall (*MapPhysToLin)(PBYTE pbPhysAddr, DWORD dwPhysSize, HANDLE *pPhysicalMemoryHandle);
BOOL	_stdcall (*UnmapPhysicalMemory)(HANDLE PhysicalMemoryHandle, PBYTE pbLinAddr);
BOOL	_stdcall (*GetPhysLong)(PBYTE pbPhysAddr, PDWORD pdwPhysVal);
BOOL	_stdcall (*SetPhysLong)(PBYTE pbPhysAddr, DWORD dwPhysVal);

static int inpoutx64_loaded = 0;
static HMODULE inpoutx64_dll;

struct symbol {
        char *name;
        void *ptr;
};

struct symbol symbol_list[] = {
        { "Out32",                      &Out32 },
        { "Inp32",                      &Inp32 },
        { "IsInpOutDriverOpen",         &IsInpOutDriverOpen },
        { "IsXP64Bit",                  &IsXP64Bit },
        { "DlPortReadPortUchar",        &DlPortReadPortUchar },
        { "DlPortWritePortUchar",       &DlPortWritePortUchar },
        { "DlPortReadPortUshort",       &DlPortReadPortUshort },
        { "DlPortWritePortUshort",      &DlPortWritePortUshort },
        { "DlPortReadPortUlong",        &DlPortReadPortUlong },
        { "DlPortWritePortUlong",       &DlPortWritePortUlong },
        { "MapPhysToLin",               &MapPhysToLin },
        { "UnmapPhysicalMemory",        &UnmapPhysicalMemory },
        { "GetPhysLong",                &GetPhysLong },
        { "SetPhysLong",                &SetPhysLong },
};

int inpoutx64_load(void)
{
        int err = 0;

        inpoutx64_dll = LoadLibrary(L"inpoutx64.dll");
        if (inpoutx64_dll == NULL) {
                return -ENODATA;
        }

        for (size_t i = 0; i < sizeof(symbol_list) / sizeof(symbol_list[0]); i++) {
                struct symbol *sym = &symbol_list[i];

                *((intptr_t *)(sym->ptr)) = (intptr_t)GetProcAddress(inpoutx64_dll, sym->name);

                if (*((intptr_t *)(sym->ptr)) == (intptr_t)NULL) {
                        err = -ENOENT;
                        goto err;
                }
        }

        return err;

err:
        FreeLibrary(inpoutx64_dll);

        return err;
}

int is_inpoutx64_driver_open(void)
{
        if (!inpoutx64_loaded)
                return 0;

        if (IsInpOutDriverOpen() == TRUE)
                return 1;

        return 0;
}

int inpoutx64_physram_read(intptr_t phys_addr, size_t size, uint8_t *bytes)
{
        HANDLE hdl;
        uint8_t *virt_addr;

        if (!inpoutx64_loaded)
                return -ENOENT;

        virt_addr = MapPhysToLin((PBYTE)phys_addr, size, &hdl);
        if (virt_addr == 0)
                return -EIO;

        memcpy(bytes, virt_addr, size);

        UnmapPhysicalMemory(hdl, virt_addr);

        return 0;
}

int inpoutx64_physram_write(intptr_t phys_addr, size_t size, uint8_t *bytes)
{
        HANDLE hdl;
        uint8_t *virt_addr;

        if (!inpoutx64_loaded)
                return -ENOENT;

        virt_addr = MapPhysToLin((PBYTE)phys_addr, size, &hdl);
        if (virt_addr == 0)
                return -EIO;

        memcpy(virt_addr, bytes, size);

        UnmapPhysicalMemory(hdl, virt_addr);

        return 0;
}

int inpoutx64_init(void)
{
        int err = 0;

        if ((err = inpoutx64_load()))
                return err;

        inpoutx64_loaded = 1;

        return err;
}

void inpoutx64_deinit(void)
{
        inpoutx64_loaded = 0;

        if (inpoutx64_dll)
                FreeLibrary(inpoutx64_dll);
}