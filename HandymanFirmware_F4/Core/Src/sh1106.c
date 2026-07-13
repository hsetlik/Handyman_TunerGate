#include "sh1106.h"
#include "stm32f4xx_hal_def.h"

// SH1106 control byte: Co=0 (no more control bytes follow), D/C# = 0 -> command
#define SH1106_CTRL_CMD  0x00
// Co=0, D/C# = 1 -> data
#define SH1106_CTRL_DATA 0x40

#define SH1106_I2C_TIMEOUT 100

static uint8_t s_buffer[SH1106_Width * SH1106_PAGES];
static bool isDisplayOn = false;

static HAL_StatusTypeDef SH1106_WriteCommand(uint8_t cmd)
{
    return HAL_I2C_Mem_Write(&SH1106_I2C, SH1106_Addr << 1, SH1106_CTRL_CMD,
                              I2C_MEMADD_SIZE_8BIT, &cmd, 1, SH1106_I2C_TIMEOUT);
}

static HAL_StatusTypeDef SH1106_WriteData(uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Write(&SH1106_I2C, SH1106_Addr << 1, SH1106_CTRL_DATA,
                              I2C_MEMADD_SIZE_8BIT, data, size, SH1106_I2C_TIMEOUT);
}

HAL_StatusTypeDef SH1106_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;

    HAL_Delay(100); // panel power-on delay

    status |= SH1106_WriteCommand(0xAE); // display off
    status |= SH1106_WriteCommand(0xD5); // set display clock divide ratio/osc freq
    status |= SH1106_WriteCommand(0x80);
    status |= SH1106_WriteCommand(0xA8); // set multiplex ratio
    status |= SH1106_WriteCommand(SH1106_Height - 1);
    status |= SH1106_WriteCommand(0xD3); // set display offset
    status |= SH1106_WriteCommand(0x00);
    status |= SH1106_WriteCommand(0x40); // set display start line = 0
    status |= SH1106_WriteCommand(0xAD); // set DC-DC pump
    status |= SH1106_WriteCommand(0x8B);
#ifdef SH1106_ROTATION_180
    status |= SH1106_WriteCommand(0xA0); // segment re-map: normal (rotated 180° from default)
    status |= SH1106_WriteCommand(0xC0); // COM scan: normal (rotated 180° from default)
#else
    status |= SH1106_WriteCommand(0xA1); // segment re-map: SEG131→col0 (default orientation)
    status |= SH1106_WriteCommand(0xC8); // COM scan: reverse (default orientation)
#endif
    status |= SH1106_WriteCommand(0xDA); // set COM pins hardware config
    status |= SH1106_WriteCommand(0x12);
    status |= SH1106_WriteCommand(0x81); // set contrast control
    status |= SH1106_WriteCommand(0x80);
    status |= SH1106_WriteCommand(0xD9); // set pre-charge period
    status |= SH1106_WriteCommand(0x1F);
    status |= SH1106_WriteCommand(0xDB); // set VCOMH deselect level
    status |= SH1106_WriteCommand(0x40);
    status |= SH1106_WriteCommand(0xA4); // entire display on/off -> resume RAM content
    status |= SH1106_WriteCommand(0xA6); // normal (not inverted) display
    status |= SH1106_WriteCommand(0x20); // page addressing mode (SH1106 default)

    SH1106_Fill(SH1106_COLOR_WHITE);
    status |= SH1106_UpdateScreen();

    status |= SH1106_WriteCommand(0xAF); // display on
    isDisplayOn = true;

    return status;
}

void SH1106_Fill(SH1106_Color color)
{
    uint8_t fill = (color == SH1106_COLOR_WHITE) ? 0xFF : 0x00;
    for (uint32_t i = 0; i < sizeof(s_buffer); i++) {
        s_buffer[i] = fill;
    }
}

void SH1106_DrawPixel(uint8_t x, uint8_t y, SH1106_Color color)
{
    if (x >= SH1106_Width || y >= SH1106_Height) {
        return;
    }

    uint16_t idx = x + (y / 8) * SH1106_Width;
    uint8_t bit = y % 8;

    if (color == SH1106_COLOR_WHITE) {
        s_buffer[idx] |= (1 << bit);
    } else {
        s_buffer[idx] &= ~(1 << bit);
    }
}

HAL_StatusTypeDef SH1106_UpdateScreen(void)
{
    HAL_StatusTypeDef status = HAL_OK;

    for (uint8_t page = 0; page < SH1106_PAGES; page++) {
        status |= SH1106_WriteCommand(0xB0 + page); // set page address

        // set column address (12-bit split across two commands), offset
        // accounts for the panel's 128-of-132 column alignment
        status |= SH1106_WriteCommand(0x00 | (SH1106_COL_OFFSET & 0x0F));
        status |= SH1106_WriteCommand(0x10 | (SH1106_COL_OFFSET >> 4));

        status |= SH1106_WriteData(&s_buffer[page * SH1106_Width], SH1106_Width);
    }

    return status;
}

void SH1106_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap,
                       uint8_t w, uint8_t h, uint8_t w_bytes, bool isWhite)
{
    SH1106_Color txt = isWhite ? SH1106_COLOR_WHITE : SH1106_COLOR_BLACK;
    SH1106_Color bkgnd = isWhite ? SH1106_COLOR_BLACK : SH1106_COLOR_WHITE;
    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            uint8_t bit_mask = 0x80 >> (col % 8);
            SH1106_Color color = (bitmap[row * w_bytes + col / 8] & bit_mask)
                                 ? txt : bkgnd;
            SH1106_DrawPixel(x + col, y + row, color);
        }
    }
}

HAL_StatusTypeDef SH1106_SetContrast(uint8_t value)
{
    HAL_StatusTypeDef status = SH1106_WriteCommand(0x81);
    status |= SH1106_WriteCommand(value);
    return status;
}

HAL_StatusTypeDef SH1106_DisplayOn(uint8_t on)
{
    HAL_StatusTypeDef onStatus = SH1106_WriteCommand(on ? 0xAF : 0xAE);
    if(onStatus == HAL_OK){
        isDisplayOn = on > 0;
    }
    return onStatus;
}

void SH1106_DrawRectangle(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, SH1106_Color col){
    for(uint8_t x = x0; x < x0 + w; ++x){
        for(uint8_t y = y0; y < y0 + h; ++y){
            SH1106_DrawPixel(x, y, col);
        }
    }
}

bool SH1106_isDisplayOn(){
    return isDisplayOn;
}
