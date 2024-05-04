CC=gcc
CFLAGS=-Wall

all: cli srv

cli: cli.c
	$(CC) $(CFLAGS) cli.c -o cli

srv: srv.c
	$(CC) $(CFLAGS) srv.c -o srv

clean:
	rm -f cli srv
