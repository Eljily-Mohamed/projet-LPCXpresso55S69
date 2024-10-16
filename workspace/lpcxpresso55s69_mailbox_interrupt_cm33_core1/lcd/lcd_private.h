/*
 * (C) 2023, E Boucharé
 */
#ifndef _LCD_COMMON_H_
#define _LCD_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include "lcd.h"
#include "fsl_gpio.h"

// timer utility: polling delay
#define delay_ms(ms)		wait_ms(CTIMER2,(ms))

void wait_ms(CTIMER_Type *tmr, uint32_t ms);

// pin configuration
void lcd_pin_cfg(void);

// SPI utility
#define LCD_SPI			SPI8		/* HS_SPI = FLexcomm8 */
#define LCD_SPINUM		8U

typedef SPI_Type		SPI;

SPI* spi_master_init(int cfg);
uint32_t spi_update_cfg(uint32_t cfg);
void spi_write(SPI *spi, uint8_t *data, uint32_t n);
void spi_write_byte(SPI *spi, uint8_t data);
void spi_write16(SPI *spi, uint16_t *data, uint32_t n);
void spi_write16_n(SPI *spi, uint16_t data, uint32_t n);


/* clock frequency and SPI config for DPY, TS and SD devices */
#define LCD_DPY_CLOCK	50000000
#define LCD_DPY_CFG		SPI_MODE3
#define LCD_TS_CLOCK	1000000
#define LCD_TS_CFG		SPI_MODE2

/* SPI Signals*/
/* D13 = SCLK = GPIO1.2
 * D12 = MISO = GPIO1.3
 * D11 = MOSI = GPIO0.26
 */

/* GPIO signals */
/* D10 = TFT CS = GPIO1.1 = LSPI_HS_SSEL1 */
#define LCD_DPY_EN		GPIO_PinWrite(GPIO,1,1,0)
#define LCD_DPY_DIS		GPIO_PinWrite(GPIO,1,1,1)

/* D9 = TFT DC = GPIO1.5 */
#define LCD_DPY_CMD		GPIO_PinWrite(GPIO,1,5,0)
#define LCD_DPY_DATA	GPIO_PinWrite(GPIO,1,5,1)

/* D3 = TFT Back Light = GPIO1.6 (not used)*/
#define LCD_DPY_BL_ON	GPIO_PinWrite(GPIO,1,6,1)
#define LCD_DPY_BL_OFF	GPIO_PinWrite(GPIO,1,6,0)

/* D8 = RT CS = GPIO1_8 (touchscreen) */
#define LCD_TS_EN		GPIO_PinWrite(GPIO,1,8,0)
#define LCD_TS_DIS		GPIO_PinWrite(GPIO,1,8,1)

/* global Display Context */
typedef struct _Display {
	Orientation		orientation;	/* screen orientation */
	uint16_t		width;			/* display width */
	uint16_t		height;			/* display height */
	uint16_t		ts_cal[4];		/* touchscreen calibration parameters */
	DC				dc;				/* drawing context */
} Display;	/* Display Context */

extern Display disp;

void lcd_dpy_init(void);

#ifdef __cplusplus
}
#endif
#endif
