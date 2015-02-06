/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Graphics Draw Functions
 * ----------------------------------------------------------------------------
 */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "jsutils.h"
#include "jsvar.h"

typedef enum {
  JSGRAPHICSTYPE_ARRAYBUFFER, ///< Write everything into an ArrayBuffer
  JSGRAPHICSTYPE_JS,          ///< Call JavaScript when we want to write something
  JSGRAPHICSTYPE_FSMC,        ///< FSMC (or fake FSMC) ILI9325 16bit-wide LCDs
  JSGRAPHICSTYPE_SDL,         ///< SDL graphics library for linux
} JsGraphicsType;

typedef enum {
  JSGRAPHICSFLAGS_NONE,
  JSGRAPHICSFLAGS_ARRAYBUFFER_ZIGZAG = 1, ///< ArrayBuffer: zig-zag (even rows reversed)
  JSGRAPHICSFLAGS_ARRAYBUFFER_VERTICAL_BYTE = 2, ///< ArrayBuffer: if 1 bpp, treat bytes as stacked vertically
  JSGRAPHICSFLAGS_SWAP_XY = 4, //< All devices: swap X and Y over
  JSGRAPHICSFLAGS_INVERT_X = 8, //< All devices: x = getWidth() - (x+1) - where x is DEVICE X
  JSGRAPHICSFLAGS_INVERT_Y = 16, //< All devices: y = getHeight() - (y+1) - where y is DEVICE Y
  JSGRAPHICSFLAGS_COLOR_BRG = 32, //< All devices: color order is BRG
  JSGRAPHICSFLAGS_COLOR_BGR = 64, //< All devices: color order is BRG
  JSGRAPHICSFLAGS_COLOR_GBR = 128, //< All devices: color order is GBR
  JSGRAPHICSFLAGS_COLOR_GRB = 256, //< All devices: color order is GRB
  JSGRAPHICSFLAGS_COLOR_RBG = 512, //< All devices: color order is RBG
} JsGraphicsFlags;

#define JSGRAPHICSFLAGS_COLOR_MASK (JSGRAPHICSFLAGS_COLOR_BRG | JSGRAPHICSFLAGS_COLOR_BGR | JSGRAPHICSFLAGS_COLOR_GBR | JSGRAPHICSFLAGS_COLOR_GRB | JSGRAPHICSFLAGS_COLOR_RBG)

#define JSGRAPHICS_FONTSIZE_4X6 (-1) // a bitmap font
#define JSGRAPHICS_FONTSIZE_CUSTOM (-2) // a custom bitmap font made from fields in the graphics object (See below)
// Positive font sizes are Vector fonts

#define JSGRAPHICS_CUSTOMFONT_BMP JS_HIDDEN_CHAR_STR"fntBmp"
#define JSGRAPHICS_CUSTOMFONT_WIDTH JS_HIDDEN_CHAR_STR"fntW"
#define JSGRAPHICS_CUSTOMFONT_HEIGHT JS_HIDDEN_CHAR_STR"fntH"
#define JSGRAPHICS_CUSTOMFONT_FIRSTCHAR JS_HIDDEN_CHAR_STR"fnt1st"

typedef struct {
  JsGraphicsType type;
  JsGraphicsFlags flags;
  unsigned short width, height; // DEVICE width and height (flags could mean the device is rotated)
  unsigned char bpp;
  unsigned int fgColor, bgColor; ///< current foreground and background colors
  short fontSize; ///< See JSGRAPHICS_FONTSIZE_ constants
  short cursorX, cursorY; ///< current cursor positions
} PACKED_FLAGS JsGraphicsData;

typedef struct JsGraphics {
  JsVar *graphicsVar; // this won't be locked again - we just know that it is already locked by something else
  JsGraphicsData data;
  unsigned char _blank; ///< this is needed as jsvGetString for 'data' wants to add a trailing zero  

  void (*setPixel)(struct JsGraphics *gfx, short x, short y, unsigned int col);
  void (*fillRect)(struct JsGraphics *gfx, short x1, short y1, short x2, short y2);
  unsigned int (*getPixel)(struct JsGraphics *gfx, short x, short y);
} PACKED_FLAGS JsGraphics;

static inline void graphicsStructInit(JsGraphics *gfx) {
  // type/width/height/bpp should be set elsewhere...
  gfx->data.flags = JSGRAPHICSFLAGS_NONE;
  gfx->data.fgColor = 0xFFFFFFFF;
  gfx->data.bgColor = 0;
  gfx->data.fontSize = JSGRAPHICS_FONTSIZE_4X6;
  gfx->data.cursorX = 0;
  gfx->data.cursorY = 0;
}

// ---------------------------------- these are in graphics.c
// Access a JsVar and get/set the relevant info in JsGraphics
bool graphicsGetFromVar(JsGraphics *gfx, JsVar *parent);
void graphicsSetVar(JsGraphics *gfx);
// ----------------------------------------------------------------------------------------------
// drawing functions - all coordinates are in USER coordinates, not DEVICE coordinates
void         graphicsSetPixel(JsGraphics *gfx, short x, short y, unsigned int col);
unsigned int graphicsGetPixel(JsGraphics *gfx, short x, short y);
void         graphicsClear(JsGraphics *gfx);
void         graphicsFillRect(JsGraphics *gfx, short x1, short y1, short x2, short y2);
void graphicsFallbackFillRect(JsGraphics *gfx, short x1, short y1, short x2, short y2); // Simple fillrect - doesn't call device-specific FR
void graphicsDrawRect(JsGraphics *gfx, short x1, short y1, short x2, short y2);
void graphicsDrawString(JsGraphics *gfx, short x1, short y1, const char *str);
void graphicsDrawLine(JsGraphics *gfx, short x1, short y1, short x2, short y2);
void graphicsFillPoly(JsGraphics *gfx, int points, short *vertices); // may overwrite vertices...
#ifndef SAVE_ON_FLASH
unsigned int graphicsFillVectorChar(JsGraphics *gfx, short x1, short y1, short size, char ch); ///< prints character, returns width
unsigned int graphicsVectorCharWidth(JsGraphics *gfx, short size, char ch); ///< returns the width of a character
#endif
void graphicsSplash(JsGraphics *gfx); ///< splash screen

void graphicsIdle(); ///< called when idling

#endif // GRAPHICS_H
