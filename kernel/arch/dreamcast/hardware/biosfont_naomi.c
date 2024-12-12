/* KallistiOS ##version##

   biosfont_naomi.c

   Copyright (C) 2024 Andy Barajas
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/syscalls.h>

#include "../util/minifont.h"

#include <kos/dbglog.h>

/*
    This module provides font rendering functionality for the Sega NAOMI platform 
    using the minifont system.  See `utils/minifont.h` for details. Since the NAOMI 
    BIOS fonts are poorly documented, this wrapper around minifont serves as an 
    alternative to ensure basic text rendering capabilities are available. 

    Supported Features:
    - Renders fixed-size 8x16 ASCII characters (33–126) using minifont.
    - Unsupported characters are replaced with spaces.
    - Handles various pixel formats: 4bpp, 8bpp, 16bpp, and 32bpp.
    - Offers functions for drawing individual characters, strings, and formatted text.
    - Supports customizable foreground and background colors, as well as transparent 
      or opaque rendering.
*/

/* Current colors/pixel format. Default to white foreground, black background
   and 16-bit drawing, so the default behavior doesn't change from what it has
   been forever. */
static uint32_t bfont_fgcolor = 0xFFFFFFFF;
static uint32_t bfont_bgcolor = 0x00000000;

static inline uint8_t bits_per_pixel() {
    return ((vid_mode->pm == PM_RGB0888) ? sizeof(uint32_t) : sizeof(uint16_t)) << 3;
}

void bfont_set_encoding(bfont_code_t enc) {
    (void)enc;

    dbglog(DBG_ERROR, "bfont_set_encoding: changing font encoding is not supported on NAOMI\n");
}

/* Set the foreground color and return the old color */
uint32_t bfont_set_foreground_color(uint32_t c) {
    uint32_t rv = bfont_fgcolor;
    bfont_fgcolor = c;
    return rv;
}

/* Set the background color and return the old color */
uint32_t bfont_set_background_color(uint32_t c) {
    uint32_t rv = bfont_bgcolor;
    bfont_bgcolor = c;
    return rv;
}

/* Given an ASCII character, find it in the minifont if possible */
const uint8_t *bfont_find_char(uint32_t ch) {
    uint32_t index;

    /* 33-126 in ASCII are 0-93 in the font */
    if(ch >= 33 && ch <= 126) {
        index = ch - 33;
        return minifont_data + index * MFONT_BYTES_PER_CHAR;
    }
    else
        return NULL;
}

/* JIS -> (kuten) -> address conversion */
uint8_t *bfont_find_char_jp(uint32_t ch) {
    (void)ch;

    dbglog(DBG_ERROR, "bfont_find_char_jp: JIS character lookup is not supported on the NAOMI\n");
    return NULL;
}

/* Half-width kana -> address conversion */
uint8_t *bfont_find_char_jp_half(uint32_t ch) {
    (void)ch;

    dbglog(DBG_ERROR, "bfont_find_char_jp_half: half-width Kana character lookup is not supported on the NAOMI\n");
    return NULL;
}

static size_t bfont_draw_opaque_space(uint8_t *buf, uint32_t bufwidth, uint32_t bg, uint8_t bpp) {
    uint8_t x, y;

    for(y = 0; y < MFONT_HEIGHT; y++) {
        /* Fill the row with the background color */
        for(x = 0; x < MFONT_WIDTH; x++) {
            if(bpp == 16) {
                *(uint16_t *)buf = bg & 0xFFFF;
                buf += 2;
            } 
            else if(bpp == 32) {
                *(uint32_t *)buf = bg;
                buf += 4;
            } 
            else if(bpp == 8) {
                *buf = bg & 0xFF;
                buf += 1;
            } 
            else if(bpp == 4) {
                if((x % 2) == 0)
                    *buf = (bg & 0xF) << 4;
                else {
                    *buf |= (bg & 0xF);
                    buf++;
                }
            }
        }

        /* Move to the next row in the buffer */
        buf += ((bufwidth - MFONT_WIDTH) * bpp) / 8;
    }

    return (MFONT_WIDTH * bpp)/8;
}

