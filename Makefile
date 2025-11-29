CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
PLATFORM ?= linux

# Auto-clean when platform changes
-include .platform
ifneq ($(LAST_PLATFORM),$(PLATFORM))
$(shell rm -f *.o tests/*.o platform/*/*.o fixtures/*.o .platform)
endif
$(shell echo "LAST_PLATFORM=$(PLATFORM)" > .platform)

ifeq ($(PLATFORM),baremetal)
	CFLAGS += -ffreestanding -fno-stack-protector -fno-pic -no-pie -m32 -DPLATFORM_BAREMETAL
	LDFLAGS = -nostdlib -T linker.ld -m32 -no-pie
	PLATFORM_SRC = platform/baremetal/baremetal.c platform/baremetal/serial.c boot.S platform/baremetal/tinyprintf.c
	TARGET = ati_tests.elf
	ISO = ati_tests.iso
	
	# Fixture handling for baremetal
	FIXTURE_BINS := $(wildcard fixtures/*.bin)
	FIXTURE_OBJS := $(FIXTURE_BINS:.bin=.o)
	FIXTURE_REGISTRY = fixtures/fixtures_registry.o
else
	LDFLAGS = -lpci
	PLATFORM_SRC = platform/linux/linux.c
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

# Bootable ISO target (baremetal only, requires grub-mkrescue, xorriso)
ifeq ($(PLATFORM),baremetal)
iso: $(TARGET)
	mkdir -p iso/boot/grub
	cp $(TARGET) iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "ATI Tests" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/$(TARGET)' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	mkdir -p /tmp/grub-iso
	TMPDIR=/tmp/grub-iso grub-mkrescue -o $(ISO) iso
	rm -rf iso
	@echo "Created $(ISO) - write to USB with: sudo dd if=$(ISO) of=/dev/sdX bs=4M status=progress"
endif

# Fixture build rules (baremetal only)
ifeq ($(PLATFORM),baremetal)
fixtures/fixtures_registry.c: $(FIXTURE_BINS) scripts/generate_fixture_registry.sh
	bash scripts/generate_fixture_registry.sh $(FIXTURE_BINS) > $@

fixtures/%.o: fixtures/%.bin
	objcopy -I binary -O elf32-i386 -B i386 \
	    --rename-section .data=.rodata.fixtures,alloc,load,readonly,data,contents \
	    --redefine-sym _binary_fixtures_$(subst -,_,$(notdir $*))_bin_start=fixture_$(subst -,_,$(notdir $*))_start \
	    --redefine-sym _binary_fixtures_$(subst -,_,$(notdir $*))_bin_end=fixture_$(subst -,_,$(notdir $*))_end \
	    --add-section .note.GNU-stack=/dev/null \
	    --set-section-flags .note.GNU-stack=contents,readonly \
	    $< $@
endif

clean:
	rm -f $(OBJS) $(TARGET) ati_tests.elf run-tests boot.o platform/*/*.o fixtures/*.o fixtures/fixtures_registry.c ati_tests.iso .platform
	rm -rf iso

compile_commands.json:
	bear -- $(MAKE) clean
	bear -- $(MAKE)

.PHONY: all clean iso
