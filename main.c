#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>
#include <winternl.h>
#include <winerror.h>
#include <psapi.h>

#include "getopt.h"
#include "logging.h"

#include "libinpoutx64/inpoutx64.h"

#define DRV_FILE        L"inpoutx64.sys"
#define NTDRVLDR_EXE    L"ntdrvldr.exe"

enum {
        CMD_READ = 0,
        CMD_READBLK,
        CMD_WRITE,
        CMD_WRITEBLK,
        CMD_DRV_INSTALL,
        CMD_DRV_REMOVE,
        NUM_OPT_CMDS,
};

static wchar_t driver_path[MAX_PATH];
static wchar_t ntdrvldr_path[MAX_PATH];

static uint32_t mmap_sz = 8;
static uint32_t rw_sz = 0;
static uint64_t rw_addr;
static uint64_t rw_value;
static uint8_t *wr_bytes;
static int no_readback = 0;

static int hexdump_print = 0;

static int cmd = -1;

static char *help_text =
"Usage:\n"
"       physmem.exe [options] read8    <addr>\n"
"       physmem.exe [options] read16   <addr>\n"
"       physmem.exe [options] read32   <addr>\n"
"       physmem.exe [options] read64   <addr>\n"
"       physmem.exe [options] readblk  <addr> <bytes>\n"
"       physmem.exe [options] write8   <addr> <value>\n"
"       physmem.exe [options] write16  <addr> <value>\n"
"       physmem.exe [options] write32  <addr> <value>\n"
"       physmem.exe [options] write64  <addr> <value>\n"
"       physmem.exe [options] writeblk <addr> <bytes> <byte0> <byte1> ...\n"
"       physmem.exe [options] driver install\n"
"       physmem.exe [options] driver remove\n"
"Options:\n"
"       -h              this help text\n"
"       -v              verbose print\n"
"       -s              no readback after writing\n"
"       -m <bytes>      mmap size, default: 8\n"
"       -C              hexdump style print\n"
"Driver:\n"
"       Install Manually:"
"               ntdrvldr.exe -n inpoutx64 ABSOLUTE_PATH_TO\\inpoutx64.sys\n"
"\n"
"       Uninstall Manually:"
"               ntdrvldr.exe -u -n inpoutx64 ABSOLUTE_PATH_TO\\inpoutx64.sys\n"
;

PBYTE _stdcall (*_MapPhysToLin)(PBYTE pbPhysAddr, DWORD dwPhysSize, HANDLE *pPhysicalMemoryHandle);
BOOL _stdcall (*_UnmapPhysicalMemory)(HANDLE PhysicalMemoryHandle, PBYTE pbLinAddr);
BOOL _stdcall (*_GetPhysLong)(PBYTE pbPhysAddr, PDWORD pdwPhysVal);
BOOL _stdcall (*_SetPhysLong)(PBYTE pbPhysAddr, DWORD dwPhysVal);

static void hexdump(const void *data, size_t size, uint64_t prefix_addr) {
        char ascii[17];
        size_t i, j;
        ascii[16] = '\0';

        for (i = 0; i < size; ++i) {
                if (i % 16 == 0)
                        printf("%016jx | ", prefix_addr + i);

                printf("%02X ", ((uint8_t *)data)[i]);
                if (((uint8_t *)data)[i] >= ' ' && ((uint8_t *)data)[i] <= '~') {
                        ascii[i % 16] = ((uint8_t *)data)[i];
                } else {
                        ascii[i % 16] = '.';
                }
                if ((i + 1) % 8 == 0 || i + 1 == size) {
                        printf(" ");
                        if ((i + 1) % 16 == 0) {
                                printf("|  %s \n", ascii);
                        } else if (i + 1 == size) {
                                ascii[(i + 1) % 16] = '\0';
                                if ((i + 1) % 16 <= 8) {
                                        printf(" ");
                                }
                                for (j = (i + 1) % 16; j < 16; ++j) {
                                        printf("   ");
                                }
                                printf("|  %s \n", ascii);
                        }
                }
        }
}

static int path_fix(wchar_t *abs_path, size_t cnt, wchar_t *filename)
{
        wchar_t image_path[MAX_PATH] = { };
        wchar_t drive_letter[16] = { };
        wchar_t dir_path[MAX_PATH] = { };
        DWORD image_cnt = ARRAY_SIZE(image_path);

        if (0 == QueryFullProcessImageName(GetCurrentProcess(), 0, image_path, &image_cnt)) {
                pr_err("QueryFullProcessImageName() failed, err = %ld\n", GetLastError());
                return -EINVAL;
        }

        _wsplitpath(image_path, drive_letter, dir_path, NULL, NULL);

        memset(abs_path, 0x00, sizeof(wchar_t) * cnt);
        snwprintf(abs_path, cnt, L"%ls%ls%ls", drive_letter, dir_path, filename);

        return 0;
}

