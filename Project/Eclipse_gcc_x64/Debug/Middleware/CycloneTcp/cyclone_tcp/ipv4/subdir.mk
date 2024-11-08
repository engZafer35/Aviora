################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.c 

C_DEPS += \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.d \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.d 

OBJS += \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.o \
./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.c Middleware/CycloneTcp/cyclone_tcp/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DPPP_SUPPORT=ENABLED -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/Communication/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-ipv4

clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-ipv4:
	-$(RM) ./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/arp_cache.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/auto_ip_misc.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/icmp.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_frag.o ./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.d ./Middleware/CycloneTcp/cyclone_tcp/ipv4/ipv4_misc.o

.PHONY: clean-Middleware-2f-CycloneTcp-2f-cyclone_tcp-2f-ipv4

