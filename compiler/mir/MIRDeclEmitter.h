// Copyright (c) Chemical Language Foundation 2025.
//
// MIR declaration emitter: emits the C declarations that the legacy 2c visitor
// used to produce — the fixed preamble, forward declarations, type aliases,
// struct/union/variant bodies, function prototypes, global declarations and
// interface vtables. Function bodies are emitted by MIR (see MIRLowerer /
// MIREmitter); this file owns everything that is not a body.
//
// It includes AST headers (like MIRTypeBuilder) because it walks declarations;
// the core MIR headers stay AST-free.

#pragma once

#include "MIRModule.h"
#include "MIRDeclPrinter.h"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

class ASTNode;
class ASTAllocator;
class BaseDefMember;
class BaseType;
class BufferedWriter;
class CompilerBinder;
class EmbeddedNode;
class ExtendableMembersContainerNode;
class FunctionDeclaration;
class GlobalInterpretScope;
class ImplDefinition;
class InterfaceDefinition;
class LocationManager;
class NameMangler;
class StructDefinition;
class TypealiasStatement;
class UnionDef;
class Value;
class VarInitStatement;
class VariablesContainer;
class VariantDefinition;

namespace mir {

class MIRTypeBuilder;

class MIRDeclEmitter {
public:
    MIRDeclEmitter(MIRModule& module, MIRTypeBuilder& types, NameMangler& mangler,
                   LocationManager& loc_man, GlobalInterpretScope& comptime_scope,
                   CompilerBinder& binder, ASTAllocator& allocator)
        : module_(module), types_(types), mangler_(mangler), loc_man_(loc_man),
          comptime_scope_(comptime_scope), binder_(binder), allocator_(allocator) {}

    /** Callback used to resolve `EmbeddedNode` declaration replacements. */
    void set_embedded_decl_repl(std::function<ASTNode*(EmbeddedNode*)> fn) {
        embedded_decl_repl_ = std::move(fn);
    }

    /** Callback used to record unimplemented static interfaces for stub generation. */
    void set_static_interface_recorder(std::function<void(InterfaceDefinition*)> fn) {
        static_recorder_ = std::move(fn);
    }

    /** Callback used to fold a global initializer to a compile-time constant. */
    void set_value_eval(std::function<Value*(Value*)> fn) { value_eval_ = std::move(fn); }

    /** Emit the fixed preamble (typedefs, malloc/free, macros). */
    void emit_preamble(BufferedWriter& w);

    // The declare pass *records* declarations into `decl_list_` (it still walks
    // the AST — it is the collection pass); `print_decl_list` renders them with
    // the AST-free `MIRDeclPrinter::decl_list_c` (action A3).
    /** Forward-declare struct/union tags for a top-level node. */
    void fwd_declare(ASTNode* node);
    /** Record `typedef ...` declarations for a top-level node. */
    void declare_type_alias(ASTNode* node);
    /** Record the full declaration of a top-level node (struct body, prototype...). */
    void declare(ASTNode* node);
    /** Print the recorded declaration list. */
    void print_decl_list(BufferedWriter& w);
    const MIRDeclList& decl_list() const { return decl_list_; }

    /** Emit the definitions owned by declarations (globals, vtables). */
    void implement(BufferedWriter& w, ASTNode* node);

    /**
     * Emit the definition of a top-level global from MIR (action A3): the
     * initializer is the constant-expression tree `init` produced by
     * `MIRLowerer::build_global_initializer`, printed by `const_expr_c`. No AST
     * value walk happens here.
     */
    void implement_global(BufferedWriter& w, ASTNode* node, ConstExprId init);

    /** Emit stub implementations for unimplemented static interfaces. */
    void end_translate(BufferedWriter& w);

    /** Mangled C name of a node. */
    std::string mangle_name(ASTNode* node);

    /** C prototype spelling of a function declaration (no trailing `;`). */
    std::string function_proto(FunctionDeclaration* decl);

private:
    // --- declare-pass helpers (record into decl_list_) ---
    void declare_contained_func(FunctionDeclaration* decl, bool terminate);
    void declare_struct_iterations(StructDefinition* def);
    void declare_union_iterations(UnionDef* def);
    void declare_variant_iterations(VariantDefinition* def);
    void declare_struct_def_only(StructDefinition* def);
    void declare_union_def_only(UnionDef* def);
    void declare_variant_def_only(VariantDefinition* def);
    void declare_impl(ImplDefinition* def);
    void declare_interface(InterfaceDefinition* def);
    void declare_var(VarInitStatement* init);
    void declare_typedef(TypealiasStatement* stmt);

    void early_declare_node(ASTNode* node);
    void early_declare_composed_variables(VariablesContainer& container);
    void early_declare_type(BaseType* type, ASTNode* container,
                            bool do_ref = false, bool do_ptr = false);

    // --- vtable helpers ---
    std::string vtable_type_name(InterfaceDefinition* interface);
    std::string vtable_name(InterfaceDefinition* interface,
                            ExtendableMembersContainerNode* def);
    void create_vtable_type(InterfaceDefinition* interface);
    void declare_vtable_extern(InterfaceDefinition* interface,
                               ExtendableMembersContainerNode* def);
    void create_vtable(BufferedWriter& w, InterfaceDefinition* interface,
                       ExtendableMembersContainerNode* def);
    void declare_vtable_primitive_extern(InterfaceDefinition* interface, BaseType* impl);
    void create_vtable_primitive(BufferedWriter& w, InterfaceDefinition* interface,
                                 ImplDefinition* def);

    // --- type/value spelling ---

    /**
     * Record the C body of an aggregate declaration (`struct`/`union`/`variant`)
     * — see MIRDeclPrinter. `what` names the construct for the (unreachable)
     * diagnostic comment.
     */
    void declare_aggregate_body(BaseType* type, const char* what);

    // --- state ---
    MIRModule& module_;
    MIRTypeBuilder& types_;
    NameMangler& mangler_;
    LocationManager& loc_man_;
    GlobalInterpretScope& comptime_scope_;
    CompilerBinder& binder_;
    ASTAllocator& allocator_;

    std::function<ASTNode*(EmbeddedNode*)> embedded_decl_repl_;
    std::function<void(InterfaceDefinition*)> static_recorder_;
    std::function<Value*(Value*)> value_eval_;

    /** The declaration pass output (see MIRDeclList.h). */
    MIRDeclList decl_list_;

    /** Containers whose early-declare walk is on the stack (cycle guard). */
    std::unordered_set<ASTNode*> early_declare_in_progress_;
};

} // namespace mir
