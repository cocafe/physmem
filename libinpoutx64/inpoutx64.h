#ifndef __LIB_INPOUTX64_H__
#define __LIB_INPOUTX64_H__

#include <windows.h>

extern void _stdcall (*Out32)(short PortAddress, short data);
extern short _stdcall (*Inp32)(short PortAddress);
extern BOOL _stdcall (*IsInpOutDriverOpen)();
extern BOOL _stdcall (*IsXP64Bit)();
extern UCHAR _stdcall (*DlPortReadPortUchar) (USHORT port);
extern void _stdcall (*DlPortWritePortUchar)(USHORT port, UCHAR Value);
extern USHORT _stdcall (*DlPortReadPortUshort) (USHORT port);
extern void _stdcall (*DlPortWritePortUshort)(USHORT port, USHORT Value);
extern ULONG _stdcall (*DlPortReadPortUlong)(ULONG port);
extern void _stdcall (*DlPortWritePortUlong)(ULONG port, ULONG Value);
extern PBYTE _stdcall (*MapPhysToLin)(PBYTE pbPhysAddr, DWORD dwPhysSize, HANDLE *pPhysicalMemoryHandle);
extern BOOL _stdcall (*UnmapPhysicalMemory)(HANDLE PhysicalMemoryHandle, PBYTE pbLinAddr);
extern BOOL _stdcall (*GetPhysLong)(PBYTE pbPhysAddr, PDWORD pdwPhysVal);
extern BOOL _stdcall (*SetPhysLong)(PBYTE pbPhysAddr, DWORD dwPhysVal);

int is_inpoutx64_driver_open(void);
int inpoutx64_physram_read(intptr_t phys_addr, size_t size, uint8_t *bytes);
int inpoutx64_physram_write(intptr_t phys_addr, size_t size, uint8_t *bytes);
int inpoutx64_init(void);
void inpoutx64_deinit(void);

#endif // __LIB_INPOUTX64_H__