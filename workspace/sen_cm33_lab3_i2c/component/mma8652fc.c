#include "mma8652fc.h"


static I2C_Type* i2c;
static i2c_master_handle_t i2c_handle;
static uint32_t mma8652_cfg;

static volatile status_t i2c_res;

#define MMA8652_ADDR				0x1D

#define MMA8652_STATUS      		0x00
#define MMA8652_OUT_X_MSB			0x01
#define MMA8652_OUT_X_LSB			0x02
#define MMA8652_OUT_Y_MSB			0x03
#define MMA8652_OUT_Y_LSB			0x04
#define MMA8652_OUT_Z_MSB			0x05
#define MMA8652_OUT_Z_LSB			0x06
#define MMA8652_F_SETUP				0x09
#define MMA8652_TRIG_CFG			0x0A
#define MMA8652_SYSMOD				0x0B
#define MMA8652_INT_SOURCE			0x0C
#define MMA8652_WHOAMI				0x0D
#define MMA8652_XYZ_DATA_CFG		0x0E
#define MMA8652_CTRL_REG1			0x2A
#define MMA8652_CTRL_REG2			0x2B
#define MMA8652_CTRL_REG3			0x2C
#define MMA8652_CTRL_REG4			0x2D
#define MMA8652_CTRL_REG5			0x2E

#define MMA8652_ID 					0x4A


static void i2c_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    /* Signal transfer success when received success status. */
   i2c_res = status;
}

/* mma8652_init
 *   initializes the sensor according to 'cfg'
 */
status_t mma8652_init(I2C_Type *base, uint32_t cfg)
{
	uint8_t buf[5];
    i2c_master_transfer_t xfer = {
    	.slaveAddress 	= MMA8652_ADDR,		// sensor address
        .direction 		= kI2C_Write,
        .subaddress 	= MMA8652_CTRL_REG1,
        .subaddressSize = 1,
        .data 			= buf,
        .dataSize 		= 5,
        .flags 			= kI2C_TransferNoStopFlag
    };

    /* get a handle to deal with all the transfers on the I2C bus */
	I2C_MasterTransferCreateHandle(base, &i2c_handle, i2c_callback, NULL);
	i2c=base;
	mma8652_cfg=cfg;

	/* initialize the sensor according to cfg */
	/* CTRL_REGx */
	buf[0] = cfg;				// CTRL_REG1: rate, resolution, STANDBY MODE
	buf[1] = 0;					// CTRL_REG2: normal mode, no sleep
	buf[2] = 0;					// CTRL_REG3: no FIFO, no sleep, so no need for wake, low level on INT
	buf[3] = (cfg>>8) & 1;		// CTRL_REG4: allow Data Ready Interrupt if in cfg
	buf[4] = 1;					// CTRL_REG5: Data Ready Interrupt routed to pin INT1
	i2c_res=kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(i2c, &i2c_handle, &xfer);

    while (i2c_res==kStatus_I2C_Busy) {
    }
    if (i2c_res!=kStatus_Success) return kStatus_Fail;

    /* XYZ_DATA_CFG */
    buf[0]=cfg>>16;
	xfer.slaveAddress 	= MMA8652_ADDR;		// sensor address
    xfer.direction 		= kI2C_Write;
    xfer.subaddress 	= MMA8652_XYZ_DATA_CFG;
    xfer.subaddressSize = 1;
    xfer.data 			= buf;
    xfer.dataSize 		= 1;
    xfer.flags 			= kI2C_TransferRepeatedStartFlag|kI2C_TransferNoStopFlag;

	i2c_res=kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(i2c, &i2c_handle, &xfer);

    while (i2c_res==kStatus_I2C_Busy) {
    }
    if (i2c_res!=kStatus_Success) return kStatus_Fail;

    /* CTRL_REG1: ACTIVE MODE */
    buf[0]=cfg | 1;
	xfer.slaveAddress 	= MMA8652_ADDR;		// sensor address
    xfer.direction 		= kI2C_Write;
    xfer.subaddress 	= MMA8652_CTRL_REG1;
    xfer.subaddressSize = 1;
    xfer.data 			= buf;
    xfer.dataSize 		= 1;
    xfer.flags 			= kI2C_TransferRepeatedStartFlag;

	i2c_res=kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(i2c, &i2c_handle, &xfer);

    while (i2c_res==kStatus_I2C_Busy) {
    }
    if (i2c_res!=kStatus_Success) return kStatus_Fail;

    /* Everything done, nice! */
    return kStatus_Success;
}

