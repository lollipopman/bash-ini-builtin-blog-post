CC:=gcc
INC:=-I /usr/include/bash -I /usr/include/bash/include -I /usr/include/bash/builtins -I /usr/lib/bash
CFLAGS:=-c -fPIC -Wall -Wextra
LDFLAGS:=--shared
INIH_FLAGS:=-DINI_CALL_HANDLER_ON_NEW_SECTION=1 -DINI_STOP_ON_FIRST_ERROR=1 -DINI_USE_STACK=0

ini.so: libinih.o ini.o
	$(CC) -o $@ $^ $(LDFLAGS)

sleep.so: sleep.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^

libinih.o: inih/ini.c
	$(CC) $(CFLAGS) $(INIH_FLAGS) -o libinih.o inih/ini.c

inih/ini.c:
	git submodule update --init

.PHONY: test
test: ini.so
	./test
	@echo Tests Passed

.PHONY: clean
clean:
	rm -f *.o *.so
