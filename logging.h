#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>
#include <wchar.h>

#include "utils.h"

#define LOG_ALWAYS_FLUSH
#define LOG_COLOR_ENABLED
#define LOG_LEVEL_COLORED
#define LOG_ERR_STREAM          stdout
//#define ZLOG_ENABLED

//
// ansi color
//
// ESC = \033 = \e
//
// set fg&bg color:
//      ESC[<FG>;<BG>m ... ESC[0m
//
// set fg color:
//      ESC[0;<FG>m ... ECS[0m
//
// reset:
//      ESC[0m
//

#define ANSI_NO_COLOR           0

#define FG_BLACK                30
#define FG_RED                  31
#define FG_GREEN                32
#define FG_YELLOW               33
#define FG_BLUE                 34
#define FG_MAGENTA              35
#define FG_CYAN                 36
#define FG_WHITE                37

/* bright */
#define FG_LT_BLACK             90
#define FG_LT_RED               91
#define FG_LT_GREEN             92
#define FG_LT_YELLOW            93
#define FG_LT_BLUE              94
#define FG_LT_MAGENTA           95
#define FG_LT_CYAN              96
#define FG_LT_WHITE             97

#define BG_BLACK                40
#define BG_RED                  41
#define BG_GREEN                42
#define BG_YELLOW               43
#define BG_BLUE                 44
#define BG_MAGENTA              45
#define BG_CYAN                 46
#define BG_WHITE                47

/* bright */
#define BG_LT_BLACK             100
#define BG_LT_RED               101
#define BG_LT_GREEN             102
#define BG_LT_YELLOW            103
#define BG_LT_BLUE              104
#define BG_LT_MAGENTA           105
#define BG_LT_CYAN              106
#define BG_LT_WHITE             107

#define LOG_COLOR_VERBOSE       FG_LT_BLACK
#define LOG_COLOR_DBG           FG_LT_GREEN
#define LOG_COLOR_INFO          FG_LT_BLUE
#define LOG_COLOR_NOTICE        FG_LT_CYAN
#define LOG_COLOR_WARN          FG_LT_YELLOW
#define LOG_COLOR_ERR           FG_RED
#define LOG_COLOR_FATAL         FG_LT_RED

enum {
        _LOG_LEVEL_VERBOSE = 0,
        _LOG_LEVEL_DEBUG,
        _LOG_LEVEL_INFO,
        _LOG_LEVEL_NOTICE,
        _LOG_LEVEL_WARN,
        _LOG_LEVEL_ERROR,
        _LOG_LEVEL_FATAL,
        NUM_LOG_LEVELS,
};

#define LOG_LEVEL_VERBOSE       BIT(_LOG_LEVEL_VERBOSE)
#define LOG_LEVEL_DEBUG         BIT(_LOG_LEVEL_DEBUG)
#define LOG_LEVEL_INFO          BIT(_LOG_LEVEL_INFO)
#define LOG_LEVEL_NOTICE        BIT(_LOG_LEVEL_NOTICE)
#define LOG_LEVEL_WARN          BIT(_LOG_LEVEL_WARN)
#define LOG_LEVEL_ERROR         BIT(_LOG_LEVEL_ERROR)
#define LOG_LEVEL_FATAL         BIT(_LOG_LEVEL_FATAL)

#define LOG_LEVEL_DEFAULT       (LOG_LEVEL_DEBUG  | \
                                 LOG_LEVEL_INFO   | \
                                 LOG_LEVEL_NOTICE | \
                                 LOG_LEVEL_WARN   | \
                                 LOG_LEVEL_ERROR  | \
                                 LOG_LEVEL_FATAL)

#define LOG_LEVEL_ALL           (LOG_LEVEL_DEBUG  | \
                                 LOG_LEVEL_INFO   | \
                                 LOG_LEVEL_NOTICE | \
                                 LOG_LEVEL_WARN   | \
                                 LOG_LEVEL_ERROR  | \
                                 LOG_LEVEL_FATAL  | \
                                 LOG_LEVEL_VERBOSE)

//
// zlog
//

// default level:
//      debug
//      info
//      notice
//      warn
//      error
//      fatal

#ifdef ZLOG_ENABLED
#include <zlog.h>

extern uint32_t zlog_inited;
#else
#define zlog_inited (0)
#endif

extern uint32_t g_logprint_level;
extern uint32_t g_logprint_colored;

#ifdef __WINNT__
#include <windows.h>

extern uint32_t g_console_alloc;
extern uint32_t g_console_show;
extern uint32_t g_console_is_hide;
extern HWND g_console_wnd;

void console_hide(void);
void console_show(int set_focus);
int console_title_set(wchar_t *title);
#endif

int logging_init(void);
int logging_exit(void);

