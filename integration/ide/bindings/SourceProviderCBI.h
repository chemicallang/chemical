// Copyright (c) Qinetik 2024.

class SourceProvider;

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
    unsigned int(*currentPosition)(struct SourceProvider*);
    bool(*eof)(struct SourceProvider*);
    unsigned int(*getLineCharNumber)(struct SourceProvider*);
    unsigned int(*getLineNumber)(struct SourceProvider*);
    bool(*hasNewLine)(struct SourceProvider*);
    bool(*increment)(struct SourceProvider*,char*,bool);
    bool(*increment_char)(struct SourceProvider*,char);
    char(*peek)(struct SourceProvider*);
    char(*peek_at)(struct SourceProvider*,int);
    char*(*readAllFromHere)(struct SourceProvider*);
    char*(*readAlpha)(struct SourceProvider*);
    char*(*readAlphaNum)(struct SourceProvider*);
    char*(*readAnnotationIdentifier)(struct SourceProvider*);
    void(*readAnnotationIdentifierInto)(struct SourceProvider*,char**);
    char*(*readAnything)(struct SourceProvider*,char);
    char(*readCharacter)(struct SourceProvider*);
    void(*readEscaping)(struct SourceProvider*,char**,char);
    char*(*readIdentifier)(struct SourceProvider*);
    bool(*readNewLineChars)(struct SourceProvider*);
    char*(*readNumber)(struct SourceProvider*);
    char*(*readUnsignedInt)(struct SourceProvider*);
    char*(*readUntil)(struct SourceProvider*,char);
    unsigned int(*readWhitespaces)(struct SourceProvider*);
    void(*readWhitespacesAndNewLines)(struct SourceProvider*);
    SourceProvider* instance;
};
