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
* ruby
* imagemagick

Then: `make`

# Running

To start the repl:

```
./bin/run-qemu <r128|r100>
```

This will search for a sibling qemu directory first (../qemu) and run the built
binary. If that doesn't exist then it will use the system install of qemu. You
can also set the binary with the `QEMU` env var.

Type ? at the serial console for help at boot.

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

Fixtures can be converted to png for viewing using the `bin/rle-to-png` tool.

# Test Coverage

* **clipping**: Scissor register latching and clipping behavior
* **pitch_offset_cntl**: Source/destination pitch and offset control registers
* **host_data**: HOST_DATA FIFO, monochrome expansion, bit packing
* **rop3**: ROP3 operations with color sources and memory blits
* **cce**: CCE engine setup and packet processing
