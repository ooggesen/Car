#ifndef BASE.H
#define BASE.H

#include "memorysaver.h"
#include "my_motor.h"
#include "my_wifi.h"
#include "my_wifi.h"
#include "my_bt.h"
#include "my_arducam.h"

void tests();
void match_com(String in);

//Debug stuff
void print_debug(String text);
void println_debug(String text);

#endif //BASE.H
