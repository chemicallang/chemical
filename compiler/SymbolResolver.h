// Copyright (c) Qinetik 2024.

#pragma once

#include "preprocess/BaseSymbolResolver.h"
#include "ASTDiagnoser.h"
#include "ast/base/AccessSpecifier.h"
#include <memory>
#include <ordered_map.h>

class ASTNode;

class FunctionType;

class AccessChain;

class FunctionDeclaration;

class Scope;

class GlobalInterpretScope;

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
    std::unordered_map<std::string_view, ASTNode*> symbols;

};

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
    std::vector<std::unique_ptr<SymResScope>> current;

    /**
     * runtime symbols that are checked for conflicts with other nodes
     * across modules, only at top level nodes
     */
    std::unordered_map<std::string, ASTNode*> runtime_symbols;

    /**
     * declares a node with string : name
     * DO NOT USE THIS FUNCTION TO DECLARE SYMBOLS
     */
    void declare_quietly(const std::string &name, ASTNode *node);

    /**
     * will try to override this function, notice the '&' in the previous pointer
     * if successfully overridden, it will modify the previous location inside the symbol table
     * to be this declaration and return true
     */
    bool overload_function(const std::string& name, ASTNode*& previous, FunctionDeclaration* declaration);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names
     * @return true if a new symbol was declared
     */
    bool declare_function_quietly(const std::string& name, FunctionDeclaration* declaration);