//
// logging macro
//

#ifdef __WINNT__
#define TT_INFO                 "INFO"
#define TT_DEBUG                "DEBUG"
#define TT_ERROR                "ERROR"
#define TT_WARNING              "WARNING"

#define MB_MSG(fmt, ...)        mb_printf(TT_INFO, MB_OK, fmt, ##__VA_ARGS__)
#define MB_MSG_ERR(fmt, ...)    mb_printf(TT_ERROR, MB_ICONERROR | MB_OK, fmt, ##__VA_ARGS__)
#define MB_MSG_INFO(fmt, ...)   mb_printf(TT_INFO, MB_ICONINFORMATION | MB_OK, fmt, ##__VA_ARGS__)

#define MB_FUNC_ERR()           MB_MSG_ERR("%s:%d %s() failed", __FILE__, __LINE__, __func__)

int mb_printf(const char *title, unsigned flags, const char *fmt, ...);
#endif // __WINNT__

/*
 * if any '\n' is contained in @msg and @bg is set,
 * you will see a color bar
 *
 * \n is supposed to insert in zlog format setting
 */

#ifdef ZLOG_ENABLED
#define zlog_bg_color(cat, lvl, fg, bg, msg, fmt...)    \
        do {                                            \
                zlog_##lvl(cat,                         \
                           "\033[" MACRO_TO_STR(fg) ";" \
                           MACRO_TO_STR(bg) "m"         \
                           msg                          \
                           "\033[0m", ##fmt);           \
        } while(0)

#define zlog_color(cat, lvl, fg, msg, fmt...)           \
        do {                                            \
                zlog_##lvl(cat, "\033[0;"               \
                           MACRO_TO_STR(fg) "m"         \
                           msg                          \
                           "\033[0m", ##fmt);           \
        } while(0)
#else
#define zlog_bg_color(cat, lvl, fg, bg, msg, fmt...)    \
        do { } while(0)

#define zlog_color(cat, lvl, fg, msg, fmt...)           \
        do { } while(0)

#define dzlog_debug(...)        do { } while (0)
#define dzlog_info(...)         do { } while (0)
#define dzlog_warn(...)         do { } while (0)
#define dzlog_error(...)        do { } while (0)
#define dzlog_fatal(...)        do { } while (0)
#define dzlog_notice(...)       do { } while (0)
#endif

#ifdef LOG_ALWAYS_FLUSH
#define LOG_FLUSH(fp) do { fflush((fp)); } while (0)
#else
#define LOG_FLUSH(fp) do { } while (0)
#endif // LOG_ALWAYS_FLUSH

