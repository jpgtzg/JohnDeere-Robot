#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdint.h>

/*
 * PCF8574 I2C backpack — default address (A2=A1=A0=0 → 0x27).
 * Change to 0x3F if your board has all address jumpers set.
 *
 * PCF8574 bit layout:
 *   bit7=D7  bit6=D6  bit5=D5  bit4=D4  bit3=BL  bit2=EN  bit1=RW  bit0=RS
 */
#define LCD_I2C_ADDR  0x27U

#define LCD_RS  (0x1U << 0U)
#define LCD_RW  (0x1U << 1U)
#define LCD_EN  (0x1U << 2U)
#define LCD_BL  (0x1U << 3U)   /* backlight — keep set to keep backlight on */
#define LCD_D4  (0x1U << 4U)
#define LCD_D5  (0x1U << 5U)
#define LCD_D6  (0x1U << 6U)
#define LCD_D7  (0x1U << 7U)

/* High-level command macros (unchanged API) */
#define LCD_Clear()          LCD_Write_Cmd(0x01U)
#define LCD_Display_ON()     LCD_Write_Cmd(0x0EU)
#define LCD_Display_OFF()    LCD_Write_Cmd(0x08U)
#define LCD_Cursor_Home()    LCD_Write_Cmd(0x02U)
#define LCD_Cursor_Blink()   LCD_Write_Cmd(0x0FU)
#define LCD_Cursor_ON()      LCD_Write_Cmd(0x0EU)
#define LCD_Cursor_OFF()     LCD_Write_Cmd(0x0CU)
#define LCD_Cursor_Left()    LCD_Write_Cmd(0x10U)
#define LCD_Cursor_Right()   LCD_Write_Cmd(0x14U)
#define LCD_Cursor_SLeft()   LCD_Write_Cmd(0x18U)
#define LCD_Cursor_SRight()  LCD_Write_Cmd(0x1CU)

void LCD_Init(void);
void LCD_Write_Cmd(uint8_t val);
void LCD_Put_Char(uint8_t c);
void LCD_Set_Cursor(uint8_t line, uint8_t column);
void LCD_Put_Str(char *str);
void LCD_Put_Num(int16_t num);
void LCD_BarGraphic(int16_t value, int16_t size);
void LCD_BarGraphicXY(int16_t pos_x, int16_t pos_y, int16_t value);

#endif /* INC_LCD_H_ */
