################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/SEN5x/sen5x_i2c.c \
../Drivers/SEN5x/sensirion_common.c \
../Drivers/SEN5x/sensirion_i2c.c \
../Drivers/SEN5x/sensirion_i2c_hal.c 

OBJS += \
./Drivers/SEN5x/sen5x_i2c.o \
./Drivers/SEN5x/sensirion_common.o \
./Drivers/SEN5x/sensirion_i2c.o \
./Drivers/SEN5x/sensirion_i2c_hal.o 

C_DEPS += \
./Drivers/SEN5x/sen5x_i2c.d \
./Drivers/SEN5x/sensirion_common.d \
./Drivers/SEN5x/sensirion_i2c.d \
./Drivers/SEN5x/sensirion_i2c_hal.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/SEN5x/%.o Drivers/SEN5x/%.su Drivers/SEN5x/%.cyclo: ../Drivers/SEN5x/%.c Drivers/SEN5x/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WLE5xx -c -I../Core/Inc -I"D:/PRJ/pollutag_node2/FW/Drivers/SEN5x" -I"D:/PRJ/pollutag_node2/FW/Drivers/BSP/STM32WLxx_Nucleo" -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I../LoRaWAN/App -I../LoRaWAN/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -I../Middlewares/Third_Party/LoRaWAN/Crypto -I../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../Middlewares/Third_Party/LoRaWAN/Mac -I../Middlewares/Third_Party/LoRaWAN/LmHandler -I../Middlewares/Third_Party/LoRaWAN/Utilities -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-SEN5x

clean-Drivers-2f-SEN5x:
	-$(RM) ./Drivers/SEN5x/sen5x_i2c.cyclo ./Drivers/SEN5x/sen5x_i2c.d ./Drivers/SEN5x/sen5x_i2c.o ./Drivers/SEN5x/sen5x_i2c.su ./Drivers/SEN5x/sensirion_common.cyclo ./Drivers/SEN5x/sensirion_common.d ./Drivers/SEN5x/sensirion_common.o ./Drivers/SEN5x/sensirion_common.su ./Drivers/SEN5x/sensirion_i2c.cyclo ./Drivers/SEN5x/sensirion_i2c.d ./Drivers/SEN5x/sensirion_i2c.o ./Drivers/SEN5x/sensirion_i2c.su ./Drivers/SEN5x/sensirion_i2c_hal.cyclo ./Drivers/SEN5x/sensirion_i2c_hal.d ./Drivers/SEN5x/sensirion_i2c_hal.o ./Drivers/SEN5x/sensirion_i2c_hal.su

.PHONY: clean-Drivers-2f-SEN5x

