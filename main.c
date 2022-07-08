#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>
#include <winternl.h>
#include <winerror.h>

#include "getopt.h"
#include "logging.h"
#include "driver.h"
#include "asmmap64.h"

#define DRV_SVC_NAME    L"asmmap64"
#define DRV_FILE        L"asmmap64.sys"

enum {
        CMD_READ = 0,
        CMD_READBLK,
        CMD_WRITE,
        CMD_WRITEBLK,
        CMD_DRV_INSTALL,
        CMD_DRV_REMOVE,
        NUM_OPT_CMDS,
};

static wchar_t g_drv_path[MAX_PATH];

static uint32_t mmap_sz = 8;
static uint32_t rw_sz = 0;
static uint64_t rw_addr;
static uint64_t rw_value;
static uint8_t *wr_bytes;
static int no_readback = 0;

static int hexdump_print = 0;

static int drv_unload = 0;
static int drv_force_remove = 0;

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
"       -x              always remove asmmap64 driver on exit\n"
"       -f              force remove driver for command \"driver remove\"\n"
"       -C              hexdump style print\n"
;

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

static int driver_path_get(void)
{
        memset(g_drv_path, 0x00, sizeof(g_drv_path));

        if (0 == GetFullPathName(DRV_FILE, sizeof(g_drv_path) / sizeof(wchar_t), g_drv_path, NULL)) {
                pr_err("failed to get file: %ls\n", DRV_FILE);
                return -EINVAL;
        }

        return 0;
}

static void help(void)
{
        fprintf(stderr, "%s", help_text);
}

static int parse_wargs(int wargc, wchar_t *wargv[])
{
        int c;

        while ((c = getopt_w(wargc, wargv, L"hm:xvsfC")) != -1) {
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

                case 'x':
                        drv_unload = 1;
                        break;

                case 'v':
                        g_logprint_level |= LOG_LEVEL_VERBOSE;
                        break;

                case 's':
                        no_readback = 1;
                        break;

                case 'f':
                        drv_force_remove = 1;
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

static int asmmap64_install(void)
{
        return driver_manage(DRV_SVC_NAME, g_drv_path, 1, 0);
}

static int asmmap64_remove(int force)
{
        return driver_manage(DRV_SVC_NAME, g_drv_path, 0, force);
}

static void addr_printf(uint64_t addr, size_t sz, uint64_t val)
{
        char _fmt[64] = { };

        sprintf(_fmt, "0x%%016jx 0x%%0%llujx\n", sz * 2);

        printf(_fmt, addr, val);
}

static int physmem_rw(void)
{
        uint8_t *virt_addr;
        int err;

        if (rw_sz == 0)
                return 0;

        if (asmmap64_open()) {
                if ((err = driver_path_get())) {
                        pr_err("failed to get %ls\n", DRV_FILE);
                        return err;
                }

                asmmap64_remove(1);
                if ((err = asmmap64_install())) {
                        pr_err("failed to install asmmap64 driver\n");
                        return err;
                }

                if ((err = asmmap64_open())) {
                        pr_err("failed to open asmmap64 driver\n");
                        asmmap64_remove(1);

                        return err;
                }
        }

        virt_addr = asmmap64_mmap(rw_addr, mmap_sz);
        if (virt_addr == NULL) {
                pr_err("failed to mmap phys addr 0x%016jx\n", rw_addr);
                goto out;
        }

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

out:
        asmmap64_close();

        if (drv_unload)
                asmmap64_remove(0);

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

        switch (cmd) {
        case CMD_READ:
        case CMD_READBLK:
        case CMD_WRITE:
        case CMD_WRITEBLK:
                err = physmem_rw();
                break;

        case CMD_DRV_INSTALL:
                err = asmmap64_install();
                break;

        case CMD_DRV_REMOVE:
                err = asmmap64_remove(drv_force_remove);
                break;

        default:
                return -EINVAL;
        }

        return err;
}
