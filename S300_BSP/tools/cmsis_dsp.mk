ifndef CMSIS_DSP_MK
CMSIS_DSP_MK := 1

# Default CMSIS-DSP root (override in project Makefile if layout differs)
CMSIS_DSP_ROOT ?= $(abspath $(CURDIR)/../../../../../CMSIS-DSP)

# Include flags and macros suitable for Cortex-M4
CMSIS_DSP_CFLAGS := -I$(CMSIS_DSP_ROOT)/Include -I$(CMSIS_DSP_ROOT)/PrivateInclude \
                    -DARM_MATH_CM4 -DDISABLEFLOAT16

# Aggregated sources (each includes all kernels in its folder)
CMSIS_DSP_SRCS := \
  $(CMSIS_DSP_ROOT)/Source/BasicMathFunctions/BasicMathFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/CommonTables/CommonTables.c \
  $(CMSIS_DSP_ROOT)/Source/InterpolationFunctions/InterpolationFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/BayesFunctions/BayesFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/MatrixFunctions/MatrixFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/ComplexMathFunctions/ComplexMathFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/QuaternionMathFunctions/QuaternionMathFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/ControllerFunctions/ControllerFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/SVMFunctions/SVMFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/DistanceFunctions/DistanceFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/StatisticsFunctions/StatisticsFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/FastMathFunctions/FastMathFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/SupportFunctions/SupportFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/FilteringFunctions/FilteringFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/TransformFunctions/TransformFunctions.c \
  $(CMSIS_DSP_ROOT)/Source/WindowFunctions/WindowFunctions.c

# Objects (preserve subdir under build/)
CMSIS_DSP_OBJS := $(addprefix $(BUILD)/,$(patsubst $(CMSIS_DSP_ROOT)/Source/%,%,$(CMSIS_DSP_SRCS:.c=.o)))

# Pattern rule for CMSIS-DSP sources
$(BUILD)/%.o: $(CMSIS_DSP_ROOT)/Source/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Optional: build static library if requested
CMSIS_DSP_BUILD_LIB ?= 0
CMSIS_DSP_LIB ?= $(BUILD)/libCMSISDSP_CM4.a

ifeq ($(CMSIS_DSP_BUILD_LIB),1)
$(CMSIS_DSP_LIB): $(CMSIS_DSP_OBJS)
	@echo AR $@
	$(AR) rcs $@ $(CMSIS_DSP_OBJS)
endif

endif # CMSIS_DSP_MK
