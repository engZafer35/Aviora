################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.c 

C_DEPS += \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.d \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.d \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.d \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.d \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.d \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.d 

OBJS += \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.o \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.o \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.o \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.o \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.o \
./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.c Middleware/CycloneTcp/cyclone_tcp/dhcp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-dhcp

clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-dhcp:
	-$(RM) ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client.o ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_fsm.o ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_client_misc.o ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_common.o ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_debug.o ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server.o ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.d ./Middleware/CycloneTcp/cyclone_tcp/dhcp/dhcp_server_misc.o

.PHONY: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-dhcp

