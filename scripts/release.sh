#!/bin/bash

#
# Copyright (c) Chemical Language Foundation 2025.
#

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

# Windows Target
windows=false
windows_tcc=false
windows_lsp=false
# Linux Target
linux=false
linux_tcc=false
linux_lsp=false
# Alpine Target
alpine=false
alpine_tcc=false
# MacOS Target
macos=false
macos_tcc=false
macos_lsp=false

# Command line parameter variables
zip_all_at_end=true
delete_dirs_at_end=true

# Loop through each command parameter
for param in "$@"; do
    if [ "$param" = "--no-zip" ]; then
        zip_all_at_end=false
    elif [ "$param" = "--no-del-dirs" ]; then
        delete_dirs_at_end=false
    elif [ "$param" = "--windows" ]; then
          windows=true
          windows_tcc=true
          windows_lsp=true
    elif [ "$param" = "--linux" ]; then
          linux=true
          linux_tcc=true
          linux_lsp=true
    elif [ "$param" = "--alpine" ]; then
          alpine=true
          alpine_tcc=true
    elif [ "$param" = "--macos" ]; then
          macos=true
          macos_tcc=true
          macos_lsp=true
    fi
done

# Debug output for the packaging target variables
echo "windows: $windows"
echo "windows_tcc: $windows_tcc"
echo "windows_lsp: $windows_tcc"
echo "linux: $linux"
echo "linux_tcc: $linux_tcc"
echo "linux_lsp: $linux_tcc"
echo "alpine: $alpine"
echo "alpine_tcc: $alpine_tcc"
echo "macos: $macos"
echo "macos_tcc: $macos_tcc"
echo "macos_lsp: $macos_lsp"

# Create directory names
windows_dir_name="windows"
linux_dir_name="linux"
alpine_dir_name="linux-alpine"
macos_dir_name="macos"
# Tcc directory names
windows_tcc_dir_name="$windows_dir_name-tcc"
linux_tcc_dir_name="$linux_dir_name-tcc"
alpine_tcc_dir_name="$alpine_dir_name-tcc"
macos_tcc_dir_name="$macos_dir_name-tcc"
# LSP directory names
windows_lsp_dir_name="$windows_dir_name-lsp"
linux_lsp_dir_name="$linux_dir_name-lsp"
macos_lsp_dir_name="$macos_dir_name-lsp"

# Create directory paths
windows_dir="out/release/$windows_dir_name"
linux_dir="out/release/$linux_dir_name"
alpine_dir="out/release/$alpine_dir_name"
macos_dir="out/release/$macos_dir_name"
# Tcc directory paths
windows_tcc_dir="out/release/$windows_tcc_dir_name"
linux_tcc_dir="out/release/$linux_tcc_dir_name"
alpine_tcc_dir="out/release/$alpine_tcc_dir_name"
macos_tcc_dir="out/release/$macos_tcc_dir_name"
# LSP directory paths
windows_lsp_dir="out/release/$windows_lsp_dir_name"
linux_lsp_dir="out/release/$linux_lsp_dir_name"
macos_lsp_dir="out/release/$macos_lsp_dir_name"

# Make directories required for each target
if [ "$windows" = true ]; then
    mkdir -p "$windows_dir/packages/tcc"
fi
if [ "$linux" = true ]; then
    mkdir -p "$linux_dir/packages/tcc"
fi
if [ "$alpine" = true ]; then
    mkdir -p "$alpine_dir/packages/tcc"
fi
if [ "$macos" = true ]; then
    mkdir -p "$macos_dir/packages/tcc"
fi
if [ "$windows_tcc" = true ]; then
    mkdir -p "$windows_tcc_dir/packages/tcc"
fi
if [ "$linux_tcc" = true ]; then
    mkdir -p "$linux_tcc_dir/packages/tcc"
fi
if [ "$alpine_tcc" = true ]; then
    mkdir -p "$alpine_tcc_dir/packages/tcc"
fi
if [ "$macos_tcc" = true ]; then
    mkdir -p "$macos_tcc_dir/packages/tcc"
fi
if [ "$windows_lsp" = true ]; then
    mkdir -p "$windows_lsp_dir/packages/tcc"
