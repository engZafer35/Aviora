################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.c 

C_DEPS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.d 

OBJS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -DNBNS_RESPONDER_SUPPORT -DBSD_SOCKET_SUPPORT -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-core

clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-core:
	-$(RM) ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ethernet_misc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ip.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_mem.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/net_misc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/nic.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/ping.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/raw_socket.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/socket_misc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_fsm.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_misc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/tcp_timer.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/core/udp.o

.PHONY: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-core

