CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g

all: task3 data

task3: task3.c
	$(CC) $(CFLAGS) -o task3 task3.c

# файл данных: читать и писать может только владелец
data: FORCE
	test -f data || echo secret > data
	chmod 600 data

clean:
	rm -f task3
	rm -rf task3.dSYM

FORCE:

.PHONY: all clean FORCE