static void help(void)
{
        fprintf(stderr, "%s", help_text);
}

static int parse_wargs(int wargc, wchar_t *wargv[])
{
        int c;

        while ((c = getopt_w(wargc, wargv, L"hm:vsC")) != -1) {
                switch (c) {
                case 'h':
                        help();
                        exit(0);

                case 'm':
                        mmap_sz = wcstoul(optarg_w, NULL, 10);
                        if (mmap_sz == 0) {
                                pr_err("invalid mmap size: %ls\n", optarg_w);
                                return -EINVAL;
                        }

                        break;

                case 'v':
                        g_logprint_level |= LOG_LEVEL_VERBOSE;
                        break;

                case 's':
                        no_readback = 1;
                        break;

                case 'C':
                        hexdump_print = 1;
                        break;

                case '?':
                        if (optopt_w == 'm')
                                pr_err("option -m needs argument\n");
                        else
                                pr_err("unknown option: -%c\n", optopt_w);

                        return -EINVAL;
                }
        }

        // for (int i = optind_w; i < wargc; i++) {
        //      pr_err("non-option args: %ls\n", wargv[i]);
        // }

        int i = optind_w;

        if (is_wstr_equal(wargv[i], L"driver")) {
                if ((wargc - 1) - i != 1)
                        goto invalid_args;

                if (is_wstr_equal(wargv[i + 1], L"install"))
                        cmd = CMD_DRV_INSTALL;
                else if (is_wstr_equal(wargv[i + 1], L"remove"))
                        cmd = CMD_DRV_REMOVE;
                else
                        goto invalid_args;
        } else if (is_wstr_equal(wargv[i], L"readblk")) {
                uint32_t sz;
                uint64_t addr;

                if ((wargc - 1) - i != 2)
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 1], L"%jx", &addr))
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 2], L"%u", &sz))
                        goto invalid_args;

                cmd = CMD_READBLK;
                rw_sz = sz;
                rw_addr = addr;
                mmap_sz = sz;
        } else if (is_wstr_equal(wargv[i], L"writeblk")) {
                uint32_t sz;
                uint64_t addr;

                if ((wargc - 1) - i < 2)
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 1], L"%jx", &addr))
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 2], L"%u", &sz))
                        goto invalid_args;

                if (((wargc - 1) - i - 2) != (int)sz) {
                        pr_err("incorrect length of byte sequence\n");
                        return -EINVAL;
                }

                wr_bytes = calloc(sz, sizeof(uint8_t));
                if (!wr_bytes)
                        return -ENOMEM;

                for (size_t j = i + 3, k = 0; j < (size_t)wargc && k < sz; j++) {
                        if (1 != swscanf(wargv[j], L"%hhx", &wr_bytes[k++])) {
                                pr_err("\"%ls\" is not hex value\n", wargv[j]);
                                free(wr_bytes);

                                return -EINVAL;
                        }
                }

                cmd = CMD_WRITEBLK;
                rw_sz = sz;
                rw_addr = addr;
                mmap_sz = sz;

                if (g_logprint_level & LOG_LEVEL_VERBOSE) {
                        pr_rawlvl(VERBOSE, "bytes to write:\n");
                        hexdump(wr_bytes, rw_sz, 0);
                }
        } else if (!wcsncmp(wargv[i], L"read", 4)) {
                uint64_t a;
                uint32_t w;

                if (1 != swscanf(wargv[i], L"read%u", &w))
                        goto invalid_cmd;

                if (w == 0 || (w != 8 && w != 16 && w != 32 && w != 64))
                        goto invalid_cmd;

                if ((wargc - 1) - i != 1)
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 1], L"%jx", &a))
                        goto invalid_args;

                cmd = CMD_READ;
                rw_sz = w / 8;
                rw_addr = a;
        } else if (!wcsncmp(wargv[i], L"write", 5)) {
                uint64_t a, v;
                uint32_t w;

                if (1 != swscanf(wargv[i], L"write%u", &w))
                        goto invalid_cmd;

                if (w == 0 || (w != 8 && w != 16 && w != 32 && w != 64))
                        goto invalid_cmd;

                if ((wargc - 1) - i != 2)
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 1], L"%jx", &a))
                        goto invalid_args;

                if (1 != swscanf(wargv[i + 2], L"%jx", &v))
                        goto invalid_args;

                cmd = CMD_WRITE;
                rw_sz = w / 8;
                rw_addr = a;
                rw_value = v;

                pr_rawlvl(VERBOSE, "write value: 0x%016jx\n", rw_value);
        } else {
                return -EINVAL;
        }

        return 0;

