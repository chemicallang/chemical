// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include "ASTDiagnoser.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/ASTAllocator.h"
#include <memory>
#include <ordered_map.h>
#include "compiler/symres/SymbolTable.h"
#include "compiler/symres/SymbolRange.h"

class ASTNode;

class AnnotableNode;

class FunctionType;

class AccessChain;

class FunctionDeclaration;

class Scope;

class GlobalInterpretScope;

class ChainValue;

class VariableIdentifier;

enum class SymResScopeKind : uint8_t {
    /**
     * a global namespace is much like default scope, however
     * only a single global scope exists, It's never created by the user
     */
    Global,
    /**
     * a module is a collection of files, that export into an object files,
     * symbols in a module that have been exported aren't supposed to collide
     * with symbols in other modules
     */
    Module,
    /**
     * a file scope is just like a default scope, however with minor
     * differences, a file scope helps also to distinguish between files
     */
    File,
    /**
     * a default scope, symbols can be declared and retrieved
     * the retrieval takes into account symbols declared in scopes above
     */
    Default,
};

/**
 * a struct representing a scope, every '{' and '}' opens and closes a scope respectively
 *
 */
struct SymResScope {

    /**
     * the kind of scope this is
     */
    SymResScopeKind kind;

    /**
     * a scope is a map of symbols, this is that map
     * The key here is string_view, so it assumes that it's always valid
     * that's because strings are stored inside AST, which is not disposed
     * until symbol resolution is completed
     */
    std::unordered_map<chem::string_view, ASTNode*> symbols;

};

struct SymbolRef {

    /**
     * the index to scope is stored
     */
    std::size_t scope_index;

    /**
     * the symbol view
     */
    chem::string_view symbol;

};

struct SymbolRefValue : SymbolRef {

    /**
     * the node symbol is pointing to
     */
    ASTNode* node;

};

class CTranslator;

/**
 * SymbolResolver is responsible for linking symbols, every symbol can declare itself
 * on a map and another node can lookup this symbol, generate an error if not found
 */
class SymbolResolver : public ASTDiagnoser {
private:

    /**
     * when traversing nodes, a node can declare itself on the map
     * this is vector of scopes, the last scope is current scope
     * The first scope is the top level scope
     */
    std::vector<SymResScope> current;

    /**
     * runtime symbols that are checked for conflicts with other nodes
     * across modules, only at top level nodes
     */
    std::unordered_map<std::string, ASTNode*> runtime_symbols;

    /**
     * declares a node with string : name
     * DO NOT USE THIS FUNCTION TO DECLARE SYMBOLS
     */
    bool declare_quietly(const chem::string_view &name, ASTNode *node);

    /**
     * will try to override this function, notice the '&' in the previous pointer
     * if successfully overridden, it will modify the previous location inside the symbol table
     * to be this declaration and return true
     */
    bool overload_function(const chem::string_view& name, ASTNode*& previous, FunctionDeclaration* declaration);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names
     * @return true if a new symbol was declared
     */
    inline bool declare_function_quietly(const chem::string_view& name, FunctionDeclaration* declaration) {
        auto found = declare_function_no_overload(name, declaration);
        if(found == nullptr) {
            return true;
        } else {
            overload_function(name, *found, declaration);
            return false;
        }
    }

public:

    /**
     * a reference to global interpret scope is required
     * which helps to resolve conditions inside compile time if statements
     * to link code
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * ast allocator is the pointer to the allocator that is used to allocate ast nodes
     * globally, because it is used to store usages of generic types, so they are never disposed
     * because usages of ast types are needed to generate implementations based on usage
     * it is currently also being used to replace function calls with variant calls, and
     * switch cases with variant cases, because these can only be replaced at symbol resolution
     */
    ASTAllocator* ast_allocator;

    /**
     * module level allocator, used to allocate things like destructors / move functions
     * for non public structs and variant definition, since they are expected to be present
     * throughout module, we use this allocator
     */
    ASTAllocator* mod_allocator;

    /**
     * this is the reference to file level allocator which is used
     * to allocate things during symbol resolution, disposed right after this file has
     * symbol resolved
     */
    ASTAllocator& allocator;

    /**
     * implicit arguments are provided using provide statements
     * we track these using this unordered map
     */
    std::unordered_map<chem::string_view, Value*> implicit_args;

    /**
     * is the codegen for arch 64bit
     */
    bool is64Bit;

