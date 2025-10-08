// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ast_fwd.h"
#include <span>

class CompilerBinder;

class GenericInstantiator;

class InstantiationsContainer;

class TypeBuilder;

class GenericInstantiatorAPI {
private:
    /**
     * this allows the class generic instantiator to convert itself to api
     * then set the owns to false, so it doesn't delete itself
     */
    bool owns = true;
public:

    /**
     * the actual pointer
     */
    GenericInstantiator* giPtr;

    /**
     * view constructor, the pointer won't be freed
     */
    inline explicit constexpr GenericInstantiatorAPI(GenericInstantiator* ptr) : giPtr(ptr), owns(false) {

    }

    /**
     * the constructor
     */
    GenericInstantiatorAPI(
        CompilerBinder& binder,
        ChildResolver& child_resolver,
        InstantiationsContainer& container,
        ASTAllocator& astAllocator,
        ASTDiagnoser& diagnoser,
        TypeBuilder& typeBuilder
    );

    /**
     * must not perform a copy
     */
    GenericInstantiatorAPI(const GenericInstantiatorAPI& other) = delete;

    /**
     * get the container
     */
    InstantiationsContainer& getContainer();

    /**
     * get the allocator
     */
    ASTAllocator& getAllocator();

    /**
     * get the diagnoser
     */
    ASTDiagnoser& getDiagnoser();

    /**
     * this will change the allocator to this
     */
    void setAllocator(ASTAllocator& allocator);

    /**
     * finalize the signature of generic type decl
     */
    void FinalizeSignature(GenericTypeDecl* decl, TypealiasStatement* impl, std::vector<TypeLoc>& generic_args);

    /**
     * finalize the signature of generic type decl
     */
    void FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations);

    /**
     * finalize the signature of all these generic function instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the function declaration
     */
    void FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations);

    /**
     * finalize the body of all these generic function instantiations (shallow copies)
     * this means inside the body, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the function declaration
     */
    void FinalizeBody(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations);

    /**
     * finalize the signature of all these generic struct instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeSignature(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations);

    /**
     * finalize the body of all these generic struct instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeBody(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations);

    /**
     * finalize the signature of all these generic union instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeSignature(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations);

    /**
     * finalize the body of all these generic union instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeBody(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations);

    /**
     * finalize the signature of all these generic interface instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeSignature(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations);

    /**
     * finalize the body of all these generic interface instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeBody(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations);

    /**
     * finalize the signature of all these generic variant instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeSignature(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations);

    /**
     * finalize the body of all these generic variant instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the struct declaration
     */
    void FinalizeBody(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations);

    /**
     * finalize the signature of all these generic impl instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the impl declaration
     */
    void FinalizeSignature(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations);

    /**
     * finalize the body of all these generic impl instantiations (shallow copies)
     * this means inside the signature, all types that are generic are replaced with concrete implementations
     * the implementation is determined with instantiation index inside the impl declaration
     */
    void FinalizeBody(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations);


    /**
     * destructor
     */
    ~GenericInstantiatorAPI();

};