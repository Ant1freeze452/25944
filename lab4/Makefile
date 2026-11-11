CC = cc
TARGET = 4_task

all: $(TARGET)

$(TARGET): 4_task.c
	$(CC) 4_task.c -o $(TARGET) -lreadline

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

