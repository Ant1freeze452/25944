CC = cc
TARGET = 6_task
DATA = 6_data.txt

all: $(TARGET)

$(TARGET): 6_task.c
	$(CC) 6_task.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(DATA)

clean:
	rm -f $(TARGET)
