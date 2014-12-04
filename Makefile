##################################################################
#
# Makefile for OpenLRSng
#

#
# If your Arduino is in a weird place, you'll need to change this.
#
ARDUINO_PATH=/usr/share/arduino

#
# Board type can be one of 6 values:
# 0 - Flytron M1 TX
# 1 - Flytron M1 RX
# 2 - Flytron M2, M3 TX, Orange-TX
# 3 - Flytron V2 RX, Hawkeye RX, HK Orange-RX
# 4 - Hawkeye TX, OpenLRSng TX
# 5 - DTF 4ch RX
# 6 - Deluxe TX
# 7 - PowerTowerRX
#
BOARD_TYPE=3
BOARD_TYPES_TX=0 2 3 4 5 6 7 8 9
BOARD_TYPES_RX=2 3 5 7 8 9

#
# You can compile all TX as TX, and all RX as either RX or TX.
# You cannot currently compile TX as RX.
# This flag controls what primary function the board will have
#
COMPILE_TX=1

#
# No real user options below here.
##################################################################

#
# You don't want to change this unless you really know that you
# need to.  CPU clock.
#
CLOCK=16000000L

#
# Board type 6 requires a different Arduino target
#
ifeq ($(BOARD_TYPE),6)
CPU=atmega32u4
USB_VID=0x2341
USB_PID=0x8036
VARIANT=leonardo
BOOTLOADER=Caterina-Leonardo.hex
else
CPU=atmega328p
USB_VID=null
USB_PID=null
VARIANT=standard
BOOTLOADER=optiboot_atmega328.hex
endif

#
# C preprocessor defines
#
DEFINES=-DBOARD_TYPE=$(BOARD_TYPE) -DCOMPILE_TX=$(COMPILE_TX) -DRFMTYPE=$(RFMTYPE)

#
# AVR GCC info
#
EXEPREFIX=avr-
ifneq (,$(wildcard $(ARDUINO_PATH)/hardware/tools/avr/bin/avr-gcc))
	EXEPATH=$(ARDUINO_PATH)/hardware/tools/avr/bin
else ifneq (,$(wildcard /usr/bin/avr-gcc))
	EXEPATH=/usr/bin
endif

#
# AVR gcc and binutils
#
CC=$(EXEPATH)/$(EXEPREFIX)gcc
CXX=$(EXEPATH)/$(EXEPREFIX)g++
AR=$(EXEPATH)/$(EXEPREFIX)ar
SIZE=$(EXEPATH)/$(EXEPREFIX)size
OBJCOPY=$(EXEPATH)/$(EXEPREFIX)objcopy

#
# Shell commands
#
RM=rm
MKDIR=mkdir
LS=ls
SED=sed
CAT=cat

#
# Styling
#
ASTYLE=astyle
ASTYLEOPTIONS=--options=none --style=1tbs --indent=spaces=2 --suffix=none --lineend=linux

#
# Compile flags
#
COPTFLAGS= -g -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
	   -fno-inline-small-functions -Wl,--relax -mcall-prologues

CFLAGS=-Wall -ffunction-sections -fdata-sections -mmcu=$(CPU) -DF_CPU=$(CLOCK) -MMD \
	-DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID) -DARDUINO=105 -D__PROG_TYPES_COMPAT__ $(DEFINES)
CXXFLAGS=-fno-exceptions

#
# Arduino libraries used, compilation settings.
#
ARDUINO_LIBS=
ARDUINO_LIB_PATH=$(ARDUINO_PATH)/libraries/
ARDUINO_LIB_DIRS=$(addprefix $(ARDUINO_LIB_PATH),$(ARDUINO_LIBS))
ARDUINO_LIB_INCL=$(addsuffix $(ARDUINO_LIBS),-I$(ARDUINO_LIB_PATH))
ARDUINO_LIB_SRCS=$(addsuffix .cpp,$(addprefix $(ARDUINO_LIB_PATH),$(ARDUINO_LIBS)/$(ARDUINO_LIBS)))
ARDUINO_LIB_OBJS=$(patsubst %.cpp, libraries/%.o, $(addsuffix .cpp,$(ARDUINO_LIBS)))

#
# Arduino variant settings
#
ARDUINO_VARIANT_PATH=$(ARDUINO_PATH)/hardware/arduino/variants/$(VARIANT)

#
# Arduino library files used, compilation settings.
#
ARDUINO_CORELIB_PATH=$(ARDUINO_PATH)/hardware/arduino/cores/arduino/
ARDUINO_CORELIB_SRCS=WInterrupts.c wiring.c wiring_shift.c wiring_digital.c \
		     wiring_pulse.c wiring_analog.c \
		     CDC.cpp Print.cpp HardwareSerial.cpp WString.cpp \
		     Stream.cpp main.cpp USBCore.cpp HID.cpp
