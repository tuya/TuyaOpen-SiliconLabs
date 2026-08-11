/*******************************************************************************
 * @file  syscalls.c
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/*
 *
 *    Atollic TrueSTUDIO Minimal System calls file
 *
 *    For more information about which c-functions
 *    need which of these lowlevel functions
 *    please consult the Newlib libc-manual
 *
 */

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>

#include "sl_component_catalog.h"
#include "tkl_uart.h"
#include "syscalls.h"
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "cmsis_os2.h"
#endif
#define IO_MAXLINE 20U // maximun read length
typedef int (*PUTCHAR_FUNC)(int a);
char *stack_ptr __asm("sp");

extern char __HeapBase[];
extern char __HeapLimit[];

/*! @brief Specification modifier flags for scanf. */
enum _debugconsole_scanf_flag {
    kSCANF_Suppress   = 0x2U,  /*!< Suppress Flag. */
    kSCANF_DestMask   = 0x7cU, /*!< Destination Mask. */
    kSCANF_DestChar   = 0x4U,  /*!< Destination Char Flag. */
    kSCANF_DestString = 0x8U,  /*!< Destination String FLag. */
    kSCANF_DestSet    = 0x10U, /*!< Destination Set Flag. */
    kSCANF_DestInt    = 0x20U, /*!< Destination Int Flag. */
    kSCANF_DestFloat  = 0x30U, /*!< Destination Float Flag. */
    kSCANF_LengthMask = 0x1f00U,
    /*!< Length Mask Flag. */    /*PRINTF_FLOAT_ENABLE */
    kSCANF_TypeSinged = 0x2000U, /*!< TypeSinged Flag. */
};

static unsigned int syscalls_format_uart_log(char *logstr, size_t cap, const char *ptr, int len)
{
    unsigned int out_len = 0;

    for (int i = 0; i < len; ++i) {
        if (out_len >= cap - 1) {
            break;
        }
        if (ptr[i] == '\n' && (i == 0 || ptr[i - 1] != '\r') && out_len < cap - 2) {
            logstr[out_len++] = '\r';
        }
        logstr[out_len++] = ptr[i];
    }

    return out_len;
}

void initialise_monitor_handles(void)
{
    /* Required by Newlib; no semihosting/monitor handles on SiWx917. */
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    _kill(status, -1);
    while (1) {
        /* Hang forever after _kill; _exit must not return. */
    }
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;

    static char  logstr[256];
    unsigned int out_len = 0;

    if ((unsigned int)len + 1 < sizeof(logstr)) {
        out_len = syscalls_format_uart_log(logstr, sizeof(logstr), ptr, len);
        tkl_uart_write(TUYA_UART_NUM_0, logstr, out_len);
    } else {
        tkl_uart_write(TUYA_UART_NUM_0, ptr, len);
    }

    return len;
}

#ifndef SL_WIFI_COMPONENT_INCLUDED
void *_sbrk(int incr)
{
    static char *heap_end = __HeapBase;
    char        *prev_heap_end;

    if ((heap_end + incr) > __HeapLimit) {
        // Not enough heap
        return (void *)-1;
    }

    prev_heap_end = heap_end;
    heap_end += incr;

    return prev_heap_end;
}
#endif

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(char *fmt_ptr, ...)
{
    (void)fmt_ptr;

    return -1;
}

int _open(char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    /* Pretend like we always fail */
    return -1;
}

int _wait(int *status)
{
    (void)status;
    errno = ECHILD;
    return -1;
}

int _unlink(char *name)
{
    (void)name;
    errno = ENOENT;
    return -1;
}

int _times(struct tms *buff)
{
    (void)buff;
    return -1;
}

int _stat(char *file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _link(char *old_link, char *new_link)
{
    (void)old_link; // This statement is added only to resolve compilation warning, value is unchanged
    (void)new_link; // This statement is added only to resolve compilation warning, value is unchanged
    errno = EMLINK;
    return -1;
}

int _fork(void)
{
    errno = EAGAIN;
    return -1;
}

int _execve(char *name, char **argv, char **env)
{
    (void)name;
    (void)argv;
    (void)env;

    errno = ENOMEM;

    return -1;
}

SL_WEAK void _putchar(char character)
{
    (void)character;
    /* Weak Newlib hook; UART output is handled via _write() -> tkl_uart_write(). */
}
