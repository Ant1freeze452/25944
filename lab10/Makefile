CC = cc
TARGET = 10_task
ARGS = ls -l

all: $(TARGET)

$(TARGET): 10_task.c
	$(CC) 10_task.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET)
