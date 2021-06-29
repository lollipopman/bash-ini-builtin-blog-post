CC=gcc
INC=-I /usr/include/bash -I /usr/include/bash/include -I /usr/include/bash/builtins -I /usr/lib/bash
CFLAGS=-c -fPIC
LDFLAGS=--shared -linih

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^

ini.so: ini.o
	$(CC) -o $@ $^ $(LDFLAGS)

test: ini.so
	bash test.sh

clean:
	rm -f ini.o ini.so
