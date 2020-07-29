NAME=clone

# SYS_OBJS := $(wildcard ./staff-objs/*.o)
ALL_SOURCES := $(wildcard ./*.c)
ALL_ASM := $(wildcard ./*.S)
ALL_OBJS := $(patsubst %.c, %.o, $(ALL_SOURCES))  $(patsubst %.S, %.o, $(ALL_ASM))  
OBJS := $(ALL_OBJS) $(SYS_OBJS)

# check that your path is defined.
ifndef CS340LX_2021_PATH
$(error CS340LX_2021_PATH is not set: this should contain the absolute path to where this directory is.  Define it in your shell's initialiation.  For example, .tcshrc for tcsh or .bashrc for bash)
endif

include $(CS240LX_2021_PATH)/libpi/includes.mk


LIBOS_PATH := $(CS340LX_2021_PATH)/libos.staff
LIBOS := $(LIBOS_PATH)/libos.a
PIX_PATH := $(CS340LX_2021_PATH)/pix.staff
MEMMAP := $(LIBOS_PATH)/memmap
START := $(LIBOS_PATH)/start.o

CFLAGS += -Wno-unused-function -Wno-unused-variable  -I$(LIBOS_PATH) -I$(PIX_PATH)

# add all the .h's as dependencies, libpi and the makefile.
DEPS := $(MEMMAP) $(LIBOS) $(wildcard ./*.h) ./Makefile 
LIBS = $(LIBOS)

all:  libs $(OBJS) $(NAME).bin make-init # $(TEST_RUN) # $(TEST_CHECK)

libs: 
	make -C $(LIBOS_PATH) 
	make -C ../binary-to-array

make-init: $(NAME).bin
	make -C ../binary-to-array
	../binary-to-array/bin-to-array $(NAME).bin > init-hack.h
	cp init-hack.h $(PIX_PATH)
	make -C $(PIX_PATH) 

$(OBJS): $(DEPS)
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(CC) -c $(CPP_ASFLAGS) $< -o $@

# rules to generate .elf->.list->.bin
%.elf: %.o $(OBJS) $(SYS_OBJS) $(MEMMAP) $(DEPS) 
	$(LD) $(START) $(OBJS) $(SYS_OBJS) -T $(MEMMAP) -o $@  $(LIBS)

%.list: %.elf
	$(OD) -D   $*.elf > $*.list
%.bin: %.list
	$(OCP) $*.elf -O binary $*.bin

%.run: %.bin
	my-install ./$*.bin

tags: 
	ctags *.[chSs] */*.[ch]

clean:
	rm -rf $(ALL_OBJS) $(NAME).bin *~ tags *.bin *.elf *.list *.img Makefile.bak  *.d *.test init-hack.h

.PHONY: libs all tags clean 

# stop make from deleting these suffix'd files.
.PRECIOUS: %.list %.elf
