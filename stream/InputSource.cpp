// Copyright (c) Qinetik 2024.

#include "FileInputSource.h"
#include <fcntl.h>   // for open, O_RDONLY
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include "utils/PathUtils.h"
#else
#include <fcntl.h>
#include <unistd.h>  // for lseek, read, close
#endif

FileInputSource::FileInputSource(const char* file_path) {
    error.kind = InputSourceErrorKind::None;
    error.message = "";
#ifdef _WIN32
//    const auto abs = absolute_path(file_path);
    fileHandle = CreateFile(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        error.kind = InputSourceErrorKind::FileNotOpen;
    }
#else
    fileDescriptor = open(file_path, O_RDONLY);
        if (fileDescriptor == -1) {
            error.kind = InputSourceErrorKind::FileNotOpen;
        }
#endif
}

size_t FileInputSource::read(char *buffer, size_t size) {
#ifdef _WIN32
    DWORD bytesRead;
    BOOL result = ReadFile(fileHandle, buffer, static_cast<DWORD>(size), &bytesRead, NULL);
    if (!result) {
        // Handle error
        error.kind = InputSourceErrorKind::ReadFailed;
        return 0;
    }
    return bytesRead;
#else
    return ::read(fileDescriptor, buffer, size);
#endif
}

off_t FileInputSource::seek(off_t offset, int whence) {
#ifdef _WIN32
    LARGE_INTEGER distance;
    distance.QuadPart = offset;
    LARGE_INTEGER newPos;
    if (!SetFilePointerEx(fileHandle, distance, &newPos, whence)) {
        // Handle error
        error.kind == InputSourceErrorKind::SeekFailed;
    }
    return newPos.QuadPart;
#else
    return lseek(fileDescriptor, offset, whence);
#endif
}

off_t FileInputSource::tell() const {
#ifdef _WIN32
    LARGE_INTEGER distance;
    distance.QuadPart = 0;
    LARGE_INTEGER newPos;
    if (!SetFilePointerEx(fileHandle, distance, &newPos, FILE_CURRENT)) {
        // Handle error
        error.kind == InputSourceErrorKind::SeekFailed;
    }
    return newPos.QuadPart;
#else
    return lseek(fileDescriptor, 0, SEEK_CUR);
#endif
}

FileInputSource::~FileInputSource() {
#ifdef _WIN32
    if (fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }
#else
    if (fileDescriptor != -1) {
        close(fileDescriptor);
    }
#endif
}
