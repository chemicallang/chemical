// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIUtils.h"

class SourceProvider;

extern "C" {

    char SourceProviderreadCharacter(SourceProvider* provider);

    bool SourceProvidereof(SourceProvider* provider);

    char SourceProviderpeek(SourceProvider* provider);

    void SourceProviderreadUntil(SourceProvider* provider, chem::string* into, char stop);

    bool SourceProviderincrement_char(SourceProvider* provider, char c);

    unsigned int SourceProvidergetLineNumber(SourceProvider* provider);

    unsigned int SourceProvidergetLineCharNumber(SourceProvider* provider);

    void SourceProviderreadEscaping(SourceProvider* provider, chem::string* value, char stopAt);

    void SourceProviderreadAnything(SourceProvider* provider, chem::string* value, char until);

    void SourceProviderreadAlpha(SourceProvider* provider, chem::string* value);

    void SourceProviderreadUnsignedInt(SourceProvider* provider, chem::string* value);

    void SourceProviderreadNumber(SourceProvider* provider, chem::string* value);

    void SourceProviderreadAlphaNum(SourceProvider* provider, chem::string* value);

    void SourceProviderreadIdentifier(SourceProvider* provider, chem::string* value);

    void SourceProviderreadAnnotationIdentifier(SourceProvider* provider, chem::string* value);

    unsigned int SourceProviderreadWhitespaces(SourceProvider* provider);

    bool SourceProviderhasNewLine(SourceProvider* provider);

    bool SourceProviderreadNewLineChars(SourceProvider* provider);

    void SourceProviderreadWhitespacesAndNewLines(SourceProvider* provider);

}