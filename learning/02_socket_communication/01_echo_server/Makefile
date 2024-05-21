# Makefile
CC = gcc
CFLAGS = -lrt -lpthread -Wall -Wextra -O2

SRC1 = server.c
SRC2 = client.c
OBJ1 = $(SRC1:.c=.o)
OBJ2 = $(SRC2:.c=.o)
TARGET1 = server
TARGET2 = client

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET2): $(OBJ2)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ1) $(OBJ2) $(TARGET1) $(TARGET2)

.PHONY: all clean