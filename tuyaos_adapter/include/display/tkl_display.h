#ifndef TKL_DISPLAY
#define TKL_DISPLAY

#include <stdint.h>

void tkl_display_init(void);

void tkl_display_flush(uint16_t w, uint16_t h, uint8_t *data, uint16_t length);

#endif /* */
