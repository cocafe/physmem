#include <stdio.h>

#ifdef __WINNT__
#include <fcntl.h>
#include <windows.h>
#include <windowsx.h>
#include <iconv.h>
#endif // __WINNT__

#include "logging.h"

uint32_t g_logprint_level = LOG_LEVEL_DEFAULT;
uint32_t g_logprint_colored = 0;

#ifdef __WINNT__

uint32_t g_console_alloc = 1;
uint32_t g_console_show = 1;
uint32_t g_console_is_hide;
HWND g_console_hwnd = NULL;

void console_show(int set_focus)
{
        if (!g_console_hwnd)
                return;

        ShowWindow(g_console_hwnd, SW_NORMAL); // SW_RESTORE
        g_console_is_hide = 0;

        if (set_focus) {
                SetFocus(g_console_hwnd);
                SetForegroundWindow(g_console_hwnd);
        }
}

void console_hide(void)
{
        if (!g_console_hwnd)
                return;

        ShowWindow(g_console_hwnd, SW_HIDE);
        g_console_is_hide = 1;
}

static void console_stdio_redirect(void)
{
        HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        int SystemOutput = _open_osfhandle((intptr_t)ConsoleOutput, _O_TEXT);
        FILE *COutputHandle = _fdopen(SystemOutput, "w");

        HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
        int SystemError = _open_osfhandle((intptr_t)ConsoleError, _O_TEXT);
        FILE *CErrorHandle = _fdopen(SystemError, "w");

        HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
        int SystemInput = _open_osfhandle((intptr_t)ConsoleInput, _O_TEXT);
        FILE *CInputHandle = _fdopen(SystemInput, "r");

        freopen_s(&CInputHandle, "CONIN$", "r", stdin);
        freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
        freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);
}

int console_init(void)
{
        if (!g_console_alloc) {
                g_console_is_hide = 1;
                return 0;
        }

        if (AllocConsole() == 0) {
                pr_err("AllocConsole(), err = %lu\n", GetLastError());
                return -1;
        }

        console_stdio_redirect();

        g_console_hwnd = GetConsoleWindow();
        if (!g_console_hwnd) {
                pr_err("GetConsoleWindow() failed\n");
                return -1;
        }

        g_console_is_hide = 0;

        return 0;
}

int console_title_set(wchar_t *title)
{
        if (!g_console_alloc || !g_console_hwnd)
                return -ENOENT;

        if (SetWindowText(g_console_hwnd, title) == 0) {
                pr_err("SetWindowText(), err=%lu\n", GetLastError());
                return -EFAULT;
        }

        return 0;
}

int console_deinit(void)
{
        if (FreeConsole() == 0) {
                pr_err("FreeConsole(), err = %lu\n", GetLastError());
                return -1;
        }

        return 0;
}

void mb_wchar_show(char *title, char *content, size_t len, uint32_t flags)
{
        wchar_t wc_title[32] = { 0 };
        wchar_t *wc_buf;

        wc_buf = calloc(len + 4, sizeof(wchar_t));
        if (!wc_buf) {
                MessageBoxW(NULL, L"ERROR", L"failed to allocate buffer for unicode string", MB_OK);
                return;
        }

        if (iconv_utf82wc(title, strlen(title), wc_title, sizeof(wc_title) - sizeof(wchar_t))) {
                MessageBoxW(NULL, L"ERROR", L"iconv_convert() failed", MB_OK);
                goto out;
        }

        if (iconv_utf82wc(content, len * sizeof(char), wc_buf, len * sizeof(wchar_t))) {
                MessageBoxW(NULL, L"ERROR", L"iconv_convert() failed", MB_OK);
                goto out;
        }

        MessageBoxW(NULL, wc_buf, wc_title, flags);

out:
        free(wc_buf);
}

int mb_vprintf(const char *title, uint32_t flags, const char *fmt, va_list arg)
{
        va_list arg2;
        char cbuf;
        char *sbuf;
        int len, ret;

        va_copy(arg2, arg);
        len = vsnprintf(&cbuf, sizeof(cbuf), fmt, arg2);
        va_end(arg2);

        if (len < 0)
                return len;

        sbuf = calloc(len + 2, sizeof(char));
        if (!sbuf)
                return -ENOMEM;

        ret = vsnprintf(sbuf, len + 1, fmt, arg);
        if (ret < 0) {
                MessageBoxA(NULL, NULL, "vsnprintf() failed", MB_OK);
                goto out;
        }

#ifdef UNICODE
        mb_wchar_show((char *)title, sbuf, ret, flags);
#else
        MessageBox(NULL, sbuf, title, flags);
#endif

out:
        free(sbuf);

        return ret;
}

int mb_printf(const char *title, uint32_t flags, const char *fmt, ...)
{
        va_list ap;
        int ret;

        va_start(ap, fmt);
        ret = mb_vprintf(title, flags, fmt, ap);
        va_end(ap);

        return ret;
}

#endif // __WINNT__

int logging_init(void)
{
#ifdef __WINNT__
        if (console_init())
                return -1;
#endif

        return 0;
}

int logging_exit(void)
{
#ifdef __WINNT__
        if (g_console_alloc && console_deinit())
                return -1;
#endif

        return 0;
}