# Common BSP build settings for S300 projects
# - Retarget newlib syscalls to UART (retarget.c)
# - Enable printf/scanf float and long long support with newlib-nano

# Add BSP includes if not already present in CFLAGS (safe to re-add)
CFLAGS += -I$(ROOT)/CMSIS/Device/PiMCHIP/S300/Include -I$(ROOT)/CMSIS/Core/Include \
          -I$(ROOT)/Drivers/BSP/Include -I$(ROOT)/Drivers/UART/Include -I$(ROOT)/Boards/S300_EVB -I$(ROOT)/Drivers/SoC/Include

# Pull in retarget.c/systick.c by default
BSP_COMMON_SRCS ?= $(ROOT)/Drivers/BSP/Source/retarget.c \
				   $(ROOT)/Drivers/BSP/Source/systick.c

# Link flags to enable float and long long formatting in newlib-nano
LDFLAGS += -u _printf_float -u _scanf_float -u _printf_long_long -u _scanf_long_long

# Generic build rule for BSP sources (retarget.c etc.)
$(BUILD)/%.o: $(ROOT)/Drivers/BSP/Source/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
