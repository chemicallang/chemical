#!/bin/bash

set -e

# Determine the operating system using uname -s
os_type="$(uname -s)"
case "$os_type" in
  Linux*)
    echo "Detected Linux environment."
    IS_WINDOWS=false
    ;;
  MINGW*|MSYS*|CYGWIN*)
    echo "Detected Windows environment."
    IS_WINDOWS=true
    ;;
  *)
    echo "Error: Unknown operating system: $os_type" >&2
    exit 1
    ;;
esac

# Functions

zip_folder() {
    local folder_name=$1
    local output_name=$2
    if [ "$IS_WINDOWS" = true ]; then
        powershell.exe -Command "Compress-Archive -Path '${folder_name}' -DestinationPath '${output_name}'"
    else
        zip -r "$output_name" "$folder_name"/*
    fi
}

unzip_folder() {
    local archive_name=$1
    local destination_folder=$2
    if [ "$IS_WINDOWS" = true ]; then
        powershell.exe -Command "Expand-Archive -Path '${archive_name}' -DestinationPath '${destination_folder}' -Force"
    else
        unzip "$archive_name" -d "$destination_folder"
    fi
}

# Use IS_WINDOWS to set platform-specific variables
if [ "$IS_WINDOWS" = true ]; then
    windows_x64=true
    windows_x64_tcc=true
    windows_x64_lsp=true
    linux_x86_64=false
    linux_x86_64_tcc=false
    linux_x86_64_lsp=false
else
    windows_x64=false
    windows_x64_tcc=false
    windows_x64_lsp=false
    linux_x86_64=true
    linux_x86_64_tcc=true
    linux_x86_64_lsp=true
fi

# Debug output for the packaging target variables
echo "linux_x86_64: $linux_x86_64"
echo "linux_x86_64_tcc: $linux_x86_64_tcc"
echo "linux_x86_64_lsp: $linux_x86_64_tcc"
echo "windows_x64: $windows_x64"
echo "windows_x64_tcc: $windows_x64_tcc"
echo "windows_x64_lsp: $windows_x64_tcc"

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
win64dirname="windows-x64"
lin64dirname="linux-x86-64"
Win64TccDirName="$win64dirname-tcc"
Lin64TccDirName="$lin64dirname-tcc"
Win64LspDirName="$win64dirname-lsp"
Lin64LspDirName="$lin64dirname-lsp"

# Create directory paths
windows64dir="out/release/$win64dirname"
linux64dir="out/release/$lin64dirname"
Win64TccDir="out/release/$Win64TccDirName"
Lin64TccDir="out/release/$Lin64TccDirName"
Win64LspDir="out/release/$Win64LspDirName"
Lin64LspDir="out/release/$Lin64LspDirName"

# Make directories required for each target
if [ "$windows_x64" = true ]; then
    mkdir -p "$windows64dir/packages/tcc"
fi
if [ "$linux_x86_64" = true ]; then
    mkdir -p "$linux64dir/packages/tcc"
fi
if [ "$windows_x64_tcc" = true ]; then
    mkdir -p "$Win64TccDir/packages/tcc"
fi
if [ "$linux_x86_64_tcc" = true ]; then
    mkdir -p "$Lin64TccDir/packages/tcc"
fi
if [ "$windows_x64_lsp" = true ]; then
    mkdir -p "$Win64LspDir/packages/tcc"
fi
if [ "$linux_x86_64_tcc" = true ]; then
    mkdir -p "$Lin64LspDir/packages/tcc"
fi

# -------------------------- windows x64

if [ "$windows_x64" = true ]; then
  # copy compiler
  cp out/build/x64-release/Compiler.exe "$windows64dir/chemical.exe"
  # copy resources
  cp -r ./lib/include "$windows64dir/resources"
  # copy tiny cc dll
  cp lib/libtcc/win-x64/libtcc.dll "$windows64dir/libtcc.dll"
  # copy the libs directory
  cp -r lang/libs "$windows64dir/"
  # unzip the tinycc package
  unzip_folder "lib/libtcc/win-x64/package.zip" "$windows64dir/packages/tcc"
fi

# -------------------------- linux x86-64

if [ "$linux_x86_64" = true ]; then
  # copy compiler
  cp out/build/x64-release-wsl/Compiler "$linux64dir/chemical"
  # copy resources
  cp -r ./lib/include "$linux64dir/resources"
  # copy tiny cc dll
  cp lib/libtcc/lin-x64/libtcc.so "$linux64dir/libtcc.so"
  # copy the libs directory
  cp -r lang/libs "$linux64dir/"
  # unzip the tinycc package
  unzip_folder "lib/libtcc/lin-x64/package.zip" "$linux64dir/packages/tcc"
fi

# -------------------------- windows x64 tcc

if [ "$windows_x64_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/x64-release/TCCCompiler.exe "$Win64TccDir/chemical.exe"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$Win64TccDir/resources"
  # copy tiny cc dll
  cp lib/libtcc/win-x64/libtcc.dll "$Win64TccDir/libtcc.dll"
  # copy the libs directory
  cp -r lang/libs "$Win64TccDir/"
  # unzip the tinycc package
  unzip_folder "lib/libtcc/win-x64/package.zip" "$Win64TccDir/packages/tcc"
fi

# -------------------------- linux x86-64 tcc

if [ "$linux_x86_64_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/x64-release-wsl/TCCCompiler "$Lin64TccDir/chemical"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$Lin64TccDir/resources"
  # copy tiny cc dll
  cp lib/libtcc/lin-x64/libtcc.so "$Lin64TccDir/libtcc.so"
  # copy the libs directory
  cp -r lang/libs "$Lin64TccDir/"
  # unzip the tinycc package
  unzip_folder "lib/libtcc/lin-x64/package.zip" "$Lin64TccDir/packages/tcc"
fi

# -------------------------- windows x64 lsp

if [ "$windows_x64_lsp" = true ]; then
  # copy lsp
  cp out/build/x64-release/ChemicalLsp.exe "$Win64LspDir/lsp.exe"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$Win64LspDir/resources"
  # copy tiny cc dll
  cp lib/libtcc/win-x64/libtcc.dll "$Win64LspDir/libtcc.dll"
  # copy the libs directory
  cp -r lang/libs "$Win64LspDir/"
  # unzip the tinycc package
  unzip_folder "lib/libtcc/win-x64/package.zip" "$Win64LspDir/packages/tcc"
fi

# -------------------------- linux x86-64 lsp

if [ "$linux_x86_64_lsp" = true ]; then
  # copy lsp
  cp out/build/x64-release-wsl/ChemicalLsp "$Lin64LspDir/lsp"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$Lin64LspDir/resources"
  # copy tiny cc dll
  cp lib/libtcc/lin-x64/libtcc.so "$Lin64LspDir/libtcc.so"
  # copy the libs directory
  cp -r lang/libs "$Lin64LspDir/"
  # unzip the tinycc package
  unzip_folder "lib/libtcc/lin-x64/package.zip" "$Lin64LspDir/packages/tcc"
fi

# ------------------------- done

if [ "$zip_all_at_end" = true ]; then
    echo "Zipping all"
    cd "out/release" || exit 1
    if [ "$windows_x64" = true ]; then
      zip_folder "$win64dirname" "windows-x64.zip"
    fi
    if [ "$linux_x86_64" = true ]; then
      zip_folder "$lin64dirname" "linux-x86-64.zip"
    fi
    if [ "$windows_x64_tcc" = true ]; then
      zip_folder "$Win64TccDirName" "windows-x64-tcc.zip"
    fi
    if [ "$linux_x86_64_tcc" = true ]; then
      zip_folder "$Lin64TccDirName" "linux-x86-64-tcc.zip"
    fi
    if [ "$windows_x64_lsp" = true ]; then
      zip_folder "$Win64LspDirName" "windows-x64-lsp.zip"
    fi
    if [ "$linux_x86_64_lsp" = true ]; then
      zip_folder "$Lin64LspDirName" "linux-x86-64-lsp.zip"
    fi
    cd ../../
fi

if [ "$delete_dirs_at_end" = true ]; then
    echo "Deleting Directories"
    if [ "$windows_x64" = true ]; then
      rm -rf "$windows64dir"
    fi
    if [ "$linux_x86_64" = true ]; then
      rm -rf "$linux64dir"
    fi
    if [ "$windows_x64_tcc" = true ]; then
      rm -rf "$Win64TccDir"
    fi
    if [ "$linux_x86_64_tcc" = true ]; then
      rm -rf "$Lin64TccDir"
    fi
    if [ "$windows_x64_lsp" = true ]; then
      rm -rf "$Win64LspDir"
    fi
    if [ "$linux_x86_64_lsp" = true ]; then
      rm -rf "$Lin64LspDir"
    fi
fi