fi
if [ "$linux_lsp" = true ]; then
    mkdir -p "$linux_lsp_dir/packages/lsp"
fi
if [ "$macos_lsp" = true ]; then
    mkdir -p "$macos_lsp_dir/packages/lsp"
fi

# -------------------------- windows

if [ "$windows" = true ]; then
  # copy compiler
  cp out/build/Compiler.exe "$windows_dir/chemical.exe"
  # copy resources
  cp -r ./lib/include "$windows_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$windows_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.dll "$windows_dir/libtcc.dll"
  cp -r lib/tcc/include "$windows_dir/packages/tcc"
  cp -r lib/tcc/lib "$windows_dir/packages/tcc"
fi

# -------------------------- linux

if [ "$linux" = true ]; then
  # copy compiler
  cp out/build/Compiler "$linux_dir/chemical"
  # copy resources
  cp -r ./lib/include "$linux_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$linux_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$linux_dir/libtcc.so"
  cp -r lib/tcc/include "$linux_dir/packages/tcc"
  cp -r lib/tcc/lib "$linux_dir/packages/tcc"
fi

# -------------------------- linux alpine

if [ "$alpine" = true ]; then
  # copy compiler
  cp out/build/Compiler "$alpine_dir/chemical"
  # copy resources
  cp -r ./lib/include "$alpine_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$alpine_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$alpine_dir/libtcc.so"
  cp -r lib/tcc/include "$alpine_dir/packages/tcc"
  cp -r lib/tcc/lib "$alpine_dir/packages/tcc"
fi

# -------------------------- macos

if [ "$macos" = true ]; then
  # copy compiler
  cp out/build/Compiler "$macos_dir/chemical"
  # copy resources
  cp -r ./lib/include "$macos_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$macos_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$macos_dir/libtcc.so"
  cp -r lib/tcc/include "$macos_dir/packages/tcc"
  cp -r lib/tcc/lib "$macos_dir/packages/tcc"
fi

# -------------------------- windows tcc

