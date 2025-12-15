# Yuval Anteby 212152896

CC = gcc
CFLAGS = -Wall -pthread
TARGET = ex3.out

all: $(TARGET)

$(TARGET): main.o BoundedBuffer.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o BoundedBuffer.o

main.o: main.c BoundedBuffer.h
	$(CC) $(CFLAGS) -c main.c

BoundedBuffer.o: BoundedBuffer.c BoundedBuffer.h
	$(CC) $(CFLAGS) -c BoundedBuffer.c

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean
