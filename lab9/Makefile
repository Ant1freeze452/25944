CC = cc
TARGET = 9_task
DATA = 9_data.txt

all: $(TARGET)

$(TARGET): 9_task.c
	$(CC) 9_task.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(DATA)

clean:
	rm -f $(TARGET)
