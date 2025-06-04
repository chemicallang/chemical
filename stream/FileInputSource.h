// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "InputSource.h"

typedef void *HANDLE;

class FileInputSource : public InputSource {
private:
#ifdef _WIN32
    HANDLE fileHandle;
#else
    int fileDescriptor;
#endif
public:

    explicit FileInputSource(const char* file_path);

    FileInputSource(const FileInputSource& other) = delete;

    FileInputSource(
        FileInputSource&& other
    ) noexcept;

    InputSourceErrorKind open(const char* file_path);

    size_t read(char* buffer, size_t size) final;

    off_t seek(off_t offset, int whence) final;

    [[nodiscard]]
    off_t tell() const final;

    ~FileInputSource() final;

};