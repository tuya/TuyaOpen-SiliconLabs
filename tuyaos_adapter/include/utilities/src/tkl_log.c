#include "tkl_log.h"

log_output_t tkl_printf = printf;

void tkl_log_output_set(log_output_t fn)
{
    if (fn != NULL) {
        tkl_printf = fn;
    }
}

void log_printhex(char *ss, const uint8_t *buffs, int length)
{
    const uint8_t *d;
    int            r;

    TKL_PRINTF("%s \r\n", ss);
    for (int i = 0; i < length; i += 16) {
        d = &buffs[i];
        r = length - i;
        if (r > 16) {
            r = 16;
        }
        for (int j = 0; j < 16; j++) {
            if (j < r) {
                TKL_PRINTF("%02x ", d[j]);
            } else {
                TKL_PRINTF("   ");
            }
        }
        TKL_PRINTF("   ");
        for (int j = 0; j < r; j++) {
            if (d[j] < ' ' || d[j] > '~') {
                TKL_PRINTF(".");
            } else {
                TKL_PRINTF("%c", d[j]);
            }
        }
        TKL_PRINTF("%s", (char *)"\r\n");
    }
    TKL_PRINTF("%s", (char *)"\r\n");
}

void log_printhex_no_newline(char *ss, const uint8_t *buffs, int length)
{
    TKL_PRINTF("%s ", ss);
    for (int i = 0; i < length; i++) {
        TKL_PRINTF("%02x ", buffs[i]);
    }
    TKL_PRINTF("%s", (char *)"\r\n");
}
