TARGET = ros


OBJ_DIR = ./obj
CROSS_COMPILE = /usr/local/arm/arm-eabi-4.4.0/bin/arm-eabi-

STRIP = $(CROSS_COMPILE)strip
OBJDUMP = $(CROSS_COMPILE)objdump
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
BIN_FLAGS = -O binary -o

#LIBS = /cygdrive/d/GNUARM/bin/../lib/gcc/arm-elf/4.1.1/libgcc.a
#LIB = /usr/local/arm/4.5.1/bin/../lib/gcc/arm-none-linux-gnueabi/4.5.1/libgcc.a
#LIBS += /usr/local/arm/4.5.1/bin/../lib/gcc/arm-none-linux-gnueabi/4.5.1/libgcc.a
#LIBS = $(shell `$(CC) -print-libgcc-file-name`)
#LIBS += -L /usr/local/arm/4.5.1/arm-none-linux-gnueabi/sys-root/usr/lib -lc
LIBS = libgcc.a

WARN_FLAGS = -Wall 
WARN_FLAGS += -Wno-attributes #remove pack warning on uint8 etc
WARN_FLAGS += -Wno-unused #remove pack warning on uint8 etc
#WARN_FLAGS += -Wpacked -Wpadded -Wshadow -Wcast-qual -Wcast-align \
			  -Wwrite-strings -Wconversion -Wsign-compare

#EXTRA_FLAGS = -Q -Ftime-report -fmem-report
#EXTRA_FLAGS += -print-search-dirs

#CFLAGS =  -march=armv6 -mlittle-endian -msoft-float -nostartfiles -nostdlib 
#CFLAGS =  -march=armv6 -mlittle-endian -nostartfiles -nostdlib 
CFLAGS =  -mlittle-endian -nostartfiles -nostdlib 
CFLAGS += -c -fomit-frame-pointer -fno-inline -fkeep-inline-functions
#CFLAGS += -mhard-float -mcpu=arm1176jzf-s
CFLAGS += -msoft-float
CFLAGS += $(WARN_FLAGS)
CFLAGS += $(EXTRA_FLAGS)
CFLAGS += -funsigned-char
CFLAGS += -Iinclude 
CFLAGS += -Iucos/include 
CFLAGS += -Ilwip/src/include -Ilwip/src/include/ipv4 -Ilwip/src/include/ipv6
CFLAGS += -DLWIP_DEBUG
CFLAGS += -g #for DWARF debug infomation
#CLFAGS += -L $(shell dirname `$(CC) -print-libgcc-file-name`) -lgcc

LDFLAGS = -Map $(TARGET).map -T ros.ld #$(LIBS)
#LDFLAGS += -L $(shell dirname `$(CC) -print-libgcc-file-name`) -lgcc
#LDFLAGS += -L '$(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`)' -lgcc

DIRS =

SRC_ASM = $(wildcard *.s) $(wildcard */*.s) $(wildcard */*/*.s)
SRC_C = $(wildcard *.c) $(wildcard */*.c) $(wildcard */*/*.c) 
SRC_C += $(wildcard */*/*/*.c) $(wildcard */*/*/*/*.c) $(wildcard */*/*/*/*/*.c) 
SRCS = $(SRC_C) $(SRC_ASM)

OBJS = $(SRC_ASM:%.s=%.o) $(SRC_C:%.c=%.o)
OBJ_ALL = $(addprefix $(OBJ_DIR)/,$(OBJS))


#bin:echo_target depend $(TARGET).elf
bin:$(TARGET).elf
	$(STRIP) $(TARGET).elf $(BIN_FLAGS) $(TARGET).bin

$(TARGET).elf: $(OBJ_ALL)
	@echo "lib:$(LIB)"
	@echo "LDFLAGS:$(LDFLAGS)"
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)
	$(OBJDUMP) -d -S $@ > $(TARGET).asm

$(OBJ_DIR)/%.o:%.c
	@-mkdir -p $(dir $@) 2> /dev/null
	@echo CC $<
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/%.o:%.s
	@-mkdir -p $(dir $@) 2> /dev/null
	@echo ASM $<
	@$(CC) $(CFLAGS) $< -o $@

#depend: Makefile $(SRCS)
	##echo $(SRCS)
	#rm -f $@
	#for f in $(SRCS); do \
		#g=`echo $$f | sed -e 's/\(.*\)\.\w/\1.o/'`; \
		#$(CC) -M $(CFLAGS) -MQ $(OBJ_DIR)/$$g $$f >> $@ ; \
	#done

#sinclude depend

echo_target:
#@echo "asm objects: $(SRC_ASM)"
#@echo "c objects: $(SRC_C)"
#@echo "all objects: $(OBJ_ALL)"
	@echo "cflags: $(CFLAGS)"

clean:
	-rm -f $(TARGET).bin
	-rm -f $(TARGET).elf
	-rm -f $(TARGET).map
	-rm -f $(TARGET).asm
	-rm -rf $(OBJ_DIR)

rebuild:
	make clean
	make -j4
