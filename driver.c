#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>
#include <winternl.h>
#include <winerror.h>

#include "driver.h"
#include "logging.h"

static int driver_install(SC_HANDLE scmgr, LPCTSTR id, LPCTSTR path)
{
        SC_HANDLE svc;

        svc = CreateService(scmgr,
                            id,
                            id,
                            SERVICE_ALL_ACCESS,
                            SERVICE_KERNEL_DRIVER,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_NORMAL,
                            path,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                           );
        if (svc == NULL) {
                DWORD err = GetLastError();

                if (err != ERROR_SERVICE_EXISTS) {
                        pr_err("CreateService() failed, err = %ld\n", err);
                        return -EINVAL;
                }
        }

        CloseServiceHandle(svc);

        return 0;
}

static int driver_remove(SC_HANDLE scmgr, LPCTSTR id)
{
        SC_HANDLE svc;
        int err = 0;

        svc = OpenService(scmgr, id, SERVICE_ALL_ACCESS);
        if (!svc)
                return -ENOENT;

        if (0 == DeleteService(svc)) {
                pr_err("DeleteService() failed, err = %ld\n", GetLastError());
                err = -EFAULT;
        }

        CloseServiceHandle(svc);

        return err;
}

static int driver_start(SC_HANDLE scmgr, LPCTSTR id)
{
        SC_HANDLE svc;
        int err = 0;

        svc = OpenService(scmgr, id, SERVICE_ALL_ACCESS);
        if (!svc)
                return -ENOENT;

        if (0 == StartService(svc, 0, NULL)) {
                DWORD _err = GetLastError();

                if (_err != ERROR_SERVICE_ALREADY_RUNNING) {
                        pr_err("StartService() failed, err = %ld\n", GetLastError());
                        err = -EFAULT;
                }
        }

        CloseServiceHandle(svc);

        return err;
}

static int driver_stop(SC_HANDLE scmgr, LPCTSTR id)
{
        SC_HANDLE svc;
        SERVICE_STATUS status = {};
        int err = 0;

        svc = OpenService(scmgr, id, SERVICE_ALL_ACCESS);
        if (!svc)
                return -ENOENT;

        if (0 == ControlService(svc, SERVICE_CONTROL_STOP, &status)) {
                pr_err("ControlService() failed, err = %ld\n", GetLastError());
                err = -EFAULT;
        }

        CloseServiceHandle(svc);

        return err;
}

static int is_driver_installed(SC_HANDLE scmgr, LPCTSTR id)
{
        SC_HANDLE svc;
        int installed = 0;

        svc = OpenService(scmgr, id, SERVICE_ALL_ACCESS);

        if (svc) {
                LPQUERY_SERVICE_CONFIG svc_config;
                DWORD sz;

                QueryServiceConfig(svc, NULL, 0, &sz);
                svc_config = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
                if (0 == QueryServiceConfig(svc, svc_config, sz, &sz)) {
                        pr_err("QueryServiceConfig() failed, err = %ld\n", GetLastError());
                        goto free;
                }

                if (svc_config->dwServiceType == SERVICE_AUTO_START ||
                    svc_config->dwServiceType == SERVICE_DEMAND_START)
                        installed = 1;
free:
                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, svc_config);
                CloseServiceHandle(svc);
        }

        return installed;
}

static int is_file_exist(LPCTSTR filepath)
{
        WIN32_FIND_DATA	ret;

        HANDLE file = FindFirstFile(filepath, &ret);
        if (file != INVALID_HANDLE_VALUE) {
                FindClose(file);
                return 1;
        }

        return 0;
}

int driver_manage(LPCTSTR id, LPCTSTR path, int install, int force)
{
        SC_HANDLE scmgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        int err;

        if (!id || !path)
                return -EINVAL;

        if (!is_file_exist(path)) {
                pr_err("file does not exist: %ls\n", path);
                return -ENOENT;
        }

        if (!scmgr) {
                pr_err("OpenSCManager() failed, err = %ld\n", GetLastError());
                return -EACCES;
        }

        if (install) {
                err = driver_install(scmgr, id, path);
                if (!err) {
                        err = driver_start(scmgr, id);
                }
        } else {
                if (!force && is_driver_installed(scmgr, id)) {
                        err = driver_stop(scmgr, id);
                        if (!err)
                                err = driver_remove(scmgr, id);
                } else {
                        err = driver_remove(scmgr, id);
                }
        }

        CloseServiceHandle(scmgr);

        return err;
}