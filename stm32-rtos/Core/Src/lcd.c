#include "lcd.h"
#include "i2c.h"
#include <stdint.h>

/*
 * All LCD communication goes through the PCF8574 I2C expander.
 * Each nibble requires two I2C writes: EN high (strobe), then EN low.
 * The backlight bit (LCD_BL) is always set to keep the backlight on.
 */

/* CPU busy-wait — ~8 cycles per iteration at 64 MHz ≈ 125 ns each. */
static void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 8U; i++);
}

static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++)
        delay_us(1000U);
}

/* Send one nibble (upper 4 bits of val) with an EN strobe */
static void lcd_write_nibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = LCD_BL | rs;
    data |= (uint8_t)((nibble & 0x01U) ? LCD_D4 : 0U);
    data |= (uint8_t)((nibble & 0x02U) ? LCD_D5 : 0U);
    data |= (uint8_t)((nibble & 0x04U) ? LCD_D6 : 0U);
    data |= (uint8_t)((nibble & 0x08U) ? LCD_D7 : 0U);

    I2C1_Write(LCD_I2C_ADDR, data | LCD_EN);
    delay_us(1U);
    I2C1_Write(LCD_I2C_ADDR, data);
    delay_us(50U);
}

static void lcd_write_byte(uint8_t val, uint8_t rs) {
    lcd_write_nibble(val >> 4U, rs);
    lcd_write_nibble(val & 0x0FU, rs);
    delay_ms(2U);
}

void LCD_Write_Cmd(uint8_t val) {
    lcd_write_byte(val, 0U);
}

void LCD_Put_Char(uint8_t c) {
    lcd_write_byte(c, LCD_RS);
}

void LCD_Init(void) {
    I2C1_Init();

    delay_ms(50U); /* wait for LCD power-on */

    /* HD44780 4-bit init sequence */
    lcd_write_nibble(0x03U, 0U);
    delay_ms(5U);
    lcd_write_nibble(0x03U, 0U);
    delay_us(150U);
    lcd_write_nibble(0x03U, 0U);
    delay_us(150U);
    lcd_write_nibble(0x02U, 0U); /* switch to 4-bit mode */
    delay_us(150U);

    LCD_Write_Cmd(0x28U); /* 2-line, 5x7 font */
    LCD_Write_Cmd(0x08U); /* display off */
    LCD_Write_Cmd(0x01U); /* clear display */
    delay_ms(2U);
    LCD_Write_Cmd(0x06U); /* entry mode: cursor increment, no shift */
    LCD_Write_Cmd(0x0FU); /* display on, cursor on, blink on */

    /* Load custom bar-graph characters into CGRAM */
    static const uint8_t cgram[8][8] = {
        { 0x11, 0x0A, 0x04, 0x1B, 0x11, 0x11, 0x11, 0x0E },
        { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
        { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
        { 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C },
        { 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E },
        { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    };
    LCD_Write_Cmd(0x40U);
    for (int i = 0; i < 64; i++)
        LCD_Put_Char(cgram[i / 8][i % 8]);

    LCD_Write_Cmd(0x80U); /* return to DDRAM address 0 */
}

void LCD_Set_Cursor(uint8_t line, uint8_t column) {
    uint8_t address = ((line - 1U) * 0x40U) + (column - 1U);
    LCD_Write_Cmd(0x80U | (address & 0x7FU));
}

void LCD_Put_Str(char *str) {
    for (int16_t i = 0; i < 16 && str[i] != 0; i++)
        LCD_Put_Char((uint8_t)str[i]);
}

void LCD_Put_Num(int16_t num) {
    int16_t p;
    int16_t f = 0;
    int8_t  ch[5];

    for (int16_t i = 0; i < 5; i++) {
        p = 1;
        for (int16_t j = 4 - i; j > 0; j--)
            p = p * 10;
        ch[i] = (int8_t)(num / p);
        if (num >= p && !f) f = 1;
        num = num - ch[i] * p;
        ch[i] = (int8_t)(ch[i] + 48);
        if (f) LCD_Put_Char((uint8_t)ch[i]);
    }
}

void LCD_BarGraphic(int16_t value, int16_t size) {
    value = value * size / 20;
    for (int16_t i = 0; i < size; i++) {
        if (value > 5) {
            LCD_Put_Char(0x05U);
            value -= 5;
        } else {
            LCD_Put_Char((uint8_t)value);
            break;
        }
    }
}

void LCD_BarGraphicXY(int16_t pos_x, int16_t pos_y, int16_t value) {
    LCD_Set_Cursor((uint8_t)pos_x, (uint8_t)pos_y);
    for (int16_t i = 0; i < 16; i++) {
        if (value > 5) {
            LCD_Put_Char(0x05U);
            value -= 5;
        } else {
            LCD_Put_Char((uint8_t)value);
            while (i++ < 16)
                LCD_Put_Char(0U);
        }
    }
}
