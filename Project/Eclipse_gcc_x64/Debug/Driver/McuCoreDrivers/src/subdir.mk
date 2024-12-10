################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/src/McuInterruptRegister.c 

C_DEPS += \
./Driver/McuCoreDrivers/src/McuInterruptRegister.d 

OBJS += \
./Driver/McuCoreDrivers/src/McuInterruptRegister.o 


# Each subdirectory must supply rules for building sources it contributes
Driver/McuCoreDrivers/src/McuInterruptRegister.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/src/McuInterruptRegister.c Driver/McuCoreDrivers/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Server_Comm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Driver-2f-McuCoreDrivers-2f-src

clean-Driver-2f-McuCoreDrivers-2f-src:
	-$(RM) ./Driver/McuCoreDrivers/src/McuInterruptRegister.d ./Driver/McuCoreDrivers/src/McuInterruptRegister.o

.PHONY: clean-Driver-2f-McuCoreDrivers-2f-src

