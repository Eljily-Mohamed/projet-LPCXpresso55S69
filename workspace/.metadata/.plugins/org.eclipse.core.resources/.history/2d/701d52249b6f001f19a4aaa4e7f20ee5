#ifndef _MMA8652FC_H_
#define _MMA8652FC_H_

#include "fsl_i2c.h"

#if 0
typedef struct _mma8652fc_t {
	I2C_Type* i2c;
	i2c_master_handle_t handle;
	volatile status_t status;
} mma8652fc_t;
#endif

#define MMA8652_RES_12				(0)
#define MMA8652_RES_8				(2)

#define MMA8652_SCALE_2G			(0<<16)
#define MMA8652_SCALE_4G			(1<<16)
#define MMA8652_SCALE_8G			(2<<16)

#define MMA8652_RATE_800			(0<<3)
#define MMA8652_RATE_400			(1<<3)
#define MMA8652_RATE_200			(2<<3)
#define MMA8652_RATE_100			(3<<3)
#define MMA8652_RATE_50				(4<<3)
#define MMA8652_RATE_12_5			(5<<3)
#define MMA8652_RATE_6_25			(6<<3)
#define MMA8652_RATE_1_56			(7<<3)

#define MMA8652_INT					(1<<8)

#define MMA8652_DATA_READY			(8)

status_t mma8652_init(I2C_Type *i2c, uint32_t cfg);
status_t mma8652_id(uint32_t *id);
status_t mma8652_status(uint8_t *st);
status_t mma8652_read_xyz(int32_t* data);

#endif
