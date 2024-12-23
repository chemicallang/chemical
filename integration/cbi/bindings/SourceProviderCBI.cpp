// Copyright (c) Qinetik 2024.

#include "SourceProviderCBI.h"
#include "stream/SourceProvider.h"

char SourceProviderreadCharacter(SourceProvider* provider)  {
    return provider->readCharacter();
}

bool SourceProvidereof(SourceProvider* provider)  {
    return provider->eof();
}

char SourceProviderpeek(SourceProvider* provider)  {
    return provider->peek();
}

bool SourceProviderincrement_char(SourceProvider* provider, char c)  {
    return provider->increment(c);
}

unsigned int SourceProvidergetLineNumber(SourceProvider* provider)  {
    return provider->getLineNumber();
}

unsigned int SourceProvidergetLineCharNumber(SourceProvider* provider)  {
    return provider->getLineCharNumber();
}

unsigned int SourceProviderreadWhitespaces(SourceProvider* provider)  {
    return provider->readWhitespaces();
}

bool SourceProviderhasNewLine(SourceProvider* provider)  {
    return provider->hasNewLine();
}

bool SourceProviderreadNewLineChars(SourceProvider* provider)  {
    return provider->readNewLineChars();
}

void SourceProviderreadWhitespacesAndNewLines(SourceProvider* provider)  {
    return provider->readWhitespacesAndNewLines();
}