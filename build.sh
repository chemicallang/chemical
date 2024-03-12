#!/bin/bash

ROOTDIR="../"

# create directory for the cmake build
directory="build"
if [ ! -d "$directory" ]; then
    mkdir "$directory"
else
    echo "Couldn't create build directory"
fi

# Move into the cmake-build-debug directory
cd build || exit

# Run cmake command
#cmake \
#    -DCMAKE_INSTALL_PREFIX="$ROOTDIR/out/host" \
#    -DCMAKE_PREFIX_PATH="$ROOTDIR/out/host" \
#    -DCMAKE_BUILD_TYPE=Release \
#    . ../
# Run cmake command with specified variables
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    . ../

# Run make command
make

# Execute chemical-llvm
./chemical-llvm