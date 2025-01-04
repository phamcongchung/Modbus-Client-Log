#include "Display.h"

void Display::clearRow(int row){
  this->setCursor(0, row);
  for (int i = 0; i < 20; i++)
    this->print(' ');
  this->setCursor(0, row);
}
