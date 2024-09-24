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

void SourceProviderreadUntil(SourceProvider* provider, chem::string* into, char stop)  {
    return provider->readUntil(into, stop);
}

bool SourceProviderincrement(SourceProvider* provider, chem::string* text, bool peek)  {
    return provider->increment({ text->data(), text->size() }, peek);
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

void SourceProviderreadEscaping(SourceProvider* provider, chem::string* value, char stopAt)  {
    return provider->readEscaping(value, stopAt);
}

void SourceProviderreadAnything(SourceProvider* provider, chem::string* value, char until)  {
    return provider->readAnything(value, until);
}

void SourceProviderreadAlpha(SourceProvider* provider, chem::string* value)  {
    return provider->readAlpha(value);
}

void SourceProviderreadUnsignedInt(SourceProvider* provider, chem::string* value)  {
    return provider->readUnsignedInt(value);
}

void SourceProviderreadNumber(SourceProvider* provider, chem::string* value)  {
    return provider->readNumber(value);
}

void SourceProviderreadAlphaNum(SourceProvider* provider, chem::string* value)  {
    return provider->readAlphaNum(value);
}

void SourceProviderreadIdentifier(SourceProvider* provider, chem::string* value)  {
    return provider->readIdentifier(value);
}

void SourceProviderreadAnnotationIdentifier(SourceProvider* provider, chem::string* value)  {
    return provider->readAnnotationIdentifier(value);
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