    /**
     * is everything linking inside a safe context
     */
    bool safe_context = true;

    /**
     * is the current context comptime, which means we're inside
     * a comptime function or a comptime block
     */
    bool comptime_context = false;

    /**
     * current function type, for which code is being linked
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * nodes that are created during symbol resolution, that must be retained
     * during symbol resolution, for example when a function with two names
     * exists, the second function creates a MultiFunctionNode and put's it on this
     * vector, which is declared on the symbol map, when function calls link with it
     * they are re-linked with correct function, based on arguments
     */
    std::vector<std::unique_ptr<ASTNode>> helper_nodes;

    /**
     * stores symbols that will be disposed after this file has been completely symbol resolved
     * for example using namespace some; this will always be disposed unless propagate annotation exists
     * above it
     */
    std::vector<SymbolRefValue> dispose_file_symbols;

    /**
     * stores symbols that will be disposed after this module has been completely symbol resolved
     * for example symbols that are internal in module are stored on this vector for disposing
     * at the end of module, struct without a public keyword (internal by default)
     */
    std::vector<SymbolRef> dispose_module_symbols;

    /**
     * constructor
     */
    SymbolResolver(
        GlobalInterpretScope& global,
        bool is64Bit,
        ASTAllocator& allocator,
        ASTAllocator* modAllocator,
        ASTAllocator* astAllocator
    );

    /**
     * a file scope begins, a file scope should not be popped, this is because
     * symbols are expected to exist in other files
     */
    unsigned int file_scope_start() {
        const auto s = current.size();
        current.emplace_back(SymResScopeKind::File);
        return (unsigned int) s;
    }

    /**
     * a module scope begins, a module scope doesn't contain no symbols
     * we'll improve the design so that module scopes don't introduce any overhead
     * of performance and memory
     * a module is just there to indicate that a module exists, it helps us delete symbols
     * only in a specific module
     */
    void module_scope_start() {
        current.emplace_back(SymResScopeKind::Module);
    }

    /**
     * when a scope begins, this should be called
     * it would put a scope on current vector
     */
    void scope_start() {
        current.emplace_back(SymResScopeKind::Default);
    }

    /**
     * when a scope ends, this should be called
     * it would pop a scope map from the current vector
     */
    void scope_end() {
        current.pop_back();
    }

    /**
     * find a symbol only in current file
     */
    ASTNode *find_in_current_file(const chem::string_view& name);

    /**
     * find a symbol on current symbol map
     */
    ASTNode *find(const chem::string_view& name);

    /**
     * find a symbol on current symbol map
     */
    ASTNode* find(std::string& name) {
        return find(chem::string_view(name.data(), name.size()));
    }

    /**
     * if the current where the symbols are being declared is a file scope
     */
    bool is_current_file_scope() {
        return current.back().kind == SymResScopeKind::File;
    }

