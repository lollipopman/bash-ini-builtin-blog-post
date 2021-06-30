CC=gcc
INC=-I /usr/include/bash -I /usr/include/bash/include -I /usr/include/bash/builtins -I /usr/lib/bash
CFLAGS=-c -fPIC
LDFLAGS=--shared
INIH_FLAGS=-DINI_CALL_HANDLER_ON_NEW_SECTION=1 -DINI_STOP_ON_FIRST_ERROR=1 -DINI_USE_STACK=0

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^

ini.so: ini.o libinih.o
	$(CC) -o $@ $^ $(LDFLAGS)

libinih.o:
	$(CC) $(CFLAGS) $(INIH_FLAGS) -o libinih.o inih/ini.c

test: ini.so
	bash test.sh

clean:
	rm -f *.o *.so
