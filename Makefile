# Makefile for the elevator project

top       = $(shell pwd)
build_dir = $(top)/build
target    = elevator

src-y += source/main.c
src-y += source/elevator.c
src-y += source/network.c

deps-y += include/types.h
deps-y += include/elevator.h
deps-y += include/network.h

src  = $(addprefix $(top)/, $(src-y))
deps = $(addprefix $(top)/, $(deps-y))

ifndef port
$(error Please define port.)
endif

ifndef floors
$(error Please define number of floors.)
endif

.PHONY: server clean

all: $(src) $(deps)
	@mkdir -p $(build_dir)
	@$(CC) -I$(top)/include $(src) -o $(build_dir)/$(target)
	@$(build_dir)/$(target) $(port) $(floors)

server:
	server/server --port $(port) --numfloors $(floors)

clean:
	@rm -r -r $(build_dir)