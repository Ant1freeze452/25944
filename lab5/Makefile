CC = cc
TARGET = 5_task
DATA = 5_data.txt

all: $(TARGET)

$(TARGET): 5_task.c
	$(CC) 5_task.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(DATA)

clean:
	rm -f $(TARGET)
