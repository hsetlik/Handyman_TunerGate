#ifndef DISPLAY_CHARS_H
#define DISPLAY_CHARS_H
#include <stdint.h>
#include "sh1106.h"

// Bitmaps are row-major, MSB-first. White pixels in the source PNG are 1;
// transparent pixels are 0.
#define DISPLAY_CHAR_W_HORIZONTAL  86
#define DISPLAY_CHAR_H_HORIZONTAL  42
#define DISPLAY_CHAR_STRIDE_HORIZONTAL 11  // bytes per row = ceil(86 / 8); 462 bytes each

#define DISPLAY_CHAR_W_VERTICAL  49
#define DISPLAY_CHAR_H_VERTICAL  42
#define DISPLAY_CHAR_STRIDE_VERTICAL 7  // bytes per row = ceil(49 / 8); 294 bytes each

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

extern const uint8_t DisplayChar_A_Vertical[294];
extern const uint8_t DisplayChar_ASharp_Vertical[294];
extern const uint8_t DisplayChar_B_Vertical[294];
extern const uint8_t DisplayChar_C_Vertical[294];
extern const uint8_t DisplayChar_CSharp_Vertical[294];
extern const uint8_t DisplayChar_D_Vertical[294];
extern const uint8_t DisplayChar_DSharp_Vertical[294];
extern const uint8_t DisplayChar_E_Vertical[294];
extern const uint8_t DisplayChar_F_Vertical[294];
extern const uint8_t DisplayChar_FSharp_Vertical[294];
extern const uint8_t DisplayChar_G_Vertical[294];
extern const uint8_t DisplayChar_GSharp_Vertical[294];

// Draw a display character bitmap at (x, y):
// SH1106_DrawBitmap(x, y, DisplayChar_A, DISPLAY_CHAR_W_HORIZONTAL, DISPLAY_CHAR_H_HORIZONTAL, DISPLAY_CHAR_STRIDE_HORIZONTAL);

#endif
