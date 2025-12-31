# ATI Tests

Hardware tests for the ATI Rage128 and R100. Used for validation of the ati-vga
QEMU display device. These tests are intended to be run on real ATI hardware
and then also run on QEMU for validation.

# Building

If running Nix or NixOS the flake.nix includes a dev shell with all required
dependencies. If not, the following are required:

* gcc
* make
* pciutils

Then: `make`

# Setup

## Baremetal
By default this compiles a baremetal elf file that can be loaded via QEMU's
`-kernel` argument. This mode requires much less setup and is the preferred
way to run tests at this point. See "Running".

## Linux
There are a lot of assumptions made about the state of the system when running
these tests. I've run them in QEMU and on real hardware running Debian Lenny and
Debian Squeeze. Before running the tests it's required to start a minimal X
environment: `Xorg :0`. The xserver-xorg-video-r128 driver must also be compiled
with DRI disabled to force use of MMIO. The version distributed with Debian
is compiled with DRI which will always use CCE (not yet supported in QEMU ati-vga).

The Xorg display should be configured at 640x480 @ 32-bit color (BGRA).

The setup is a bit involved, hopefully I can provide a QCOW2 image in the future.

# Running

## Baremetal

To start the baremetal repl:
```
qemu-system-x86_64 \
  -accel kvm \
  -vga none \
  -device ati-vga,model=rage128p \
  -machine pc \
  -cpu athlon,3dnow=off,3dnowext=off \
  -m 1024 \
  -d unimp,guest_errors \
  -D /tmp/qemu-debug.log \
  -trace "ati_*" \
  -kernel "ati_tests.elf" \
  -append "repl" \
  -serial mon:stdio
```

Type ? at the serial console for help at boot.

## Linux
To run all tests: `./run-tests`.

Individual tests can be run with: `./run-tests <TEST_NAME1> <TEST_NAME2>`.

The tests will require root privileges to access hardware.

# Development

Adding tests to existing files in **/tests** is easy:
1) Add a new function prefixed with test_*.
1) Add a new `register_test()` call to the test registration function.

Adding a new tests file is only a little bit more involved:
1) Create a new file in **/tests**.
1) Add a test and a registration function.
1) Call the registration function from `register_all_tests()` in main.c.

# Fixtures

The fixtures directory contains dumps of VRAM for comparison in subsequent
test runs. If a fixture doesn't exist the test will ask you if you would like
to create it. Obviously this should probably only be done on _real_ hardware
for comparison against later runs on the emulated card.

# Test Coverage

* **clipping**: Scissor register latching and clipping behavior
* **pitch_offset_cntl**: Source/destination pitch and offset control registers
* **host_data**: HOST_DATA FIFO, monochrome expansion, bit packing
* **rop3**: ROP3 operations with color sources and memory blits
* **cce**: CCE engine setup and packet processing
