################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.c 

C_DEPS += \
./Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.d 

OBJS += \
./Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.c Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-loopback

clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-loopback:
	-$(RM) ./Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.d ./Middleware/CycloneTcp/cyclone_tcp/drivers/loopback/loopback_driver.o

.PHONY: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-loopback

