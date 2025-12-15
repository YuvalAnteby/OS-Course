# Yuval Anteby 212152896

CC = gcc
CFLAGS = -Wall -pthread
TARGET = ex3.out

all: $(TARGET)

$(TARGET): main.o bounded_buffer.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o bounded_buffer.o

main.o: main.c bounded_buffer.h
	$(CC) $(CFLAGS) -c main.c

boundedbuffer.o: bounded_buffer.c bounded_buffer.h
	$(CC) $(CFLAGS) -c bounded_buffer.c

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean
