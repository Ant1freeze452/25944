CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

all: lab4

lab4: lab4.c
	@if [ "$$($(CC) -print-file-name=libreadline.so)" = "libreadline.so" ]; then \
		$(CC) $(CFLAGS) lab4.c -o lab4 -l:libreadline.so.8; \
	else \
		$(CC) $(CFLAGS) lab4.c -o lab4 -lreadline; \
	fi

clean:
	rm -f lab4

.PHONY: all clean
