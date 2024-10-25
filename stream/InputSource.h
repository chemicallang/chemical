// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

enum class InputSourceErrorKind {
    None,
    FileNotOpen,
    ReadFailed,
    SeekFailed
};

struct InputSourceError {
    InputSourceErrorKind kind;
    std::string message;
};

class InputSource {
protected:
/**
 * any error that happens will be stored here
 */
InputSourceError error;
public:

    /**
     * read the amount into buffer
     */
    virtual size_t read(char* buffer, size_t size) = 0;

    /**
     * seek to given position
     */
    virtual off_t seek(off_t offset, int whence) = 0;

    /**
     * get the current position
     */
    virtual off_t tell() const = 0;

    /**
     * check if has error
     */
    bool has_error() {
        return error.kind != InputSourceErrorKind::None;
    }

    /**
     * get the kind of error
     */
    InputSourceErrorKind error_kind() {
        return error.kind;
    }

    /**
     * get the message of error
     */
    std::string error_message() {
        return error.message;
    }

    /**
     * destructor
     */
    virtual ~InputSource() = default;

};
