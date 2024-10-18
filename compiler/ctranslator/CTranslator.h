// Copyright (c) Qinetik 2024.

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "ast/base/BaseType.h"
#include "compiler/clangfwd.h"
#include "compiler/chem_clang.h"
#include "ast/base/Visitor.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "ordered_map.h"

class CTranslator;

/**
 * when provided a clang builtin type pointer
 * it'll make a chemical type
 *
 * this expects a builtin type of the same kind, which is being used to index it on vector type_makers
 */
typedef BaseType*(*CTypeMakerFn)(ASTAllocator&, clang::BuiltinType*);

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

namespace clang {
    class DiagnosticsEngine;
}

/**
 * The point of this class to provide storage for translation process
 * for example storage for indexed types, errors and stuff
 */
class CTranslator {
public:

    /**
     * the reference to allocator
     */
    ASTAllocator& allocator;

    /**
     * a single invocation is expected,
     * c translator cannot be used for multiple invocations
     */
    std::mutex translation_mutex;

    /**
     * the diagnostics engine is used to collect diagnostics from clang across
     * multiple invocations
     */
    llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags_engine;

    /**
     * errors that occurred during translation
     */
    std::vector<CTranslatorError> errors;

    /**
     * is translating for target 64bit
     */
    bool is64Bit;

    /**
     * this can be set to false, when there's only a single invocation
     * of translate, in multiple invocations, headers are checked in each module
     * whether declared or not
     */
    bool check_decls_across_invocations = true;

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
    std::vector<ASTNode*> nodes;

    /**
     * when we translate a declaration, we store it here on this map
     * we can reuse this declarations everywhere, these nodes are retained for
     * the duration this translator is retained
     */
    std::unordered_map<std::size_t, ASTNode*> translated_map;

    /**
     * these nodes have been declared in module, so must not be declared again
     * we only check nodes that we re-use from translated_map
     */
    std::unordered_map<ASTNode*, bool> declared_in_module;

    /**
     * a map between clang declarations that we translated to our nodes
     */
    std::unordered_map<clang::Decl*, ASTNode*> declarations;

    /**
     * current clang unit being translated
     */
    clang::ASTUnit* current_unit;

    /**
     * these are nodes that should be added before adding a node
     * these nodes were created by translation
     */
//    std::vector<ASTNode*> before_nodes;

    /**
     * nodes being added belong to this parent node
     */
    ASTNode* parent_node = nullptr;

    /**
     * constructor
     */
    CTranslator(ASTAllocator& allocator, bool is64Bit);

    /**
     * initializes type makers
     */
    void init_type_makers();

    /**
     * initializes node makers
     */
    void init_node_makers();

    /**
     * whenever a module begins this function should be called to make
     * the translate re-import the declarations from headers that aren't imported
     * in this module
     */
    void module_begin();

    /**
     * declaration will be translated without checking for declared or not in other headers
     */
    void translate_no_check(clang::Decl* decl);

    /**
     * declaration will be checked whether declared or not in other headers
     */
    void translate_checking(clang::Decl* decl, clang::SourceManager& sourceMan);

    /**
     * translate the given clang ast unit
     */
    void translate(clang::ASTUnit* unit);

    /**
     * translate using given arguments
     * and get the ASTUnit
     * CAUTION: ASTUnit must be freed, It also doesn't use a mutex
     * which must be used when translating
     */
    clang::ASTUnit* get_unit(
            const char** args_begin,
            const char** args_end,
            const char* resources_path
    );

    /**
     * get unit helper function,
     * CAUTION: ASTUnit must be freed, it also doesn't use a mutex
     */
    clang::ASTUnit* get_unit(
        const char* exe_path,
        const char* abs_path,
        const char* resources_path
    );

    /**
     * translate using given arguments
     * prefer this method, because it's faster
     */
    void translate(
        const char** args_begin,
        const char** args_end,
        const char* resources_path
    );

    /**
     * a helper method to translate
     * pref this method, because it's faster
     */
    void translate(
            const char *exe_path,
            const char *abs_path,
            const char *resources_path
    );

    /**
     * translate unit using given arguments
     */
    void translate(std::vector<std::string>& args, const char* resources_path);

    /**
     * a helper function to get unit for a single header file
     * CAUTION: just like get_unit, it returns a released pointer and doesn't use
     * a mutex
     */
    clang::ASTUnit* get_unit_for_header(
        const std::string_view& exe_path,
        const std::string_view& header_path,
        const char* resources_path
    );

    /**
     * when given a c qualified type, it constructs a chemical type
     */
    BaseType* make_type(clang::QualType* type);

    /**
     * makes an expression from the given clang's expression
     */
    Value* make_expr(clang::Expr* expr);

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
     * create an enum declaration from the given enum decl
     */
    EnumDeclaration* make_enum(clang::EnumDecl* decl);

    /**
     * create a var init declaration from the given var decl
     */
    VarInitStatement* make_var_init(clang::VarDecl* decl);

    /**
     * put an error in errors, called when an error occurs during translation
     */
    void error(const std::string& err);

    /**
     * destructor
     */
    ~CTranslator();

};