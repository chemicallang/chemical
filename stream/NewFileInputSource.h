// Copyright (c) Chemical Language Foundation 2025.

// mmap_file.hpp
#pragma once

#include "NewInputSource.h"
#include <cstdint>
#include "std/chem_string.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <basetsd.h>
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
typedef void *HANDLE;
typedef const wchar_t* PATH_STR;
#else
typedef const char* PATH_STR;
#endif

enum class FileErrKind {

};

struct FileErr {
    int code;               // errno or GetLastError()
    std::string category;   // "system", "generic", etc.
    std::string message;    // human-readable message
    std::string operation;  // e.g., "CreateFileW", "open", etc.
    std::string path;       // file path for context

    // Convert to human-readable string
    [[nodiscard]]
    std::string format() const {
        std::string oss;
        oss.append("Error during ");
        oss.append(operation); oss.append(" on '");
        oss.append(path); oss.append("': "); oss.append(message);
        oss.append(" (code "); oss.append(std::to_string(code)); oss.append(", category: "); oss.append(category); oss.append(")");
        return oss;
    }
};

class FileErrHeapPtr {
public:
    FileErr* ptr;
    inline constexpr FileErrHeapPtr(FileErr* ptr) : ptr(ptr) {

    }
    inline FileErr* release() {
        FileErr* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }
    inline FileErr* operator->() const noexcept {
        return ptr;
    }
    inline FileErr& operator*() const noexcept {
        return *ptr;
    }
    ~FileErrHeapPtr() {
        delete ptr;
    }
};

class MemoryMappedFile : public NewInputSource {
private:

    // the error is allocated on the heap
    FileErrHeapPtr err = FileErrHeapPtr(nullptr);

#if defined(_WIN32)
    HANDLE      fileHandle_ = INVALID_HANDLE_VALUE;
    HANDLE      mappingHandle_ = nullptr;
#else
    // On POSIX we don't need to keep fd after successful mmap; for fallback read we don't keep it either.
    // No extra members required.
#endif

    // owns fallback buffer when mapping failed
    char* fallbackBuf_ = nullptr;

public:

    MemoryMappedFile() noexcept = default;

    // open and map (throws std::system_error on failure)
    explicit MemoryMappedFile(PATH_STR path);

    FileErrHeapPtr open(PATH_STR path) {
        return map_or_fallback(path);
    }

    ~MemoryMappedFile();

    // Non-copyable
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    // Movable
    MemoryMappedFile(MemoryMappedFile&&) noexcept;
    MemoryMappedFile& operator=(MemoryMappedFile&&) noexcept;

    // Inspectors
    [[nodiscard]] const char* data() const noexcept { return data_; }
    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] bool is_open() const noexcept { return size_ != 0 || data_ != nullptr; }

    // Close explicitly (noexcept)
    void close() noexcept;

private:
    FileErrHeapPtr map_or_fallback(PATH_STR path);
    void fallback_read(PATH_STR path, uint64_t fileSize);

};
