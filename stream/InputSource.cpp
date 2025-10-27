// Copyright (c) Chemical Language Foundation 2025.

// Copyright (c) Chemical Language Foundation 2025.

#include "FileInputSource.h"
#include "std/except.h"

#include <system_error>
#include <stdexcept>
#include <memory>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#endif

// memory_mapped_file.cpp
#include <fstream>
#include <system_error>

FileInputSource::FileInputSource(const std::filesystem::path& path) {
    err = map_or_fallback(path);
}

FileInputSource::~FileInputSource() {
    close();
}

FileInputSource::FileInputSource(FileInputSource&& o) noexcept {
    data_ = o.data_;
    size_ = o.size_;
#if defined(_WIN32)
    fileHandle_ = o.fileHandle_;
    mappingHandle_ = o.mappingHandle_;
    o.fileHandle_ = INVALID_HANDLE_VALUE;
    o.mappingHandle_ = nullptr;
#endif
    fallbackBuf_ = o.fallbackBuf_;
    o.fallbackBuf_ = nullptr;
    o.data_ = nullptr;
    o.size_ = 0;
}

FileInputSource& FileInputSource::operator=(FileInputSource&& o) noexcept {
    if (this != &o) {
        close();
        data_ = o.data_;
        size_ = o.size_;
#if defined(_WIN32)
        fileHandle_ = o.fileHandle_;
        mappingHandle_ = o.mappingHandle_;
        o.fileHandle_ = INVALID_HANDLE_VALUE;
        o.mappingHandle_ = nullptr;
#endif
        fallbackBuf_ = o.fallbackBuf_;
        o.fallbackBuf_ = nullptr;
        o.data_ = nullptr;
        o.size_ = 0;
    }
    return *this;
}

void FileInputSource::close() noexcept {
    if (!data_ && size_ == 0) {
        // nothing to do
    } else {
#if defined(_WIN32)
        if (data_) {
            UnmapViewOfFile(const_cast<LPVOID>(static_cast<const void*>(data_)));
            data_ = nullptr;
        }
        if (mappingHandle_) {
            CloseHandle(mappingHandle_);
            mappingHandle_ = nullptr;
        }
        if (fileHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(fileHandle_);
            fileHandle_ = INVALID_HANDLE_VALUE;
        }
#else
        if (data_ && !fallbackBuf_) {
            // we mapped the region; unmap it
            munmap(const_cast<char*>(data_), size_);
            data_ = nullptr;
        }
#endif
        delete[] fallbackBuf_;
        fallbackBuf_ = nullptr;
        size_ = 0;
        data_ = nullptr;
    }
}

#if defined(_WIN32)
inline std::string utf16_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0,
                                          wstr.data(), (int)wstr.size(),
                                          nullptr, 0,
                                          nullptr, nullptr);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0,
                        wstr.data(), (int)wstr.size(),
                        result.data(), size_needed,
                        nullptr, nullptr);
    return result;
}
#endif

