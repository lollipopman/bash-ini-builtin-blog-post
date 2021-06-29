CC=gcc
INC=-I /usr/include/bash -I /usr/include/bash/include -I /usr/include/bash/builtins -I /usr/lib/bash
CFLAGS=-c -fPIC
LDFLAGS=--shared -L . -linih

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^

ini.so: ini.o libinih.a
	$(CC) -o $@ $^ $(LDFLAGS)

libinih.a:
	$(CC) $(CFLAGS) -o libinih.o inih/ini.c
	ar r libinih.a libinih.o

test: ini.so
	bash test.sh

clean:
	rm -f ini.o ini.so
