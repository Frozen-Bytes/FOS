#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
#

V 			:= @
TOP 		:= .
OBJDIR 		:= obj
TOOLPREFIX 	:= /opt/cross/bin/i386-elf-
QEMU 		:= qemu-system-i386
PERL		:= perl
IMAGE 		:= $(OBJDIR)/fos.img
LOGSDIR 	:= logs
LOGFILE 	:= $(shell date --iso-8601)

CC			:= $(TOOLPREFIX)gcc -m32 -pipe
GCC_LIB 	:= $(shell $(CC) -print-libgcc-file-name)
AS			:= $(TOOLPREFIX)as --32
AR			:= $(TOOLPREFIX)ar
LD			:= $(TOOLPREFIX)ld -m elf_i386
OBJCOPY		:= $(TOOLPREFIX)objcopy
OBJDUMP		:= $(TOOLPREFIX)objdump
NM			:= $(TOOLPREFIX)nm


# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS	:= -O0 -fno-builtin -I$(TOP) -MD -Wall -Wno-format -Wno-unused -Werror -fno-stack-protector -gstabs

# Linker flags for FOS user programs
ULDFLAGS := -T user/user.ld

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=

# Make sure that 'all' is the first target
all:
	$(V)mkdir -p $(LOGSDIR)

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: %.o \
 	$(OBJDIR)/boot/%.o \
	$(OBJDIR)/kern/%.o \
	$(OBJDIR)/lib/%.o \
	$(OBJDIR)/user/%.o

KERN_CFLAGS := $(CFLAGS) -DFOS_KERNEL
USER_CFLAGS := $(CFLAGS) -DFOS_USER


# Include Makefrags for subdirectories
include boot/Makefrag
include kern/Makefrag
include lib/Makefrag
include user/Makefrag


# Emulators

QEMUEXTRAS	= -chardev stdio,id=char0,mux=on,logfile=$(LOGSDIR)/$(LOGFILE).log,logappend=on -parallel chardev:char0
QEMUOPTS 	= -drive file=$(IMAGE),media=disk,format=raw -smp 1 -m 256 $(QEMUEXTRAS)

qemu: all
	$(V)$(QEMU) $(QEMUOPTS)

qemu-gdb: all
	$(QEMU) $(QEMUOPTS) -S -s

bochs: all
	bochs -q

bochs-gdb: all
	bochs-gdb -qf ".bochsrc-debug"

# For deleting the build

clean:
	rm -rf $(OBJDIR)


# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps


.PHONY: all clean qemu
