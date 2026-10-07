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

    /** Forward-declare struct/union tags for a top-level node. */
    void fwd_declare(BufferedWriter& w, ASTNode* node);

    /** Emit `typedef ...` declarations for a top-level node. */
    void declare_type_alias(BufferedWriter& w, ASTNode* node);

    /** Emit the full declaration of a top-level node (struct body, prototype...). */
    void declare(BufferedWriter& w, ASTNode* node);

    /** Emit the definitions owned by declarations (globals, vtables). */
    void implement(BufferedWriter& w, ASTNode* node);

    /** Emit stub implementations for unimplemented static interfaces. */
    void end_translate(BufferedWriter& w);

    /** Mangled C name of a node. */
    std::string mangle_name(ASTNode* node);

    /** C prototype spelling of a function declaration (no trailing `;`). */
    std::string function_proto(FunctionDeclaration* decl);

private:
    // --- declare-pass helpers ---
    void declare_contained_func(BufferedWriter& w, FunctionDeclaration* decl, bool terminate);
    void declare_struct_iterations(BufferedWriter& w, StructDefinition* def);
    void declare_union_iterations(BufferedWriter& w, UnionDef* def);
    void declare_variant_iterations(BufferedWriter& w, VariantDefinition* def);
    void declare_struct_def_only(BufferedWriter& w, StructDefinition* def);
    void declare_union_def_only(BufferedWriter& w, UnionDef* def);
    void declare_variant_def_only(BufferedWriter& w, VariantDefinition* def);
    void declare_impl(BufferedWriter& w, ImplDefinition* def);
    void declare_interface(BufferedWriter& w, InterfaceDefinition* def);
    void declare_var(BufferedWriter& w, VarInitStatement* init);
    void declare_typedef(BufferedWriter& w, TypealiasStatement* stmt);

    void early_declare_node(BufferedWriter& w, ASTNode* node);
    void early_declare_composed_variables(BufferedWriter& w, VariablesContainer& container);
    void early_declare_type(BufferedWriter& w, BaseType* type, ASTNode* container,
                            bool do_ref = false, bool do_ptr = false);

    // --- vtable helpers ---
    void vtable_type_name(BufferedWriter& w, InterfaceDefinition* interface);
    void vtable_name(BufferedWriter& w, InterfaceDefinition* interface,
                     ExtendableMembersContainerNode* def);
    void create_vtable_type(BufferedWriter& w, InterfaceDefinition* interface);
    void declare_vtable_extern(BufferedWriter& w, InterfaceDefinition* interface,
                               ExtendableMembersContainerNode* def);
    void create_vtable(BufferedWriter& w, InterfaceDefinition* interface,
                       ExtendableMembersContainerNode* def);
    void declare_vtable_primitive_extern(BufferedWriter& w, InterfaceDefinition* interface,
                                         BaseType* impl);
    void create_vtable_primitive(BufferedWriter& w, InterfaceDefinition* interface,
                                 ImplDefinition* def);

    // --- type/value spelling ---
    std::string type_spelling(BaseType* type);
    std::string decl_spelling(BaseType* type, const std::string& name);
    TypeId function_type_of(FunctionDeclaration* decl);
    std::string const_value_expr(Value* value);
    std::string struct_default_expr(StructDefinition* sd);
    void write_escaped_string(BufferedWriter& w, const std::string& s);

    /** Emit one struct/union member (including anonymous aggregates). */
    void emit_member(BufferedWriter& w, BaseDefMember* member, int indent);

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

    /** Containers whose early-declare walk is on the stack (cycle guard). */
    std::unordered_set<ASTNode*> early_declare_in_progress_;
};

} // namespace mir
