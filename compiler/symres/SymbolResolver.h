// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include "std/chem_string.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/ASTAllocator.h"
#include "compiler/symres/SymbolTable.h"
#include "compiler/symres/SymbolRange.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "compiler/generics/InstantiationsContainer.h"

class ASTNode;

class AnnotableNode;

class FunctionType;

class AccessChain;

class FunctionDeclaration;

class ImportPathHandler;

class Scope;

class GlobalInterpretScope;

class ChainValue;

class VariableIdentifier;

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
#ifdef DEBUG
private:
    /**
     * stored during debug only
     */
    chem::string debug_name;
#endif
public:

    /**
     * the symbol view
     */
    chem::string_view symbol;

    /**
     * the node symbol is pointing to
     */
    ASTNode* node;

#ifdef DEBUG
    /**
     * constructor
     */
    inline SymbolRef(
            chem::string_view symbol,
            ASTNode* node
    ) : symbol(symbol), node(node), debug_name(symbol) {

    }
#else
    /**
     * constructor
     */
    inline SymbolRef(
        chem::string_view symbol,
        ASTNode* node
    ) : symbol(symbol), node(node) {

    }
#endif


};

class CTranslator;

/**
 * SymbolResolver is responsible for linking symbols, every symbol can declare itself
 * on a map and another node can lookup this symbol, generate an error if not found
 */
class SymbolResolver : public ASTDiagnoser {
private:

    /**
     * the symbol table is the one used to store symbols inside
     */
    SymbolTable table;

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
    bool overload_function(const chem::string_view& name, ASTNode* const previous, FunctionDeclaration* declaration);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names
     * @return true if a new symbol was declared
     */
    inline bool declare_function_quietly(const chem::string_view& name, FunctionDeclaration* declaration) {
        const auto previous = table.declare_no_shadow_sym(name, (ASTNode*) declaration);
        if(previous == nullptr) {
            return true;
        } else {
            if(table.is_in_current_scope(previous)) {
                overload_function(name, previous->activeNode, declaration);
                return false;
            } else {
                dup_sym_error(name, previous->activeNode, (ASTNode*) declaration);
                // shadow the symbol
                table.declare(name, (ASTNode*) declaration);
                return true;
            };
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
     * a reference to path handler allows resolution of paths in import statements during
     * symbol resolution
     */
    ImportPathHandler& path_handler;

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
     * this contains the generic instantiations
     */
    InstantiationsContainer& instantiations_container;

    /**
     * the generic instantiator, that instantiates concrete generic implementations
     */
    GenericInstantiatorAPI genericInstantiator;

    /**
     * implicit arguments are provided using provide statements
     * we track these using this unordered map
     */
    std::unordered_map<chem::string_view, Value*> implicit_args;

    /**
     * the files we've declared
     */
    std::unordered_map<chem::string_view, Scope&> declared_files;

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
     * this is only enabled during symbol resolution of a generic container (function or struct)
     * this allows us to postpone creation of types for variables that could be generic, because
     * generic containers are instantiated by GenericInstantiator, which also takes the responsibility
     * of doing this
     */
    bool generic_context = false;

    /**
     * this is made true, when linking signature of a file, during link signature, values
     * shouldn't be moved, create_type shouldn't be called on values because they may haven't
     * been linked yet
     */
    bool linking_signature = false;

    /**
     * current function type, for which code is being linked
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * instead of declaring the file symbols right when they are found, we store the file symbols
     * and when we link the file, we take symbol range (start and end index) of the private symbols in this vector
     * and enable them in a nested file scope, which ends after linking the file dropping the private symbols
     */
    std::vector<SymbolRef> stored_file_symbols;

    /**
     * constructor
     */
    SymbolResolver(
            GlobalInterpretScope& global,
            ImportPathHandler& handler,
            InstantiationsContainer& container,
            bool is64Bit,
            ASTAllocator& allocator,
            ASTAllocator* modAllocator,
            ASTAllocator* astAllocator
    );

    /**
     * change the ast allocator to given allocator
     */
    void setASTAllocator(ASTAllocator& newASTAllocator) {
        ast_allocator = &newASTAllocator;
        genericInstantiator.setAllocator(newASTAllocator);
    }

    /**
     * global scope start
     */
    inline void global_scope_start() {
        table.scope_start(SymResScopeKind::Global);
    }

    /**
     * a module scope begins, a module scope doesn't contain no symbols
     * we'll improve the design so that module scopes don't introduce any overhead
     * of performance and memory
     * a module is just there to indicate that a module exists, it helps us delete symbols
     * only in a specific module
     */
    inline int module_scope_start() {
        return table.scope_start_index(SymResScopeKind::Module);
    }

    /**
     * a file scope begins, a file scope should not be popped, this is because
     * symbols are expected to exist in other files
     */
    inline int file_scope_start() {
        return table.scope_start_index(SymResScopeKind::File);
    }

    /**
     * when a scope begins, this should be called
     * it would put a scope on current vector
     */
    inline void scope_start() {
        table.scope_start(SymResScopeKind::Default);
    }

    /**
     * get symbol scope at index
     */
    [[nodiscard]]
    inline const SymbolScope* get_scope_at_index(int index) const noexcept {
        return table.get_scope_at_index(index);
    }

    /**
     * get the symbols vector
     */
    [[nodiscard]]
    const std::vector<SymbolEntry>& get_symbols() const noexcept {
        return table.get_symbols();
    }

    /**
     * ends the scope, keeps the symbol entries so they can imported later
     */
    inline void file_scope_end(int scope_index) {
        table.drop_all_scopes_from(scope_index);
    }

    /**
     * ends the scope, keeps the symbol entries so they can imported later
     */
    inline void module_scope_end(int scope_index) {
        table.drop_all_scopes_from(scope_index);
    }

    /**
     * when a scope ends, this should be called
     * it would pop a scope map from the current vector
     */
    inline void scope_end() {
        table.scope_end();
    }

    /**
     * find a symbol on current symbol map
     */
    ASTNode* find(const chem::string_view& name) {
        return table.resolve(name);
    }

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
        return table.get_last_scope_kind() == SymResScopeKind::File;
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
     * declares a symbol for which entry already exists
     */
    inline void declare_entry(const SymbolEntry* entry, int index) {
        table.declare_entry(entry, index);
    }

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
     * this will link the given body sequentially, backing the moved identifiers and chains
     * into the given vectors, which you can restore later
     */
    void link_body_seq_backing_moves(
            Scope& scope,
            std::vector<VariableIdentifier*>& moved_ids,
            std::vector<AccessChain*>& moved_chains
    );

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
     * declare and link file in one shot, the symbols inside the file are erased
     */
    void declare_and_link_file(Scope& scope, const std::string& abs_path);

    /**
     * do not symbol resolve the file, just import it, it will just declare the symbols inside
     */
    void import_file(std::vector<ASTNode*>& nodes, const std::string_view& path, bool restrict_public);

    /**
     * enable file symbols for given scope index
     */
    void enable_file_symbols(const SymbolRange& range);

    /**
     * error for when the value doesn't satisfy the requires type
     */
    void unsatisfied_type_err(Value* value, BaseType* type);

    /**
     * will clear the symbol resolver, to make it ready for another compilation
     */
    void clear() {
        table.clear();
        declared_files.clear();
        stored_file_symbols.clear();
        current_func_type = nullptr;
    }

};