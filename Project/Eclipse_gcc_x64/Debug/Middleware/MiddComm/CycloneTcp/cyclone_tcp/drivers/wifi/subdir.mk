################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.c 

C_DEPS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.d 

OBJS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-wifi

clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-wifi:
	-$(RM) ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/bcm43362_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp32_wifi_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/esp8266_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/mrf24wg_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wf200_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/wilc1000_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/wifi/winc1500_driver.o

.PHONY: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-wifi

