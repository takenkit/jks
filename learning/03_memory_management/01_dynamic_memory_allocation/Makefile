# Makefile
CC = gcc
CFLAGS = -lrt -lpthread -Wall -Wextra -O2

SRC = main.c
OBJ = $(SRC:.c=.o)
TARGET = dynamic_memory_allocation 

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean