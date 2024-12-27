/*
 * Hasan Buğra Uslu G211210009  2C
 * Tarık Kalyoncu  G211210030 2C
 */

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = shell
SOURCES = main.c shell.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)


