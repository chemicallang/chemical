// Copyright (c) Qinetik 2024.

#pragma once

namespace clang {
    class ASTUnit;
}

struct ErrorMsg {
    const char *filename_ptr; // can be null
    size_t filename_len;
    const char *msg_ptr;
    size_t msg_len;
    const char *source; // valid until the ASTUnit is freed. can be null
    unsigned line; // 0 based
    unsigned column; // 0 based
    unsigned offset; // byte offset into source
};

clang::ASTUnit *ClangLoadFromCommandLine(const char **args_begin, const char **args_end,
                                         struct ErrorMsg **errors_ptr, size_t *errors_len, const char *resources_path);