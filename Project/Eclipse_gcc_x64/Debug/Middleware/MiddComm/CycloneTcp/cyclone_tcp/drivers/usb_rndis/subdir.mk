################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.c 

C_DEPS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.d 

OBJS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-usb_rndis

clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-usb_rndis:
	-$(RM) ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_debug.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/rndis_driver.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_desc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/drivers/usb_rndis/usbd_rndis.o

.PHONY: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-drivers-2f-usb_rndis

