/*
 * (C) 2023, E Boucharé
 */
#include <stdio.h>

#include "lcd.h"
#include "lcd_private.h"

/* see https://github.com/adafruit/Adafruit_STMPE610 for register definition */
#define CHIP_ID_REG					0x00	/* default factory ID */
  #define CHIP_ID_DEFAULT 	0x0811

#define ID_VER_REG					0x02	/* default factory version */
  #define ID_VER_DEFAULT    0x03

#define SYS_CTRL1_REG				0x03	/* Reset Control */
  #define SYS_CTRL1_RESET   0x00

#define SYS_CTRL2_REG				0x04	/* Clock Control */
#define SPI_CFG_REG					0x08	/* SPI interface configuration */

#define INT_CTRL_REG				0x09	/* Interrupt control */
  #define INT_CTRL_POL_HIGH	0x04
  #define INT_CTRL_POL_LOW	0x00
  #define INT_CTRL_EDGE     0x02
  #define INT_CTRL_LEVEL    0x00
  #define INT_CTRL_ENABLE   0x01
  #define INT_CTRL_DISABLE  0x00

#define INT_EN_REG					0x0A	/* Interrupt enable */
  #define INT_EN_TOUCHDET   0x01
  #define INT_EN_FIFOTH     0x02
  #define INT_EN_FIFOOF     0x04
  #define INT_EN_FIFOFULL   0x08
  #define INT_EN_FIFOEMPTY  0x10
  #define INT_EN_ADC        0x40
  #define INT_EN_GPIO       0x80

#define INT_STA_REG					0x0B	/* Interrupt status */
  #define INT_STA_TOUCHDET 0x01

/* GPIO */
#define GPIO_SET_PIN_REG			0x10
#define GPIO_CLR_PIN_REG			0x11
#define GPIO_DIR_REG				0x13
#define GPIO_ALT_FUNCT_REG			0x17


#define ADC_CTRL1_REG				0x20	/* ADC control */
  #define ADC_CTRL1_12BIT 0x08
  #define ADC_CTRL1_10BIT 0x00

#define ADC_CTRL2_REG				0x21	/* ADC control */
  #define ADC_CTRL2_1_625MHZ  0x00
  #define ADC_CTRL2_3_25MHZ   0x01
  #define ADC_CTRL2_6_5MHZ    0x02

#define TSC_CTRL_REG				0x40	/* Touch-screen controller setup */
  #define TSC_CTRL_EN         0x01
  #define TSC_CTRL_XYZ        0x00
  #define TSC_CTRL_XY         0x02
  #define TSC_CTRL_STA        0x80 /* bit set to one if touched */

#define TSC_CFG_REG					0x41	/* Touchscreen controller configuration */
  #define TSC_CFG_1SAMPLE     0x00
  #define TSC_CFG_2SAMPLE     0x40
  #define TSC_CFG_4SAMPLE     0x80
  #define TSC_CFG_8SAMPLE     0xC0
  #define TSC_CFG_DELAY_10US  0x00
  #define TSC_CFG_DELAY_50US  0x08
  #define TSC_CFG_DELAY_100US 0x10
  #define TSC_CFG_DELAY_500US 0x18
  #define TSC_CFG_DELAY_1MS   0x20
  #define TSC_CFG_DELAY_5MS   0x28
  #define TSC_CFG_DELAY_10MS  0x30
  #define TSC_CFG_DELAY_50MS  0x38
  #define TSC_CFG_SETTLE_10US 0x00
  #define TSC_CFG_SETTLE_100US 0x01
  #define TSC_CFG_SETTLE_500US 0x02
  #define TSC_CFG_SETTLE_1MS  0x03
  #define TSC_CFG_SETTLE_5MS  0x04
  #define TSC_CFG_SETTLE_10MS 0x05
  #define TSC_CFG_SETTLE_50MS 0x06
  #define TSC_CFG_SETTLE_100MS 0x07

#define FIFO_TH_REG					0x4A	/* FIFO level to generate interrupt */

#define FIFO_STA_REG				0x4B	/* Current status of FIFO */
  #define FIFO_STA_RESET    0x01
  #define FIFO_STA_OFLOW    0x80
  #define FIFO_STA_FULL     0x40
  #define FIFO_STA_EMPTY    0x20
  #define FIFO_STA_THTRIG   0x10


#define FIFO_SIZE_REG				0x4C	/* Current filled level of FIFO */

/* Data port for TSC data address */
#define TSC_DATA_X_REG				0x4D
#define TSC_DATA_Y_REG				0x4F
#define TSC_FRACTION_Z_REG			0x56

#define TSC_DATA_XYZ_REG			0x52



#define TSC_I_DRIVE_REG				0x58	/* Touchscreen controller drive I */
  #define TSC_I_DRIVE_20MA 0x00
  #define TSC_I_DRIVE_50MA 0x01

/* Low level access protocol
 *   write: w    addr     | data
 *   read:  w (0x80|addr) | (0x80|addr+1) |       0
 *          r             \    data[0]    \    data[1]
 */
static void lcd_ts_write(uint8_t reg, uint8_t val)
{
	uint8_t data[2]={reg,val};
	LCD_TS_EN;
	spi_write(LCD_SPI,data,2);
	LCD_TS_DIS;
}

static void lcd_ts_read(uint8_t addr, uint8_t *val)
{
	uint8_t data[2]={0x80|addr,0};
	LCD_TS_EN;
	spi_write(LCD_SPI,data,2);
	LCD_TS_DIS;
	*val = data[1];
}

static void lcd_ts_read16(uint8_t addr, uint16_t *val)
{
	uint8_t data[3]={0x80|addr,0x80|(addr+1),0};
	LCD_TS_EN;
	spi_write(LCD_SPI,data,3);
	LCD_TS_DIS;
	*val = (data[1]<<8)|data[2];
}

