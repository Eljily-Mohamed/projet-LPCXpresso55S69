/*
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"

#define MAIN3

#ifdef MAIN1

#include "fsl_i2c.h"

#define I2C4				((I2C_Type *)FLEXCOMM4)
#define I2C4_MASTER_CLK		12000000

#define MMA8652FC_ADDR		0x1D


static volatile status_t i2c_res;

static void i2c_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    /* Signal transfer success when received success status. */
   i2c_res = status;
}

static status_t mma8652_id(I2C_Type *base, i2c_master_handle_t* handle, uint32_t *id)
{
	uint8_t txBuff[1];
    i2c_master_transfer_t xfer = {
    	.slaveAddress 	= MMA8652FC_ADDR,			// sensor address
        .direction 		= kI2C_Read,
        .subaddress 	= 0x0D,					// whoami register offset
        .subaddressSize = 1,
        .data 			= txBuff,
        .dataSize 		= 1,
        .flags 			= kI2C_TransferDefaultFlag
    };
	i2c_res=kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(base, handle, &xfer);

    while (i2c_res==kStatus_I2C_Busy) {
    }

    if (i2c_res==kStatus_Success) *id=txBuff[0];

    return i2c_res;
}

/*
 * @brief   Application entry point.
 */
int main(void) {
	uint32_t id=0;

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("I2C example\r\n");
    /* setup a default configuration */
    i2c_master_config_t fc4_config = {
      .enableMaster = true,
      .baudRate_Bps = 400000,		/* mode Fast */
      .enableTimeout = false
    };
    i2c_master_handle_t i2c4_handle;

    /* Init I2C master. */
    I2C_MasterInit(I2C4, &fc4_config, I2C4_MASTER_CLK);
    I2C_MasterTransferCreateHandle(I2C4, &i2c4_handle, i2c_callback, NULL);

    if (mma8652_id(I2C4, &i2c4_handle, &id)==kStatus_Success) {
    	PRINTF("id=%d\r\n",id);
    }

    /* Enter an infinite loop */
    while(1) {
    }

    return 0 ;
}
#endif

#ifdef MAIN2
#include "fsl_i2c.h"
#include "component/mma8652fc.h"

#define I2C4_MASTER_CLK		12000000

int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("I2C example\r\n");
    /* setup the I2C bus */
    i2c_master_config_t fc4_config = {
      .enableMaster = true,
      .baudRate_Bps = 400000,		/* mode Fast */
      .enableTimeout = false
    };

    I2C_MasterInit(I2C4, &fc4_config, I2C4_MASTER_CLK);

    mma8652_init(I2C4, MMA8652_RATE_1_56|MMA8652_SCALE_8G|MMA8652_RES_8);

    PRINTF("Enter the loop\n");
    /* Enter an infinite loop */
    while(1) {
    	uint8_t status;
    	mma8652_status(&status);
    	if (status & MMA8652_DATA_READY) {
    		int32_t data[3];
    		mma8652_read_xyz(data);
    		PRINTF("x=%d, y=%d, z=%d\n", data[0], data[1], data[2]);
    	}
    }

    return 0 ;
}

#endif

#ifdef MAIN3
#include "fsl_i2c.h"
#include "fsl_pint.h"
#include "component/mma8652fc.h"

#define I2C4_MASTER_CLK		12000000

static volatile int cmd=1;

void pint_cb(pint_pin_int_t pintr, uint32_t pmatch_status) {
    switch (pintr) {
    case kPINT_PinInt0:
    	cmd=1;
    	break;
     default:
    	break;
    }
}

int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("I2C example\r\n");
    /* setup the I2C bus */
    i2c_master_config_t fc4_config = {
      .enableMaster = true,
      .baudRate_Bps = 400000,		/* mode Fast */
      .enableTimeout = false
    };

    RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
    I2C_MasterInit(I2C4, &fc4_config, I2C4_MASTER_CLK);
    NVIC_SetPriority(FLEXCOMM4_IRQn,3);

    mma8652_init(I2C4, MMA8652_RATE_6_25|MMA8652_SCALE_2G|MMA8652_RES_12|MMA8652_INT);

    /* Initialize PINT */
    PINT_Init(PINT);

    /* Setup Pin Interrupt 0 for falling edge and enable callback */
    PINT_PinInterruptConfig(PINT, kPINT_PinInt0, kPINT_PinIntEnableFallEdge, pint_cb);
    PINT_EnableCallbackByIndex(PINT, kPINT_PinInt0);
    NVIC_SetPriority(PIN_INT0_IRQn,2);

    PRINTF("Enter the loop\n");
    /* Enter an infinite loop */
    while(1) {
    	if (cmd) {
    		int32_t data[3];
    		mma8652_read_xyz(data);
    		PRINTF("x=%d, y=%d, z=%d\r\n", (int)data[0], (int)data[1], (int)data[2]);
    		cmd=0;
    	}
    }

    return 0 ;
}

#endif
