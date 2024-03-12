#!/bin/bash

# Move into the cmake-build-debug directory
cd build || exit

# Run make command
make

# Execute chemical-llvm
./chemical-llvm