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
-include lcd/fonts/subdir.mk
-include lcd/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/lists/subdir.mk
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


BUILD_ARTIFACT_NAME := lpcxpresso55s69_mailbox_interrupt_cm33_core1
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf

# Tool invocations
lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -no-warn-rwx-segments -Xlinker -Map="lpcxpresso55s69_mailbox_interrupt_cm33_core1.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m33+nodsp -mthumb -T lpcxpresso55s69_mailbox_interrupt_cm33_core1_Debug.ld -o "lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf" $(OBJS) $(USER_OBJS) $(LIBS) && arm-none-eabi-objcopy --target elf32-littlearm --verbose --strip-all --remove-section=.ARM.attributes --redefine-sym __vectors_start__=__vectors_start___core_m33slave --keep-symbol __vectors_start___core_m33slave --redefine-sym _data=__start_data_core_m33slave --keep-symbol __start_data_core_m33slave --redefine-sym _edata=__end_data_core_m33slave --keep-symbol __end_data_core_m33slave --remove-section=".bss*" --remove-section=".noinit*" --rename-section .ARM.exidx=".core_m33slave.ARM.exidx" --rename-section .ARM.extab=".core_m33slave.ARM.extab" --rename-section .text=".core_m33slave" --rename-section .data=".core_m33slave.data" --rename-section .data_RAM2=".core_m33slave.data_RAM2" "lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf" "lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf.o" && mcux-fixelf "lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf.o"
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf"; # arm-none-eabi-objcopy -v -O binary "lpcxpresso55s69_mailbox_interrupt_cm33_core1.axf" "lpcxpresso55s69_mailbox_interrupt_cm33_core1.bin" ; # checksum -p LPC55S69 -d "lpcxpresso55s69_mailbox_interrupt_cm33_core1.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
