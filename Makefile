SHELL=/bin/bash
CC:=gcc
CFLAGS:=-c -Wall -Wextra -fPIC
BASH_FLAGS:=$(shell pkgconf --cflags bash)
LDFLAGS:=--shared
INIH_FLAGS:=-DINI_CALL_HANDLER_ON_NEW_SECTION=1 -DINI_STOP_ON_FIRST_ERROR=1 \
	-DINI_USE_STACK=0

ini.so: inih/ini.o

%.so: %.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

inih/ini.o: CFLAGS += $(INIH_FLAGS)
ini.o: CFLAGS += $(BASH_FLAGS)
sleep.o: CFLAGS += $(BASH_FLAGS)

inih/ini.c:
	git submodule update --init

.PHONY: test
test: ini.so
	./test
	@echo Tests Passed

.PHONY: clean
clean:
	shopt -s globstar; rm -f **/*.o **/*.so
