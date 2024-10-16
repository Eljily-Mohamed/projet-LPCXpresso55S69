/*
 * bench.h
 *
 *  Created on: Sep 26, 2024
 *      Author: mohamed
 */

#ifndef BENCH_H_
#define BENCH_H_
#define LCD_COLOR_BLACK  0x0000
#define LCD_COLOR_RED    0xF800
#define LCD_COLOR_GREEN  0x07E0
#define LCD_COLOR_BLUE   0x001F
#define LCD_COLOR_WHITE  0xFFFF


#include <stdio.h>
#include "calc.h"


header* draw_x_y_z (Calc *cc, header *hd);


#endif /* BENCH_H_ */