#define ___pr_wrapped(fp, color, msg, fmt...)           \
        do {                                            \
                fprintf(fp, msg, ##fmt);                \
                LOG_FLUSH(fp);                          \
        } while (0)

#ifdef LOG_LEVEL_COLORED
#define __pr_wrapped __pr_color
#else
#define __pr_wrapped ___pr_wrapped
#endif // LOG_LEVEL_COLORED

/*
 * if any '\n' is contained in @msg and @bg is set,
 * you will see a color bar
 */
#define ___pr_bg_color(fp, fg, bg, cr, msg, fmt...)     \
        do {                                            \
                if (!g_logprint_colored)  {             \
                        ___pr_wrapped(fp, 0,            \
                                      msg, ##fmt);      \
                        break;                          \
                }                                       \
                                                        \
                fprintf(fp,                             \
                        "\033[" MACRO_TO_STR(fg) ";"    \
                        MACRO_TO_STR(bg) "m"            \
                        msg                             \
                        "\033[0m", ##fmt);              \
                if (cr)                                 \
                        fprintf(fp, "\n");              \
        } while(0)

#define __pr_bg_color(fg, bg, msg, fmt...)              \
                ___pr_bg_color(stdout, fg, bg, 1, msg, ##fmt)

#define __pr_color(fp, color, msg, fmt...)              \
        do {                                            \
                if (!g_logprint_colored)  {             \
                        ___pr_wrapped(fp, 0,            \
                                      msg, ##fmt);      \
                        break;                          \
                }                                       \
                                                        \
                fprintf(fp,                             \
                        "\033[0;"                       \
                        MACRO_TO_STR(color) "m"         \
                        msg                             \
                        "\033[0m", ##fmt);              \
        } while(0)

#ifdef LOG_COLOR_ENABLED
#define pr_color(color, msg, fmt...)                    \
                __pr_color(stdout, color, msg, ##fmt)

#define pr_bg_color(fg, bg, msg, fmt...)                \
                __pr_bg_color(fg, bg, msg, ##fmt);
#else
#define pr_color(color, msg, fmt...)                    \
                ___pr_wrapped(stdout, color, msg, ##fmt)

#define pr_bg_color(fg, bg, msg, fmt...)                \
                ___pr_wrapped(stdout, 0, msg, ##fmt)
#endif // LOG_COLOR_ENABLED

#define pr_color_func(color, msg, fmt...)               \
        do {                                            \
                pr_color(color, "%s(): ", __func__);    \
                pr_color(color, msg, ##fmt);            \
                LOG_FLUSH(fp);                          \
        } while(0)

#define pr_bg_color_func(fg, bg, msg, fmt...)           \
        do {                                            \
                pr_color(fg, "%s(): ", __func__);       \
                __pr_bg_color(fg, bg, msg, ##fmt);      \
        } while(0)

#define pr_dbg(msg, fmt...)                                                     \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_debug(msg, ##fmt);                                \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_DEBUG))                      \
                        break;                                                  \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_DBG, "%s(): ", __func__);        \
                __pr_wrapped(stdout, LOG_COLOR_DBG, msg, ##fmt);                \
        } while(0)

#define pr_verbose(msg, fmt...)                                                 \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_debug(msg, ##fmt);                                \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_VERBOSE))                    \
                        break;                                                  \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_VERBOSE, "%s(): ", __func__);    \
                __pr_wrapped(stdout, LOG_COLOR_VERBOSE, msg, ##fmt);            \
        } while(0)

#define pr_info(msg, fmt...)                                                    \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_info(msg, ##fmt);                                 \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_INFO))                       \
                        break;                                                  \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_INFO, "%s(): ", __func__);       \
                __pr_wrapped(stdout, LOG_COLOR_INFO, msg, ##fmt);               \
        } while(0)

#define pr_raw(msg, fmt...)                                                     \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_info(msg, ##fmt);                                 \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_INFO, msg, ##fmt);               \
        } while(0)

#define pr_rawlvl(lvl, msg, fmt...)                                             \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_info(msg, ##fmt);                                 \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_##lvl))                      \
                        break;                                                  \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_##lvl, msg, ##fmt);              \
        } while(0)

#define pr_info_once(msg, fmt...)                                               \
        do {                                                                    \
                static uint8_t __t = 0;                                         \
                if (__t)                                                        \
                        break;                                                  \
                                                                                \
                if (zlog_inited)                                                \
                        dzlog_info(msg, ##fmt);                                 \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_NOTICE, "%s(): ", __func__);     \
                __pr_wrapped(stdout, LOG_COLOR_NOTICE, msg, ##fmt);             \
                __t = 1;                                                        \
        } while(0)

#define pr_notice(msg, fmt...)                                                  \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_notice(msg, ##fmt);                               \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_NOTICE))                     \
                        break;                                                  \
                                                                                \
                __pr_wrapped(stdout, LOG_COLOR_NOTICE, "%s(): ", __func__);     \
                __pr_wrapped(stdout, LOG_COLOR_NOTICE, msg, ##fmt);             \
        } while(0)

#define pr_err(msg, fmt...)                                                     \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_error(msg, ##fmt);                                \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_ERROR))                      \
                        break;                                                  \
                                                                                \
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_ERR, "%s(): ", __func__);\
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_ERR, msg, ##fmt);        \
        } while(0)

#define pr_err_once(msg, fmt...)                                                \
        do {                                                                    \
                static uint8_t __t = 0;                                         \
                if (__t)                                                        \
                        break;                                                  \
                                                                                \
                if (zlog_inited)                                                \
                        dzlog_error(msg, ##fmt);                                \
                                                                                \
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_ERR, "%s(): ", __func__);\
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_ERR, msg, ##fmt);        \
                __t = 1;                                                        \
        } while(0)

#define pr_warn(msg, fmt...)                                                    \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_warn(msg, ##fmt);                                 \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_WARN))                       \
                        break;                                                  \
                                                                                \
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_WARN, "%s(): ", __func__);   \
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_WARN, msg, ##fmt);           \
        } while(0)

#define pr_fatal(msg, fmt...)                                                   \
        do {                                                                    \
                if (zlog_inited)                                                \
                        dzlog_fatal(msg, ##fmt);                                \
                                                                                \
                if (!(g_logprint_level & LOG_LEVEL_FATAL))                      \
                        break;                                                  \
                                                                                \
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_FATAL, "%s(): ", __func__);  \
                __pr_wrapped(LOG_ERR_STREAM, LOG_COLOR_FATAL, msg, ##fmt);          \
        } while(0)

// debug color:
// pr_verbose("VERBOSE\n");
// pr_dbg("DEBUG\n");
// pr_info("INFO\n");
// pr_notice("NOTICE\n");
// pr_warn("WARN\n");
// pr_err("ERROR\n");
// pr_fatal("FATAL\n");

#endif /* __LOGGING_H__ */