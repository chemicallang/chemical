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
    unsigned int(*currentPosition)(struct SourceProviderCBI* self);
    char(*readCharacter)(struct SourceProviderCBI* self);
    bool(*eof)(struct SourceProviderCBI* self);
    char(*peek)(struct SourceProviderCBI* self);
    char(*peek_at)(struct SourceProviderCBI* self, int offset);
    void(*readUntil)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self, char stop);
    bool(*increment)(struct SourceProviderCBI* self, char* text, bool peek);
    bool(*increment_char)(struct SourceProviderCBI* self, char c);
    unsigned int(*getLineNumber)(struct SourceProviderCBI* self);
    unsigned int(*getLineCharNumber)(struct SourceProviderCBI* self);
    void(*readEscaping)(struct SourceProviderCBI* self, struct chem::string* value, char stopAt);
    void(*readAnything)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self, char until);
    void(*readAlpha)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self);
    void(*readUnsignedInt)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self);
    void(*readNumber)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self);
    void(*readAlphaNum)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self);
    void(*readIdentifier)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self);
    void(*readAnnotationIdentifier)(struct chem::string* __chx_struct_ret_param_xx, struct SourceProviderCBI* self);
    unsigned int(*readWhitespaces)(struct SourceProviderCBI* self);
    bool(*hasNewLine)(struct SourceProviderCBI* self);
    bool(*readNewLineChars)(struct SourceProviderCBI* self);
    void(*readWhitespacesAndNewLines)(struct SourceProviderCBI* self);
    SourceProvider* instance;
};

/**
 * this function should be called on cbi, to make it a valid binding
 * now cbi is ready to be invoked, to use it, pass it to a user
 */
void init_source_provider_cbi(SourceProviderCBI* cbi, SourceProvider* provider);