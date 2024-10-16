################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include component/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := sen_cm33_lab3_i2c
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: sen_cm33_lab3_i2c.axf

# Tool invocations
sen_cm33_lab3_i2c.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/home/mohamed/apps/MCUXpresso_11.10.0_3148/workspace/sen_cm33_lab3_i2c/libs" -Xlinker -Map="sen_cm33_lab3_i2c.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -T sen_cm33_lab3_i2c_Debug.ld -o "sen_cm33_lab3_i2c.axf" $(OBJS) $(USER_OBJS) $(LIBS) -lpower_hardabi
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) sen_cm33_lab3_i2c.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "sen_cm33_lab3_i2c.axf"; # arm-none-eabi-objcopy -v -O binary "sen_cm33_lab3_i2c.axf" "sen_cm33_lab3_i2c.bin" ; # checksum -p LPC55S69 -d "sen_cm33_lab3_i2c.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
