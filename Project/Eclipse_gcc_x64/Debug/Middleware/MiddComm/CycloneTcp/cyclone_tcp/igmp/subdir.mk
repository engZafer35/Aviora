################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.c \
/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.c 

C_DEPS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.d \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.d 

OBJS += \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.o \
./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.o 


# Each subdirectory must supply rules for building sources it contributes
Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.o: /home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.c Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DGPL_LICENSE_TERMS_ACCEPTED -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppZMeterGw/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Application/AppComm/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/Boards" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Driver/McuCoreDrivers/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/Midd_OS/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddZModem/inc" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/common" -I"/home/zafer/eclipse-workspace/Z_Meter_Gateway/Middleware/MiddComm/CycloneTcp/cyclone_tcp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-igmp

clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-igmp:
	-$(RM) ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_common.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_host_misc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_router_misc.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping.o ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.d ./Middleware/MiddComm/CycloneTcp/cyclone_tcp/igmp/igmp_snooping_misc.o

.PHONY: clean-Middleware-2f-MiddComm-2f-CycloneTcp-2f-cyclone_tcp-2f-igmp

