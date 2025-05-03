#include "lcd_util.h"
#include <stdint.h>


uint8_t tick_glyph[8] = {
  0b00000,
  0b00000,
  0b00001,
  0b00010,
  0b10100,
  0b01000,
  0b00000,
};

uint8_t legacy_glyph[8] = {
  0b10000,
  0b10000,
  0b10111,
  0b11100,
  0b00111,
  0b00100,
  0b00111,
};

void lcd_define_glyphs()
{
  lcd.createChar(TICK_CHAR, tick_glyph);
  lcd.createChar(LEGACY_CHAR, legacy_glyph);
}

