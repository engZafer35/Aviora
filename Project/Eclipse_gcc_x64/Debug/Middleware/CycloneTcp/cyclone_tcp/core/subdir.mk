################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ethernet.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ip.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/net.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/net_mem.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/net_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/nic.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ping.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/socket.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/udp.c 

C_DEPS += \
./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.d \
./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.d \
./Middleware/CycloneTcp/cyclone_tcp/core/ethernet.d \
./Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/core/ip.d \
./Middleware/CycloneTcp/cyclone_tcp/core/net.d \
./Middleware/CycloneTcp/cyclone_tcp/core/net_mem.d \
./Middleware/CycloneTcp/cyclone_tcp/core/net_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/core/nic.d \
./Middleware/CycloneTcp/cyclone_tcp/core/ping.d \
./Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.d \
./Middleware/CycloneTcp/cyclone_tcp/core/socket.d \
./Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp.d \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.d \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.d \
./Middleware/CycloneTcp/cyclone_tcp/core/udp.d 

OBJS += \
./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.o \
./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.o \
./Middleware/CycloneTcp/cyclone_tcp/core/ethernet.o \
./Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/core/ip.o \
./Middleware/CycloneTcp/cyclone_tcp/core/net.o \
./Middleware/CycloneTcp/cyclone_tcp/core/net_mem.o \
./Middleware/CycloneTcp/cyclone_tcp/core/net_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/core/nic.o \
./Middleware/CycloneTcp/cyclone_tcp/core/ping.o \
./Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.o \
./Middleware/CycloneTcp/cyclone_tcp/core/socket.o \
./Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp.o \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.o \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.o \
./Middleware/CycloneTcp/cyclone_tcp/core/udp.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/ethernet.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ethernet.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/ip.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ip.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/net.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/net.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/net_mem.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/net_mem.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/net_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/net_misc.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/nic.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/nic.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/ping.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/ping.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/socket.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/socket.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/tcp.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/core/udp.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/core/udp.c Middleware/CycloneTcp/cyclone_tcp/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-core

clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-core:
	-$(RM) ./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.d ./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket.o ./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.d ./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_misc.o ./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.d ./Middleware/CycloneTcp/cyclone_tcp/core/bsd_socket_options.o ./Middleware/CycloneTcp/cyclone_tcp/core/ethernet.d ./Middleware/CycloneTcp/cyclone_tcp/core/ethernet.o ./Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.d ./Middleware/CycloneTcp/cyclone_tcp/core/ethernet_misc.o ./Middleware/CycloneTcp/cyclone_tcp/core/ip.d ./Middleware/CycloneTcp/cyclone_tcp/core/ip.o ./Middleware/CycloneTcp/cyclone_tcp/core/net.d ./Middleware/CycloneTcp/cyclone_tcp/core/net.o ./Middleware/CycloneTcp/cyclone_tcp/core/net_mem.d ./Middleware/CycloneTcp/cyclone_tcp/core/net_mem.o ./Middleware/CycloneTcp/cyclone_tcp/core/net_misc.d ./Middleware/CycloneTcp/cyclone_tcp/core/net_misc.o ./Middleware/CycloneTcp/cyclone_tcp/core/nic.d ./Middleware/CycloneTcp/cyclone_tcp/core/nic.o ./Middleware/CycloneTcp/cyclone_tcp/core/ping.d ./Middleware/CycloneTcp/cyclone_tcp/core/ping.o ./Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.d ./Middleware/CycloneTcp/cyclone_tcp/core/raw_socket.o ./Middleware/CycloneTcp/cyclone_tcp/core/socket.d ./Middleware/CycloneTcp/cyclone_tcp/core/socket.o ./Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.d ./Middleware/CycloneTcp/cyclone_tcp/core/socket_misc.o ./Middleware/CycloneTcp/cyclone_tcp/core/tcp.d ./Middleware/CycloneTcp/cyclone_tcp/core/tcp.o ./Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.d ./Middleware/CycloneTcp/cyclone_tcp/core/tcp_fsm.o ./Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.d ./Middleware/CycloneTcp/cyclone_tcp/core/tcp_misc.o ./Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.d ./Middleware/CycloneTcp/cyclone_tcp/core/tcp_timer.o ./Middleware/CycloneTcp/cyclone_tcp/core/udp.d ./Middleware/CycloneTcp/cyclone_tcp/core/udp.o

.PHONY: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-core