    /**
     * duplicate symbol error
     */
    void dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node);

    /**
     * duplicate symbol error
     */
    inline void dup_sym_error(std::string& name, ASTNode* previous, ASTNode* new_node) {
        dup_sym_error(chem::string_view(name.data(), name.size()), previous, new_node);
    }

    /**
     * duplicate runtime symbol error
     */
    void dup_runtime_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node);

    /**
     * declare a symbol that will override the previous symbol if exists
     */
    void declare_overriding(const chem::string_view &name, ASTNode* node);

    /**
     * declare a symbol that will disposed at the end of this module
     */
    void declare(const chem::string_view &name, ASTNode *node);

    /**
     * declare name for which you own a string name
     */
    inline void declare(std::string& name, ASTNode* node) {
        declare(chem::string_view(name.data(), name.size()), node);
    }

    /**
     * declare a symbol that will be disposed at the end of this file instead of module
     */
    void declare_file_disposable(const chem::string_view &name, ASTNode *node);

    /**
     * declare a symbol that will be disposed at the end of this file instead of module
     */
    inline void declare_file_disposable(std::string &name, ASTNode *node) {
        declare_file_disposable(chem::string_view(name.data(), name.size()), node);
    }

    /**
     * this is the ultimate declare function for a node (except top level functions)
     * it will declare the node according to the given specifier, private symbols disposed at the end of file
     * internal symbols disposed at the end of module, has_runtime dictates whether it's runtime_name will be declared
     */
    void declare_node(const chem::string_view& name, ASTNode* node, AccessSpecifier specifier, bool has_runtime);

    /**
     * this is the ultimate declare function for a node (except top level functions)
     * it will declare the node according to the given specifier, private symbols disposed at the end of file
     * internal symbols disposed at the end of module, has_runtime dictates whether it's runtime_name will be declared
     */
    inline void declare_node(std::string& name, ASTNode* node, AccessSpecifier specifier, bool has_runtime) {
        declare_node(chem::string_view(name.data(), name.size()), node, specifier, has_runtime);
    }

    /**
     * declare a exported symbol
     */
    void declare_exported(const chem::string_view &name, ASTNode *node) {
        declare_quietly(name, node);
    }

    /**
     * declare a runtime symbol
     */
    void declare_runtime(const chem::string_view& name, ASTNode* node);

    /**
     * declare an exported runtime symbol
     */
    void declare_exported_runtime(const chem::string_view& name, const chem::string_view& runtime_name, ASTNode* node);

    /**
     * symbol will be undeclared if present, otherwise error if error_out
     */
    bool undeclare(const chem::string_view& name);

    /**
     * symbol will be undecalred if present, only in the current scope
     */
    bool undeclare_in_current_file(const chem::string_view& name);

    /**
     * undeclare in current module
     */
    bool undeclare_in_current_module(const chem::string_view& name);

    /**
     * symbol will be undeclared in other files (not current file)
     * only a single symbol is undeclared
     */
    bool undeclare_in_scopes_above(const chem::string_view& name, int until);

    /**
     * symbol will be checked for duplicates in other files (not current file)
     * if a single symbol exists in other files, an dup sym error is created
     */
    bool dup_check_in_scopes_above(const chem::string_view& name, ASTNode* new_node, int until);

    /**
     * declare the function, without overloading, will NOT declare the function if a symbol already exists
     * @return the symbol that already exists, if no declaration took place
     */
    ASTNode** declare_function_no_overload(const chem::string_view& name, FunctionDeclaration* declaration);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names, it will declare function with access specifier
     * internal, which means function is visible to files in current module
     */
    void declare_function(const chem::string_view& name, FunctionDeclaration* declaration);

    /**
     * helper method that should be used to declare functions that are private
     */
    void declare_private_function(const chem::string_view& name, FunctionDeclaration* declaration);

    /**
     * this is the ultimate function that is used to declare functions, it will take into account
     * it's access specifier, it will also overload the function (in current scope, if feasible)
     */
    void declare_function(const chem::string_view& name, FunctionDeclaration* decl, AccessSpecifier specifier);

    /**
     * an exported function is declared using this method
     */
    void declare_exported_function(const chem::string_view& name, FunctionDeclaration* declaration) {
        declare_function_quietly(name, declaration);
    }

    /**
     * declare exported runtime function
     */
    void declare_exported_runtime_func(const chem::string_view& name, const chem::string_view& runtime_name, FunctionDeclaration* decl) {
        declare_function_quietly(name, decl);
        declare_runtime(runtime_name, (ASTNode*) decl);
    }

    /**
     * symbol resolves a file
     */
    void resolve_file(Scope& scope, const std::string& abs_path);

    /**
     * top level declare all the symbols in a file
     */
    SymbolRange tld_declare_file(Scope& scope, const std::string& abs_path);

    /**
     * link the signatures in the file
     */
    void link_signature_file(Scope& scope, const std::string& abs_path, const SymbolRange& range);

    /**
     * should be called, after tld_declare_file, if file's top level symbols have already been declared
     */
    void link_file(Scope& scope, const std::string& abs_path, const SymbolRange& range);

    /**
     * do not symbol resolve the file, just import it, it will just declare the symbols inside
     */
    void import_file(std::vector<ASTNode*>& nodes, const std::string_view& path, bool restrict_public);

    /**
     * enable file symbols for given scope index
     */
    void enable_file_symbols(const SymbolRange& range);

    /**
     * should be called after symbol resolving a single file
     * the passed absolute path is used to provide diagnostics only
     */
    void dispose_all_file_symbols(const std::string_view& abs_path);

    /**
     * provide a range of file symbols to dispose
     */
    void dispose_file_symbols_now(const std::string_view& abs_path, const SymbolRange& range);

    /**
     * should be called after symbol resolving a single module
     * the passed module name is used to provide diagnostics only
     */
    void dispose_module_symbols_now(const std::string& module_name);

    /**
     * error for when the value doesn't satisfy the requires type
     */
    void unsatisfied_type_err(Value* value, BaseType* type);

};