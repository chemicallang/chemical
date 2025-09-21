// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIUtils.h"
#include <cstdint>
#include <stddef.h>

class SourceProvider;

extern "C" {

    void SourceProviderincrement(SourceProvider* provider);

    char SourceProviderreadCharacter(SourceProvider* provider);

    uint32_t SourceProviderreadCodePoint(SourceProvider* provider);

    uint32_t SourceProviderutf8_decode_peek(SourceProvider* provider, size_t *out_len);

    void SourceProviderincrementCodepoint(SourceProvider*, uint32_t cp, size_t len);

    bool SourceProvidereof(SourceProvider* provider);

    char SourceProviderpeek(SourceProvider* provider);

    bool SourceProviderincrement_char(SourceProvider* provider, char c);

    unsigned int SourceProvidergetLineNumber(SourceProvider* provider);

    unsigned int SourceProvidergetLineCharNumber(SourceProvider* provider);

    unsigned int SourceProviderreadWhitespaces(SourceProvider* provider);

    bool SourceProviderhasNewLine(SourceProvider* provider);

    bool SourceProviderreadNewLineChars(SourceProvider* provider);

    void SourceProviderreadWhitespacesAndNewLines(SourceProvider* provider);

}