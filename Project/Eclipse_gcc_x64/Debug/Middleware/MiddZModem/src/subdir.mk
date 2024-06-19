################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddDigitalIOControl.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddEventTimer.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddGsmModule.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddMCUCore.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddStorage.c 

C_DEPS += \
./Middleware/MiddZModem/src/MiddDigitalIOControl.d \
./Middleware/MiddZModem/src/MiddEventTimer.d \
./Middleware/MiddZModem/src/MiddGsmModule.d \
./Middleware/MiddZModem/src/MiddMCUCore.d \
./Middleware/MiddZModem/src/MiddStorage.d 

OBJS += \
./Middleware/MiddZModem/src/MiddDigitalIOControl.o \
./Middleware/MiddZModem/src/MiddEventTimer.o \
./Middleware/MiddZModem/src/MiddGsmModule.o \
./Middleware/MiddZModem/src/MiddMCUCore.o \
./Middleware/MiddZModem/src/MiddStorage.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddZModem/src/MiddDigitalIOControl.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddDigitalIOControl.c Middleware/MiddZModem/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddZModem/src/MiddEventTimer.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddEventTimer.c Middleware/MiddZModem/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddZModem/src/MiddGsmModule.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddGsmModule.c Middleware/MiddZModem/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddZModem/src/MiddMCUCore.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddMCUCore.c Middleware/MiddZModem/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddZModem/src/MiddStorage.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/src/MiddStorage.c Middleware/MiddZModem/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddZModem-2f-src

clean-Middleware-2f-MiddZModem-2f-src:
	-$(RM) ./Middleware/MiddZModem/src/MiddDigitalIOControl.d ./Middleware/MiddZModem/src/MiddDigitalIOControl.o ./Middleware/MiddZModem/src/MiddEventTimer.d ./Middleware/MiddZModem/src/MiddEventTimer.o ./Middleware/MiddZModem/src/MiddGsmModule.d ./Middleware/MiddZModem/src/MiddGsmModule.o ./Middleware/MiddZModem/src/MiddMCUCore.d ./Middleware/MiddZModem/src/MiddMCUCore.o ./Middleware/MiddZModem/src/MiddStorage.d ./Middleware/MiddZModem/src/MiddStorage.o

.PHONY: clean-Middleware-2f-MiddZModem-2f-src

