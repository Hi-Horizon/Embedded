################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Eink_interface/Src/Eink_driver.c \
../Drivers/Eink_interface/Src/Eink_driver_ImgGen.c \
../Drivers/Eink_interface/Src/Eink_driver_LL.c 

OBJS += \
./Drivers/Eink_interface/Src/Eink_driver.o \
./Drivers/Eink_interface/Src/Eink_driver_ImgGen.o \
./Drivers/Eink_interface/Src/Eink_driver_LL.o 

C_DEPS += \
./Drivers/Eink_interface/Src/Eink_driver.d \
./Drivers/Eink_interface/Src/Eink_driver_ImgGen.d \
./Drivers/Eink_interface/Src/Eink_driver_LL.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Eink_interface/Src/%.o Drivers/Eink_interface/Src/%.su: ../Drivers/Eink_interface/Src/%.c Drivers/Eink_interface/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G474xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Eink_interface-2f-Src

clean-Drivers-2f-Eink_interface-2f-Src:
	-$(RM) ./Drivers/Eink_interface/Src/Eink_driver.d ./Drivers/Eink_interface/Src/Eink_driver.o ./Drivers/Eink_interface/Src/Eink_driver.su ./Drivers/Eink_interface/Src/Eink_driver_ImgGen.d ./Drivers/Eink_interface/Src/Eink_driver_ImgGen.o ./Drivers/Eink_interface/Src/Eink_driver_ImgGen.su ./Drivers/Eink_interface/Src/Eink_driver_LL.d ./Drivers/Eink_interface/Src/Eink_driver_LL.o ./Drivers/Eink_interface/Src/Eink_driver_LL.su

.PHONY: clean-Drivers-2f-Eink_interface-2f-Src

