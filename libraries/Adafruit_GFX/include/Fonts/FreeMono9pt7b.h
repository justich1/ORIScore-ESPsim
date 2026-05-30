#pragma once

#include <Adafruit_GFX.h>

#ifndef PROGMEM
#define PROGMEM
#endif

static const uint8_t FreeMono9pt7bBitmaps[] PROGMEM = { 0 };

static const GFXglyph FreeMono9pt7bGlyphs[] PROGMEM = {
    { 0, 0, 0, 0, 0, 0 }
};

static const GFXfont FreeMono9pt7b PROGMEM = {
    (uint8_t*)FreeMono9pt7bBitmaps,
    (GFXglyph*)FreeMono9pt7bGlyphs,
    0x20,
    0x7E,
    18
};