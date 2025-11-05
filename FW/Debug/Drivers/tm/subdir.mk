################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/tm/tm_stm32_ds18b20.c \
../Drivers/tm/tm_stm32_onewire.c 

OBJS += \
./Drivers/tm/tm_stm32_ds18b20.o \
./Drivers/tm/tm_stm32_onewire.o 

C_DEPS += \
./Drivers/tm/tm_stm32_ds18b20.d \
./Drivers/tm/tm_stm32_onewire.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/tm/%.o Drivers/tm/%.su: ../Drivers/tm/%.c Drivers/tm/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WLE5xx -c -I../Core/Inc -I"C:/PRJ/LoRa-E5_weather_and_pollution/Drivers/BSP/STM32WLxx_Nucleo" -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I../LoRaWAN/App -I../LoRaWAN/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -I../Middlewares/Third_Party/LoRaWAN/Crypto -I../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../Middlewares/Third_Party/LoRaWAN/Mac -I../Middlewares/Third_Party/LoRaWAN/LmHandler -I../Middlewares/Third_Party/LoRaWAN/Utilities -I"C:/PRJ/LoRa-E5_weather_and_pollution/Drivers/tm" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-tm

clean-Drivers-2f-tm:
	-$(RM) ./Drivers/tm/tm_stm32_ds18b20.d ./Drivers/tm/tm_stm32_ds18b20.o ./Drivers/tm/tm_stm32_ds18b20.su ./Drivers/tm/tm_stm32_onewire.d ./Drivers/tm/tm_stm32_onewire.o ./Drivers/tm/tm_stm32_onewire.su

.PHONY: clean-Drivers-2f-tm

