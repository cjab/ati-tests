CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS = -lpci

SRCS = main.c ati.c tests/clipping.c tests/pitch_offset_cntl.c tests/host_data_buffering.c
OBJS = $(SRCS:.c=.o)
TARGET = run-tests

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