FileErrHeapPtr FileInputSource::map_or_fallback(const std::filesystem::path& path) {
    // clear previous state if any
    close();

#if defined(_WIN32)
    fileHandle_ = CreateFileW(path.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (fileHandle_ == INVALID_HANDLE_VALUE) {
        return new FileErr{
                static_cast<int>(GetLastError()),
                "system",
                std::system_category().message(GetLastError()),
                "CreateFileW",
                utf16_to_utf8(path)
        };
    }

    LARGE_INTEGER size_li;
    if (!GetFileSizeEx(fileHandle_, &size_li)) {
        auto err = static_cast<int>(GetLastError());
        CloseHandle(fileHandle_);
        fileHandle_ = INVALID_HANDLE_VALUE;
        return new FileErr{
                err,
                "system",
                std::system_category().message(err),
                "GetFileSizeEx",
                utf16_to_utf8(path)
        };
    }

    auto fileSize = static_cast<uint64_t>(size_li.QuadPart);
    if (fileSize == 0) {
        // empty file: nothing to map; close file handle
        CloseHandle(fileHandle_);
        fileHandle_ = INVALID_HANDLE_VALUE;
        size_ = 0;
        data_ = nullptr;
        return nullptr;
    }

    mappingHandle_ = CreateFileMappingW(fileHandle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mappingHandle_) {
        // fallback to read
        fallback_read(path, fileSize);
        if (fileHandle_ != INVALID_HANDLE_VALUE) { CloseHandle(fileHandle_); fileHandle_ = INVALID_HANDLE_VALUE; }
        return nullptr;
    }

    // Map view
    LPVOID view = MapViewOfFile(mappingHandle_, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        // cleanup mapping handle and fallback
        CloseHandle(mappingHandle_);
        mappingHandle_ = nullptr;
        fallback_read(path, fileSize);
        if (fileHandle_ != INVALID_HANDLE_VALUE) { CloseHandle(fileHandle_); fileHandle_ = INVALID_HANDLE_VALUE; }
        return nullptr;
    }

    // Keep mappingHandle_ (so we can CloseHandle on destruction).
    // We can close fileHandle_ if we like (mapping keeps view alive), but keep it for safety or close it to free resources:
    CloseHandle(fileHandle_);
    fileHandle_ = INVALID_HANDLE_VALUE;

    data_ = static_cast<const char*>(view);
    size_ = static_cast<size_t>(fileSize);

#else
    // POSIX path
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return new FileErr{
            errno,
            "generic",
            std::generic_category().message(errno),
            "open",
            path
        };
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        int e = errno;
        ::close(fd);
        return new FileErr{
            e,
            "generic",
            std::generic_category().message(e),
            "fstat",
            path
        };
    }

    uint64_t fileSize = static_cast<uint64_t>(st.st_size);
    if (fileSize == 0) {
        ::close(fd);
        size_ = 0;
        data_ = nullptr;
        return nullptr;
    }

    if (fileSize > static_cast<uint64_t>(SIZE_MAX)) {
        ::close(fd);
        return new FileErr{
            EOVERFLOW,
            "generic",
            std::generic_category().message(EOVERFLOW),
            "file size check",
            path
        };
    }

    void* mapped = mmap(nullptr, static_cast<size_t>(fileSize), PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        // fallback: read into heap buffer
        fallback_read(path, fileSize);
        ::close(fd);
        return nullptr;
    }

    // mmap succeeded: can close fd
    ::close(fd);
    data_ = static_cast<const char*>(mapped);
    size_ = static_cast<size_t>(fileSize);
#endif

    return nullptr; // success
}

#ifdef _WIN32
constexpr size_t get_dword_max() {
    return static_cast<size_t>(~static_cast<DWORD>(0));
}
#endif

void FileInputSource::fallback_read(const std::filesystem::path& path, uint64_t fileSize) {
    if (fileSize == 0) {
        data_ = nullptr;
        size_ = 0;
        return;
    }
    if (fileSize > static_cast<uint64_t>(SIZE_MAX))
        CHEM_THROW_SYSTEM(EOVERFLOW, "generic", nullptr, "file too large for fallback allocation");

    size_t sz = static_cast<size_t>(fileSize);
    std::unique_ptr<char[]> buf(new char[sz]);
#if defined(_WIN32)
    // Read file contents using ReadFile
    // Re-open for read
    HANDLE fh = CreateFileW(path.c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            nullptr,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            nullptr);
    if (fh == INVALID_HANDLE_VALUE) {
        CHEM_THROW_SYSTEM(GetLastError(), "system", nullptr, "CreateFileW (fallback) failed");
    }
    DWORD read = 0;
    DWORD toRead = (sz > static_cast<size_t>(~static_cast<DWORD>(0))?
                    ~static_cast<DWORD>(0) : static_cast<DWORD>(sz));
    // Read in loop if file is larger than DWORD max (rare)
    size_t offset = 0;
    while (offset < sz) {
        DWORD thisRead = (DWORD)std::min<size_t>(sz - offset, toRead);
        BOOL ok = ReadFile(fh, buf.get() + offset, thisRead, &read, nullptr);
        if (!ok) {
            CloseHandle(fh);
            CHEM_THROW_SYSTEM(GetLastError(), "system", std::system_category(), "ReadFile failed in fallback");
        }
        offset += read;
        if (read == 0) break; // EOF
    }
    CloseHandle(fh);
#else
    // POSIX: open and read
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) CHEM_THROW_SYSTEM(errno, "generic", nullptr, "open failed (fallback)");
    size_t offset = 0;
    while (offset < sz) {
        ssize_t r = ::read(fd, buf.get() + offset, sz - offset);
        if (r < 0) {
            int e = errno;
            ::close(fd);
            CHEM_THROW_SYSTEM(e, "generic", nullptr, "read failed (fallback)");
        }
        if (r == 0) break;
        offset += static_cast<size_t>(r);
    }
    ::close(fd);
#endif

    fallbackBuf_ = buf.release();
    data_ = fallbackBuf_;
    size_ = sz;
}
