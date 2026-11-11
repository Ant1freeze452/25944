CC = cc
TARGET = 7_task
DATA = 7_data.txt

all: $(TARGET)

$(TARGET): 7_task.c
	$(CC) 7_task.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(DATA)

clean:
	rm -f $(TARGET)
