#pragma once

#include <LiquidCrystal.h>

extern LiquidCrystal lcd;

const char TICK_CHAR = 1;
const char LEGACY_CHAR = 2;

void lcd_define_glyphs();

