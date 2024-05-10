// Copyright (c) Qinetik 2024.

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "ast/base/BaseType.h"
#include "compiler/clangfwd.h"
#include "compiler/chem_clang.h"
#include "ast/base/ASTNode.h"

class CTranslator;

/**
 * when provided a clang builtin type pointer
 * it'll make a chemical type
 *
 * this expects a builtin type of the same kind, which is being used to index it on vector type_makers
 */
typedef BaseType*(*CTypeMakerFn)(clang::BuiltinType*);

/**
 * Node maker fn
 */
typedef ASTNode*(*CNodeMakerFn)(CTranslator*, clang::Decl*);

/**
 * a simple struct to represent errors during translation
 * we'll expand this later
 */
struct CTranslatorError {
    std::string message;
};

/**
 * The point of this class to provide storage for translation process
 * for example storage for indexed types, errors and stuff
 */
class CTranslator {
public:

    /**
     * errors that occurred during translation
     */
    std::vector<CTranslatorError> errors;

    /**
     * type makers functions vector
     * enum BuiltinType::Kind, this enum is used as an index on this vector
     * last enum entry is being used as size of the vector
     */
    std::vector<CTypeMakerFn> type_makers = std::vector<CTypeMakerFn>(ZigClangBuiltinTypeOMPIterator + 1);

    /**
     * these allow indexing decl kind enum, to provide functions that can make nodes
     * last enum entry is being used as size of the vector
     */
    std::vector<CNodeMakerFn> node_makers = std::vector<CNodeMakerFn>(ZigClangDeclTranslationUnit + 1);

    /**
     * this is the result after translation
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * these are nodes that should be added before adding a node
     * these nodes were created by tra
     */
    std::vector<std::unique_ptr<ASTNode>> before_nodes;

    /**
     * constructor
     */
    CTranslator();

    /**
     * initializes type makers
     */
    void init_type_makers();

    /**
     * initializes node makers
     */
    void init_node_makers();

    /**
     * dispatches nodes added to before_nodes vector into nodes
     */
    void dispatch_before();

    /**
     * when given a c qualified type, it constructs a chemical type
     */
    BaseType* make_type(clang::QualType* type);

    /**
     * when provided a pointer to function declaration
     * it'll make a function declaration
     */
    FunctionDeclaration* make_func(clang::FunctionDecl* decl);

    /**
     * create a typealias statement from typedef declaration
     */
    TypealiasStatement* make_typealias(clang::TypedefDecl* decl);

    /**
     * creates a struct definition from the given record decl
     */
    StructDefinition* make_struct(clang::RecordDecl* decl);

    /**
     * put an error in errors, called when an error occurs during translation
     */
    void error(const std::string& err);

};