static long map(long x, long in_min, long in_max, long out_min, long out_max) {
  if (x < in_min) {
    x = in_min;
  }
  if (x > in_max) {
    x = in_max;
  }
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
/****************************************************************
 * API
 ****************************************************************/
uint8_t lcd_ts_get_data(uint16_t *x, uint16_t *y, uint8_t *z) {
	uint8_t  data[5];
	uint8_t  samples, cnt;
	uint32_t sum_sample_x = 0, sum_sample_y = 0;
	uint16_t sum_sample_z = 0;
	long xi, yi;
	
	lcd_ts_read(FIFO_SIZE_REG, &cnt);
	if (!cnt) return 0;
	
	samples = cnt;
	
	while (cnt) {
		/* each sample is a packed byte array
		      Byte0    |    Byte1    |    Byte2   |    Byte3
           [11:4] of X | [3:0] of X  | [7:0] of Y | [7:0] of Z
                       | [11:8] of Y |            |
		*/
		uint16_t sample_coord1, sample_coord2;
		uint8_t  sample_z;
#if 1
		data[0] = data[1] = data[2] = data[3] = 0xD7;
		data[4] = 0;
		LCD_TS_EN;
		spi_write(LCD_SPI,data,5);
		LCD_TS_DIS;
		sample_coord1   = (data[1]<<4) | (data[2] >> 4);
		sample_coord2   = ((data[2] & 0x0F)<<8) | data[3];
		sample_z      = data[4];
#else
		data[0] = 0xD2; data[1] = 0xD3; data[2] = 0xD4; data[3] = 0xD5;
		data[4] = 0;
		for (uint8_t i = 0; i < 4; i++) {
			lcd_ts_read(0x52+i,data+i);
		}
		sample_coord1   = (data[0]<<4) | (data[1] >> 4);
		sample_coord2   = ((data[1] & 0x0F)<<8) | data[2];
		sample_z      = data[3];
#endif
		sum_sample_x += sample_coord1;
		sum_sample_y += sample_coord2;
		sum_sample_z += sample_z;
		cnt--;
	}
	xi = map(sum_sample_x / samples, disp.ts_cal[0], disp.ts_cal[1], 0, LCD_WIDTH-1);
	yi = map(sum_sample_y / samples, disp.ts_cal[2], disp.ts_cal[3], 0, LCD_HEIGHT-1);
	switch (disp.orientation) {
	case PORTRAIT:
		*x = xi;
		*y = yi;
		break;
	case LANDSCAPE:
		*x = yi;
		*y = LCD_WIDTH-1-xi;
		break;
	case PORTRAIT_INV:
		*x = LCD_WIDTH-1-xi;
		*y = LCD_HEIGHT-1-yi;
		break;
	case LANDSCAPE_INV:
		*x = LCD_HEIGHT-1-yi;
		*y = xi;
		break;
	}
	*z = sum_sample_z / samples;
//	printf("x=%d, y=%d, z=%d, samples=%d\r\n",*x,*y,*z,samples);
	
	lcd_ts_write(FIFO_STA_REG, FIFO_STA_RESET); // clear FIFO
	lcd_ts_write(FIFO_STA_REG, 0);                    // unreset
	
	return samples;
}

int lcd_ts_touched(void)
{
	uint8_t ctrl;
	lcd_ts_read(TSC_CTRL_REG, &ctrl);
	if (ctrl & 0x80) return 1;
	
	return 0;
}

void lcd_ts_id(uint8_t *ver, uint16_t *id)
{
	lcd_ts_read16(CHIP_ID_REG, id);
	lcd_ts_read(ID_VER_REG, ver);
//	printf("TSC ver=%x, id=%x\n",(unsigned)*ver,(unsigned)*id);
}

void lcd_ts_init(void)
{
	uint16_t id;
	uint8_t ver;
	
	LCD_TS_DIS;
	lcd_switch_to(LCD_TS);
	
	lcd_ts_write(SYS_CTRL1_REG,2);								// software reset
	delay_ms(10);
	lcd_ts_write(SYS_CTRL2_REG,0);
	lcd_ts_read16(CHIP_ID_REG, &id);
	lcd_ts_read(ID_VER_REG, &ver);
	lcd_ts_write(TSC_CTRL_REG, TSC_CTRL_XYZ | TSC_CTRL_EN);		// XYZ and enable!
//	lcd_ts_write(INT_EN_REG, INT_EN_TOUCHDET);					// Enable "touch detected" IRQ
//	lcd_ts_write(ADC_CTRL1_REG, ADC_CTRL1_10BIT | (0x6 << 4));	// 96 clocks per conversion
//	lcd_ts_write(ADC_CTRL2_REG, ADC_CTRL2_6_5MHZ);
	lcd_ts_write(TSC_CFG_REG, TSC_CFG_4SAMPLE | TSC_CFG_DELAY_1MS | TSC_CFG_SETTLE_5MS);
	lcd_ts_write(TSC_FRACTION_Z_REG, 0x6);
	lcd_ts_write(FIFO_TH_REG, 1);
	lcd_ts_write(FIFO_STA_REG, FIFO_STA_RESET);					// Reset FIFO
	lcd_ts_write(FIFO_STA_REG, 0);   							// unreset
	lcd_ts_write(TSC_I_DRIVE_REG, TSC_I_DRIVE_50MA);
	lcd_ts_write(INT_STA_REG, 0xFF); // reset all ints
//	lcd_ts_write(INT_CTRL_REG, INT_CTRL_POL_LOW | INT_CTRL_EDGE | INT_CTRL_ENABLE);
//	printf("TSC ver=%x, id=%x\r\n",(unsigned)ver,(unsigned)id);
}