/* Draws one character to an output buffer of bit depth in bits per pixel */
static uint16_t *bfont_draw_one_row(uint16_t *buffer, uint16_t word, bool opaque, uint32_t fg, uint32_t bg, uint8_t bpp) {
    uint8_t x;
    uint32_t color = 0x0000;
    uint16_t write16 = 0x0000;
    uint16_t oldcolor = *buffer;

    if((bpp == 4)||(bpp == 8)) {
        /* For 4 or 8bpp we have to go 2 or 4 pixels at a time to properly write out in all cases. */
        uint8_t bMask = (bpp==4) ? 0xf : 0xff;
        uint8_t pix = 16/bpp;
        for(x = 0; x < MFONT_WIDTH; x++) {
            if(x%pix == 0) {
                oldcolor = *buffer;
                write16 = 0x0000;
            }

            if(word & (0x080 >> x))
                write16 |= fg << (bpp * (x % pix));
            else {
                if(opaque)
                    write16 |= bg << (bpp * (x % pix));
                else
                    write16 |= oldcolor & (bMask << (bpp * (x % pix)));
            }
            if(x % pix == (pix - 1)) 
                *buffer++ = write16;
        }

        /* Handle the remaining pixels in the last group, if necessary */
        if(x % pix != 0)
            *buffer++ = write16;
    }
    else { /* 16 or 32 */
        for(x = 0; x < MFONT_WIDTH; x++, buffer++) {
            if(word & (0x080 >> x))
                color = fg;
            else {
                if(opaque)
                    color = bg;
                else
                    continue;
            }
            if(bpp == 16) 
                *buffer = color & 0xffff;
            else if(bpp == 32) {
                *(uint32_t *)buffer = color; 
                buffer++;
            }
        }
    }

    return buffer;
}

size_t bfont_draw_ex(void *buffer, uint32_t bufwidth, uint32_t fg, uint32_t bg, 
                     uint8_t bpp, bool opaque, uint32_t c, bool wide, bool iskana) {
    const uint8_t *ch;
    uint16_t word;
    uint8_t y;
    uint8_t *buf = (uint8_t *)buffer;

    /* Just making sure we can draw the character we want to */
    if(bufwidth < MFONT_WIDTH) {
        dbglog(DBG_ERROR, "bfont_draw_ex: buffer is too small to draw into\n");
        return 0;
    }

    /* If they're requesting a wide or kana character, draw space instead */
    if(wide || iskana) {
        dbglog(DBG_WARNING, "bfont_draw_ex: can't draw wide or kana for NAOMI\n");

        if(opaque)
            return bfont_draw_opaque_space(buf, bufwidth, bg, bpp);

        return (MFONT_WIDTH * bpp)/8;
    }

    /* Translate the character */
    ch = bfont_find_char(c);

    /* Unsupported character, draw a space */
    if(ch == NULL) {
        /* Draw an opaque space */
        if(opaque)
            return bfont_draw_opaque_space(buf, bufwidth, bg, bpp);

        return (MFONT_WIDTH * bpp)/8;
    }    

    /* Process each row of the 8x16 character */
    for(y = 0; y < MFONT_HEIGHT; y++, ch++) {
        /* Extract the 8 bits for this row */
        word = *ch;

        /* Draw the row */
        buf = (uint8_t *)bfont_draw_one_row((uint16_t *)buf, word, opaque, fg, bg, bpp);

        /* Move to the next row in the buffer */
        buf += ((bufwidth - MFONT_WIDTH) * bpp) / 8;
    }

    /* Return the horizontal distance covered in bytes */
    return (MFONT_WIDTH * bpp)/8;
}

/* Draw half-width kana */
size_t bfont_draw_thin(void *buffer, uint32_t bufwidth, bool opaque, uint32_t c, bool iskana) {
    return bfont_draw_ex(buffer, bufwidth, bfont_fgcolor, bfont_bgcolor, 
                         bits_per_pixel(), opaque, c, false, iskana);
}

/* Compat function */
size_t bfont_draw(void *buffer, uint32_t bufwidth, bool opaque, uint32_t c) {
    return bfont_draw_ex(buffer, bufwidth, bfont_fgcolor, bfont_bgcolor, 
                        bits_per_pixel(), opaque, c, false, false);
}

