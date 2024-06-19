################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.c 

C_DEPS += \
./Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.d 

OBJS += \
./Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.c Middleware/MiddComm/Midd_OS/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-Midd_OS-2f-src

clean-Middleware-2f-MiddComm-2f-Midd_OS-2f-src:
	-$(RM) ./Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.d ./Middleware/MiddComm/Midd_OS/src/Midd_OSPort_Posix.o

.PHONY: clean-Middleware-2f-MiddComm-2f-Midd_OS-2f-src

