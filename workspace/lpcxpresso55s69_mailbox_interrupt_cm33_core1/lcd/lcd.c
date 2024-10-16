/*
 * (C) 2023, E Boucharé
 */
#include "lcd.h"
#include "lcd_private.h"

Display disp = {
	.orientation = PORTRAIT,
	.width = LCD_WIDTH,
	.height = LCD_HEIGHT,
	.ts_cal = {320,3800,220,3800},
	.dc = {{0,0,LCD_WIDTH,LCD_HEIGHT}, WHITE, BLACK, &fixed12, DIR_HORIZONTAL|ALIGN_SE, 1}
};

/* switch config to use one of LCD_DPY, LCD_TS, LCD_SD */
void lcd_switch_to(uint32_t cfg)
{
	static int lcd_cur = -1;
	
	if (lcd_cur == (int)cfg) return;
	
	switch (lcd_cur) {
	case LCD_DPY:
		LCD_DPY_DIS;
		break;
	case LCD_TS:
		LCD_TS_DIS;
		break;
	default:
		break;
	}
	
	lcd_cur = spi_update_cfg(cfg);
}

/* lcd board initialisation */
void lcd_init(void)
{
	// configure pins and spi
	lcd_pin_cfg();
	spi_master_init(LCD_DPY);
	
	// init display
	lcd_dpy_init();
	LCD_DPY_DIS;
	
	// init touchscreen
	lcd_ts_init();
	LCD_TS_DIS;
}
