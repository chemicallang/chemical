// Copyright (c) Qinetik 2024.

#pragma once

class SourceProvider;
namespace chem {
    struct string;
}

/**
 * A CBI is a Compiler Binding Interface
 * when user calls functions in the compiler from the code written that is currently
 * being processed by the compiler, The user uses CBI to call those functions
 *
 * A CBI contains struct members and functions as function pointers in order
 * A pointer to the actual instance is placed at the end of the struct, which will be used
 * to call the actual functions, This ordering is very important
 *
 * CBI interfaces are generated from our chemical code present in lang/lib/compiler
 * using t2c with options --inline-struct-member-fn-types --cpp-like
 * a instance member field is required, which is added to the end
 */
struct SourceProviderCBI {
    unsigned int(*currentPosition)(struct SourceProviderCBI*);
    char(*readCharacter)(struct SourceProviderCBI*);
    bool(*eof)(struct SourceProviderCBI*);
    char(*peek)(struct SourceProviderCBI*);
    char(*peek_at)(struct SourceProviderCBI*,int);
    char*(*readUntil)(struct SourceProviderCBI*,char);
    bool(*increment)(struct SourceProviderCBI*,char*,bool);
    bool(*increment_char)(struct SourceProviderCBI*,char);
    char*(*readAllFromHere)(struct SourceProviderCBI*);
    unsigned int(*getLineNumber)(struct SourceProviderCBI*);
    unsigned int(*getLineCharNumber)(struct SourceProviderCBI*);
    void(*readEscaping)(struct SourceProviderCBI*,char*,char);
    char*(*readAnything)(struct SourceProviderCBI*,char);
    char*(*readAlpha)(struct SourceProviderCBI*);
    char*(*readUnsignedInt)(struct SourceProviderCBI*);
    void(*readNumber)(struct chem::string*, struct SourceProviderCBI*);
    char*(*readAlphaNum)(struct SourceProviderCBI*);
    char*(*readIdentifier)(struct SourceProviderCBI*);
    void(*readAnnotationIdentifierInto)(struct SourceProviderCBI*,char*);
    char*(*readAnnotationIdentifier)(struct SourceProviderCBI*);
    unsigned int(*readWhitespaces)(struct SourceProviderCBI*);
    bool(*hasNewLine)(struct SourceProviderCBI*);
    bool(*readNewLineChars)(struct SourceProviderCBI*);
    void(*readWhitespacesAndNewLines)(struct SourceProviderCBI*);
    SourceProvider* instance;
};

/**
 * this function should be called on cbi, to make it a valid binding
 * now cbi is ready to be invoked, to use it, pass it to a user
 */
void init_source_provider_cbi(SourceProviderCBI* cbi, SourceProvider* provider);