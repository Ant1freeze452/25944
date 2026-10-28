CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

TARGET = z4

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(TARGET).c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	$(RM) $(TARGET) $(TARGET).exe *.o
