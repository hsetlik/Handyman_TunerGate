#ifndef DISPLAY_CHARS_H
#define DISPLAY_CHARS_H
#include <stdint.h>
#include "sh1106.h"

// Bitmaps are row-major, MSB-first, 11 bytes/row x 42 rows = 462 bytes each.
// White pixels in the source PNG are 1; transparent pixels are 0.
#define DISPLAY_CHAR_W  86
#define DISPLAY_CHAR_H  42
#define DISPLAY_CHAR_STRIDE 11  // bytes per row

extern const uint8_t DisplayChar_A[462];
extern const uint8_t DisplayChar_ASharp[462];
extern const uint8_t DisplayChar_B[462];
extern const uint8_t DisplayChar_C[462];
extern const uint8_t DisplayChar_CSharp[462];
extern const uint8_t DisplayChar_D[462];
extern const uint8_t DisplayChar_DSharp[462];
extern const uint8_t DisplayChar_E[462];
extern const uint8_t DisplayChar_F[462];
extern const uint8_t DisplayChar_FSharp[462];
extern const uint8_t DisplayChar_G[462];
extern const uint8_t DisplayChar_GSharp[462];

// Draw a display character bitmap at (x, y):
// SH1106_DrawBitmap(x, y, DisplayChar_A, DISPLAY_CHAR_W, DISPLAY_CHAR_H, DISPLAY_CHAR_STRIDE);

#endif
