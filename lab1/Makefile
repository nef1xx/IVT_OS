CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic
TARGET  = myshell
OBJS    = myshell.o builtins.o


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


myshell.o: myshell.c builtins.h
	$(CC) $(CFLAGS) -c myshell.c


builtins.o: builtins.c builtins.h
	$(CC) $(CFLAGS) -c builtins.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
