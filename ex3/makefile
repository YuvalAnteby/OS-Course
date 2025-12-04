CC = g++
CFLAGS = -std=c++11 -pthread -Wall -g
TARGET = ex3.out
SRCS = main.cpp BoundedBuffer.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# This rule says: to turn a .cpp into a .o, use this command
.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o