CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS = -lpci

SRCS = main.c ati.c $(wildcard tests/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = run-tests

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

compile_commands.json:
	bear -- $(MAKE) clean
	bear -- $(MAKE)

.PHONY: all clean
