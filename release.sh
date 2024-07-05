#!/bin/sh

# Scripts required to build the project are broken at the moment

#"D:/Software/JetBrains/CLion 2023.3.4/bin/cmake/win/x64/bin/cmake.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_CXX_COMPILER="cl.exe" -DCMAKE_BUILD_TYPE=Release -S D:/chemical -B D:/chemical/out/build/x64-release --fresh

#"D:/Software/JetBrains/CLion 2023.3.4/bin/cmake/win/x64/bin/cmake.exe" --build D:/chemical/out/build/x64-release

# Assembling Release from already built package

version=v1.0.1

# Targets to build
linux_x86_64=true
windows_x64=true
linux_x86_64_tcc=true
windows_x64_tcc=true

# Command line parameter variables
zip_all_at_end=true
delete_dirs_at_end=true

# Loop through each command parameter
for param in "$@"; do
    if [ "$param" = "--no-zip" ]; then
        zip_all_at_end=false
    elif [ "$param" = "--no-del-dirs" ]; then
        delete_dirs_at_end=false
    fi
done

# Create directory names
win64dirname=windows-x64-$version
lin64dirname=linux-x86-64-$version
Win64TccDirName=windows-x64-tcc-$version
Lin64TccDirName=linux-x86-64-tcc-$version

# Create directory paths
windows64dir=out/release/$win64dirname
linux64dir=out/release/$lin64dirname
Win64TccDir=out/release/$Win64TccDirName
Lin64TccDir=out/release/$Lin64TccDirName

# Make directories required for each target
if [ "$windows_x64" = true ]; then
mkdir -p $windows64dir/packages/tcc
fi
if [ "$linux_x86_64" = true ]; then
mkdir -p $linux64dir/packages/tcc
fi
if [ "$windows_x64_tcc" = true ]; then
mkdir -p $Win64TccDir/packages/tcc
fi
if [ "$linux_x86_64_tcc" = true ]; then
mkdir -p $Lin64TccDir/packages/tcc
fi

# -------------------------- windows x64

if [ "$windows_x64" = true ]; then
  # copy compiler
  cp out/build/x64-release/Compiler.exe $windows64dir/chemical.exe
  # copy lsp
  cp out/build/x64-release/ChemicalLsp.exe $windows64dir/ChemicalLsp.exe
  # copy resources
  cp -r ./lib/include $windows64dir/resources
  # copy tiny cc dll
  cp lib/libtcc/win-x64/libtcc.dll $windows64dir/libtcc.dll
  # unzip the tinycc package
  unzip lib/libtcc/win-x64/package.zip -d $windows64dir/packages/tcc
fi

# -------------------------- linux x86-64

if [ "$linux_x86_64" = true ]; then
  # copy compiler
  cp out/build/x64-release-wsl/Compiler $linux64dir/chemical
  # copy lsp
  cp out/build/x64-release-wsl/ChemicalLsp $linux64dir/ChemicalLsp
  # copy resources
  cp -r ./lib/include $linux64dir/resources
  # copy tiny cc dll
  cp lib/libtcc/lin-x64/libtcc.so $linux64dir/libtcc.so
  # unzip the tinycc package
  unzip lib/libtcc/lin-x64/package.zip -d $linux64dir/packages/tcc
fi

# -------------------------- windows x64 tcc

if [ "$windows_x64_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/x64-release/TCCCompiler.exe $Win64TccDir/chemical.exe
  # copy lsp
  cp out/build/x64-release/ChemicalLsp.exe $Win64TccDir/ChemicalLsp.exe
  # copy resources
  # cp -r ./lib/include $Win64TccDir/resources
  # copy tiny cc dll
  cp lib/libtcc/win-x64/libtcc.dll $Win64TccDir/libtcc.dll
  # unzip the tinycc package
  unzip lib/libtcc/win-x64/package.zip -d $Win64TccDir/packages/tcc
fi

# -------------------------- linux x86-64 tcc

if [ "$linux_x86_64_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/x64-release-wsl/TCCCompiler $Lin64TccDir/chemical
  # copy lsp
  cp out/build/x64-release-wsl/ChemicalLsp $Lin64TccDir/ChemicalLsp
  # copy resources
  # cp -r ./lib/include $Lin64TccDir/resources
  # copy tiny cc dll
  cp lib/libtcc/lin-x64/libtcc.so $Lin64TccDir/libtcc.so
  # unzip the tinycc package
  unzip lib/libtcc/lin-x64/package.zip -d $Lin64TccDir/packages/tcc
fi

# ------------------------- done

if [ "$zip_all_at_end" = true ]; then
    echo "Zipping all";
    cd out/release || return
    if [ "$windows_x64" = true ]; then
      zip -r windows-x64.zip $win64dirname/*
    fi
    if [ "$linux_x86_64" = true ]; then
      zip -r linux-x86-64.zip $lin64dirname/*
    fi
    if [ "$windows_x64_tcc" = true ]; then
     zip -r windows-x64-tcc.zip $Win64TccDirName/*
    fi
    if [ "$linux_x86_64_tcc" = true ]; then
      zip -r linux-x86-64-tcc.zip $Lin64TccDirName/*
    fi
    cd ../../
fi

if [ "$delete_dirs_at_end" = true ]; then
    echo "Deleting Directories";
    if [ "$windows_x64" = true ]; then
      rm -rf $windows64dir
    fi
    if [ "$linux_x86_64" = true ]; then
      rm -rf $linux64dir
    fi
    if [ "$windows_x64_tcc" = true ]; then
      rm -rf $Win64TccDir
    fi
    if [ "$linux_x86_64_tcc" = true ]; then
      rm -rf $Lin64TccDir
    fi
fi