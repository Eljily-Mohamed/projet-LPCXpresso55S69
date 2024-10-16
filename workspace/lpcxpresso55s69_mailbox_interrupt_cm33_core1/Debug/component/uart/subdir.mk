################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/uart/fsl_adapter_usart.c 

C_DEPS += \
./component/uart/fsl_adapter_usart.d 

OBJS += \
./component/uart/fsl_adapter_usart.o 


# Each subdirectory must supply rules for building sources it contributes
component/uart/%.o: ../component/uart/%.c component/uart/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33_nodsp -DCPU_LPC55S69JBD100_cm33_core1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__MULTICORE_M33SLAVE -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/source" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/lcd" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/utilities" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/drivers" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/device" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/startup" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/component/uart" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/component/lists" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/CMSIS" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/board" -I"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/lpcxpresso55s69_mailbox_interrupt_cm33_core1/lpcxpresso55s69/driver_examples/mailbox/interrupt/cm33_core1" -O0 -fno-common -g3 -gdwarf-4 -mcpu=cortex-m33+nodsp -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33+nodsp -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-component-2f-uart

clean-component-2f-uart:
	-$(RM) ./component/uart/fsl_adapter_usart.d ./component/uart/fsl_adapter_usart.o

.PHONY: clean-component-2f-uart

