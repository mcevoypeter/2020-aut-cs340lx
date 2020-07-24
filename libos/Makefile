TARGET=libos.a

# SYS_OBJS = ckalloc.o ck-gc.o gc-asm.o
# these are object files we give out.
SYS_OBJS := $(wildcard ./staff-objs/*.o)
ALL_SOURCES := $(wildcard ./*.c ./libc/*.c)
ALL_ASM := $(wildcard ./*.S)
ALL_OBJS := $(patsubst %.c, %.o, $(ALL_SOURCES))  $(patsubst %.S, %.o, $(ALL_ASM))  
OBJS := $(ALL_OBJS) $(SYS_OBJS)

# check that your path is defined.
ifndef CS240LX_2021_PATH
$(error CS240LX_2021_PATH is not set: this should contain the absolute path to where this directory is.  Define it in your shell's initialiation.  For example, .tcshrc for tcsh or .bashrc for bash)
endif
ifndef CS340LX_2021_PATH
$(error CS340LX_2021_PATH is not set: this should contain the absolute path to where this directory is.  Define it in your shell's initialiation.  For example, .tcshrc for tcsh or .bashrc for bash)
endif

include $(CS240LX_2021_PATH)/libpi/includes.mk

MEMMAP := ./memmap
CFLAGS += -Wno-unused-function -Wno-unused-variable  -I$(CS340LX_2021_PATH)/pix

# for assembly
CPP_ASFLAGS += -I$(CS340LX_2021_PATH)/pix

# add all the .h's as dependencies, libpi and the makefile.
DEPS := $(MEMMAP) $(LPI) $(wildcard ./*.h) ./Makefile 
LIBS = $(LPI)  

all:  libs $(OBJS) $(TARGET) # $(TEST_RUN) # $(TEST_CHECK)

libs: 
	make -C $(LPP) 

$(OBJS): $(DEPS)
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(CC) -c $(CPP_ASFLAGS) $< -o $@


# we bundle start.s with the library: just make different ones for each
# different start so that the library is internally consistent.
$(TARGET): $(OBJS) $(SYS_OJBS) $(DEPS)
	arm-none-eabi-ar crf $(TARGET) $(OBJS) $(SYS_OBJS)
	$(OD) -D $@ > $@.list

# rules to generate .elf->.list->.bin
%.elf: %.o $(OBJS) $(SYS_OBJS) $(MEMMAP) $(DEPS) 
	$(LD) $(START) $(OBJS) $(SYS_OBJS) -T $(LPP)/memmap -o $@  $(LIBS)

%.list: %.elf
	$(OD) -D   $*.elf > $*.list
%.bin: %.list
	$(OCP) $*.elf -O binary $*.bin

%.run: %.bin
	my-install ./$*.bin

tags: 
	ctags *.[chSs] */*.[ch]

clean:
	rm -rf $(ALL_OBJS) $(NAME).bin *~ tags *.bin *.elf *.list *.img Makefile.bak  *.d *.test *.a

.PHONY: libs all tags clean 

# stop make from deleting these suffix'd files.
.PRECIOUS: %.list %.elf
