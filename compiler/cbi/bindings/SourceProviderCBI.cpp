// Copyright (c) Chemical Language Foundation 2025.

#include "SourceProviderCBI.h"
#include "stream/SourceProvider.h"

void SourceProviderincrement(SourceProvider* provider) {
    provider->increment();
}

char SourceProviderreadCharacter(SourceProvider* provider)  {
    return provider->readCharacter();
}

uint32_t SourceProviderreadCodePoint(SourceProvider* provider) {
    return provider->readCodePoint();
}

uint32_t SourceProviderutf8_decode_peek(SourceProvider* provider, size_t *out_len) {
    return provider->utf8_decode_peek(*out_len);
}

void SourceProviderincrementCodepoint(SourceProvider* provider, uint32_t cp, size_t len) {
    provider->incrementCodepoint(cp, len);
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