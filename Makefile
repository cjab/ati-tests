CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O1 -MMD -MP
PLATFORM ?= baremetal
BUILD_DIR = build

# Auto-clean when platform changes
-include $(BUILD_DIR)/.platform
ifneq ($(LAST_PLATFORM),$(PLATFORM))
$(shell rm -rf $(BUILD_DIR))
endif

ifeq ($(PLATFORM),baremetal)
	CFLAGS += -ffreestanding -fno-stack-protector -fno-pic -no-pie -m32 -DPLATFORM_BAREMETAL
	LDFLAGS = -nostdlib -T platform/baremetal/linker.ld -m32 -no-pie
	PLATFORM_SRC = platform/baremetal/baremetal.c platform/baremetal/serial.c platform/baremetal/boot.S platform/baremetal/tinyprintf.c
	TARGET = ati_tests.elf
	ISO = ati_tests.iso
	
	# Fixture handling for baremetal (RLE compressed)
	FIXTURE_RLES := $(wildcard fixtures/*.rle)
	FIXTURE_OBJS := $(patsubst fixtures/%.rle,$(BUILD_DIR)/fixtures/%.o,$(FIXTURE_RLES))
	FIXTURE_REGISTRY = $(BUILD_DIR)/fixtures/fixtures_registry.o
else
	LDFLAGS = -lpci
	PLATFORM_SRC = platform/linux/linux.c
	TARGET = run-tests
	FIXTURE_OBJS =
	FIXTURE_REGISTRY =
endif

# Test source files from all test directories
TEST_SRCS = $(wildcard tests/common/*.c) $(wildcard tests/r128/*.c) $(wildcard tests/r100/*.c)

COMMON_SRCS = main.c ati/ati.c ati/r128.c ati/r100.c ati/cce.c repl/repl.c repl/cce_cmd.c repl/dump_cmd.c $(TEST_SRCS)
SRCS = $(COMMON_SRCS) $(PLATFORM_SRC)

# Transform source paths to build paths
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRCS))) \
       $(patsubst %.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SRCS))) \
       $(FIXTURE_OBJS) $(FIXTURE_REGISTRY)

# Generated register definitions
REGS_DIR = ati/registers
REGS_GEN = bin/generate_registers
COMMON_REGS_HDR = $(REGS_DIR)/common_regs_gen.h
R128_REGS_HDR = $(REGS_DIR)/r128_regs_gen.h
R100_REGS_HDR = $(REGS_DIR)/r100_regs_gen.h
ALL_REGS_HDRS = $(COMMON_REGS_HDR) $(R128_REGS_HDR) $(R100_REGS_HDR)

all: $(ALL_REGS_HDRS) $(TARGET)

# Regenerate register headers from YAML
$(COMMON_REGS_HDR): $(REGS_DIR)/common.yaml $(REGS_GEN)
	$(REGS_GEN) --chip common -o $@

$(R128_REGS_HDR): $(REGS_DIR)/r128.yaml $(REGS_GEN)
	$(REGS_GEN) --chip r128 --prefix R128_ -o $@

$(R100_REGS_HDR): $(REGS_DIR)/r100.yaml $(REGS_GEN)
	$(REGS_GEN) --chip r100 --prefix R100_ -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Pattern rule for C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for assembly files
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Include auto-generated dependency files (if they exist)
-include $(OBJS:.o=.d)

# Store platform marker in build dir
$(shell mkdir -p $(BUILD_DIR) && echo "LAST_PLATFORM=$(PLATFORM)" > $(BUILD_DIR)/.platform)

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

# Fixture build rules (baremetal only, RLE compressed)
$(BUILD_DIR)/fixtures/fixtures_registry.c: $(FIXTURE_RLES) bin/generate_fixture_registry
	@mkdir -p $(dir $@)
	bin/generate_fixture_registry $(FIXTURE_RLES) > $@

$(BUILD_DIR)/fixtures/fixtures_registry.o: $(BUILD_DIR)/fixtures/fixtures_registry.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fixtures/%.o: fixtures/%.rle
	@mkdir -p $(dir $@)
	objcopy -I binary -O elf32-i386 -B i386 \
	    --rename-section .data=.rodata.fixtures,alloc,load,readonly,data,contents \
	    --redefine-sym _binary_fixtures_$(subst -,_,$(notdir $*))_rle_start=fixture_$(subst -,_,$(notdir $*))_start \
	    --redefine-sym _binary_fixtures_$(subst -,_,$(notdir $*))_rle_end=fixture_$(subst -,_,$(notdir $*))_end \
	    --add-section .note.GNU-stack=/dev/null \
	    --set-section-flags .note.GNU-stack=contents,readonly \
	    $< $@
endif

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET) ati_tests.elf run-tests ati_tests.iso
	rm -f $(ALL_REGS_HDRS)

# Regenerate all register headers
regen:
	$(REGS_GEN) --chip common -o $(COMMON_REGS_HDR)
	$(REGS_GEN) --chip r128 --prefix R128_ -o $(R128_REGS_HDR)
	$(REGS_GEN) --chip r100 --prefix R100_ -o $(R100_REGS_HDR)

compile_commands.json:
	bear -- $(MAKE) clean
	bear -- $(MAKE)

.PHONY: all clean regen iso