/* Draw wide character */
size_t bfont_draw_wide(void *buffer, uint32_t bufwidth, bool opaque, uint32_t c) {
    return bfont_draw_ex(buffer, bufwidth, bfont_fgcolor, bfont_bgcolor, 
                         bits_per_pixel(), opaque, c, true, false);
}

void bfont_draw_str_ex(void *buffer, uint32_t width, uint32_t fg, uint32_t bg, 
                       uint8_t bpp, bool opaque, const char *str) {
    uint16_t nChr;
    uint32_t line_start = 0;
    uint8_t *buf = (uint8_t *)buffer;

    while(*str) {
        nChr = *str & 0xff;

        if(nChr == '\n') {
            /* Move to the beginning of the next line */
            buf = (uint8_t *)buffer + line_start + (width * MFONT_HEIGHT * (bpp / 8));
            line_start = buf - (uint8_t *)buffer;
            str++;
            continue;
        }
        else if(nChr == '\t') {
            /* Draw four spaces on the current line */
            if(opaque) {
                buf += bfont_draw_opaque_space(buf, width, bg, bpp);
                buf += bfont_draw_opaque_space(buf, width, bg, bpp);
                buf += bfont_draw_opaque_space(buf, width, bg, bpp);
                buf += bfont_draw_opaque_space(buf, width, bg, bpp);
            }
            else /* Spaces are always single width characters */
                buf += (4 * ((MFONT_WIDTH * bpp)/8));
            
            str++;
            continue;
        }

        buf += bfont_draw_ex(buf, width, fg, bg, bpp, opaque, nChr, false, false);

        str++;
    }
}

void bfont_draw_str_ex_vfmt(void *buffer, uint32_t width, uint32_t fg, uint32_t bg,
                            uint8_t bpp, bool opaque, const char *fmt,
                            va_list *var_args) {
    /* Maximum of 2400 characters onscreen */
    char string[2400];

    vsnprintf(string, sizeof(string), fmt, *var_args);
    bfont_draw_str_ex(buffer, width, fg, bg, bpp, opaque, string);
}

/* Draw string of characters */
void bfont_draw_str_ex_fmt(void *buffer, uint32_t width, uint32_t fg, uint32_t bg, uint8_t bpp,
                           bool opaque, const char *fmt, ...) {
    va_list var_args;
    va_start(var_args, fmt);

    bfont_draw_str_ex_vfmt(buffer, width, fg, bg, bpp, opaque, fmt, &var_args);

    va_end(var_args);
}

void bfont_draw_str(void *buffer, uint32_t width, bool opaque, const char *str) {
    bfont_draw_str_ex(buffer, width, bfont_fgcolor, bfont_bgcolor,
                     bits_per_pixel(), opaque, str);
}

void bfont_draw_str_fmt(void *buffer, uint32_t width, bool opaque, const char *fmt,
                        ...) {
    va_list var_args;
    va_start(var_args, fmt);

    bfont_draw_str_ex_vfmt(buffer, width, bfont_fgcolor, bfont_bgcolor,
                           bits_per_pixel(), opaque, fmt, &var_args);

    va_end(var_args);
}

void bfont_draw_str_vram_vfmt(uint32_t x, uint32_t y, uint32_t fg, 
                              uint32_t bg, bool opaque, const char *fmt, 
                              va_list *var_args) {
    uint32_t bpp = bits_per_pixel();
    void *vram = vram_s;
    uint32_t offset = (y * vid_mode->width + x);

    if(bpp == 16)
        vram = (uint16_t *)vram + offset;
    else if(bpp == 32)
        vram = (uint32_t *)vram + offset;

    bfont_draw_str_ex_vfmt(vram, vid_mode->width, fg, bg, bpp, opaque, fmt,
                           var_args);
}

void bfont_draw_str_vram_fmt(uint32_t x, uint32_t y, bool opaque, 
                            const char *fmt, ...) {
    va_list var_args;
    va_start(var_args, fmt);
    
    bfont_draw_str_vram_vfmt(x, y, bfont_fgcolor, bfont_bgcolor, opaque, fmt, 
                            &var_args);

    va_end(var_args);
}

uint8_t *bfont_find_icon(bfont_vmu_icon_t icon) {
    (void)icon;

    dbglog(DBG_ERROR, "bfont_find_icon: VMU icons are not supported on NAOMI\n");
    return NULL;
}
