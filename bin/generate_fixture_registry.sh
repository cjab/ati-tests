#!/usr/bin/env bash
# Auto-generate fixture registry for baremetal builds
set -e

echo "/* Auto-generated - do not edit */"
echo "#include <stddef.h>"
echo "#include <stdint.h>"
echo ""

# Forward declarations
for binfile in "$@"; do
    name=$(basename "$binfile" .bin)
    echo "extern const uint8_t fixture_${name}_start[];"
    echo "extern const uint8_t fixture_${name}_end[];"
done

echo ""
echo "typedef struct {"
echo "    const char *name;"
echo "    const uint8_t *start;"
echo "    const uint8_t *end;"
echo "} fixture_entry_t;"
echo ""
echo "const fixture_entry_t fixture_registry[] = {"

for binfile in "$@"; do
    name=$(basename "$binfile" .bin)
    echo "    {\"$name\", fixture_${name}_start, fixture_${name}_end},"
done

echo "    {NULL, NULL, NULL}"
echo "};"
