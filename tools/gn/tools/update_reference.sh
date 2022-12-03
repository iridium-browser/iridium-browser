#!/bin/sh

# Check for the existance of the AUTHORS file as an easy way to determine if
# it's being run from the correct directory.
if test -f "AUTHORS"; then
    echo Building gn...
    ninja -C out gn
    echo Generating new docs/reference.md...
    out/gn help --markdown all > docs/reference.md
else
    echo Please run this command from the GN checkout root directory.
fi
