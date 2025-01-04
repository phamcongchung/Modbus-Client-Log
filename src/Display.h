#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal_I2C.h>

class Display : public LiquidCrystal_I2C{
public:
  Display(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
   : LiquidCrystal_I2C(lcd_Addr, lcd_cols, lcd_rows){}

  void clearRow(int row);
};

#endif