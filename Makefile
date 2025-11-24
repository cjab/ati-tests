CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
PLATFORM ?= linux

ifeq ($(PLATFORM),baremetal)
	PLATFORM_SRC = platform/platform_baremetal.c
	TARGET = ati_tests.elf
else
	LDFLAGS = -lpci
	PLATFORM_SRC = platform/platform_linux.c
	TARGET = run-tests
endif

COMMON_SRCS = main.c ati.c $(wildcard tests/*.c)
SRCS = $(COMMON_SRCS) $(PLATFORM_SRC)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) ati_tests.elf run-tests

compile_commands.json:
	bear -- $(MAKE) clean
	bear -- $(MAKE)

.PHONY: all clean