ARDUINO_CORELIB_OBJS= $(patsubst %.c, libraries/%.o, $(patsubst %.cpp, libraries/%.o, $(ARDUINO_CORELIB_SRCS)))


#
# Arduino stdc library files used, compilation settings.
#
ARDUINO_LIBC_PATH=$(ARDUINO_PATH)/hardware/arduino/cores/arduino/avr-libc/
ARDUINO_LIBC_SRCS=malloc.c realloc.c

#
# Master include path
#
INCLUDE=-I$(ARDUINO_CORELIB_PATH) -I$(ARDUINO_VARIANT_PATH) $(ARDUINO_LIB_INCL) -I.

#
# Project folders
#
LIBRARIES_FOLDER=libraries
OUT_FOLDER=out

#
# Target object files
#
OBJS=openLRSng.o $(ARDUINO_LIB_OBJS) $(LIBRARIES_FOLDER)/libcore.a

#
# Master target
#
all: mkdirs openLRSng.hex

#
# From here down are build rules
#
VPATH := $(ARDUINO_LIB_DIRS) $(ARDUINO_CORELIB_PATH) $(ARDUINO_LIBC_PATH)

define ino-command
	@$(CXX) -c $(COPTFLAGS) $(CXXFLAGS) $(CFLAGS) $(INCLUDE) -o $@ -x c++ $<
endef
define cc-command
	@$(CC) -c $(COPTFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<
endef
define cxx-command
	@$(CXX) -c $(COPTFLAGS) $(CXXFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<
endef

.PHONY: all clean upload astyle 433 868 915 allfw

%.o: %.ino
	$(ino-command)

%.o: %.c
	$(cc-command)

%.o: %.cpp
	$(cxx-command)

$(LIBRARIES_FOLDER)/%.o: %.c
	$(cc-command) 2>/dev/null

$(LIBRARIES_FOLDER)/%.o: %.cpp
	$(cxx-command) 2>/dev/null

#
# Other targets
#
clean: clean_compilation_products
	@$(RM) -rf $(OUT_FOLDER)

clean_compilation_products:
	@$(RM) -rf $(LIBRARIES_FOLDER)
	@$(RM) -f *.[aod] *.elf *.eep *.d *.hex

mkdirs:
	@$(MKDIR) -p $(LIBRARIES_FOLDER)

openLRSng.hex: $(OBJS)
	@$(CC) -Os -Wl,--gc-sections -mmcu=$(CPU) -o openLRSng.elf $(OBJS) -L$(LIBRARIES_FOLDER) -lm
	@$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load \
		--no-change-warnings --change-section-lma .eeprom=0 \
		openLRSng.elf openLRSng.eep
	@$(OBJCOPY) -O ihex -R .eeprom openLRSng.elf openLRSng.hex
	@echo "NOTE: Deployment size is text + data."
	@$(SIZE) openLRSng.elf
	@$(SED) "/:00000001FF/d" openLRSng.hex > openLRSngBL.hex
	@$(CAT) bootloaders/$(BOOTLOADER) >> openLRSngBL.hex

$(LIBRARIES_FOLDER)/libcore.a: $(ARDUINO_CORELIB_OBJS)
	@$(AR) rcs $(LIBRARIES_FOLDER)/libcore.a $(ARDUINO_CORELIB_OBJS)

astyle:
	$(ASTYLE) $(ASTYLEOPTIONS) openLRSng.ino *.h

433 868 915:
	$(RM) -rf $(OUT_FOLDER)/$@
	$(MKDIR) -p $(OUT_FOLDER)/$@
	$(foreach type, $(BOARD_TYPES_RX), make -s RFMTYPE=$@ COMPILE_TX=0 BOARD_TYPE=$(type) clean_compilation_products all && cp openLRSng.hex $(OUT_FOLDER)/$@/RX-$(type).hex && cp openLRSngBL.hex $(OUT_FOLDER)/$@/RX-$(type)-bl.hex;)
	$(foreach type, $(BOARD_TYPES_TX), make -s RFMTYPE=$@ COMPILE_TX=1 BOARD_TYPE=$(type) clean_compilation_products all && cp openLRSng.hex $(OUT_FOLDER)/$@/TX-$(type).hex && cp openLRSngBL.hex $(OUT_FOLDER)/$@/TX-$(type)-bl.hex;)
	make -s RFMTYPE=$@ COMPILE_TX=0 BOARD_TYPE=8 CLOCK=8000000 clean_compilation_products all && cp openLRSng.hex $(OUT_FOLDER)/$@/RX-8-8MHz.hex && cp openLRSngBL.hex $(OUT_FOLDER)/$@/RX-8-8MHz-bl.hex
	$(LS) -l $(OUT_FOLDER)

allfw: 433 868 915
	$(LS) -lR $(OUT_FOLDER)

