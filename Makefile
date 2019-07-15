# cross-platform directory manipulation
ifeq ($(shell echo $$OS),$$OS)
    MAKEDIR = if not exist "$(1)" mkdir "$(1)"
    RM = rmdir /S /Q "$(1)"
else
    MAKEDIR = '$(SHELL)' -c "mkdir -p \"$(1)\""
    RM = '$(SHELL)' -c "rm -rf \"$(1)\""
endif


.SUFFIXES:
.SUFFIXES: .cpp .o

vpath %.cpp .
vpath %.c .
vpath %.h .

# Boiler-plate
###############################################################################
# Project settings
SRC_DIR := ./
OBJ_DIR := ./build/obj
BIN_DIR := ./build/bin
BPROJECT := ./build/bin/BLACKRAINLOADER_v101.DER

POKITO_EMU_dir := /home/ramzi/PokittoEmu

# Project settings
###############################################################################
# Objects and Paths

C_SRC := $(shell find $(SRC_DIR) -name '*.c')
CXX_SRC := $(shell find $(SRC_DIR) -name '*.cpp')
C_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRC))
CXX_OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CXX_SRC))


INCLUDE_PATHS += -I./.
INCLUDE_PATHS += -I./SDFileSystem
INCLUDE_PATHS += -I./SDFileSystem/FATFileSystem
INCLUDE_PATHS += -I./SDFileSystem/FATFileSystem/ChaN
INCLUDE_PATHS += -I./mbed-pokitto
INCLUDE_PATHS += -I./mbed-pokitto/api
INCLUDE_PATHS += -I./mbed-pokitto/common
INCLUDE_PATHS += -I./mbed-pokitto/hal
INCLUDE_PATHS += -I./mbed-pokitto/targets
INCLUDE_PATHS += -I./mbed-pokitto/targets/cmsis
INCLUDE_PATHS += -I./mbed-pokitto/targets/cmsis/TARGET_NXP
INCLUDE_PATHS += -I./mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X
INCLUDE_PATHS += -I./mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/TOOLCHAIN_GCC_ARM
INCLUDE_PATHS += -I./mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/TOOLCHAIN_GCC_ARM/TARGET_LPC11U68
INCLUDE_PATHS += -I./mbed-pokitto/targets/cmsis/TOOLCHAIN_GCC
INCLUDE_PATHS += -I./mbed-pokitto/targets/hal
INCLUDE_PATHS += -I./mbed-pokitto/targets/hal/TARGET_NXP
INCLUDE_PATHS += -I./mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X

LINKER_SCRIPT := loader_test.ld

# Objects and Paths
###############################################################################
# Tools and Flags

#embits Toolchain
TOOLCHAIN:=/opt/em_armgcc/bin

#arduino Toolchain
#TOOLCHAIN:=/opt/arm-none-eabi-gcc/4.8.3-2014q1/bin

#arduino Toolchain
#TOOLCHAIN:=/opt/gcc-arm-none-eabi-8-2018-q4/bin

LPCRC := ../build/lpcrc


AS      = $(TOOLCHAIN)/arm-none-eabi-gcc -x assembler-with-cpp -c -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fmessage-length=0 -fno-exceptions -fno-builtin -ffunction-sections -fdata-sections -funsigned-char -MMD -fno-delete-null-pointer-checks -fomit-frame-pointer -Os -g1 -DMBED_RTOS_SINGLE_THREAD -mcpu=cortex-m0plus -mthumb
CC      = $(TOOLCHAIN)/arm-none-eabi-gcc 
CPP     = $(TOOLCHAIN)/arm-none-eabi-g++ 
LD      = $(TOOLCHAIN)/arm-none-eabi-gcc
ELF2BIN = $(TOOLCHAIN)/arm-none-eabi-objcopy
PREPROC = $(TOOLCHAIN)/arm-none-eabi-cpp 


C_FLAGS := -Os -c -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fmessage-length=0 -fno-exceptions -fno-builtin -ffunction-sections -fdata-sections -funsigned-char -MMD -fno-delete-null-pointer-checks -fomit-frame-pointer -g0 -DMBED_RTOS_SINGLE_THREAD -mcpu=cortex-m0plus -mthumb
C_FLAGS +=-DMBED_RTOS_SINGLE_THREAD  -DTARGET_LPC11U68 -D__MBED__=1 -DDEVICE_I2CSLAVE=1 -DTARGET_LIKE_MBED -DTARGET_NXP -D__MBED_CMSIS_RTOS_CM -DDEVICE_RTC=1 -DTOOLCHAIN_object -D__CMSIS_RTOS -DTOOLCHAIN_GCC -DTARGET_CORTEX_M -DTARGET_M0P -DTARGET_UVISOR_UNSUPPORTED -DDEVICE_SERIAL=1 -DDEVICE_INTERRUPTIN=1 -DTARGET_LPCTarget -DTARGET_CORTEX -DDEVICE_I2C=1 -D__CORTEX_M0PLUS -DTARGET_FF_ARDUINO -DTARGET_RELEASE -DMBED_BUILD_TIMESTAMP=1507563655.72 -DARM_MATH_CM0PLUS -DTARGET_LPC11U6X -DDEVICE_SLEEP=1 -DTOOLCHAIN_GCC_ARM -DDEVICE_SPI=1 -DDEVICE_ANALOGIN=1 -DDEVICE_PWMOUT=1 -DTARGET_LIKE_CORTEX_M0 -DPROJ_HIRES=0 -DPROJ_STARTUPLOGO=0 -DPROJ_GAMEBUINO=0 -DPROJ_GBSOUND=0 -DPROJ_STREAMING_MUSIC=0 -DPROJ_ENABLE_SYNTH=0 -DPROJ_ENABLE_SOUND=0 -DBASE_ADDRESS=131072 -DBYPASS_SYSTEMINIT=1 -DMINILOADHIGH -DBUBBLESORT 

