################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_flexcomm.c \
../drivers/fsl_gpio.c \
../drivers/fsl_i2c.c \
../drivers/fsl_inputmux.c \
../drivers/fsl_pint.c \
../drivers/fsl_power.c \
../drivers/fsl_reset.c \
../drivers/fsl_usart.c 

C_DEPS += \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_flexcomm.d \
./drivers/fsl_gpio.d \
./drivers/fsl_i2c.d \
./drivers/fsl_inputmux.d \
./drivers/fsl_pint.d \
./drivers/fsl_power.d \
./drivers/fsl_reset.d \
./drivers/fsl_usart.d 

OBJS += \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_flexcomm.o \
./drivers/fsl_gpio.o \
./drivers/fsl_i2c.o \
./drivers/fsl_inputmux.o \
./drivers/fsl_pint.o \
./drivers/fsl_power.o \
./drivers/fsl_reset.o \
./drivers/fsl_usart.o 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c drivers/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -DSDK_DEBUGCONSOLE=0 -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/component/serial_manager" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/CMSIS" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/drivers" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/device" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/utilities" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/component/lists" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/component/uart" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/board" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/source" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/startup" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/trustzone" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-drivers

clean-drivers:
	-$(RM) ./drivers/fsl_clock.d ./drivers/fsl_clock.o ./drivers/fsl_common.d ./drivers/fsl_common.o ./drivers/fsl_flexcomm.d ./drivers/fsl_flexcomm.o ./drivers/fsl_gpio.d ./drivers/fsl_gpio.o ./drivers/fsl_i2c.d ./drivers/fsl_i2c.o ./drivers/fsl_inputmux.d ./drivers/fsl_inputmux.o ./drivers/fsl_pint.d ./drivers/fsl_pint.o ./drivers/fsl_power.d ./drivers/fsl_power.o ./drivers/fsl_reset.d ./drivers/fsl_reset.o ./drivers/fsl_usart.d ./drivers/fsl_usart.o

.PHONY: clean-drivers