/* mma8652_id
 *   get the sensor identifier
 */
status_t mma8652_id(uint32_t *id)
{
	uint8_t buf[1];
    i2c_master_transfer_t xfer = {
    	.slaveAddress 	= MMA8652_ADDR,			// sensor address
        .direction 		= kI2C_Read,
        .subaddress 	= MMA8652_WHOAMI,		// whoami register offset
        .subaddressSize = 1,
        .data 			= buf,
        .dataSize 		= 1,
        .flags 			= kI2C_TransferDefaultFlag
    };
	i2c_res=kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(i2c, &i2c_handle, &xfer);

    while (i2c_res==kStatus_I2C_Busy) {
    }

    if (i2c_res==kStatus_Success) *id=buf[0];

    return i2c_res;
}

/* mma8652_status
 *   returns the current sensor status (to enable polling)
 */
status_t mma8652_status(uint8_t *st)
{
	uint8_t buf[1];
    i2c_master_transfer_t xfer = {
    	.slaveAddress 	= MMA8652_ADDR,			// sensor address
        .direction 		= kI2C_Read,
        .subaddress 	= MMA8652_STATUS,
        .subaddressSize = 1,
        .data 			= buf,
        .dataSize 		= 1,
        .flags 			= kI2C_TransferDefaultFlag
    };
	i2c_res=kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(i2c, &i2c_handle, &xfer);

    while (i2c_res==kStatus_I2C_Busy) {
    }

    if (i2c_res==kStatus_Success) *st=buf[0];

    return i2c_res;
}

/* mma8652_read_xyz
 *   returns the XYZ values
 */

status_t mma8652_read_xyz(int32_t* data) {
    uint32_t scale = (mma8652_cfg >> 16) & 3; // full scale 2G, 4G, 8G
    uint8_t buf[6];
    i2c_master_transfer_t xfer = {
        .slaveAddress = MMA8652_ADDR,
        .direction = kI2C_Read,
        .subaddress = MMA8652_OUT_X_MSB, // Le registre de départ qui contient la partie MSB de l'axe X
        .subaddressSize = 1,
        .data = buf,
        .dataSize = 6,
        .flags = kI2C_TransferDefaultFlag
    };

    //Ajustement du dataSize en fonction de la résolution
    //Si la résolution configurée est de 8 bits (MMA8652_RES_8), la taille des données à lire est réduite à 3 octets (un seul octet par axe)

    if (mma8652_cfg & MMA8652_RES_8) xfer.dataSize = 3;

    //La transaction I2C est initiée de manière non bloquante. Cela signifie que la fonction commence le transfert et retourne immédiatement, tandis que le transfert se poursuit en arrière-plan. Le statut i2c_res est d'abord défini à kStatus_I2C_Busy pour indiquer que la transaction est en cours.

    i2c_res = kStatus_I2C_Busy;
    I2C_MasterTransferNonBlocking(i2c, &i2c_handle, &xfer);

    while (i2c_res == kStatus_I2C_Busy) {
    }

    if (i2c_res != kStatus_Success) return kStatus_Fail;

    /* Combine MSB and LSB for X, Y, Z */
    int16_t x = (int16_t)((buf[0] << 8) | buf[1]) >> 4;
    int16_t y = (int16_t)((buf[2] << 8) | buf[3]) >> 4;
    int16_t z = (int16_t)((buf[4] << 8) | buf[5]) >> 4;

    /* Convert according to the table 15 */

    switch(scale) {
        case 0: // ±2g
            data[0] = ((1000 * x + 512) >> 10);
            data[1] = ((1000 * y + 512) >> 10);
            data[2] = ((1000 * z + 512) >> 10);
            break;
        case 1: // ±4g
            data[0] = ((1000 * x + 256) >> 9);
            data[1] = ((1000 * y + 256) >> 9);
            data[2] = ((1000 * z + 256) >> 9);
            break;
        case 2: // ±8g
            data[0] = ((1000 * x + 128) >> 8);
            data[1] = ((1000 * y + 128) >> 8);
            data[2] = ((1000 * z + 128) >> 8);
            break;
        default:
            return kStatus_Fail;
    }

    return i2c_res;
}

