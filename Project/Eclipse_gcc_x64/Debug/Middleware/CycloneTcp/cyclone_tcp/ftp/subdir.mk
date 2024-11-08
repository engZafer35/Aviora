################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.c 

C_DEPS += \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.d 

OBJS += \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.c Middleware/CycloneTcp/cyclone_tcp/ftp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-ftp

clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-ftp:
	-$(RM) ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_misc.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_client_transport.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_commands.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_control.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_data.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_misc.o ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.d ./Middleware/CycloneTcp/cyclone_tcp/ftp/ftp_server_transport.o

.PHONY: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-ftp

