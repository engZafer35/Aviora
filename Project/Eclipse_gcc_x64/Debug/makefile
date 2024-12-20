################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middleware/MiddZModem/src/subdir.mk
-include Middleware/MiddComm/Midd_OS/src/subdir.mk
-include Middleware/CycloneTcp/common/subdir.mk
-include Driver/McuCoreDrivers/src/subdir.mk
-include Application/AppZMeterGw/src/subdir.mk
-include Application/AppZMeterGw/Protocols/src/subdir.mk
-include Application/AppZMeterGw/Headend/src/subdir.mk
-include Application/AppZMeterGw/AppMessageHandlers/src/subdir.mk
-include Application/AppComm/src/subdir.mk
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


BUILD_ARTIFACT_NAME := Z_Meter_Gateway
BUILD_ARTIFACT_EXTENSION :=
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: main-build

# Main-build Target
main-build: Z_Meter_Gateway

# Tool invocations
Z_Meter_Gateway: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "Z_Meter_Gateway" $(OBJS) $(USER_OBJS) $(LIBS) -lpthread -lpcap
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Z_Meter_Gateway
	-@echo ' '

.PHONY: all clean dependents main-build

-include ../makefile.targets
