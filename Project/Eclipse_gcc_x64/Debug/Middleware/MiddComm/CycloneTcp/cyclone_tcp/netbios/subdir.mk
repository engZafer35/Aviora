################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.c 

C_DEPS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.d 

OBJS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-netbios

clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-netbios:
	-$(RM) ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_client.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_common.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/netbios/nbns_responder.o

.PHONY: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-netbios

