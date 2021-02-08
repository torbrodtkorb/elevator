# Makefile for the elevator project

top = $(shell pwd)
builddir  = $(top)/build
target = $(builddir)/elevator

# Include Makefiles, this will update headders and src with a relative path 
include $(top)/include/Makefile
include $(top)/source/Makefile

deps_glob = $(addprefix $(top)/, $(headers))
src_glob = $(addprefix $(top)/, $(src))

# Add include path
cpflags += -I$(top)/include

.PHONY: all clean

all: $(target).elf
	@$< $(arg)

%.elf: $(src_glob) $(deps_glob)
	@mkdir -p $(dir $@)
	@$(CC) $(cpflags) $(src_glob) -o $@

clean:
	@rm -r -f $(builddir)
