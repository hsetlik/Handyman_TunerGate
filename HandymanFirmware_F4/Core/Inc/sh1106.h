#ifndef SH1106_H
#define SH1106_H
#include "main.h"
#include <stdint.h>

// definitions for the hardware
#define SH1106_Addr 0x3D
#define SH1106_Width 128
#define SH1106_Height 64
#define SH1106_I2C hi2c1

// SH1106 has 132x64 SRAM but only 128 columns are wired to the panel,
// centered with a 2px offset on most 1.3" boards
#define SH1106_COL_OFFSET 2

#define SH1106_PAGES (SH1106_Height / 8)

typedef enum {
    SH1106_COLOR_BLACK = 0,
    SH1106_COLOR_WHITE = 1
} SH1106_Color;

// Returns HAL_OK on success, or the first HAL error encountered during init
HAL_StatusTypeDef SH1106_Init(void);

// Local framebuffer helpers- call SH1106_UpdateScreen() to push to the panel
void SH1106_Fill(SH1106_Color color);
void SH1106_DrawPixel(uint8_t x, uint8_t y, SH1106_Color color);
HAL_StatusTypeDef SH1106_UpdateScreen(void);

HAL_StatusTypeDef SH1106_SetContrast(uint8_t value);
HAL_StatusTypeDef SH1106_DisplayOn(uint8_t on);

#endif
