# GNU build of the Keil target "Target 1" (STM32F103C8, Cortex-M3).
# Device macro STM32F10X_MD matches the 64 KB / 20 KB part selected in
# project.uvprojx. USE_STDPERIPH_DRIVER and the include paths match Cads.

TARGET := project
BUILD  := build

PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
NM      := $(PREFIX)nm
READELF := $(PREFIX)readelf

CPU      := -mcpu=cortex-m3 -mthumb
DEFS     := -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER
INCLUDES := -Istart -Ilibrary -Iuser -Isystem -IHardware

CFLAGS  := $(CPU) -std=c99 -Os -ffunction-sections -fdata-sections \
           -Wall -Wno-unused-variable -Wno-unused-but-set-variable \
           -Wno-missing-braces \
           $(DEFS) $(INCLUDES)
ASFLAGS := $(CPU) -x assembler-with-cpp
LDFLAGS := $(CPU) -Tgcc/STM32F103C8Tx.ld \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map \
           --specs=nano.specs --specs=nosys.specs \
           -Wl,-u,_printf_float -lc -lm

C_SRCS := \
	start/core_cm3.c \
	start/system_stm32f10x.c \
	library/misc.c \
	library/stm32f10x_adc.c \
	library/stm32f10x_bkp.c \
	library/stm32f10x_can.c \
	library/stm32f10x_cec.c \
	library/stm32f10x_crc.c \
	library/stm32f10x_dac.c \
	library/stm32f10x_dbgmcu.c \
	library/stm32f10x_dma.c \
	library/stm32f10x_exti.c \
	library/stm32f10x_flash.c \
	library/stm32f10x_fsmc.c \
	library/stm32f10x_gpio.c \
	library/stm32f10x_i2c.c \
	library/stm32f10x_iwdg.c \
	library/stm32f10x_pwr.c \
	library/stm32f10x_rcc.c \
	library/stm32f10x_rtc.c \
	library/stm32f10x_sdio.c \
	library/stm32f10x_spi.c \
	library/stm32f10x_tim.c \
	library/stm32f10x_usart.c \
	library/stm32f10x_wwdg.c \
	system/Delay.c \
	system/timer.c \
	Hardware/OLED.c \
	Hardware/MYIIC.c \
	Hardware/MPU6050.c \
	Hardware/encoder.c \
	Hardware/serial.c \
	Hardware/motor.c \
	Hardware/PWM.c \
	Hardware/BlueSerial.c \
	Hardware/NRF24L01.c \
	Hardware/TCRT5000.c \
	user/main.c \
	user/stm32f10x_it.c \
	user/PID.c

ASM_SRCS := gcc/startup_stm32f10x_md.s

OBJS := $(C_SRCS:%.c=$(BUILD)/%.o) $(ASM_SRCS:%.s=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean check

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin
	$(SIZE) $<

$(BUILD)/$(TARGET).elf: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

check: all $(BUILD)/pid_test
	@test "$$($(READELF) -h $(BUILD)/$(TARGET).elf | awk '/Machine:/{print $$2}')" = "ARM"
	@$(NM) $(BUILD)/$(TARGET).elf | grep -q ' T main'
	@$(NM) $(BUILD)/$(TARGET).elf | grep -q ' T PID_Update'
	@$(NM) $(BUILD)/$(TARGET).elf | grep -q ' T TIM1_UP_IRQHandler'
	@$(NM) $(BUILD)/$(TARGET).elf | grep -q ' T Blueserial_Init'
	@python3 -c "import pathlib,sys; n=pathlib.Path('$(BUILD)/$(TARGET).bin').stat().st_size; print(f'flash_image_bytes={n}'); sys.exit(0 if n<=65536 else 1)"
	@$(BUILD)/pid_test
	@echo "STM32F103C8 image fits in 64 KB flash and the balance PID law matches expected outputs"

$(BUILD)/pid_test: gcc/host/pid_test.c user/PID.c user/PID.h gcc/host/stm32f10x.h
	@mkdir -p $(dir $@)
	gcc -std=c99 -Wall -Werror -Iuser -Igcc/host -o $@ gcc/host/pid_test.c user/PID.c -lm

clean:
	rm -rf $(BUILD)

-include $(DEPS)
