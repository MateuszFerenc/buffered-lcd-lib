TARGET = firmware

OUTPUT_DIR = Bin
SRC_DIR = Src

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(addprefix $(OUTPUT_DIR)/./,$(notdir $(SRC:.c=.o)))
LST = $(OBJ:.o=.lst)
C_DEPS = $(OBJ:.o=.d)
# LIB_DIR = ../Src/Lib
# SRC += $(wildcard $(LIB_DIR)/*.c)
# OBJ += $(notdir $(LIB_SRC:.c=.o))


DEVICE = atmega16a
FREQ = 14745600

OBJCOPY = avr-objcopy
AVRSIZE = avr-size
AVROBJDUMP = avr-objdump
AR = avr-ar

MCU = -mmcu=${DEVICE}
SPEED = -DF_CPU=${FREQ}UL

CC = avr-gcc
CFLAGS += -Os
# -Oz
CFLAGS += -Wfatal-errors
#	-Wfatal-errors              Exit on the first error occurred.
CFLAGS += -Wall
#	-Wall                       Enable most warning messages.
CFLAGS += -funsigned-char
#	-funsigned-char             Make "char" unsigned by default.
CFLAGS += -funsigned-bitfields
#	-funsigned-bitfields        When "signed" or "unsigned" is not given make the \
                              bitfield unsigned.
CFLAGS += -fpack-struct
#	-fpack-struct               Pack structure members together without holes.
CFLAGS += -fshort-enums
#	-fshort-enums               Use the narrowest integer type possible for \
                              enumeration types.
CFLAGS += -ffunction-sections
#	-ffunction-sections         Place each function into its own section.
CFLAGS += -fdata-sections
#	-fdata-sections             Place data items into their own section.
CFLAGS += -std=gnu99
# gnu11
#	-std=gnu99                  Conform to the ISO 1999 C standard with GNU \
                              extensions.
CFLAGS += -fdiagnostics-color=always
CFLAGS += -Wa,-adhlns=$(OUTPUT_DIR)/./$(@F:.o=.s)
#  	-Wa,...:      tell GCC to pass this to the assembler.
#    	-adhlns...: create assembler listing

MORE_CFLAGS = -MMD -MP -MF $(OUTPUT_DIR)/./$(@F:.o=.d) -MT $(OUTPUT_DIR)/./$(@F)
#-MMD -MP -MF$(notdir $($<:.c=.d)) -MT$(notdir $($<:.c=.d))
#   -MD                         Generate make dependencies and compile.
#   -MF <file>                  Write dependency output to the given file.
#   -MMD                        Like -MD but ignore system header files.
#   -MP                         Generate phony targets for all headers.
#   -MT <target>                Add an unquoted target.

LDFLAGS = -Os
LDFLAGS += -Wl,--gc-sections
#	-Wl,<options>            Pass comma-separated <options> on to the linker.
#		--gc-sections               Remove unused sections (on some targets)
LDFLAGS += -Wl,-Map=$(OUTPUT_DIR)/./$(TARGET).map,--cref
#  	-Wl,...:     tell GCC to pass this to linker.
#    	-Map:      create map file
#    		--cref:    add cross reference to  map file
LDFLAGS += -Wl,-u,vfprintf -lprintf_min
LDFLAGS += -Wl,--relax
LDFLAGS += -fdiagnostics-color=always

COMPILATION_OUTPUT = 2>$(OUTPUT_DIR)/./"compilation_output_$(@F:.o=).txt"

PROGRAMMER = USBasp
AVRDUDE = avrdude
AVRDUDE_FLAGS = -c ${PROGRAMMER} -p ${DEVICE}

DEL=del

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include $(C_DEPS)

all: build size

build: $(OUTPUT_DIR)/./$(TARGET).hex
eep: $(OUTPUT_DIR)/./$(TARGET).eep
debug: $(OUTPUT_DIR)/./$(TARGET).lst


$(OUTPUT_DIR)/./%.o: $(SRC_DIR)/./%.c
	@echo
	@echo ######   Compiling: $<   ######   
	${CC} ${CFLAGS} ${MORE_CFLAGS} ${MCU} ${SPEED} -c $< -o $@ $(COMPILATION_OUTPUT)

$(OUTPUT_DIR)/./$(TARGET).elf: $(OBJ)
	@echo
	@echo ######   Linking: $^   ######   
	${CC} ${LDFLAGS} ${MCU} $^ -o $@

$(OUTPUT_DIR)/./%.hex: $(OUTPUT_DIR)/./%.elf
	@echo
	@echo ######   Converting $< to $@   ######   
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

size: $(OUTPUT_DIR)/./$(TARGET).elf
	@echo
	${AVRSIZE} --mcu=${DEVICE} -C -x $<  
	${AVRSIZE} -B -x $< --mcu=${DEVICE} -d

$(OUTPUT_DIR)/./%.eep: $(OUTPUT_DIR)/./$(TARGET).elf
	@echo
	@echo ######   Getting EEPROM image: $@   ######   
	${OBJCOPY} -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $< $@

clean:
	@echo
	@echo ######   Cleaning up   ######   
	-${DEL} "$(OUTPUT_DIR)\*.elf"
	-${DEL} "$(OUTPUT_DIR)\*.hex"
	-${DEL} "$(OUTPUT_DIR)\*.lst"
	-${DEL} "$(OUTPUT_DIR)\*.o"
	-${DEL} "$(OUTPUT_DIR)\*.d"
	-${DEL} "$(OUTPUT_DIR)\*.s"
	-${DEL} "$(OUTPUT_DIR)\*.eep"
	-${DEL} "$(OUTPUT_DIR)\*.map"
	-${DEL} "$(OUTPUT_DIR)\compilation_output*"

$(OUTPUT_DIR)/./%.lst: $(OUTPUT_DIR)/./$(TARGET).elf
	@echo
	@echo ######   Creating listing file: $@ for debug   ######   
	${AVROBJDUMP} -h -S -s $< > $@

install: $(OUTPUT_DIR)/./${TARGET}.hex
	@echo
	@echo ######   Programming device: $(DEVICE) with: $<   ######   
	${AVRDUDE} ${AVRDUDE_FLAGS} -U flash:w:$< -B 0.5

program_eeprom:	$(OUTPUT_DIR)/./${TARGET}.eep
	@echo
	@echo ######   Writing $< to $(DEVICE) EEPROM   ######   
	${AVRDUDE} ${AVRDUDE_FLAGS} -U eeprom:w:$< -B 0.5

.DEFAULTGOAL: all
.PHONY: all build eep debug install program_eeprom clean