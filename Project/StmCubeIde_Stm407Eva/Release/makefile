################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Core/Startup/subdir.mk
-include Aviora/Middleware/MiddZModem/src/subdir.mk
-include Aviora/Middleware/MiddComm/Midd_OS/src/subdir.mk
-include Aviora/Middleware/MiddComm/Midd_FS/fs/subdir.mk
-include Aviora/Middleware/FreeRTOS/Source/portable/MemMang/subdir.mk
-include Aviora/Middleware/FreeRTOS/Source/portable/GCC/ARM_CM4F/subdir.mk
-include Aviora/Middleware/FreeRTOS/Source/CMSIS_RTOS_V2/subdir.mk
-include Aviora/Middleware/FreeRTOS/Source/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/syslog/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/sntp/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/ppp/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/mqtt/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/llmnr/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/ipv6/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/igmp/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/icecast/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/echo/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/drivers/eth/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/dns_sd/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/dns/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/dhcpv6/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
-include Aviora/Middleware/CycloneTcp/cyclone_tcp/coap/subdir.mk
-include Aviora/Middleware/CycloneTcp/common/subdir.mk
-include Aviora/Driver/StmCubeIDE/Src/subdir.mk
-include Aviora/Driver/StmCubeIDE/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Aviora/Driver/McuCoreDrivers/src/subdir.mk
-include Aviora/Driver/DeviceDrivers/src/subdir.mk
-include Aviora/Driver/DeviceDrivers/ENC28J60/src/subdir.mk
-include Aviora/Customers/Stm407Eva/Network/subdir.mk
-include Aviora/Customers/Stm407Eva/Logger/subdir.mk
-include Aviora/Application/AppZMeterGw/Units/System/src/subdir.mk
-include Aviora/Application/AppZMeterGw/Units/DataBus/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/TimeService/src/time_modules/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/TimeService/src/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/System/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Protocol/src/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Protocol/Test_Server/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Network/src/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Network/Interfaces/Linux/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Network/Interfaces/CycloneTcp/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Logger/subdir.mk
-include Aviora/Application/AppZMeterGw/Services/Display/subdir.mk
-include Aviora/Application/AppComm/src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Aviora_STM407EVA
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Aviora_STM407EVA.elf \

MAP_FILES += \
Aviora_STM407EVA.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Aviora_STM407EVA.list \


# All Target
all: main-build

# Main-build Target
main-build: Aviora_STM407EVA.elf secondary-outputs

# Tool invocations
Aviora_STM407EVA.elf Aviora_STM407EVA.map: $(OBJS) $(USER_OBJS) C:\Users\zafer\Aviora\Project\StmCubeIde_Stm407Eva\STM32F407VGTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Aviora_STM407EVA.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\zafer\Aviora\Project\StmCubeIde_Stm407Eva\STM32F407VGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Aviora_STM407EVA.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Aviora_STM407EVA.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Aviora_STM407EVA.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Aviora_STM407EVA.elf Aviora_STM407EVA.list Aviora_STM407EVA.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
