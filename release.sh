#!/bin/sh

# Scripts required to build the project are broken at the moment

#"D:/Software/JetBrains/CLion 2023.3.4/bin/cmake/win/x64/bin/cmake.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_CXX_COMPILER="cl.exe" -DCMAKE_BUILD_TYPE=Release -S D:/chemical -B D:/chemical/out/build/x64-release --fresh

#"D:/Software/JetBrains/CLion 2023.3.4/bin/cmake/win/x64/bin/cmake.exe" --build D:/chemical/out/build/x64-release

# Assembling Release from already built package

version=v1.0

zip_all_at_end=false
delete_dirs_at_end=false

# Loop through each command parameter
for param in "$@"; do
    if [ "$param" = "--zip" ]; then
        zip_all_at_end=true
    elif [ "$param" = "--del-dirs" ]; then
        delete_dirs_at_end=true
    fi
done

win64dirname=windows-x64-$version
lin64dirname=linux-x86-64-$version
windows64dir=out/release/$win64dirname
linux64dir=out/release/$lin64dirname

mkdir -p $windows64dir
mkdir -p $linux64dir

# make packages directories in each arch
mkdir -p $windows64dir/packages/tcc
mkdir -p $linux64dir/packages/tcc

# -------------------------- windows x64

# copy compiler
cp out/build/x64-release/Compiler.exe $windows64dir/Compiler.exe
# copy lsp
cp out/build/x64-release/ChemicalLsp.exe $windows64dir/ChemicalLsp.exe
# copy resources
cp -r ./lib/include $windows64dir/resources
# copy tiny cc dll
cp thirdparty/libtcc/win-x64/libtcc.dll $windows64dir/libtcc.dll
# unzip the tinycc package
unzip thirdparty/libtcc/win-x64/package.zip -d $windows64dir/packages/tcc

# -------------------------- linux x86-64

# copy compiler
cp out/build/x64-release-wsl/Compiler $linux64dir/Compiler
# copy lsp
cp out/build/x64-release-wsl/ChemicalLsp $linux64dir/ChemicalLsp
# copy resources
cp -r ./lib/include $linux64dir/resources
# copy tiny cc dll
cp thirdparty/libtcc/lin-x64/libtcc.so $linux64dir/libtcc.so
# unzip the tinycc package
unzip thirdparty/libtcc/lin-x64/package.zip -d $linux64dir/packages/tcc


# ------------------------- done

if [ "$zip_all_at_end" = true ]; then
    echo "Zipping all";
    cd out/release || return
    zip -r windows-x64.zip $win64dirname/*
    zip -r linux-x86-64.zip $lin64dirname/*
    cd ../../
fi

if [ "$delete_dirs_at_end" = true ]; then
    echo "Deleting Directories";
    rm -rf $windows64dir
    rm -rf $linux64dir
fi