#CXX_FLAGS := -Os -fno-rtti -Wvla -c -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fmessage-length=0 -fno-exceptions -fno-builtin -ffunction-sections -fdata-sections -funsigned-char -MMD -fno-delete-null-pointer-checks -fomit-frame-pointer  -g0 -DMBED_RTOS_SINGLE_THREAD -mcpu=cortex-m0plus -mthumb -fno-use-cxa-atexit
CXX_FLAGS:= -mcpu=cortex-m0plus -mthumb -Os -c -fno-exceptions -fno-rtti -std=gnu++11 -Wextra -Wall -fomit-frame-pointer -fdata-sections -ffunction-sections -g1 -funsigned-char -Wno-unused-parameter -Wno-missing-field-initializers -fmessage-length=0 -fno-builtin -fno-delete-null-pointer-checks -fno-use-cxa-atexit
CXX_FLAGS += -DTARGET_LPC11U68 -D__MBED__=1 -DDEVICE_I2CSLAVE=1 -DTARGET_LIKE_MBED -DTARGET_NXP -D__MBED_CMSIS_RTOS_CM -DDEVICE_RTC=1 -DTOOLCHAIN_object -D__CMSIS_RTOS -DTOOLCHAIN_GCC -DTARGET_CORTEX_M -DTARGET_M0P -DTARGET_UVISOR_UNSUPPORTED -DDEVICE_SERIAL=1 -DDEVICE_INTERRUPTIN=1 -DTARGET_LPCTarget -DTARGET_CORTEX -DDEVICE_I2C=1 -D__CORTEX_M0PLUS -DTARGET_FF_ARDUINO -DTARGET_RELEASE -DMBED_BUILD_TIMESTAMP=1507563655.72 -DARM_MATH_CM0PLUS -DTARGET_LPC11U6X -DDEVICE_SLEEP=1 -DTOOLCHAIN_GCC_ARM -DDEVICE_SPI=1 -DDEVICE_ANALOGIN=1 -DDEVICE_PWMOUT=1 -DTARGET_LIKE_CORTEX_M0 -DPROJ_HIRES=0 -DPROJ_STARTUPLOGO=0 -DPROJ_GAMEBUINO=0 -DPROJ_GBSOUND=0 -DPROJ_STREAMING_MUSIC=0 -DPROJ_ENABLE_SYNTH=0 -DPROJ_ENABLE_SOUND=0 -DBASE_ADDRESS=131072 -DBYPASS_SYSTEMINIT=1 -DMINILOADHIGH -DBUBBLESORT -DMBED_RTOS_SINGLE_THREAD 

LD_FLAGS :=-Os -Wl,--gc-sections -Wl,-n --specs=nano.specs -mcpu=cortex-m0plus -mthumb -flto
#LD_SYS_LIBS :=-Wl,--start-group -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys  -Wl,--end-group
LC_SYS_LIBS := -Wl,-Map=./build/vin/loa.der.map -specs=nano.specs  -Wl,--gc-sections -Wl,-n -lm -lc -lsupc++ -lgcc -lnosys 


# Tools and Flags
###############################################################################
# Rules

.PHONY: all size clean run release analyse

all: $(BPROJECT).bin size
	+@$(call MAKEDIR,$(OBJ_DIR))
	size $(BPROJECT).elf


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Compile: $(notdir $<)"
	@$(CC) $(C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Compile: $(notdir $<)"
	@$(CPP) $(CXX_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(BPROJECT).elf: $(C_OBJ) $(CXX_OBJ) 
	+@$(call MAKEDIR,$(dir $@))
	+@echo "link: $(notdir $@)"
	@$(LD) $(LD_FLAGS) -Wl,-script=$(LINKER_SCRIPT) -o $(C_OBJ) $(CXX_OBJ) $(LD_SYS_LIBS) --output $@


$(BPROJECT).bin: $(BPROJECT).elf
	$(ELF2BIN) -O binary $< $@
	+@echo "===== bin file ready to flash: $@ =====" 

run: $(BPROJECT).bin
	$(POKITO_EMU_dir)/PokittoEmu.exe -I $(POKITO_EMU_dir)/sd.img $(BPROJECT).bin

release: $(C_OBJ) $(CXX_OBJ) 
	+@echo "link: $(notdir $@)"
	@$(LD) $(LD_FLAGS) -Wl,-script=loader101.ld $(C_OBJ) $(CXX_OBJ) $(LD_SYS_LIBS) --output $(BPROJECT)_release.elf
	@$(ELF2BIN) -O binary $(BPROJECT)_release.elf $(BPROJECT)
	@$(LPCRC) $(BPROJECT)

analyse: $(BPROJECT).elf
	+@echo "analyse: $(notdir $@)"
	$(TOOLCHAIN)/arm-none-eabi-readelf -s $(BPROJECT).elf > ./build/analyse.txt

# Rules
###############################################################################
# Dependencies

DEPS = $(C_OBJ:.o=.d) $(CXX_OBJ:.o=.d)
-include $(DEPS)
# endif

# Dependencies
###############################################################################

clean :
	$(call RM,$(OBJ_DIR))