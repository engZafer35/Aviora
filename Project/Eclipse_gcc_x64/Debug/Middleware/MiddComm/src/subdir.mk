################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/src/Midd_OSPort_None.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/src/Midd_OSPort_Posix.c 

C_DEPS += \
./Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.d \
./Middleware/MiddComm/src/Midd_OSPort_None.d \
./Middleware/MiddComm/src/Midd_OSPort_Posix.d 

OBJS += \
./Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.o \
./Middleware/MiddComm/src/Midd_OSPort_None.o \
./Middleware/MiddComm/src/Midd_OSPort_Posix.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.c Middleware/MiddComm/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/src/Midd_OSPort_None.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/src/Midd_OSPort_None.c Middleware/MiddComm/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/src/Midd_OSPort_Posix.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/src/Midd_OSPort_Posix.c Middleware/MiddComm/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-src

clean-Middleware-2f-MiddComm-2f-src:
	-$(RM) ./Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.d ./Middleware/MiddComm/src/Midd_OSPort_FreeRTOS.o ./Middleware/MiddComm/src/Midd_OSPort_None.d ./Middleware/MiddComm/src/Midd_OSPort_None.o ./Middleware/MiddComm/src/Midd_OSPort_Posix.d ./Middleware/MiddComm/src/Midd_OSPort_Posix.o

.PHONY: clean-Middleware-2f-MiddComm-2f-src

