// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIUtils.h"

class SourceProvider;

extern "C" {

    char SourceProviderreadCharacter(SourceProvider* provider);

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