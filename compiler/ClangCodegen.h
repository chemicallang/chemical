// Copyright (c) Qinetik 2024.

#pragma once

#include "clangfwd.h"
#include <memory>

class ClangCodegenImpl;

enum class ManglerKind {
    Microsoft,
    Itanium
};

class FunctionDeclaration;

#ifdef _WIN32
static constexpr ManglerKind DefManglerKind = ManglerKind::Microsoft;
#else
static constexpr ManglerKind DefManglerKind = ManglerKind::Itanium;
#endif

/**
 * allows to abstract away details of clang code generation
 * which is used to generate code for C and C++, mangle function names
 */
class ClangCodegen {
public:

    /**
     * the implementation that hides the details of implementation
     */
    std::unique_ptr<ClangCodegenImpl> impl;

    /**
     * the constructor
     */
    ClangCodegen(std::string target_triple, ManglerKind manglerKind = DefManglerKind);

    /**
     * switch mangler kind to given
     */
    void switch_mangler_kind(ManglerKind kind);

    /**
     * switches the language to C++
     */
    void switch_to_cpp();

    /**
     * switches the language to C
     */
    void switch_to_c();

    /**
     * This will give out the C++ mangled name for the given clang function
     */
    std::string mangled_name(clang::FunctionDecl* decl);

    /**
     * This will give out the C++ mangled name for the given chemical function
     */
    std::string mangled_name(FunctionDeclaration* decl);

    /**
     * destructor
     */
    ~ClangCodegen();

};