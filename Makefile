CC=gcc
INC=-I /usr/include/bash -I /usr/include/bash/include -I /usr/include/bash/builtins -I /usr/lib/bash
CFLAGS=-c -fPIC
LDFLAGS=--shared -L . -linih
INIH_FLAGS=-DINI_CALL_HANDLER_ON_NEW_SECTION=1 -DINI_STOP_ON_FIRST_ERROR=1 -DINI_USE_STACK=0

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^

ini.so: ini.o libinih.a
	$(CC) -o $@ $^ $(LDFLAGS)

libinih.a:
	$(CC) $(CFLAGS) $(INIH_FLAGS) -o libinih.o inih/ini.c
	ar r libinih.a libinih.o

test: ini.so
	bash test.sh

clean:
	rm -f *.o *.a *.so
