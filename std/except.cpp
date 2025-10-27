// Copyright (c) Chemical Language Foundation 2025.


#include "except.h"
#include <cstdlib>
#include <cstdio>      // for std::fprintf
#include <cstring>     // for std::strerror
#include <cerrno>
#include <string>

namespace chem::detail {

    [[noreturn]] void abort_runtime_msg(const char* msg) noexcept {
        if (msg && *msg) {
            std::fprintf(stderr, "fatal: %s\n", msg);
        } else {
            std::fprintf(stderr, "fatal: unknown runtime error\n");
        }
        std::fflush(stderr);
        std::abort();
    }

    [[noreturn]] void abort_system_msg(int err, const char* cat_name, const char* cat_message, const char* msg) noexcept {
        // Prepare a human-readable text for the error. Try to use cat_message if provided,
        // otherwise fall back to strerror(err) on POSIX-style platforms.
        const char* err_text = nullptr;
        static thread_local std::string temp_buf;

        if (cat_message && *cat_message) {
            err_text = cat_message;
        } else {
            // Try POSIX strerror if err looks like errno; strerror is safe here for reporting.
            // If strerror returns nullptr use a fallback.
            if (err != 0) {
                const char* s = std::strerror(err);
                if (s) {
                    temp_buf = s;
                    err_text = temp_buf.c_str();
                }
            }
        }

        if (msg && *msg) {
            if (err_text) {
                if (cat_name && *cat_name) {
                    std::fprintf(stderr, "fatal: %s (code %d, category=%s: %s)\n", msg, err, cat_name, err_text);
                } else {
                    std::fprintf(stderr, "fatal: %s (code %d: %s)\n", msg, err, err_text);
                }
            } else {
                if (cat_name && *cat_name) {
                    std::fprintf(stderr, "fatal: %s (code %d, category=%s)\n", msg, err, cat_name);
                } else {
                    std::fprintf(stderr, "fatal: %s (code %d)\n", msg, err);
                }
            }
        } else {
            if (err_text) {
                if (cat_name && *cat_name) {
                    std::fprintf(stderr, "fatal: error (code %d, category=%s: %s)\n", err, cat_name, err_text);
                } else {
                    std::fprintf(stderr, "fatal: error (code %d: %s)\n", err, err_text);
                }
            } else {
                if (cat_name && *cat_name) {
                    std::fprintf(stderr, "fatal: error (code %d, category=%s)\n", err, cat_name);
                } else {
                    std::fprintf(stderr, "fatal: error (code %d)\n", err);
                }
            }
        }
        std::fflush(stderr);
        std::abort();
    }

} // namespace chem::detail
