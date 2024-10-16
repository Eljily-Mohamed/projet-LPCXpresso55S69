/*
 * (C) 2023, E Boucharé
 */
#include "lcd_private.h"

#include "fsl_gpio.h"
#include "fsl_ctimer.h"
#include "fsl_spi.h"
#include "fsl_flexcomm.h"
#include <assert.h>

/* Timer utility */
static CTIMER_Type *const s_ctimerBases[] = CTIMER_BASE_PTRS;
static const clock_ip_name_t s_ctimerClocks[] = CTIMER_CLOCKS;

static uint32_t CTIMER_GetInstance(CTIMER_Type *base)
{
    uint32_t instance;
    uint32_t ctimerArrayCount = (sizeof(s_ctimerBases) / sizeof(s_ctimerBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ctimerArrayCount; instance++) {
        if (s_ctimerBases[instance] == base) break;
    }

    assert(instance < ctimerArrayCount);

    return instance;
}

void wait_ms(CTIMER_Type *tmr, uint32_t ms)
{
    uint32_t id=CTIMER_GetInstance(tmr);
    uint32_t clk=CLOCK_GetCTimerClkFreq(id);
    CLOCK_EnableClock(s_ctimerClocks[id]);
    tmr->CTCR = 0;							// count on rising edgle of pclk
    tmr->TCR = 1<<1;						// timer stop and reset
    tmr->MR[0] = 10*ms-1;					// Compare-hit with ms
    tmr->MCR = (1<<2)|(1<<1);				// Stop and reset on MR0
    tmr->PR = clk/10000-1;	// prescaler freq at 10kHz
    tmr->IR = 0xFF;							// Reset IRQ flags
    tmr->TCR=1;								// count

    while (tmr->TCR & 1);					// wait until count is done
}

/* Pin configuration */
void lcd_pin_cfg(void)
{
    gpio_pin_config_t out_cfg = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 1U
    };
    
    CLOCK_AttachClk(kMAIN_CLK_to_HSLSPI);	/*!< Switch HSLSPI to MAIN_CLK */
    CLOCK_AttachClk(kMAIN_CLK_to_CTIMER2);	/*!< Switch CTIMER2 to MAIN_CLK */

    /* Enables the clock for the I/O controller */
    CLOCK_EnableClock(kCLOCK_Iocon);
    
    /* Enables the clock for the GPIO1 module */
    CLOCK_EnableClock(kCLOCK_Gpio1);

    /* Initialize pin PIO1_1 (pin 59): TFT_CS */
    GPIO_PinInit(GPIO,1,1<<1U, &out_cfg);

    /* Initialize pin PIO1_5 (pin 31): TFT_DC (Data/Command) */
    GPIO_PinInit(GPIO, 1, 1<<5U, &out_cfg);

    /* Initialize pin PIO1_8 (pin 24): RT_CS (Touchscreen)  */
    GPIO_PinInit(GPIO, 1, 1<<8U, &out_cfg);

	/* SPI8 MOSI */
    IOCON->PIO[0][26] = ((IOCON->PIO[0][26] &
                          /* Mask bits to zero which are setting */
                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                         /* Selects pin function.
                          * : PORT026 (pin 60) is configured as HS_SPI_MOSI. */
                         | IOCON_PIO_FUNC(0x09u)

                         /* Select Digital mode.
                          * : Enable Digital mode.
                          * Digital input is enabled. */
                         | IOCON_PIO_DIGIMODE(1));
	/* TFT_CS */
    IOCON->PIO[1][1] = ((IOCON->PIO[1][1] &
                         /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT11 (pin 59) is configured as PIO1_1. */
                        | IOCON_PIO_FUNC(0)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(1));
	/* SPI8 SCLK */
    IOCON->PIO[1][2] = ((IOCON->PIO[1][2] &
                         /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT12 (pin 61) is configured as HS_SPI_SCK. */
                        | IOCON_PIO_FUNC(6)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(1));
	/* SPI8 MISO */
    IOCON->PIO[1][3] = ((IOCON->PIO[1][3] &
                         /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT13 (pin 62) is configured as HS_SPI_MISO. */
                        | IOCON_PIO_FUNC(6)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(1));
	/* TFT_DC */
    IOCON->PIO[1][5] = ((IOCON->PIO[1][5] &
                         /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

                        /* Selects pin function.
                         * : PORT15 (pin 31) is configured as PIO1_5. */
                        | IOCON_PIO_FUNC(0)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(1));
	/* RT_CS */
    IOCON->PIO[1][8] = ((IOCON->PIO[1][8] &
                         /* Mask bits to zero which are setting */
                         (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK | IOCON_PIO_ASW_MASK)))

                        /* Selects pin function.
                         * : PORT18 (pin 24) is configured as PIO1_8. */
                        | IOCON_PIO_FUNC(0)

                        /* Select Digital mode.
                         * : Enable Digital mode.
                         * Digital input is enabled. */
                        | IOCON_PIO_DIGIMODE(1)

                        /* Analog switch input control.
                         * : For pins PIO0_9, PIO0_11, PIO0_12, PIO0_15, PIO0_18, PIO0_31, PIO1_0 and PIO1_9,
                         * analog switch is closed (enabled).
                         * For the other pins, analog switch is open (disabled). */
                        | IOCON_PIO_ASW(0));
}

/* SPI utility */
#define SPI_MODE0				(0U<<4)
#define SPI_MODE1				(1U<<4)
#define SPI_MODE2				(2U<<4)
#define SPI_MODE3				(3U<<4)

#define SPI_LSB_FIRST			(1<<3)
#define SPI_DATA_8				(0x7<<24)
#define SPI_DATA_16				(0xF<<24)

typedef struct _spi_master_cfg_t {
	uint32_t	flags;		/* cfg flags */
	uint32_t	baud;		/* baud rate in it per second */
} spi_master_cfg_t;

const spi_master_cfg_t spi_cfg[] = {
	{
		.flags = LCD_DPY_CFG,
		.baud  = LCD_DPY_CLOCK
	}, {
		.flags = LCD_TS_CFG,
		.baud  = LCD_TS_CLOCK
	}
};


uint32_t spi_update_cfg(uint32_t cfg)
{
	SPI *spi=LCD_SPI;
	uint32_t clk_hz = CLOCK_GetFlexCommClkFreq(LCD_SPINUM);
	// disable SPI
	spi->CFG &= ~1;
	// change clock
	spi->DIV = (((clk_hz * 10U) / spi_cfg[cfg].baud + 5U) / 10U - 1U) & 0xFFFF;
	// update CFG
	spi->CFG = (spi->CFG & ~((3<<4)|(1<<3))) | spi_cfg[cfg].flags;
	// enable SPI
	spi->CFG |= 1;
	
	return cfg;
}

SPI* spi_master_init(int cfg)
{
	SPI *spi=LCD_SPI;

	if (cfg<2) {
		FLEXCOMM_Init(spi, FLEXCOMM_PERIPH_SPI);
		
		// configure SPI baudrate
		spi->DIV = (((CLOCK_GetFlexCommClkFreq(LCD_SPINUM) * 10U) / spi_cfg[cfg].baud + 5U) / 10U - 1U) & 0xFFFF;
	    
	    /* configure SPI master mode */
	    spi->CFG = (1<<2) | spi_cfg[cfg].flags;
	
	    /* enable FIFOs */
	    spi->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;
	    spi->FIFOCFG |= SPI_FIFOCFG_ENABLETX_MASK | SPI_FIFOCFG_ENABLERX_MASK;

	    /* set FIFOTRIG : generate interrupts 
	       - when TxFIFO is empty (TXLVL=0)
	       - when RxFIFO is not empty (RXLVL=0)
	     */
	    spi->FIFOTRIG = 3;
	
	    /* Set the delay configuration. */
	    spi->DLY = 0;
	
	    spi->CFG |= SPI_CFG_ENABLE_MASK;
	    
	    return spi;
	}
	return NULL;
}

/* send data n 8bit data */
void spi_write(SPI *spi, uint8_t *data, uint32_t n)
{
	for (int i=0; i<n; i++) {
		while ((spi->FIFOSTAT & (1<<5))==0) ;	// TxFIFO full, wait
		spi->FIFOWR=(uint32_t)data[i] | (7U<<24);
		while ((spi->FIFOSTAT & (1<<6))==0) ;	// RxFIFO empty wait
		data[i]=spi->FIFORD;
	}
}

/* send data 1 8bit data */
void spi_write_byte(SPI *spi, uint8_t data)
{
	while ((spi->FIFOSTAT & (1<<5))==0) ;	// TxFIFO full, wait
	spi->FIFOWR=(uint32_t)data | (7U<<24);
	while ((spi->FIFOSTAT & (1<<6))==0) ;	// RxFIFO empty wait
	spi->FIFORD;
}

/* send data n 16bit data */
void spi_write16(SPI *spi, uint16_t *data, uint32_t n)
{
	for (int i=0; i<n; i++) {
		while ((spi->FIFOSTAT & (1<<5))==0) ;	// TxFIFO full, wait
		spi->FIFOWR=(uint32_t)data[i] | (0xF<<24);
		while ((spi->FIFOSTAT & (1<<6))==0) ;	// RxFIFO empty wait
		data[i]=spi->FIFORD;
	}
}

/* send data n times the same 16bit data */
void spi_write16_n(SPI *spi, uint16_t data, uint32_t n)
{
	for (int i=0; i<n; i++) {
		while ((spi->FIFOSTAT & (1<<5))==0) ;	// TxFIFO full, wait
		spi->FIFOWR=(uint32_t)data | (0xF<<24);
		while ((spi->FIFOSTAT & (1<<6))==0) ;	// RxFIFO empty wait
		spi->FIFORD;
	}
}
