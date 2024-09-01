// Copyright (c) Qinetik 2024.

#pragma once

#include "preprocess/BaseSymbolResolver.h"
#include "ASTDiagnoser.h"
#include <memory>
#include <ordered_map.h>

class ASTNode;

class FunctionType;

class FunctionDeclaration;

class Scope;

enum class SymResScopeKind : uint8_t {
    /**
     * a default scope, symbols can be declared and retrieved
     * the retrieval takes into account symbols declared in scopes above
     */
    Default,
    /**
     * a global namespace is much like default scope, however
     * only a single global scope exists, It's never created by the user
     */
    Global,
    /**
     * a file scope is just like a default scope, however with minor
     * differences, a file scope helps also to distinguish between files
     */
    File
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
    std::vector<SymResScope> current = {{ SymResScopeKind::Global }};

public:

    /**
     * a file scope begins, a file scope should not be popped, this is because
     * symbols are expected to exist in other files
     */
    void file_scope_start() {
        current.emplace_back(SymResScopeKind::File);
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
    std::vector<std::string_view> dispose_file_symbols;

    /**
     * constructor
     */
    explicit SymbolResolver(bool is64Bit);

    /**
     * if the current where the symbols are being declared is a file scope
     */
    bool is_current_file_scope() {
        return current.back().kind == SymResScopeKind::File;
    }

    /**
     * duplicate symbol error
     */
    void dup_sym_error(const std::string& name, ASTNode* previous, ASTNode* new_node);

    /**
     * declares a node with string : name
     */
    void declare(const std::string &name, ASTNode *node);

    /**
     * symbol will be undeclared if present, otherwise error if error_out
     */
    bool undeclare(const std::string_view& name);

    /**
     * symbol will be undeclared in other files (not current file)
     * only a single symbol is undeclared
     */
    bool undeclare_in_other_files(const std::string& name);

    /**
     * symbol will be checked for duplicates in other files (not current file)
     * if a single symbol exists in other files, an dup sym error is created
     */
    bool dup_check_in_other_files(const std::string& name, ASTNode* new_node);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names
     */
    void declare_function(const std::string& name, FunctionDeclaration* declaration);

    /**
     * symbol resolves a file
     */
    void resolve_file(Scope& scope, const std::string& abs_path);

    /**
     * should be called after symbol resolving a single file
     * the passed absolute path is used to provide diagnostics only
     */
    void dispose_file_symbols_now(const std::string& abs_path);

};