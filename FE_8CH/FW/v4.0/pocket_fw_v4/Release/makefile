################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := cs-rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_SIZE += \
pocket_fw_v4.siz \


# All Target
all: pocket_fw_v4.elf secondary-outputs

# Tool invocations
pocket_fw_v4.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-none-eabi-gcc -mcpu=arm926ej-s -marm -mthumb-interwork -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -T "C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3\/fw_build/fx3_fw/fx3.ld" -nostartfiles -Xlinker --gc-sections -L"C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3\/fw_lib/1_3_5/fx3_release" -L"C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3\ARM GCC\/lib/gcc/arm-none-eabi/4.8.1" -L"C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3\ARM GCC\/arm-none-eabi/lib" -Wl,-Map,"pocket_fw_v4.map" -Wl,-d  -Wl,--no-wchar-size-warning -Wl,--entry,CyU3PFirmwareEntry -o "pocket_fw_v4.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

pocket_fw_v4.siz: pocket_fw_v4.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "pocket_fw_v4.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(S_UPPER_DEPS) pocket_fw_v4.elf
	-@echo ' '

post-build:
	-@echo 'Generate boot-loadable binary image'
	-elf2img.exe -i pocket_fw_v4.elf -o pocket_fw_v4.img -v
	-@echo ' '

secondary-outputs: $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
