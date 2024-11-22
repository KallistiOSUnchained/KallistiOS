/* KallistiOS ##version##

   util/minifont.c
   Copyright (C) 2020 Lawrence Sebald

*/

#include <string.h>
#include <dc/minifont.h>
#include "minifont.h"

int minifont_draw(uint16 *buffer, uint32 bufwidth, uint32 c) {
    int pos, i, j, k;
    uint8 byte;
    uint16 *cur;

    if(c < 33 || c > 126)
        return MFONT_WIDTH;

    pos = (c - 33) * MFONT_BYTES_PER_CHAR;

    for(i = 0; i < MFONT_HEIGHT; ++i) {
        cur = buffer;

        for(j = 0; j < MFONT_WIDTH / 8; ++j) {
            byte = minifont_data[pos + (i * (MFONT_WIDTH / 8)) + j];

            for(k = 0; k < 8; ++k) {
                if(byte & (1 << (7 - k)))
                    *cur++ = 0xFFFF;
                else
                    ++cur;
            }
        }

        buffer += bufwidth;
    }

    return MFONT_WIDTH;
}

int minifont_draw_str(uint16 *buffer, uint32 bufwidth, const char *str) {
    char c;
    int adv = 0;

    while((c = *str++)) {
        adv += minifont_draw(buffer + adv, bufwidth, c);
    }

    return adv;
}