public:

    /**
     * a reference to global interpret scope is required
     * which helps to resolve conditions inside compile time if statements
     * to link code
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * a file scope begins, a file scope should not be popped, this is because
     * symbols are expected to exist in other files
     */
    void file_scope_start() {
        current.emplace_back(new SymResScope(SymResScopeKind::File));
    }

    /**
     * a module scope begins, a module scope doesn't contain no symbols
     * we'll improve the design so that module scopes don't introduce any overhead
     * of performance and memory
     * a module is just there to indicate that a module exists, it helps us delete symbols
     * only in a specific module
     */
    void module_scope_start() {
        current.emplace_back(new SymResScope(SymResScopeKind::Module));
    }

    /**
     * when a scope begins, this should be called
     * it would put a scope on current vector
     */
    void scope_start() {
        current.emplace_back(new SymResScope(SymResScopeKind::Default));
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
    ASTNode *find_in_current_file(const std::string& name);

    /**
     * find a symbol on current symbol map
     */
    ASTNode *find(const std::string& name);

    /**
     * is the codegen for arch 64bit
     */
    bool is64Bit;

    /**
     * when true, re-declaring same symbol will override it
     */
    bool override_symbols = false;

    /**
     * when preprocessing, during symbol resolution, after resolving certain values
     * we process them (replace them / modify them) for example implicit constructors work this way
     * when this flag is true, values that call implicit constructors are replaced with actual calls
     * to implicit constructors
     */
    bool preprocess = true;

    /**
     * current function type, for which code is being linked
     */
    FunctionType* current_func_type = nullptr;

    /**
     * nodes that are created during symbol resolution, that must be retained
     * during symbol resolution, for example when a function with two names
     * exists, the second function creates a MultiFunctionNode and put's it on this
     * vector, which is declared on the symbol map, when function calls link with it
     * they are re-linked with correct function, based on arguments
     */
    std::vector<std::unique_ptr<ASTNode>> helper_nodes;

    /**
     * When generating code for a file, we generate code sequentially for each node, even generics
     * We cannot code_gen upon detecting usage of generic node because it looses the context, the node may be present in
     * another node, which calls code_gen differently or sets some parameters before calling it
     *
     * If generic node in current file, we detect usage at symbol resolution, and generate code later sequentially
     * since we already know what types use the generic node
     *
     * when in another file, we only call code_gen on nodes present in the current file
     * so if a generic node imported from another file has a different use of type, we must now generate code for it
     *
     * since this generic node is not present in current file, when use of a different type is detected at symbol resolution
     * the node is put on this map, then before generating code for the file, we generate code for these nodes
     */
    tsl::ordered_map<ASTNode*, bool> imported_generic;

    /**
     * stores symbols that will be disposed after this file has been completely symbol resolved
     * for example using namespace some; this will always be disposed unless propagate annotation exists
     * above it
     */
    std::vector<std::pair<SymResScope*, std::string_view>> dispose_file_symbols;

    /**
     * stores symbols that will be disposed after this module has been completely symbol resolved
     * for example symbols that are internal in module are stored on this vector for disposing
     * at the end of module, struct without a public keyword (internal by default)
     */
    std::vector<std::pair<SymResScope*, std::string_view>> dispose_module_symbols;

    /**
     * constructor
     */
    SymbolResolver(GlobalInterpretScope& global, bool is64Bit);

    /**
     * if the current where the symbols are being declared is a file scope
     */
    bool is_current_file_scope() {
        return current.back()->kind == SymResScopeKind::File;
    }

    /**
     * duplicate symbol error
     */
    void dup_sym_error(const std::string& name, ASTNode* previous, ASTNode* new_node);

    /**
     * duplicate runtime symbol error
     */
    void dup_runtime_sym_error(const std::string& name, ASTNode* previous, ASTNode* new_node);

    /**
     * declare a symbol that will disposed at the end of this module
     */
    void declare(const std::string &name, ASTNode *node);

    /**
     * declare a symbol that will be disposed at the end of this file instead of module
     */
    void declare_file_disposable(const std::string &name, ASTNode *node);

    /**
     * this is the ultimate declare function for a node (except top level functions)
     * it will declare the node according to the given specifier, private symbols disposed at the end of file
     * internal symbols disposed at the end of module, has_runtime dictates whether it's runtime_name will be declared
     */
    void declare_node(const std::string& name, ASTNode* node, AccessSpecifier specifier, bool has_runtime);

    /**
     * declare a exported symbol
     */
    void declare_exported(const std::string &name, ASTNode *node) {
        declare_quietly(name, node);
    }

    /**
     * declare a runtime symbol
     */
    void declare_runtime(const std::string& name, ASTNode* node);

    /**
     * declare an exported runtime symbol
     */
    void declare_exported_runtime(const std::string& name, const std::string& runtime_name, ASTNode* node);

    /**
     * symbol will be undeclared if present, otherwise error if error_out
     */
    bool undeclare(const std::string_view& name);

    /**
     * symbol will be undecalred if present, only in the current scope
     */
    bool undeclare_in_current_file(const std::string_view& name);

    /**
     * undeclare in current module
     */
    bool undeclare_in_current_module(const std::string_view& name);

    /**
     * symbol will be undeclared in other files (not current file)
     * only a single symbol is undeclared
     */
    bool undeclare_in_scopes_above(const std::string_view& name, int until);

    /**
     * symbol will be checked for duplicates in other files (not current file)
     * if a single symbol exists in other files, an dup sym error is created
     */
    bool dup_check_in_scopes_above(const std::string& name, ASTNode* new_node, int until);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names, it will declare function with access specifier
     * internal, which means function is visible to files in current module
     */
    void declare_function(const std::string& name, FunctionDeclaration* declaration);

    /**
     * helper method that should be used to declare functions that are private
     */
    void declare_private_function(const std::string& name, FunctionDeclaration* declaration);

    /**
     * this is the ultimate function that is used to declare functions, it will take into account
     * it's access specifier, it will also overload the function (in current scope, if feasible)
     */
    void declare_function(const std::string& name, FunctionDeclaration* decl, AccessSpecifier specifier);

    /**
     * an exported function is declared using this method
     */
    void declare_exported_function(const std::string& name, FunctionDeclaration* declaration) {
        declare_function_quietly(name, declaration);
    }

    /**
     * declare exported runtime function
     */
    void declare_exported_runtime_func(const std::string& name, const std::string& runtime_name, FunctionDeclaration* decl) {
        declare_function_quietly(name, decl);
        declare_runtime(runtime_name, (ASTNode*) decl);
    }

    /**
     * symbol resolves a file
     */
    void resolve_file(Scope& scope, const std::string& abs_path);

    /**
     * should be called after symbol resolving a single file
     * the passed absolute path is used to provide diagnostics only
     */
    void dispose_file_symbols_now(const std::string& abs_path);

    /**
     * should be called after symbol resolving a single module
     * the passed module name is used to provide diagnostics only
     */
    void dispose_module_symbols_now(const std::string& module_name);

    /**
     * an access is found that partially matches the given access chain
     */
    AccessChain* find_partially_matching_moved_chain(AccessChain* chain);

    /**
     * an access chain inside the current function, is said to be moved
     */
    bool mark_moved(AccessChain* chain);

};