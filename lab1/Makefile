CC = cc
TARGET = 1_task

all: $(TARGET)

$(TARGET): main.c
	$(CC) main.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET)