invalid_cmd:
                pr_err("invalid command: %ls\n", wargv[i]);
                return -EINVAL;

invalid_args:
                pr_err("invalid args for %ls\n", wargv[i]);
                return -EINVAL;
}

static void addr_printf(uint64_t addr, size_t sz, uint64_t val)
{
        char _fmt[64] = { };

        sprintf(_fmt, "0x%%016jx 0x%%0%llujx\n", sz * 2);

        printf(_fmt, addr, val);
}

static int physmem_rw(void)
{
        HANDLE map_hdl;
        uint8_t *virt_addr;

        if (rw_sz == 0)
                return 0;

        virt_addr = MapPhysToLin((PBYTE)rw_addr, mmap_sz, &map_hdl);
        if (virt_addr == NULL) {
                pr_err("failed to mmap phys addr 0x%016jx\n", rw_addr);
                return -EIO;
        }

        pr_rawlvl(VERBOSE, "read/write size: %u byte(s)\n", rw_sz);
        pr_rawlvl(VERBOSE, "phys addr: 0x%016jx\n", rw_addr);
        pr_rawlvl(VERBOSE, "virt addr: 0x%016jx\n", (intptr_t)virt_addr);

        switch (cmd) {
        case CMD_READBLK:
                hexdump(virt_addr, rw_sz, rw_addr);
                break;

        case CMD_WRITEBLK:
                for (size_t i = 0; i < rw_sz; i++) {
                        virt_addr[i] = wr_bytes[i];
                }

                if (!no_readback) {
                        hexdump(virt_addr, rw_sz, rw_addr);
                }

                free(wr_bytes);

                break;

        case CMD_READ:
                if (!hexdump_print) {
                        ptr_byte_read(virt_addr, rw_sz, &rw_value);
                        addr_printf(rw_addr, rw_sz, rw_value);
                } else {
                        hexdump(virt_addr, rw_sz, rw_addr);
                }

                break;

        case CMD_WRITE:
                ptr_byte_write(virt_addr, rw_sz, rw_value);

                if (!no_readback) {
                        if (!hexdump_print) {
                                ptr_byte_read(virt_addr, rw_sz, &rw_value);
                                addr_printf(rw_addr, rw_sz, rw_value);
                        } else {
                                hexdump(virt_addr, rw_sz, rw_addr);
                        }
                }

                break;

        default:
                break;
        }

        UnmapPhysicalMemory(map_hdl, virt_addr);

        return 0;
}

int driver_manage(int uninstall)
{
        STARTUPINFO startupinfo = { 0 };
        PROCESS_INFORMATION procinfo = { 0 };
        wchar_t real_cmd[PATH_MAX * 2] = { 0 };

        path_fix(driver_path, ARRAY_SIZE(driver_path), DRV_FILE);
        path_fix(ntdrvldr_path, ARRAY_SIZE(ntdrvldr_path), NTDRVLDR_EXE);

        snwprintf(real_cmd, ARRAY_SIZE(real_cmd), L"\"%ls\" %ls -n inpoutx64 \"%ls\"",
                  ntdrvldr_path, uninstall ? L"-u" : L" ", driver_path);

        pr_info("%ls\n", real_cmd);

        if (FALSE == CreateProcess(ntdrvldr_path, real_cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startupinfo, &procinfo)) {
                pr_err("failed to execute external program, err = 0x%08lx\n", GetLastError());
                return -EFAULT;
        }

        return 0;
}

int wmain(int wargc, wchar_t *wargv[])
{
        int err = 0;

        setbuf(stdout, NULL);

        if (wargc == 1) {
                help();
                exit(-EINVAL);
        }

        if ((err = parse_wargs(wargc, wargv)))
                return err;

        inpoutx64_init();
        if (!is_inpoutx64_driver_open()) {
                pr_err("Failed to open Inpoutx64 driver\n");
                inpoutx64_deinit();

                return -EIO;
        }

        switch (cmd) {
        case CMD_READ:
        case CMD_READBLK:
        case CMD_WRITE:
        case CMD_WRITEBLK:
                err = physmem_rw();
                break;

        case CMD_DRV_INSTALL:
                err = driver_manage(0);
                break;

        case CMD_DRV_REMOVE:
                err = driver_manage(1);
                break;

        default:
                return -EINVAL;
        }

        inpoutx64_deinit();

        return err;
}