if [ "$windows_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/TCCCompiler.exe "$windows_tcc_dir/chemical.exe"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$windows_tcc_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$windows_tcc_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.dll "$windows_tcc_dir/libtcc.dll"
  cp -r lib/tcc/include "$windows_tcc_dir/packages/tcc"
  cp -r lib/tcc/lib "$windows_tcc_dir/packages/tcc"
fi

# -------------------------- linux tcc

if [ "$linux_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/TCCCompiler "$linux_tcc_dir/chemical"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$linux_tcc_dir/resources"
  # copy tiny cc dll
  # copy the libs directory
  cp -r lang/libs "$linux_tcc_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$linux_tcc_dir/libtcc.so"
  cp -r lib/tcc/include "$linux_tcc_dir/packages/tcc"
  cp -r lib/tcc/lib "$linux_tcc_dir/packages/tcc"
fi

# -------------------------- linux alpine tcc

if [ "$alpine_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/TCCCompiler "$alpine_tcc_dir/chemical"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$linux_tcc_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$alpine_tcc_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$alpine_tcc_dir/libtcc.so"
  cp -r lib/tcc/include "$alpine_tcc_dir/packages/tcc"
  cp -r lib/tcc/lib "$alpine_tcc_dir/packages/tcc"
fi

# -------------------------- macos tcc

if [ "$macos_tcc" = true ]; then
  # copy tcc compiler
  cp out/build/TCCCompiler "$macos_tcc_dir/chemical"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$macos_tcc_dir/resources"
  # copy tiny cc dll
  # copy the libs directory
  cp -r lang/libs "$macos_tcc_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$macos_tcc_dir/libtcc.so"
  cp -r lib/tcc/include "$macos_tcc_dir/packages/tcc"
  cp -r lib/tcc/lib "$macos_tcc_dir/packages/tcc"
fi

# -------------------------- windows lsp

if [ "$windows_lsp" = true ]; then
  # copy lsp
  cp out/build/ChemicalLsp.exe "$windows_lsp_dir/lsp.exe"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$windows_lsp_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$windows_lsp_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.dll "$windows_lsp_dir/libtcc.dll"
  cp -r lib/tcc/include "$windows_lsp_dir/packages/tcc"
  cp -r lib/tcc/lib "$windows_lsp_dir/packages/tcc"
fi

# -------------------------- linux lsp

if [ "$linux_lsp" = true ]; then
  # copy lsp
  cp out/build/ChemicalLsp "$linux_lsp_dir/lsp"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$linux_lsp_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$linux_lsp_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$linux_lsp_dir/libtcc.so"
  cp -r lib/tcc/include "$linux_lsp_dir/packages/tcc"
  cp -r lib/tcc/lib "$linux_lsp_dir/packages/tcc"
fi

# -------------------------- macos lsp

if [ "$macos_lsp" = true ]; then
  # copy lsp
  cp out/build/ChemicalLsp "$macos_lsp_dir/lsp"
  # copy resources (tcc build doesn't need resources)
  # cp -r ./lib/include "$macos_lsp_dir/resources"
  # copy the libs directory
  cp -r lang/libs "$macos_lsp_dir/"
  # copy the tinycc package
  cp lib/tcc/libtcc.so "$macos_lsp_dir/libtcc.so"
  cp -r lib/tcc/include "$macos_lsp_dir/packages/tcc"
  cp -r lib/tcc/lib "$macos_lsp_dir/packages/tcc"
fi

# ------------------------- done

if [ "$zip_all_at_end" = true ]; then
    echo "Zipping all"
    cd "out/release" || exit 1
    if [ "$windows" = true ]; then
      zip_folder "$windows_dir_name" "windows.zip"
    fi
    if [ "$linux" = true ]; then
      zip_folder "$linux_dir_name" "linux.zip"
    fi
    if [ "$alpine" = true ]; then
      zip_folder "$alpine_dir_name" "linux-alpine.zip"
    fi
    if [ "$macos" = true ]; then
      zip_folder "$macos_dir_name" "macos.zip"
    fi
    if [ "$windows_tcc" = true ]; then
      zip_folder "$windows_tcc_dir_name" "windows-tcc.zip"
    fi
    if [ "$linux_tcc" = true ]; then
      zip_folder "$linux_tcc_dir_name" "linux-tcc.zip"
    fi
    if [ "$alpine_tcc" = true ]; then
      zip_folder "$alpine_tcc_dir_name" "linux-alpine-tcc.zip"
    fi
    if [ "$macos_tcc" = true ]; then
      zip_folder "$macos_tcc_dir_name" "macos-tcc.zip"
    fi
    if [ "$windows_lsp" = true ]; then
      zip_folder "$windows_lsp_dir_name" "windows-lsp.zip"
    fi
    if [ "$linux_lsp" = true ]; then
      zip_folder "$linux_lsp_dir_name" "linux-lsp.zip"
    fi
    if [ "$macos_lsp" = true ]; then
      zip_folder "$macos_lsp_dir_name" "macos-lsp.zip"
    fi
    cd ../../
fi

if [ "$delete_dirs_at_end" = true ]; then
    echo "Deleting Directories"
    if [ "$windows" = true ]; then
      rm -rf "$windows_dir"
    fi
    if [ "$linux" = true ]; then
      rm -rf "$linux_dir"
    fi
    if [ "$alpine" = true ]; then
      rm -rf "$alpine_dir"
    fi
    if [ "$macos" = true ]; then
      rm -rf "$macos_dir"
    fi
    if [ "$windows_tcc" = true ]; then
      rm -rf "$windows_tcc_dir"
    fi
    if [ "$linux_tcc" = true ]; then
      rm -rf "$linux_tcc_dir"
    fi
    if [ "$alpine_tcc" = true ]; then
      rm -rf "$alpine_tcc_dir"
    fi
    if [ "$macos_tcc" = true ]; then
      rm -rf "$macos_tcc_dir"
    fi
    if [ "$windows_lsp" = true ]; then
      rm -rf "$windows_lsp_dir"
    fi
    if [ "$linux_lsp" = true ]; then
      rm -rf "$linux_lsp_dir"
    fi
    if [ "$macos_lsp" = true ]; then
      rm -rf "$macos_lsp_dir"
    fi
fi