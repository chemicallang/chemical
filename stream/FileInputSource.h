// Copyright (c) Qinetik 2024.

#pragma once

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "InputSource.h"

class FileInputSource : public InputSource {
private:
#ifdef _WIN32
    HANDLE fileHandle;
#else
    int fileDescriptor;
#endif
public:

    std::string filePath;

    explicit FileInputSource(const std::string &file_path);

    size_t read(char* buffer, size_t size) final;

    off_t seek(off_t offset, int whence) final;

    [[nodiscard]]
    off_t tell() const final;

    std::string get_file_path() final {
        return filePath;
    }

    ~FileInputSource() final;

};