#
# Copyright (c) Qinetik 2025.
#

cmake -S . -B cmake-build-debugwsl -G "Ninja Multi-Config" -DCMAKE_BUILD_TYPE=Debug

cmake --build ../cmake-build-debugwsl --config Debug --target Compiler

cmake --build ../cmake-build-debugwsl --config Debug --target TCCCompiler

cmake --build ../cmake-build-debugwsl --config Debug --target ChemicalLsp || true