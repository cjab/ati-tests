CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
PLATFORM ?= linux

ifeq ($(PLATFORM),baremetal)
	CFLAGS += -ffreestanding -fno-stack-protector -fno-pic -no-pie -m32 -DPLATFORM_BAREMETAL
	LDFLAGS = -nostdlib -T linker.ld -m32 -no-pie
	PLATFORM_SRC = platform/platform_baremetal.c platform/serial.c boot.S platform/tinyprintf.c
	TARGET = ati_tests.elf
	
	# Fixture handling for baremetal
	FIXTURE_BINS := $(wildcard fixtures/*.bin)
	FIXTURE_OBJS := $(FIXTURE_BINS:.bin=.o)
	FIXTURE_REGISTRY = fixtures/fixtures_registry.o
else
	LDFLAGS = -lpci
	PLATFORM_SRC = platform/platform_linux.c
	TARGET = run-tests
	FIXTURE_OBJS =
	FIXTURE_REGISTRY =
endif

COMMON_SRCS = main.c ati.c $(wildcard tests/*.c)
SRCS = $(COMMON_SRCS) $(PLATFORM_SRC)
OBJS = $(filter %.o,$(SRCS:.c=.o) $(SRCS:.S=.o)) $(FIXTURE_OBJS) $(FIXTURE_REGISTRY)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# Fixture build rules (baremetal only)
ifeq ($(PLATFORM),baremetal)
fixtures/fixtures_registry.c: $(FIXTURE_BINS) scripts/generate_fixture_registry.sh
	bash scripts/generate_fixture_registry.sh $(FIXTURE_BINS) > $@

fixtures/%.o: fixtures/%.bin
	objcopy -I binary -O elf32-i386 -B i386 \
	    --rename-section .data=.rodata.fixtures,alloc,load,readonly,data,contents \
	    --redefine-sym _binary_fixtures_$(subst -,_,$(notdir $*))_bin_start=fixture_$(subst -,_,$(notdir $*))_start \
	    --redefine-sym _binary_fixtures_$(subst -,_,$(notdir $*))_bin_end=fixture_$(subst -,_,$(notdir $*))_end \
	    $< $@
endif

clean:
	rm -f $(OBJS) $(TARGET) ati_tests.elf run-tests fixtures/*.o fixtures/fixtures_registry.c

compile_commands.json:
	bear -- $(MAKE) clean
	bear -- $(MAKE)

.PHONY: all clean
