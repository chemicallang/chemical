// Copyright (c) Chemical Language Foundation 2025.

#include "2cASTVisitor.h"
#include <memory>
#include <ostream>
#include <random>
#include <ranges>
#include <iostream>
#include "compiler/mangler/NameMangler.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Break.h"
#include "ast/statements/DestructStmt.h"
#include "2cBackendContext.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/types/DynamicType.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/If.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/LoopBlock.h"
#include "ast/types/LinkedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include "ast/types/AnyType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/Int128Type.h"
#include "ast/types/IntNType.h"
#include "ast/types/IntType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/LongType.h"
#include "ast/types/ShortType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/UIntType.h"
#include "ast/types/ComplexType.h"
#include "ast/types/ULongType.h"
#include "ast/types/UShortType.h"
#include "ast/types/VoidType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/values/UShortValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IntValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/BlockValue.h"
#include "ast/values/IncDecValue.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/ValueNode.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/UCharValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/ExtractionValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/Int128Value.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/NullValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/NewTypedValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/IsValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ULongValue.h"
#include "preprocess/utils/RepresentationUtils.h"
#include "ast/utils/ASTUtils.h"
#include <sstream>
#include "CValueDeclVisitor.h"
#include "CBeforeStmtVisitor.h"
#include "CAfterStmtVisitor.h"
#include "ast/structures/InitBlock.h"

ToCAstVisitor::ToCAstVisitor(
    GlobalInterpretScope& scope,
    NameMangler& mangler,
    std::ostream *output,
    ASTAllocator& allocator,
    LocationManager& manager
) : comptime_scope(scope), mangler(mangler), output(output), allocator(allocator), declarer(new CValueDeclarationVisitor(*this)),
    tld(*this, declarer.get()), ASTDiagnoser(manager)
{
    before_stmt = std::make_unique<CBeforeStmtVisitor>(*this);
    after_stmt = std::make_unique<CAfterStmtVisitor>(*this);
    destructor = std::make_unique<CDestructionVisitor>(*this);
}

void ToCAstVisitor::declare_before_translation(std::vector<ASTNode*>& nodes) {
    // declare the top level things with this visitor
    for(const auto node : nodes) {
        tld.visit(node);
    }
}

void ToCAstVisitor::translate_after_declaration(std::vector<ASTNode*>& nodes) {
    // take out values like lambda from within functions
    for(const auto node : nodes) {
        declarer->visit(node);
    }
    // writing
    for(const auto node : nodes) {
        visit(node);
    }
}

void ToCAstVisitor::fwd_declare(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::NamespaceDecl:
            for(const auto child : node->as_namespace_unsafe()->nodes) {
                fwd_declare(child);
            }
            break;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
            new_line();
            write("struct ");
            mangle(node);
            write(';');
            break;
        case ASTNodeKind::UnionDecl:
            new_line();
            write("union ");
            mangle(node);
            write(';');
            break;
        case ASTNodeKind::GenericStructDecl:{
            const auto decl = node->as_gen_struct_def_unsafe();
            for(const auto inst : decl->instantiations) {
                fwd_declare(inst);
            }
            break;
        }
        case ASTNodeKind::GenericVariantDecl:{
            const auto decl = node->as_gen_variant_decl_unsafe();
            for(const auto inst : decl->instantiations) {
                fwd_declare(inst);
            }
            break;
        }
        case ASTNodeKind::GenericUnionDecl:{
            const auto decl = node->as_gen_union_decl_unsafe();
            for(const auto inst : decl->instantiations) {
                fwd_declare(inst);
            }
            break;
        }
        default:
            break;
    }
}

void ToCAstVisitor::declare_type_alias(ASTNode* node) {
    auto& visitor = *this;
    switch(node->kind()) {
        case ASTNodeKind::TypealiasStmt:{
            tld.VisitTypealiasStmt(node->as_typealias_unsafe());
            break;
        }
        case ASTNodeKind::GenericTypeDecl: {
            for(const auto child : node->as_gen_type_decl_unsafe()->instantiations) {
                tld.VisitTypealiasStmt(child->as_typealias_unsafe());
            }
            break;
        }
        case ASTNodeKind::NamespaceDecl:
            for(const auto child : node->as_namespace_unsafe()->nodes) {
                declare_type_alias(child);
            }
            break;
        case ASTNodeKind::IfStmt: {
            const auto stmt = node->as_if_stmt_unsafe();
            if (stmt->computed_scope.has_value()) {
                auto scope = stmt->computed_scope.value();
                if (scope) {
                    for (const auto child: scope->nodes) {
                        declare_type_alias(child);
                    }
                }
                return;
            }
            break;
        }
        default:
            break;
    }
}

void ToCAstVisitor::fwd_declare(BaseType* type) {
    auto& visitor = *this;
    switch(type->kind()) {
        case BaseTypeKind::Reference:
            fwd_declare(type->as_reference_type_unsafe()->type);
            return;
        case BaseTypeKind::Pointer:
            fwd_declare(type->as_pointer_type_unsafe()->type);
            return;
        case BaseTypeKind::Linked:
            fwd_declare(type->as_linked_type_unsafe()->linked);
            return;
        case BaseTypeKind::Dynamic:
            fwd_declare(type->as_dynamic_type_unsafe()->referenced);
            return;
        case BaseTypeKind::Array:
            fwd_declare(type->as_array_type_unsafe()->elem_type);
            return;
        case BaseTypeKind::Generic:
            fwd_declare(type->as_generic_type_unsafe()->referenced->linked);
            for(const auto ty : type->as_generic_type_unsafe()->types) {
                fwd_declare(ty);
            }
            return;
        default:
            return;
    }
}

void ToCAstVisitor::external_declare(std::vector<ASTNode*>& nodes) {
    auto& vis = tld;
    auto prev = vis.external_module;
    vis.external_module = true;
    // declare the top level things with this visitor
    for(auto& node : nodes) {
        vis.visit(node);
    }
    vis.external_module = prev;
}

void ToCAstVisitor::external_implement(std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::GenericFuncDecl:
                VisitGenericFuncDecl(node->as_gen_func_decl_unsafe());
                break;
            case ASTNodeKind::GenericStructDecl:
                VisitGenericStructDecl(node->as_gen_struct_def_unsafe());
                break;
            case ASTNodeKind::GenericUnionDecl:
                VisitGenericUnionDecl(node->as_gen_union_decl_unsafe());
                break;
            case ASTNodeKind::GenericInterfaceDecl:
                VisitGenericInterfaceDecl(node->as_gen_interface_decl_unsafe());
                break;
            case ASTNodeKind::GenericVariantDecl:
                VisitGenericVariantDecl(node->as_gen_variant_decl_unsafe());
                break;
            case ASTNodeKind::NamespaceDecl:
                external_implement(node->as_namespace_unsafe()->nodes);
                break;
            default:
                break;
        }
    }
}

class ToCAstVisitor;

// will write a scope to visitor
void scope_no_parens(ToCAstVisitor& visitor, Scope& scope) {
    visitor.indentation_level+=1;
    visitor.visit(&scope);
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
}

// will write a scope to visitor
void scope(ToCAstVisitor& visitor, Scope& scope) {
    visitor.write('{');
    scope_no_parens(visitor, scope);
    visitor.write('}');
}

void write_encoded(ToCAstVisitor& visitor, const chem::string_view& value) {
    auto& out = *visitor.output;
    for(char c : value) {
        write_escape_encoded(out, c);
    }
}

void write_type_post_id(ToCAstVisitor& visitor, BaseType* type) {
    if(type->kind() == BaseTypeKind::Array) {
        visitor.write('[');
        auto arrType = ((ArrayType*) type);
        if(arrType->has_array_size()) {
            // TODO write to stream directly, do not convert to string
            visitor.write_str(std::to_string(arrType->get_array_size()));
        }
        visitor.write(']');
        if(arrType->elem_type->kind() == BaseTypeKind::Array) {
            write_type_post_id(visitor, arrType->elem_type);
        }
    }
}

#define struct_passed_param_name "__chx_struct_ret_param_xx"
#define static_interface_passed_param_name "__chx_interface_self"
#define fn_call_struct_var_name "chx_fn_cl_struct"

// without the parent node name
static void struct_name(ToCAstVisitor& visitor, ExtendableMembersContainerNode* def) {
    visitor.mangler.mangle_no_parent(*visitor.output, def);
}

void node_name(ToCAstVisitor& visitor, ASTNode* node) {
    if(!node) return;
    visitor.mangler.mangle(*visitor.output, node);
}

// nodes inside namespaces for example namespace name is written before their name
void node_parent_name(ToCAstVisitor& visitor, ASTNode* node, bool take_parent = true) {
    auto current = take_parent ? (node ? node->parent() : nullptr) : node;
    if(current) {
        visitor.mangler.mangle(*visitor.output, current);
    }
}

void func_type_with_id(ToCAstVisitor& visitor, FunctionType* type, const chem::string_view& id);

void type_with_id(ToCAstVisitor& visitor, BaseType* type, const chem::string_view& id) {
    const auto func_type = type->as_function_type();
    if(func_type != nullptr && !func_type->isCapturing()) {
        func_type_with_id(visitor, func_type, id);
    } else {
        visitor.visit(type);
        if(!id.empty() && id != "_") {
            visitor.space();
            visitor.write(id);
        }
        write_type_post_id(visitor, type);
    }
}

void param_type_with_id(ToCAstVisitor& visitor, BaseType* type, const chem::string_view& id) {
    if(type->kind() != BaseTypeKind::Dynamic && type->isStructLikeType()) {
        PointerType ptr_type(type, true);
        type_with_id(visitor, &ptr_type, id);
    } else {
        type_with_id(visitor, type, id);
    }
}

void write_struct_return_param(ToCAstVisitor& visitor, FunctionType* decl) {
    const auto func = decl->as_function();
    if(func && func->is_constructor_fn()) {
        visitor.write("struct ");
        visitor.mangle(func->parent());
        visitor.write("* ");
        visitor.write("this");
    } else {
        visitor.visit(decl->returnType);
        visitor.write("* ");
        visitor.write(struct_passed_param_name);
    }
}

// don't know what to call '(' struct 'name' ')'
void write_struct_def_value_call(ToCAstVisitor& visitor, StructDefinition* def) {
    visitor.write('(');
    visitor.write("struct ");
    visitor.mangle(def);
    visitor.write(')');
}

void func_type_params(ToCAstVisitor& visitor, FunctionType* decl, unsigned i = 0, bool has_params_before = false) {
    auto is_struct_return = visitor.pass_structs_to_initialize && decl->returnType->isStructLikeType();
    auto func = decl->as_function();
    if((is_struct_return || (func && func->is_constructor_fn())) && !(func && func->is_copy_fn())) {
        if(has_params_before) {
            visitor.write(", ");
        }
        write_struct_return_param(visitor, decl);
        has_params_before = true;
    }
    FunctionParam* param;
    auto size = decl->isVariadic() ? decl->params.size() - 1 : decl->params.size();
    while(i < size) {
        param = decl->params[i];
        if(has_params_before) {
            visitor.write(", ");
        }
        param_type_with_id(visitor, param->type, param->name);
        has_params_before = true;
        i++;
    }
    if(decl->isVariadic()) {
        if(has_params_before) {
            visitor.write(", ");
        }
        visitor.write("...");
    }
}

void accept_func_return(ToCAstVisitor& visitor, BaseType* type) {
    if(visitor.pass_structs_to_initialize && type->isStructLikeType()) {
        visitor.write("void");
    } else {
        visitor.visit(type);
    }
}

// func_type is declared with it's return type and name
// the last take_parent allows to skip appending one direct parent name, useful
// when the interface name is to be used, so interface appends the name in given name parameter
// take_parent is true, so this function skips direct parent but grandparents and other names are appended
void accept_func_return_with_name(ToCAstVisitor& visitor, FunctionType* func_type, const chem::string_view& name, bool is_static) {
    if(is_static) {
        visitor.write("static ");
    }
    accept_func_return(visitor, func_type->returnType);
    visitor.space();
    visitor.write(name);
}

void accept_func_return_with_name(ToCAstVisitor& visitor, FunctionDeclaration* func_decl, bool is_static) {
    if(func_decl->is_extern()) {
        visitor.write("extern ");
    }
    if(is_static) {
        visitor.write("static ");
    }
    accept_func_return(visitor, func_decl->returnType);
    visitor.space();
    visitor.mangle(func_decl);
}

void func_type_with_id_no_params(ToCAstVisitor& visitor, FunctionType* type, const chem::string_view& id) {
    accept_func_return(visitor, type->returnType);
    visitor.write('(');
    visitor.write('*');
    visitor.write(id);
    visitor.write(")(");
    if(type->isCapturing()) {
        visitor.write("void*");
        if(!type->params.empty()) {
            visitor.write(',');
        }
    }
}

void func_ptr_array_type(ToCAstVisitor& visitor, ArrayType* arrType, FunctionType* type, const chem::string_view& id) {
    accept_func_return(visitor, type->returnType);
    visitor.write('(');
    visitor.write('*');
    visitor.write(id);
    write_type_post_id(visitor, arrType);
    visitor.write(")(");
    if(type->isCapturing()) {
        visitor.write("void*");
        if(!type->params.empty()) {
            visitor.write(',');
        }
    }
    func_type_params(visitor, type);
    visitor.write(")");
}

void func_type_with_id(ToCAstVisitor& visitor, FunctionType* type, const chem::string_view& id) {
    func_type_with_id_no_params(visitor, type, id);
    func_type_params(visitor, type);
    visitor.write(")");
}

bool should_void_pointer_to_self(BaseType* type, const chem::string_view& id, unsigned index, bool overrides) {
    if(index == 0 && type->kind() == BaseTypeKind::Reference && ((ReferenceType*) type)->type->kind() == BaseTypeKind::Linked && (id == "self" || id == "this")) {
        if(((PointerType*) type)->type->linked_node()->as_interface_def() || (((PointerType*) type)->type->linked_node()->as_struct_def() && overrides)) {
            return true;
        }
    }
    return false;
}

void allocate_struct_by_name_no_init(ToCAstVisitor& visitor, ASTNode* def, const chem::string_view& name) {
    auto k = def->kind();
    if(k == ASTNodeKind::UnionDecl || k == ASTNodeKind::UnnamedUnion) {
        visitor.write("union ");
    } else {
        visitor.write("struct ");
    }
    visitor.mangle(def);
    visitor.write(' ');
    visitor.write(name);
}

//void param_type_with_id(ToCAstVisitor& visitor, BaseType* type, const std::string& id, unsigned index, bool overrides) {
//    if(should_void_pointer_to_self(type, id, index, overrides)) {
//        visitor->write("void* ");
//        visitor->write(id);
//        return;
//    }
//    type_with_id(visitor, type, id);
//}

BaseType* get_func_param_type(ASTNode* node) {
    if(node) {
        const auto param = node->as_func_param();
        return param ? param->type : nullptr;
    }
    return nullptr;
}

ASTNode* get_func_param_ref_node(ASTNode* node) {
    if(!node) return nullptr;
    auto param = node->as_func_param();
    if(!param) return nullptr;
    return param->type->get_direct_linked_node();
}

StructDefinition* get_func_param_ref_struct(ASTNode* node) {
    if(!node) return nullptr;
    auto param = node->as_func_param();
    if(!param) return nullptr;
    return param->type->get_direct_linked_struct();
}

inline void vtable_name(ToCAstVisitor& visitor, InterfaceDefinition* interface, StructDefinition* definition) {
    visitor.mangler.mangle_vtable_name(*visitor.output, interface, definition);
}

std::pair<InterfaceDefinition*, StructDefinition*> get_dyn_obj_impl(BaseType* type, Value* value) {
    if(!type) return {nullptr, nullptr};
    auto pure_type = type->canonical();
    if(pure_type->kind() == BaseTypeKind::Dynamic) {
        const auto dyn_type = ((DynamicType*) pure_type);
        const auto interface = dyn_type->linked_node()->as_interface_def();
        if(interface && type->isStructLikeType()) {
            const auto linked = value->known_type();
            const auto def = linked->linked_struct_def();
            if(def) {
                return { interface, def };
            }
        }
    }
    return {nullptr, nullptr};
}

// structs, or variants or references to them are passed in functions as pointers
// if you took address of using '&' of the parameter that is already reference or pointer
// we must not write '&' in the output C
bool is_value_param_hidden_pointer(Value* value) {
    const auto linked = value->linked_node();
    if(linked) {
        switch(linked->kind()) {
            case ASTNodeKind::FunctionParam:{
                const auto type = linked->as_func_param_unsafe()->type;
                if(type->is_reference()) {
                    return true;
                }
                return type->kind() != BaseTypeKind::Dynamic && type->isStructLikeType();
            }
            default:
                return false;
        }
    } else {
        return false;
    }
}

bool is_value_param_pointer_or_ref_like(Value* value) {
    const auto linked = value->linked_node();
    if(linked) {
        switch(linked->kind()) {
            case ASTNodeKind::FunctionParam:{
                const auto type = linked->as_func_param_unsafe()->type;
                if(type->is_pointer_or_ref()) {
                    return true;
                }
                return type->kind() != BaseTypeKind::Dynamic && type->isStructLikeType();
            }
            default:
                return false;
        }
    } else {
        return false;
    }
}

bool is_value_param_pointer_like(Value* value) {
    return is_value_param_pointer_or_ref_like(value);
}

// structs, or variants or references to them are passed in functions as pointers
// if you took address of using '&' of the parameter that is already reference or pointer
// we must not write '&' in the output C
bool is_value_type_pointer_like(Value* value) {
    const auto linked = value->linked_node();
    if(linked) {
        switch(linked->kind()) {
            case ASTNodeKind::FunctionParam:{
                const auto type = linked->as_func_param_unsafe()->type;
                if(type->is_pointer_or_ref()) {
                    return true;
                }
                return type->kind() != BaseTypeKind::Dynamic && type->isStructLikeType();
            }
            case ASTNodeKind::VarInitStmt: {
                const auto type = linked->as_var_init_unsafe();
                const auto known_ty = type->known_type();
                if(known_ty && known_ty->is_pointer_or_ref()) {
                    return true;
                }
                return false;
            }
            default:
                return false;
        }
    } else {
        return false;
    }
}

bool implicit_mutate_value_for_dyn_obj(ToCAstVisitor& visitor, BaseType* type, Value* value, void(*value_visit)(ToCAstVisitor& visitor, Value* value)) {
//    (__chemical_fat_pointer__){ &sm, (void*) &PhoneSmartPhone }
    auto dyn_obj = get_dyn_obj_impl(type, value);
    if(!dyn_obj.first) return false;
    visitor.write('(');
    visitor.write(visitor.fat_pointer_type);
    visitor.write(')');
    visitor.write('{');
    visitor.space();
    if(!is_value_param_pointer_like(value)) {
        visitor.write('&');
    }
    value_visit(visitor, value);
    visitor.write(',');
    visitor.write("(void*) &");
    vtable_name(visitor, dyn_obj.first, dyn_obj.second);
    visitor.space();
    visitor.write('}');
    return true;
}

//bool implicit_mutate_value(ToCAstVisitor& visitor, BaseType* type, Value* value, void(*value_visit)(ToCAstVisitor& visitor, Value* value)) {
//    return implicit_mutate_value_for_dyn_obj(visitor, type, value, value_visit);
//}

bool implicit_mutate_value_default(ToCAstVisitor& visitor, BaseType* type, Value* value) {
    return implicit_mutate_value_for_dyn_obj(visitor, type, value, [](ToCAstVisitor& visitor, Value* value) -> void {
        visitor.visit(value);
    });
}

// when a value is being stored or passed as a reference we must write address of it
// even if it means storing it in a temporary variable
// for example my_func(3) <-- here parameter takes a constant reference, which means r values are allowed
// so we must store 3 in a temporary variable and pass address of it
// this returns whether the value has already been written, should not be re-written
bool write_value_for_ref_type(ToCAstVisitor& visitor, Value* val, ReferenceType* ref_type) {
    if(Value::isValueKindRValue(val->val_kind())) {
        const auto temp_var = visitor.get_local_temp_var_name();
        visitor.write("({ ");
        visitor.visit(ref_type->type);
        visitor.write(' ');
        visitor.write_str(temp_var);
        visitor.write(" = ");
        visitor.visit(val);
        visitor.write("; &");
        visitor.write_str(temp_var);
        visitor.write("; })");
        return true;
    } else if(!is_value_param_pointer_like(val)){
        visitor.write('&');
        return false;
    }
    return false;
}

Value* evaluate_comptime_func(
        ToCAstVisitor& visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call
) {
    const auto prev = visitor.comptime_scope.current_func_type;
    visitor.comptime_scope.current_func_type = visitor.current_func_type;
    auto value = func_decl->call(&visitor.comptime_scope, visitor.allocator, call, nullptr, false);
    visitor.comptime_scope.current_func_type = prev;
    if(!value) {
        visitor.error("comptime function call didn't return anything", call);
        return nullptr;
    }
    auto eval_call = value->evaluated_value(visitor.comptime_scope);
    visitor.evaluated_func_calls[call] = eval_call;
    return eval_call;
}

Value* evaluated_func_val(
        ToCAstVisitor& visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call
) {
    Value* eval;
    auto found_eval = visitor.evaluated_func_calls.find(call);
    if(found_eval == visitor.evaluated_func_calls.end()) {
        eval = evaluate_comptime_func(visitor, func_decl, call);
    } else {
        eval = found_eval->second;
    }
    return eval;
}

inline Value* evaluated_func_val_proper(ToCAstVisitor& visitor, FunctionDeclaration* func_decl, FunctionCall* call) {
    return evaluated_func_val(visitor, func_decl, call);
}

void call_implicit_constructor_no_alloc(ToCAstVisitor& visitor, FunctionDeclaration* imp_constructor, Value* value, const chem::string_view& var_name, bool is_var_ptr) {
    const auto new_expected_type = imp_constructor->params[0]->type;
    visitor.mangle(imp_constructor);
    visitor.write("(");
    if(!is_var_ptr) {
        visitor.write('&');
    }
    visitor.write(var_name);
    visitor.write(", ");
    visitor.accept_mutating_value_explicit(new_expected_type, value, false);
    visitor.write(")");
}

void call_implicit_constructor_with_name(ToCAstVisitor& visitor, FunctionDeclaration* imp_constructor, Value* value, bool take_addr, const chem::string_view& var_name) {
    visitor.write("({");
    // TODO check if we need to destroy this so we should allocate it in before stmt visitor
    allocate_struct_by_name_no_init(visitor, imp_constructor->parent(), var_name);
    visitor.write("; ");
    call_implicit_constructor_no_alloc(visitor, imp_constructor, value, var_name, false);
    visitor.write("; ");
    if(take_addr) {
        visitor.write('&');
    }
    visitor.write(var_name);
    visitor.write("; })");
}

void call_implicit_constructor(ToCAstVisitor& visitor, FunctionDeclaration* imp_constructor, Value* value, bool take_addr) {
    const auto temp_name = visitor.get_local_temp_var_name();
    call_implicit_constructor_with_name(visitor, imp_constructor, value, take_addr, chem::string_view(temp_name.data(), temp_name.size()));
}

void ToCAstVisitor::accept_mutating_value_explicit(BaseType* type, Value* value, bool assigning_value) {
    // automatically passing address to a reference type
    if(!assigning_value && type && type->kind() == BaseTypeKind::Reference && !value->is_ref(allocator)) {
        const auto ref_type = type->as_reference_type_unsafe();
        if(write_value_for_ref_type(*this, value, ref_type)) {
            return;
        }
    }
    if(type) {
        // automatic dereference
        if (type->get_direct_linked_canonical_node() != nullptr && is_value_param_hidden_pointer(value)) {
            write('*');
        } else {
            const auto value_type = value->create_type(allocator);
            const auto derefType = value_type->getAutoDerefType(type);
            if (derefType) {
                write('*');
            }
        }
        // capturing function type
        const auto canonical = type->canonical();
        if(canonical->kind() == BaseTypeKind::CapturingFunction && value->kind() == ValueKind::LambdaFunc) {
            const auto cap_type = canonical->as_capturing_func_type_unsafe();
            const auto instance = cap_type->instance_type->get_direct_linked_canonical_node();
            if(instance->kind() == ASTNodeKind::StructDecl) {
                const auto makeFn = instance->child("make");
                if(makeFn && makeFn->kind() == ASTNodeKind::FunctionDecl) {
                    const auto makeFnDecl = makeFn->as_function_unsafe();
                    const auto imp_cons = makeFnDecl;
                    if(imp_cons->is_comptime()) {
                        const auto imp_cons_call = call_with_arg(imp_cons, value, type, allocator, *this);
                        const auto eval = evaluated_func_val_proper(*this, imp_cons, imp_cons_call);

                        auto aliases = declarer->aliases;
                        auto found = aliases.find(value);
                        if(found == aliases.end()) {
                            write("[error: couldn't find lambda alias]");
                            return;
                        }

                        write("*({ ");
                        write(fat_pointer_type);
                        write("* ");
                        write(found->second);
                        write("_pair");
                        write(" = ");
                        VisitLambdaFunction(value->as_lambda_func_unsafe());
                        write("; &");

                        accept_mutating_value_explicit(type, eval, assigning_value);
                        write(';');
                        write(" })");

                    } else {
                        call_implicit_constructor(*this, imp_cons, value, false);
                    }
                }
            }
            return;
        }
    }
    // mutating value
    if(!implicit_mutate_value_default(*this, type, value)) {
        visit(value);
    }
}

void ToCAstVisitor::accept_mutating_value(BaseType* type, Value* value, bool assigning_value) {
    if(type) {
        const auto imp_cons = type->implicit_constructor_for(allocator, value);
        if (imp_cons) {
            if(imp_cons->is_comptime()) {
                const auto imp_cons_call = call_with_arg(imp_cons, value, type, allocator, *this);
                const auto eval = evaluated_func_val_proper(*this, imp_cons, imp_cons_call);
                accept_mutating_value_explicit(type, eval, assigning_value);
            } else {
                call_implicit_constructor(*this, imp_cons, value, false);
            }
            return;
        }
    }
    accept_mutating_value_explicit(type, value, assigning_value);
}

VariableIdentifier* get_single_id(Value* value) {
    switch(value->kind()) {
        case ValueKind::Identifier:
            return value->as_identifier_unsafe();
        case ValueKind::AccessChain: {
            auto& values = value->as_access_chain_unsafe()->values;
            if(values.size() == 1) {
                return values.back()->as_identifier();
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
}

std::string* get_drop_flag(CDestructionVisitor& visitor, ASTNode* initializer);

void set_moved_ref_drop_flag(ToCAstVisitor& visitor, Value* value) {
    const auto id = get_single_id(value);
    if(id) {
        const auto flag = get_drop_flag(*visitor.destructor, id->linked);
        if(flag) {
            visitor.write(*flag);
            visitor.write(" = false;");
        }
    }
}

void func_call_args(ToCAstVisitor& visitor, FunctionCall* call, FunctionType* func_type, unsigned i = 0) {
    auto prev_value = visitor.nested_value;
    visitor.nested_value = true;
    bool has_value_before = false;
    std::string temp_struct_name;
    std::string d_ref_name;
    const auto total_args = call->values.size();
    while(i < total_args) {

        if (has_value_before) {
            visitor.write(", ");
        } else {
            has_value_before = true;
        }

        auto param = func_type->func_param_for_arg_at(i);
        auto val = call->values[i];
        bool is_destructible_ref = false;
        const auto param_type = param->type->canonical();

        // passing a function call or struct value to a reference, whereas the struct is destructible
        if(param->type->kind() == BaseTypeKind::Reference && (val->kind() == ValueKind::StructValue || val->is_chain_func_call())) {
            const auto container = val->create_type(visitor.allocator)->get_members_container();
            if(container && container->destructor_func() != nullptr) {
                auto found_ref = visitor.destructible_refs.find(val);
                if(found_ref != visitor.destructible_refs.end()) {
                    is_destructible_ref = true;
                    visitor.write("({ ");
                    visitor.write_str(found_ref->second);
                    visitor.write(" = ");
                    d_ref_name = found_ref->second;
                }
            }
        }

        const auto imp_cons = param->type->implicit_constructor_for(visitor.allocator, val);
        if(imp_cons) {
            if(imp_cons->is_comptime()) {
                const auto comptime_imp_call = call_with_arg(imp_cons, val, param->type, visitor.allocator, visitor);
                const auto eval = evaluated_func_val_proper(visitor, imp_cons, comptime_imp_call);
                // change the value to evaluated value
                val = eval;
            } else {
                call_implicit_constructor(visitor, imp_cons, val, true);
                i++;
                continue;
            }
        }
        const auto param_type_kind = param->type->kind();
        const auto isStructLikeTypePtr = param->type->kind() != BaseTypeKind::Dynamic && param->type->isStructLikeType();
        const auto isStructLikeTypeDecl = isStructLikeTypePtr && param_type->kind() != BaseTypeKind::CapturingFunction;
        bool is_movable_or_copyable_struct = val->requires_memcpy_ref_struct(param->type);
        bool is_memcpy_ref_str = is_movable_or_copyable_struct && !val->is_ref_moved();
        if(is_movable_or_copyable_struct) {
            if (is_memcpy_ref_str) {
                visitor.write("({ ");
                temp_struct_name = visitor.get_local_temp_var_name();
                visitor.visit(param->type);
                visitor.write(' ');
                visitor.write(temp_struct_name);
                visitor.write(" = ");
                if (is_value_param_pointer_like(val)) {
                    visitor.write('*');
                }
            } else {
                visitor.write("({ ");
                set_moved_ref_drop_flag(visitor, val);
                visitor.space();
                if (is_value_param_pointer_like(val)) {
                    visitor.write('*');
                }
            }
        }
        const auto is_param_type_ref = param_type_kind == BaseTypeKind::Reference;
        bool accept_value = true;
        if(isStructLikeTypePtr && !is_memcpy_ref_str) {
            visitor.write('&');
        } else if(is_param_type_ref && !val->is_ptr_or_ref(visitor.allocator)) {
            accept_value = !write_value_for_ref_type(visitor, val, param->type->as_reference_type_unsafe());
        }
        auto base_type = val->create_type(visitor.allocator);
        if(isStructLikeTypeDecl || (is_param_type_ref && !val->is_stored_ptr_or_ref(visitor.allocator))) {
            auto allocated = visitor.local_allocated.find(val);
            if(allocated != visitor.local_allocated.end()) {
                visitor.write(allocated->second);
            } else if(accept_value) {
                visitor.visit(val);
            }
        } else if(!val->reference() && base_type->pure_type(visitor.allocator)->kind() == BaseTypeKind::Array) {
            visitor.write('(');
            visitor.visit(base_type);
            visitor.write("[])");
            visitor.accept_mutating_value_explicit(param->type, val, false);
        } else {
            visitor.accept_mutating_value_explicit(param->type, val, false);
        }
        if(is_movable_or_copyable_struct) {
            if(is_memcpy_ref_str) {
                visitor.write("; &");
                visitor.write(temp_struct_name);
                visitor.write("; })");
            } else {
                visitor.write("; })");
            }
        }

        if(is_destructible_ref) {
            visitor.write("; ");
            visitor.write_str(d_ref_name);
            visitor.write("; })");
        }

        i++;
    }
    const auto func_param_size = func_type->expectedArgsSize();
    while(i < func_param_size) {
        auto param = func_type->func_param_for_arg_at(i);
        if (param) {
            if(param->defValue) {
                if (has_value_before) {
                    visitor.write(", ");
                } else {
                    has_value_before = true;
                }
                visitor.visit(param->defValue);
            } else if(!func_type->isInVarArgs(i)) {
                visitor.error(call) << "function param '" << param->name << "' doesn't have a default value, however no argument exists for it";
            }
        } else {
#ifdef DEBUG
            throw std::runtime_error("couldn't get param");
#endif
        }
        i++;
    }
    visitor.nested_value = prev_value;
}

// void access_chain(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned end, unsigned size);

void func_container_name(ToCAstVisitor& visitor, FunctionDeclaration* func_node);

#define variant_type_variant_name "__chx__vt_621827"

void value_alloca(ToCAstVisitor& visitor, const chem::string_view& identifier, BaseType* type, Value* value) {
    type_with_id(visitor, type, identifier);
    const auto var = type->get_direct_linked_variant();
    if(var) {
        visitor.write(" = ");
        visitor.write("{ .");
        visitor.write(variant_type_variant_name);
        visitor.write(" = ");
        visitor.write_str(std::to_string(var->variables().size()));
        visitor.space();
        visitor.write('}');
    }
    visitor.write(';');
}

void write_accessor(ToCAstVisitor& visitor, Value* current, Value* next) {
    if(next && next->as_index_op()) return;
    auto linked = current->linked_node();
    if(linked && linked->as_namespace()) {
        return;
    }
    if(is_value_param_pointer_like(current)) {
        visitor.write("->");
        return;
    }
//    if(linked && linked->as_base_func_param()){
//        const auto node = linked->as_base_func_param()->type->get_direct_linked_node();
//        if(node && (node->as_struct_def() || node->as_variant_def())) {
//            visitor.write("->");
//            return;
//        }
//    }
    auto type = current->create_type(visitor.allocator);
    const auto pure_type = type->pure_type(visitor.allocator);
    const auto pure_type_kind = pure_type->kind();
    if(pure_type_kind == BaseTypeKind::Reference) {
        const auto linked_kind = linked->kind();
        if(ASTNode::isBaseDefMember(linked_kind) || linked_kind == ASTNodeKind::VariantCaseVariable) {
            // but stored references become pointers
            visitor.write("->");
            return;
        }
        visitor.write('.');
        return;
    } else if(pure_type_kind == BaseTypeKind::Pointer) {
        visitor.write("->");
        return;
    }
//    if (current->value_type() == ValueType::Pointer) {
//        visitor.write("->");
//    } else {
        visitor.write('.');
//    }
}

//void write_self_arg(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned int grandpa_index, FunctionCall* call, bool force_no_pointer) {
//    auto grandpa = values[grandpa_index];
//    if(!grandpa->is_pointer() && !force_no_pointer) {
//        visitor.write('&');
//    }
//    access_chain(visitor, values, grandpa_index + 1, grandpa_index + 1);
//}

//bool write_self_arg_bool(ToCAstVisitor& visitor, FunctionType* func_type, std::vector<ChainValue*>& values, unsigned int grandpa_index, FunctionCall* call, bool force_no_pointer) {
//    if(func_type->has_self_param()) {
//        write_self_arg(visitor, values, grandpa_index, call, force_no_pointer);
//        return true;
//    } else {
//        return false;
//    }
//}

void write_self_arg(ToCAstVisitor& visitor, ChainValue* grandpa, FunctionCall* call) {
    if(!grandpa->is_pointer() && !is_value_type_pointer_like(grandpa)) {
        visitor.write('&');
    }
    visitor.visit(grandpa);
}

bool write_self_arg_bool_no_pointer(ToCAstVisitor& visitor, FunctionType* func_type, ChainValue* grandpa, FunctionCall* call) {
    if(func_type->has_self_param()) {
        visitor.visit(grandpa);
        return true;
    } else {
        return false;
    }
}

//void visit_evaluated_func_val(
//        ToCAstVisitor& visitor,
//        Visitor* actual_visitor,
//        FunctionDeclaration* func_decl,
//        FunctionCall* call,
//        Value* eval,
//        const chem::string_view& assign_id = ""
//) {
//    auto returns_struct = func_decl->returnType->isStructLikeType();
//    FunctionCall* remove_alloc = nullptr;
//    bool write_semi = false;
//    if(!assign_id.empty() && returns_struct) {
//        const auto eval_kind = eval->val_kind();
//        if(eval_kind == ValueKind::StructValue) {
//            const auto eval_struct_val = eval->as_struct_value_unsafe();
//            visitor.write(assign_id);
//            visitor.write(" = ");
//            write_struct_def_value_call(visitor, eval_struct_val->linked_struct());
//            write_semi = true;
//        } else if(eval_kind == ValueKind::AccessChain) {
//            auto& chain = eval->as_access_chain_unsafe()->values;
//            auto last = chain[chain.size() - 1];
//            const auto last_func_call = last->as_func_call();
//            if(last_func_call) {
//                visitor.local_allocated[last] = assign_id.str();
//                remove_alloc = last_func_call;
//            }
//        }
//    }
//    eval->accept(actual_visitor);
//    if(remove_alloc) {
//        auto found = visitor.local_allocated.find(remove_alloc);
//        if(found != visitor.local_allocated.end()) {
//            visitor.local_allocated.erase(found);
//        }
//    }
//    if(write_semi) {
//        visitor.write(';');
//    }
//}

// when values are declared with initializer that contain equal symbol in C
// for example int x = 5; <----
// now if the value is a struct value, that is not possible
//bool is_declared_with_initializer(ASTAllocator& allocator, Value* value) {
//    switch(value->val_kind()) {
//        case ValueKind::Int:
//        case ValueKind::UInt:
//        case ValueKind::Short:
//        case ValueKind::UShort:
//        case ValueKind::BigInt:
//        case ValueKind::UBigInt:
//        case ValueKind::Char:
//        case ValueKind::UChar:
//        case ValueKind::Long:
//        case ValueKind::ULong:
//        case ValueKind::String:
//        case ValueKind::Expression:
//        case ValueKind::NegativeValue:
//        case ValueKind::NotValue:
//        case ValueKind::NumberValue:
//            return true;
//        case ValueKind::Identifier:
//        case ValueKind::AccessChain:
//        case ValueKind::FunctionCall:
//        case ValueKind::IndexOperator:
//            return !value->create_type(allocator)->isStructLikeType();
//        default:
//            return false;
//    }
//}

void value_assign_default(ToCAstVisitor& visitor, const chem::string_view& identifier, BaseType* type, Value* value, bool write_id = true) {
    if(write_id) {
        visitor.write(identifier);
        write_type_post_id(visitor, type);
    }
    visitor.write(" = ");

    if(value->is_ref_moved()) {
        // since we're moving the value here
        // what we must do is set the drop flag to false
        visitor.write("({ ");
        set_moved_ref_drop_flag(visitor, value);
        visitor.space();
        visitor.accept_mutating_value(type, value, true);
        visitor.write("; })");
    } else {
        visitor.accept_mutating_value(type, value, true);
    }
    visitor.write(';');
}

void value_init_default(ToCAstVisitor& visitor, const chem::string_view& identifier, BaseType* type, Value* value) {
    const auto struct_value = value->as_struct_value();
    bool write_id = true;
    auto type_kind = type->kind();
    switch(type_kind) {
        case BaseTypeKind::Array: {
            const auto arr_type = (ArrayType*) type;
            auto elem_type = arr_type->elem_type->as_function_type();
            if (elem_type && !elem_type->isCapturing()) {
                func_ptr_array_type(visitor, arr_type, elem_type, identifier);
                write_id = false;
            } else {
                visitor.visit(type);
            }
            break;
        }
        case BaseTypeKind::Function: {
            const auto func_type = type->as_function_type();
            if (!func_type->isCapturing()) {
                func_type_with_id(visitor, func_type, identifier);
                write_id = false;
            } else {
                visitor.visit(type);
            }
            break;
        }
        default:
            visitor.visit(type);
            break;
    }
    visitor.space();
    value_assign_default(visitor, identifier, type, value, write_id);
}

void value_alloca_store(ToCAstVisitor& visitor, const chem::string_view& identifier, BaseType* type, Value* value) {
    if(value) {
        auto value_kind = value->val_kind();
        if(value_kind == ValueKind::IfValue || value_kind == ValueKind::SwitchValue || value_kind == ValueKind::LoopValue) {
            value_alloca(visitor, identifier, type, value);
            visitor.new_line_and_indent();
            visitor.visit(value);
            return;
        }
//        auto value_chain = value->as_access_chain_unsafe();
//        if(value_kind == ValueKind::AccessChain && type->isStructLikeType()) {
//            const auto call = value_chain->values.back()->as_func_call();
//            if(call) {
//                const auto node = call->parent_val->linked_node();
//                if(!node || !ASTNode::isVariantMember(node->kind())) {
//                    value_assign_default(visitor, identifier, type, value);
//                    return;
//                }
//            }
//        }
//        if(type->value_type() == ValueType::Struct && value_kind == ValueKind::AccessChain && value_chain->values.back()->as_func_call()) {
//            value_assign_default(visitor, identifier, type, value);
//        } else {
            value_init_default(visitor, identifier, type, value);
//        }
    } else {
        value_alloca(visitor, identifier, type, value);
    }
}

void var_init_top_level(ToCAstVisitor& visitor, VarInitStatement* init, BaseType* init_type, bool is_static, bool initialize = true, bool is_extern = false) {
    if(init->is_comptime()) {
        return;
    }
    if(is_static) {
        visitor.write("static ");
    } else if(is_extern) {
        visitor.write("extern ");
    }
    const auto mangled = visitor.mangler.mangle(init);
    value_alloca_store(visitor, chem::string_view(mangled), init_type, initialize ? init->value : nullptr);
}

void var_init(ToCAstVisitor& visitor, VarInitStatement* init, BaseType* init_type) {
    if(init->is_comptime()) {
        return;
    }
    value_alloca_store(visitor, init->name_view(), init_type, init->value);
}

void allocate_struct_by_name(ToCAstVisitor& visitor, ASTNode* def, const chem::string_view& name, Value* initializer = nullptr) {
    allocate_struct_by_name_no_init(visitor, def, name);
    if(initializer) {
        visitor.write(" = ");
        visitor.visit(initializer);
    }
    visitor.write(';');
    visitor.new_line_and_indent();
}

void allocate_fat_pointer_by_name(ToCAstVisitor& visitor, const chem::string_view& name, Value* initializer = nullptr) {
    visitor.write(visitor.fat_pointer_type);
    visitor.write(' ');
    visitor.write(name);
    if(initializer) {
        visitor.write(" = ");
        visitor.visit(initializer);
    }
    visitor.write(';');
    visitor.new_line_and_indent();
}

void allocate_fat_pointer_for_value(ToCAstVisitor& visitor, Value* value, const chem::string_view& name, Value* initializer) {
    auto found = visitor.local_allocated.find(value);
    if(found != visitor.local_allocated.end()) {
        // already allocated
        return;
    }
    visitor.local_allocated[value] = name.str();
    allocate_fat_pointer_by_name(visitor, name, initializer);
}

void allocate_struct_for_value(ToCAstVisitor& visitor, ExtendableMembersContainerNode* def, Value* value, const chem::string_view& name, Value* initializer) {
    auto found = visitor.local_allocated.find(value);
    if(found != visitor.local_allocated.end()) {
        // already allocated
        return;
    }
    visitor.local_allocated[value] = name.str();
    allocate_struct_by_name(visitor, def, name, initializer);
}

void allocate_struct_for_func_call(ToCAstVisitor& visitor, ExtendableMembersContainerNode* def, FunctionCall* call, FunctionType* func_type, const chem::string_view& name, Value* initializer = nullptr) {
    if(func_type->returnType->kind() == BaseTypeKind::Generic) {
        allocate_struct_for_value(visitor, def, call, name, initializer);
    } else {
        allocate_struct_for_value(visitor, def, call, name, initializer);
    }
}

std::string allocate_temp_struct(ToCAstVisitor& visitor, ASTNode* def_node, Value* initializer) {
    auto struct_name = visitor.get_local_temp_var_name();
    allocate_struct_by_name(visitor, def_node, chem::string_view(struct_name.data(), struct_name.size()));
    return struct_name;
}

void write_implicit_args(ToCAstVisitor& visitor, FunctionType* func_type, FunctionCall* call, bool has_comma_before);

void CBeforeStmtVisitor::VisitFunctionCall(FunctionCall *call) {

    const auto linked = call->parent_val->linked_node();
    // enum member can't be called, we're using it as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;
    const auto func_decl = ASTNode::isFunctionDecl(linked_kind) ? linked->as_function_unsafe() : nullptr;

    // handle variant calls
    if(linked_kind == ASTNodeKind::VariantMember) {
        return;
    }

    // handling comptime functions
    if(func_decl && func_decl->is_comptime()) {
        // TODO
//        const auto value = evaluated_func_val(visitor, func_decl, call);
//        visit(value);
        return;
    }

    // visit the values
    RecursiveValueVisitor::VisitFunctionCall(call);

    // get function type
    const auto func_type = call->function_type(visitor.allocator);

    // functions that return struct are handled in this block of code
    if(!func_decl || !func_decl->is_comptime()) {
        const auto returnType = func_type->returnType->canonical();
        const auto returnTypeKind = returnType->kind();
        if (returnTypeKind != BaseTypeKind::Dynamic) {
            if(returnTypeKind == BaseTypeKind::CapturingFunction) {
                const auto capType = returnType->as_capturing_func_type_unsafe();
                const auto instanceType = capType->instance_type;
                const auto return_linked = instanceType->get_direct_linked_node();
                const auto temp_name = visitor.get_local_temp_var_name();
                const auto temp_name_view = chem::string_view(temp_name);
                allocate_struct_by_name_no_init(visitor, return_linked, temp_name_view);
                write(';');
                visitor.new_line_and_indent();
                visitor.local_allocated[call] = temp_name;
            } else {
                const auto return_linked = returnType->get_direct_linked_node();
                if (return_linked) {
                    const auto returnKind = return_linked->kind();
                    if (returnKind == ASTNodeKind::StructDecl || returnKind == ASTNodeKind::VariantDecl || returnKind == ASTNodeKind::UnionDecl) {
                        const auto temp_name = visitor.get_local_temp_var_name();
                        const auto temp_name_view = chem::string_view(temp_name);
                        allocate_struct_by_name_no_init(visitor, return_linked, temp_name_view);
                        write(';');
                        visitor.new_line_and_indent();
                        visitor.local_allocated[call] = temp_name;
                    }
                }
            }
        }
    }

    // when transferring structs / function calls that return structs directly to reference params
    // we must take the responsibility of destructing them, however we need to maintain a reference to be able to do this
    auto i = 0;
    if(func_type) {
        const auto total_args = call->values.size();
        while (i < total_args) {
            auto arg = call->values[i];
            auto param = func_type->func_param_for_arg_at(i);
            // passing a function call or struct value to a reference, whereas the struct is destructible
            if (param->type->kind() == BaseTypeKind::Reference && (arg->kind() == ValueKind::StructValue || arg->is_chain_func_call())) {
                const auto container = arg->create_type(visitor.allocator)->get_members_container();
                if (container && container->destructor_func() != nullptr) {
                    // struct has a destructor, we must allocate a reference, so it can set it to us
                    visitor.visit(param->type->as_reference_type_unsafe()->type);
                    visitor.write('*');
                    visitor.space();
                    auto temp_name = visitor.get_local_temp_var_name();
                    visitor.write_str(temp_name);
                    visitor.write(';');
                    visitor.new_line_and_indent();
                    visitor.destructible_refs[arg] = temp_name;
                }
            }
            i++;
        }
    }

}

void func_name(ToCAstVisitor& visitor, Value* ref, FunctionDeclaration* func_decl) {
    visitor.mangler.mangle_no_parent(*visitor.output, func_decl);
}

void func_name(ToCAstVisitor& visitor, FunctionDeclaration* func_decl) {
    visitor.mangler.mangle_no_parent(*visitor.output, func_decl);
}

void write_implicit_args(ToCAstVisitor& visitor, FunctionType* func_type, FunctionCall* call, bool has_comma_before = true) {
    if(func_type->isExtensionFn()) {
        const auto firstParam = func_type->params[0];
        const auto grandpa = get_parent_from(call->parent_val);
        if(grandpa) {
            if(!has_comma_before) {
                visitor.write(", ");
            }
            const auto till_grandpa = build_parent_chain(call->parent_val, visitor.allocator);
            write_self_arg(visitor, till_grandpa, call);
            has_comma_before = false;
        } else {
            visitor.error("couldn't get the self parameter for the extension function receiver parameter", firstParam);
        }
    }
    for(const auto param : func_type->params) {
        if(param->is_implicit()) {
            if(param->name == "self") {
                const auto grandpa = get_parent_from(call->parent_val);
                if(grandpa) {
                    if(!has_comma_before) {
                        visitor.write(", ");
                    }
                    const auto till_grandpa = build_parent_chain(call->parent_val, visitor.allocator);
                    write_self_arg(visitor, till_grandpa, call);
                    has_comma_before = false;
                } else if(visitor.current_func_type) {
                    auto self_param = visitor.current_func_type->get_self_param();
                    if(self_param) {
                        if(!has_comma_before) {
                            visitor.write(", ");
                        }
                        visitor.write(param->name);
                        has_comma_before = false;
                    } else {
//                        visitor->error("No self param can be passed to a function, because current function doesn't take a self arg");
                    }
                }
            } else if(param->name == "other") {
                // skipping it
                // TODO check it
            } else {
                auto found = visitor.implicit_args.find(param->name);
                if(found != visitor.implicit_args.end()) {
                    if(!has_comma_before) {
                        visitor.write(", ");
                    }
                    auto type = found->second->create_type(visitor.allocator);
                    const auto node = type->get_direct_linked_node();
                    if(node && ASTNode::isStoredStructType(node->kind())) {
                        visitor.write('&');
                    }
                    visitor.visit(found->second);
                    has_comma_before = false;
                } else {
                    const auto between = visitor.current_func_type->implicit_param_for(param->name);
                    if(between) {
                        if(!has_comma_before) {
                            visitor.write(", ");
                        }
                        visitor.write(between->name);
                        has_comma_before = false;
                    } else {
                        visitor.error(call) << "couldn't find implicit argument with name '" << param->name << "'";
                    }
                }
            }
        } else {
            break;
        }
    }
    if (!has_comma_before && !call->values.empty()) {
        visitor.write(", ");
    }
};

void CBeforeStmtVisitor::VisitVariableIdentifier(VariableIdentifier *identifier) {

}

void CBeforeStmtVisitor::VisitAccessChain(AccessChain *chain) {

    RecursiveValueVisitor::VisitAccessChain(chain);

}

void write_variant_call(ToCAstVisitor& visitor, FunctionCall* call) {

    const auto member = call->parent_val->linked_node()->as_variant_member();
    const auto linked = member->parent();
    const auto index = linked->direct_child_index(member->name);

    visitor.write("(struct ");
    visitor.mangle(linked);
    visitor.write(") ");
    visitor.write("{ ");
    visitor.write_str(std::to_string(index));
    visitor.write(", ");
    unsigned i = 0;
    auto prev_nested = visitor.nested_value;
    visitor.nested_value = true;
    for(auto& value : member->values) {
        visitor.write('.');
        visitor.write(member->name);
        visitor.write('.');
        visitor.write(value.second->name);
        visitor.write(" = ");
        const auto val = call->values[i];
        if(val->is_ref_moved()) {
            // since we're moving the value here
            // what we must do is set the drop flag to false
            visitor.write("({ ");
            set_moved_ref_drop_flag(visitor, val);
            visitor.space();
            visitor.accept_mutating_value(value.second->type, val, false);
            visitor.write("; })");
        } else {
            visitor.accept_mutating_value(value.second->type, val, false);
        }
        visitor.write(", ");
        i++;
    }
    visitor.nested_value = prev_nested;
    visitor.write('}');

}

void CBeforeStmtVisitor::process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const chem::string_view& identifier) {
    auto eval = evaluated_func_val(visitor, decl, call);
    const auto eval_struct_val = eval->as_struct_value();
    if(eval_struct_val) {
        allocate_struct_by_name(visitor, eval_struct_val->linked_extendable(), identifier);
        // TODO remove this statement once green (identifier is known)
        // UNLESS comptime can't resolve identifier without it
        visitor.local_allocated[eval] = identifier.str();
    }
    process_init_value(eval, identifier);
}

void CBeforeStmtVisitor::process_init_value(Value* value, const chem::string_view& identifier) {
    if(!value) return;
    const auto val_type = value->create_type(visitor.allocator);
    if(!val_type->isStructLikeType()) return;
    FunctionCall* call_ptr;
    switch(value->val_kind()) {
        case ValueKind::AccessChain:
            call_ptr = value->as_access_chain_unsafe()->values.back()->as_func_call();
            if(!call_ptr) return;
            break;
        case ValueKind::FunctionCall:
            call_ptr = value->as_func_call_unsafe();
            break;
        default:
            return;
    }
    auto& call = *call_ptr;
    const auto parent = call.parent_val->linked_node();
    const auto parent_kind = parent->kind();
    if(ASTNode::isFunctionDecl(parent_kind) && parent->as_function_unsafe()->is_comptime()) {
        process_comp_time_call(parent->as_function_unsafe(), &call, identifier);
        return;
    } else {
        auto func_type = call.function_type(visitor.allocator);
        if(func_type) {
            auto linked = func_type->returnType->linked_node();
            if(linked) {
                const auto linked_kind = linked->kind();
                if (linked_kind == ASTNodeKind::StructDecl) {
                    allocate_struct_for_func_call(visitor, linked->as_struct_def_unsafe(), &call, func_type, identifier);
                } else if (linked_kind == ASTNodeKind::VariantDecl) {
                    allocate_struct_for_func_call(visitor, linked->as_variant_def_unsafe(), &call, func_type, identifier);
                } else if (linked_kind == ASTNodeKind::InterfaceDecl && func_type->returnType->pure_type(visitor.allocator)->kind() == BaseTypeKind::Dynamic) {
                    allocate_fat_pointer_for_value(visitor, &call, identifier, nullptr);
                }
            }
        }
    }
}

void CBeforeStmtVisitor::VisitVarInitStmt(VarInitStatement *init) {
//    if (!init->type) {
//        init->type = init->value->create_type(visitor.allocator);
//    }
    RecursiveValueVisitor::VisitVarInitStmt(init);
}

CTopLevelDeclarationVisitor::CTopLevelDeclarationVisitor(
    ToCAstVisitor &visitor,
    CValueDeclarationVisitor *value_visitor
) : SubVisitor(visitor), value_visitor(value_visitor) {
    declared.reserve(30);
}

enum class DestructionJobType {
    Default,
    Array
};

struct DestructionJob {
    DestructionJobType type;
    chem::string_view self_name;
    std::string drop_flag_name;
    ASTNode* initializer;
    union {
        struct {
            MembersContainer* parent_node;
            FunctionDeclaration* destructor;
            bool is_pointer;
        } default_job;
        struct {
            int array_size;
            MembersContainer* linked;
            FunctionDeclaration* destructorFunc;
        } array_job;
    };
};

class CDestructionVisitor : public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    bool destroy_current_scope = true;

    bool new_line_before = true;

    std::vector<DestructionJob> destruct_jobs;

    void destruct(
            const chem::string_view& self_name,
            MembersContainer* linked,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void conditional_destruct(
            const chem::string_view& condition,
            const chem::string_view& self_name,
            MembersContainer* linked,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void queue_destruct(
            const chem::string_view& self_name,
            ASTNode* initializer,
            MembersContainer* linked,
            bool is_pointer = false,
            bool has_drop_flag = true
    );

    std::string* get_drop_flag_name(ASTNode* initializer) {
        for(auto& d : destruct_jobs) {
            if(d.initializer == initializer) {
                return &d.drop_flag_name;
            }
        }
        return nullptr;
    }

    void queue_destruct(const chem::string_view& self_name, ASTNode* initializer, FunctionCall* call);

    void destruct_arr_ptr(const chem::string_view& self_name, Value* array_size, MembersContainer* linked, FunctionDeclaration* destructor);

    void destruct_arr(const chem::string_view& self_name, int array_size, MembersContainer* linked, FunctionDeclaration* destructor) {
        IntValue siz(array_size, ZERO_LOC);
        destruct_arr_ptr(self_name, &siz, linked, destructor);
    }

    void destruct(const DestructionJob& job, Value* current_return);

    bool queue_destruct_arr(const chem::string_view& self_name, ASTNode* initializer, BaseType* elem_type, int array_size);

    void VisitVarInitStmt(VarInitStatement *init);

    void dispatch_jobs_from_no_clean(int begin);

    void dispatch_jobs_from(int begin);

    void queue_destruct_decl_params(FunctionType* decl);

    void process_init_value(VarInitStatement *init, Value* value);

    void reset() final {
        destroy_current_scope = true;
        new_line_before = true;
        destruct_jobs.clear();
    }

};

std::string* get_drop_flag(CDestructionVisitor& visitor, ASTNode* initializer) {
    return visitor.get_drop_flag_name(initializer);
}

void assign_statement(ToCAstVisitor& visitor, AssignStatement* assign) {
    auto type = assign->lhs->create_type(visitor.allocator)->pure_type(visitor.allocator);
    // assignment to a reference, automatic dereferencing
    if(type->kind() == BaseTypeKind::Reference) {
        visitor.write('*');
    }
    if(type->requires_moving() && !assign->lhs->is_ref_moved()) {
        auto container = type->linked_node()->as_members_container();
        auto destr = container->destructor_func();
        func_container_name(visitor, destr);
        visitor.write('(');
        visitor.write('&');
        visitor.visit(assign->lhs);
        visitor.write(')');
        visitor.write(';');
        visitor.new_line_and_indent();
    }
    const auto prev_nested = visitor.nested_value;
    visitor.nested_value = true;
    visitor.visit(assign->lhs);
    visitor.nested_value = prev_nested;
    visitor.write(' ');
    if(assign->assOp != Operation::Assignment) {
        visitor.write_str(to_string(assign->assOp));
    }
    visitor.write('=');
    visitor.write(' ');

    const auto value = assign->value;
    if(value->is_ref_moved()) {
        // since we're moving the value here
        // what we must do is set the drop flag to false
        visitor.write("({ ");
        set_moved_ref_drop_flag(visitor, value);
        visitor.space();
        visitor.accept_mutating_value(type, value, true);
        visitor.write("; })");
    } else {
        visitor.accept_mutating_value(type, value, true);
    }

}

void CAfterStmtVisitor::destruct_chain(AccessChain *chain, bool destruct_last) {
    int index = ((int) chain->values.size()) - (destruct_last ? 1 : 2);
    while(index >= 0) {
        const auto call = chain->values[index]->as_func_call();
        if(call) {
            const auto decl = call->safe_linked_func();
            if(decl && decl->is_comptime()) {
                auto eval = visitor.evaluated_func_calls.find(call);
                if(eval != visitor.evaluated_func_calls.end()) {
                    const auto comp_chain = eval->second->as_access_chain();
                    if(comp_chain) {
                        destruct_chain(comp_chain, true);
                    } else {
                        visit(eval->second);
                    }
                } else {
                    std::cerr << "[2c] warn: evaluated function call value not found in after statement visitor" << std::endl;
                }
                return;
            }
            auto func_type = call->function_type(visitor.allocator);
            if(func_type->returnType->isStructLikeType()) {
                auto linked = func_type->returnType->linked_node();
                if(linked->as_struct_def()) {
                    const auto struct_def = linked->as_struct_def();
                    auto destructor = struct_def->destructor_func();
                    if(destructor) {
                        auto destructible = visitor.local_allocated.find(call);
                        if (destructible != visitor.local_allocated.end()) {
                            visitor.destructor->destruct(
                                    chem::string_view(destructible->second.data(), destructible->second.size()),
                                    struct_def,
                                    destructor,
                                    false
                            );
                        }
                    }
                }
            }
        }
        index--;
    }
}

void CAfterStmtVisitor::VisitAccessChain(AccessChain *chain) {
    RecursiveValueVisitor::VisitAccessChain(chain);
//    destruct_chain(chain, chain->is_node());
}

void CAfterStmtVisitor::VisitFunctionCall(FunctionCall *call) {

    const auto linked = call->parent_val->linked_node();
    // enum member can't be called, we're using it as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;
    const auto func_decl = ASTNode::isFunctionDecl(linked_kind) ? linked->as_function_unsafe() : nullptr;

    // handle variant calls
    if(linked_kind == ASTNodeKind::VariantMember) {
        return;
    }

    // handling comptime functions
    if(func_decl && func_decl->is_comptime()) {
        // TODO
//        const auto value = evaluated_func_val(visitor, func_decl, call);
//        visit(value);
        return;
    }

    // get function type
    const auto func_type = call->function_type(visitor.allocator);

    // functions that return struct are handled in this block of code
    if(destruct_call) {
        if ((!func_decl || !func_decl->is_comptime())) {
            const auto returnType = func_type->returnType->canonical();
            const auto returnTypeKind = returnType->kind();
            if (returnTypeKind != BaseTypeKind::Dynamic) {
                const auto return_linked = returnType->get_direct_linked_node();
                if (return_linked) {
                    const auto returnKind = return_linked->kind();
                    if (returnKind == ASTNodeKind::StructDecl || returnKind == ASTNodeKind::VariantDecl ||
                        returnKind == ASTNodeKind::UnionDecl) {
                        auto container = return_linked->as_members_container_unsafe();
                        const auto destr = container->destructor_func();
                        if (destr) {
                            auto found = visitor.local_allocated.find(call);
                            auto temp_name = found != visitor.local_allocated.end() ? chem::string_view(found->second) : chem::string_view("NOT_ALLOCATED");
                            visitor.new_line_and_indent();
                            visitor.mangle(destr);
                            visitor.write("(&");
                            visitor.write(temp_name);
                            visitor.write(");");
                        }
                    }
                }
            }
        }
    }

    // visit the values
    if(destruct_call) {
        visit(call->parent_val);
        destruct_call = false;
        for(auto val : call->values) {
            visit(val);
        }
        destruct_call = true;
    } else {
        destruct_call = true;
        visit(call->parent_val);
        destruct_call = false;
        for(auto val : call->values) {
            visit(val);
        }
    }

    unsigned int i = 0;
    for(auto& val : call->values) {

        if(func_type) {
            const auto arg = val;
            const auto param = func_type->func_param_for_arg_at(i);
            // passing a function call or struct value to a reference, whereas the struct is destructible
            if (param->type->kind() == BaseTypeKind::Reference && (arg->kind() == ValueKind::StructValue || arg->is_chain_func_call())) {
                const auto container = param->type->as_reference_type_unsafe()->type->get_members_container();
                if (container) {
                    const auto destr = container->destructor_func();
                    if (destr) {
                        visitor.new_line_and_indent();
                        visitor.mangle(destr);
                        visitor.write('(');
                        visitor.write_str(visitor.destructible_refs[arg]);
                        visitor.write(");");
                    }
                }
            }
        }


        const auto chain = val->as_access_chain();
        if(chain) {
            // if we ever pass struct as a reference, where struct is created at call time
            // we can set destruct_last to true, to destruct the struct after this call
//            destruct_chain(chain, false);
        } else {
                visit(val);
        }
        i++;
    }
}

void CDestructionVisitor::destruct(const chem::string_view& self_name, MembersContainer* parent_node, FunctionDeclaration* destructor, bool is_pointer) {
    if(new_line_before) {
        visitor.new_line_and_indent();
    }
    visitor.mangle(destructor);
    visitor.write('(');
    if(destructor->has_self_param()) {
        if(!is_pointer) {
            visitor.write('&');
        }
        visitor.write(self_name);
    }
    visitor.write(')');
    visitor.write(';');
    if(!new_line_before) {
        visitor.new_line_and_indent();
    }
}

void CDestructionVisitor::conditional_destruct(
        const chem::string_view& condition,
        const chem::string_view& self_name,
        MembersContainer* linked,
        FunctionDeclaration* destructor,
        bool is_pointer
) {
    if(new_line_before) {
        visitor.new_line_and_indent();
    }
    const auto prev = new_line_before;
    visitor.write("if(");
    visitor.write(condition);
    visitor.write(") {");
    visitor.indentation_level += 1;
    new_line_before = true;
    destruct(self_name, linked, destructor, is_pointer);
    visitor.indentation_level -= 1;
    visitor.new_line_and_indent();
    visitor.write('}');
    new_line_before = prev;
    if(!new_line_before) {
        visitor.new_line_and_indent();
    }
}

void init_drop_flag(ToCAstVisitor& visitor, const chem::string_view& drop_flag) {
    visitor.new_line_and_indent();
    visitor.write("_Bool ");
    visitor.write(drop_flag);
    visitor.write(" = ");
    visitor.write("true;");
}

void CDestructionVisitor::queue_destruct(const chem::string_view& self_name, ASTNode* initializer, MembersContainer* linked, bool is_pointer, bool has_drop_flag) {
    if(!linked) return;
    auto destructorFunc = linked->destructor_func();
    if(destructorFunc) {
        std::string drop_flag;
        if(has_drop_flag) {
            drop_flag = visitor.get_local_temp_var_name();
            init_drop_flag(visitor, chem::string_view(drop_flag));
        }
        destruct_jobs.emplace_back(DestructionJob{
                .type = DestructionJobType::Default,
                .self_name = self_name,
                .drop_flag_name = std::move(drop_flag),
                .initializer = initializer,
                .default_job = {
                        linked,
                        destructorFunc,
                        is_pointer
                }
        });
    }
}

void CDestructionVisitor::queue_destruct(const chem::string_view& self_name, ASTNode* initializer, FunctionCall* call) {
    auto return_type = call->create_type(visitor.allocator);
    const auto linked = return_type->get_direct_linked_node();
    if(linked) {
        const auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::VariantMember) {
            const auto member = linked->as_variant_member_unsafe();
            const auto variant = member->parent();
            queue_destruct(self_name, initializer, variant);
            return;
        }
        queue_destruct(self_name, initializer, linked->as_extendable_members_container_node());
    }
}

void CDestructionVisitor::destruct_arr_ptr(const chem::string_view &self_name, Value* array_size, MembersContainer* parent_node, FunctionDeclaration* destructorFunc) {

    std::string arr_val_itr_name = visitor.get_local_temp_var_name();
    visitor.new_line_and_indent();
    visitor.write("for(int ");
    visitor.write(arr_val_itr_name);
    visitor.write(" = ");
    visitor.visit(array_size);
    visitor.write(" - 1");
    visitor.write("; ");
    visitor.write(arr_val_itr_name);
    visitor.write(" >= 0;");
    visitor.write(arr_val_itr_name);
    visitor.write("--){");
    visitor.indentation_level++;
    std::string name = self_name.str() + "[" + arr_val_itr_name + "]";
    destruct(chem::string_view(name.data(), name.size()), parent_node, destructorFunc, false);
    visitor.indentation_level--;
    visitor.new_line_and_indent();
    visitor.write('}');

}

void CDestructionVisitor::destruct(const DestructionJob& job, Value* current_return) {
    if(current_return) {
        const auto returnKind = current_return->val_kind();
        if(returnKind == ValueKind::AccessChain) {
            const auto chain = current_return->as_access_chain_unsafe();
            if(chain->values.size() == 1) {
                auto id = chain->values.back()->as_identifier();
                if(id && id->linked == job.initializer) {
                    return;
                }
            }
        }
        if(returnKind == ValueKind::Identifier) {
            const auto id = current_return->as_identifier_unsafe();
            if (id->linked == job.initializer) {
                return;
            }
        }
    }
    switch(job.type) {
        case DestructionJobType::Default:
            if(job.drop_flag_name.empty()) {
                destruct(job.self_name, job.default_job.parent_node, job.default_job.destructor, job.default_job.is_pointer);
            } else {
                conditional_destruct(chem::string_view(job.drop_flag_name), job.self_name, job.default_job.parent_node, job.default_job.destructor, job.default_job.is_pointer);
            }
            break;
        case DestructionJobType::Array:
            destruct_arr(job.self_name, job.array_job.array_size, job.array_job.linked, job.array_job.destructorFunc);
            break;
    }
}

void CDestructionVisitor::dispatch_jobs_from_no_clean(int begin) {
    int i = ((int) destruct_jobs.size()) - 1;
    while (i >= (int) begin) {
        destruct(destruct_jobs[i], nullptr);
        i--;
    }
}

void CDestructionVisitor::dispatch_jobs_from(int begin) {
    dispatch_jobs_from_no_clean(begin);
    auto itr = destruct_jobs.begin() + begin;
    destruct_jobs.erase(itr, destruct_jobs.end());
}

void CDestructionVisitor::queue_destruct_decl_params(FunctionType* decl) {
    for(auto& d_param : decl->params) {
        const auto container = d_param->type->get_direct_linked_container();
        if(container) {
            const auto destructor_func = container->destructor_func();
            if (destructor_func) {
                queue_destruct(d_param->name, d_param, container->as_extendable_members_container_node(), true);
            }
        }
    }
}

bool CDestructionVisitor::queue_destruct_arr(const chem::string_view& self_name, ASTNode* initializer, BaseType *elem_type, int array_size) {
    if(elem_type->isStructLikeType()) {
        auto struc_def = elem_type->get_members_container();
        if(struc_def) {
            const auto destructorFunc = struc_def->destructor_func();
            if (!destructorFunc) {
                return false;
            }
            destruct_jobs.emplace_back(DestructionJob{
                .type = DestructionJobType::Array,
                .self_name = self_name,
                .initializer = initializer,
                .array_job = {
                    array_size,
                    struc_def,
                    destructorFunc
                }
            });
            return true;
        }
    }
    return false;
}

void CDestructionVisitor::process_init_value(VarInitStatement *init, Value* init_value) {
    const auto init_val_kind = init_value->val_kind();
    if(init_val_kind == ValueKind::AccessChain) {
        auto chain = init_value->as_access_chain_unsafe();
        const auto last_func_call = chain->values.back()->as_func_call();
        if(last_func_call) {
            queue_destruct(init->name_view(), init, last_func_call);
            return;
        } else {
            if(chain->is_moved()) {
                auto init_type = init->known_type();
                auto linked = init_type->linked_node();
                if(!linked) {
                    return;
                }
                queue_destruct(init->name_view(), init, linked->as_extendable_members_container_node());
            }
            return;
        }
    } else if(init_val_kind == ValueKind::FunctionCall) {
        queue_destruct(init->name_view(), init, init_value->as_func_call_unsafe());
        return;
    }
    if(init_val_kind == ValueKind::Identifier && init_value->as_identifier_unsafe()->is_moved) {
        auto init_type = init->known_type();
        auto linked = init_type->linked_node();
        if(!linked) {
            visitor.error("couldn't destruct var init", init);
            return;
        }
        queue_destruct(init->name_view(), init, linked->as_extendable_members_container_node());
        return;
    }
    if(init_val_kind == ValueKind::ArrayValue) {
        auto array_val = init_value->as_array_value_unsafe();
        auto elem_type = array_val->element_type(visitor.allocator);
        queue_destruct_arr(init->name_view(), init, elem_type, array_val->array_size());
        return;
    }
//    auto variant_call = init_value->as_variant_call_unsafe();
//    if(init_val_kind == ValueKind::VariantCall) {
//        const auto def = variant_call->get_definition();
//        queue_destruct(init->identifier(), init, variant_call->generic_iteration, def);
//        return;
//    }
    if(init_val_kind == ValueKind::StructValue) {
        auto struct_val = init_value->as_struct_value_unsafe();
        queue_destruct(init->name_view(), init, struct_val->linked_struct());
    }
}

void CDestructionVisitor::VisitVarInitStmt(VarInitStatement *init) {
    const auto pure_t = init->known_type()->pure_type(visitor.allocator);
    const auto pure_t_kind = pure_t->kind();
    if(pure_t_kind == BaseTypeKind::Pointer || pure_t_kind == BaseTypeKind::Reference) {
        return;
    }
    if(init->value) {
        auto init_value = init->value;
        auto chain = init_value->as_access_chain();
        if(chain) {
            const auto func_call = chain->values.back()->as_func_call();
            if(func_call) {
                auto decl = func_call->safe_linked_func();
                if (decl && decl->is_comptime()) {
                    auto found = visitor.evaluated_func_calls.find(func_call);
                    if (found != visitor.evaluated_func_calls.end()) {
                        init_value = found->second;
                    } else {
                        std::cerr << "[2c] warn: evaluated function call value not found in after statement visitor for " << func_call->representation() << std::endl;
                    }
                }
            }
        }
        process_init_value(init, init_value);
        return;
    } else {
        if(init->type->isStructLikeType()) {
            auto container = init->type->get_members_container();
            if (container) {
                queue_destruct(init->name_view(), init, container);
            }
        } else if(init->type->kind() == BaseTypeKind::Array) {
            auto type = (ArrayType*) init->type.getType();
            if(type->has_array_size()) {
                queue_destruct_arr(init->name_view(), init, type->elem_type, type->get_array_size());
            } else {
                // cannot destruct array type without size
            }
        }
    }
}

// this will also destruct given function type's params at the end of scope
void scope_no_parens(ToCAstVisitor& visitor, Scope& scope, FunctionType* decl) {
    unsigned begin = visitor.destructor->destruct_jobs.size();
    visitor.indentation_level+=1;
    visitor.destructor->queue_destruct_decl_params(decl);
    visitor.visit_scope(&scope, (int) begin);
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
}

// this will also destruct given function type's params at the end of scope
inline void scope(ToCAstVisitor& visitor, Scope& scope, FunctionType* decl) {
    visitor.write('{');
    scope_no_parens(visitor, scope, decl);
    visitor.write('}');
}

//std::string func_type_alias(ToCAstVisitor& visitor, FunctionType* type) {
//    std::string alias = "__chx_functype_";
//    alias += std::to_string(random(100,999)) + "_";
//    alias += std::to_string(visitor.declarer->func_type_num++);
//    func_type_with_id(visitor, type, chem::string_view(alias.data(), alias.size()));
//    visitor.declarer->aliases[type] = alias;
//    return alias;
//}

//std::string typedef_func_type(ToCAstVisitor& visitor, FunctionType* type) {
//    visitor.new_line_and_indent();
//    visitor.write("typedef ");
//    auto alia = func_type_alias(visitor, type);
//    visitor.write(';');
//    return alia;
//}

//void func_call(ToCAstVisitor& visitor, FunctionCall* call, FunctionType* func_type) {
//    visitor.write('(');
//    func_call_args(visitor, call, func_type);
//    visitor.write(')');
////    if(!visitor->nested_value) {
////        visitor->write(';');
////    }
//}

void call_struct_member_fn(
        ToCAstVisitor& visitor,
        BaseType* mem_type,
        const chem::string_view& member_name,
        FunctionDeclaration*(*choose_func)(MembersContainer* container)
) {
    const auto mem_def = mem_type->get_members_container();
    if(!mem_def) {
        return;
    }
    auto func = choose_func(mem_def);
    if (!func) {
        return;
    }
    visitor.new_line_and_indent();
    visitor.mangle(func);
    visitor.write('(');
    if (func->has_self_param()) {
        visitor.write("&self->");
        visitor.write(member_name);
    }
    visitor.write(')');
    visitor.write(';');
}

void call_struct_member_delete_fn(ToCAstVisitor& visitor, BaseType* mem_type, const chem::string_view& mem_name) {
    if (mem_type->isStructLikeType()) {
        call_struct_member_fn(visitor, mem_type, mem_name, [](MembersContainer* def) -> FunctionDeclaration* {
            return def->destructor_func();
        });
    }
}

bool has_destructor(BaseType* mem_type) {
    if (mem_type->isStructLikeType()) {
        const auto container = mem_type->get_members_container();
        if(!container) return false;
        return container->destructor_func() != nullptr;
    } else {
        return false;
    }
}

bool has_any_destructor(std::vector<CapturedVariable*>& caps) {
    for(const auto cap : caps) {
        if(has_destructor(cap->known_type())) {
            return true;
        }
    }
    return false;
}

void CValueDeclarationVisitor::VisitLambdaFunction(LambdaFunction *lamb) {
    RecursiveValueVisitor::VisitLambdaFunction(lamb);
    std::string lamb_name = "__chemda_";
    lamb_name += std::to_string(random(100,999)) + "_";
    lamb_name += std::to_string(lambda_num++);
    if(!lamb->captureList.empty()) {
        visitor.new_line_and_indent();
        visitor.write("struct ");
        std::string capture_struct_name = lamb_name + "_cap";
        visitor.write(capture_struct_name);
        visitor.space();
        visitor.write('{');
        visitor.indentation_level += 1;
        for(auto& var : lamb->captureList) {
            aliases[var] = capture_struct_name;
            visitor.new_line_and_indent();
            if(var->capture_by_ref) {
                PointerType pointer(var->linked->known_type(), ZERO_LOC);
                visitor.visit(&pointer);
            } else {
                visitor.visit(var->linked->known_type());
            }
            visitor.space();
            visitor.write(var->name);
            visitor.write(';');
        }
        visitor.indentation_level -= 1;
        visitor.new_line_and_indent();
        visitor.write("};");

        // writing the destructor
        if(has_any_destructor(lamb->captureList)) {
            visitor.new_line_and_indent();
            visitor.write("void ");
            visitor.write(lamb_name);
            visitor.write("_cap_destr(struct ");
            visitor.write(lamb_name);
            visitor.write("_cap* self) {");
            visitor.indentation_level += 1;
            for (const auto var: lamb->captureList) {
                call_struct_member_delete_fn(visitor, var->known_type(), var->name);
            }
            visitor.indentation_level -= 1;
            visitor.new_line_and_indent();
            visitor.write("}");
        } else {
            visitor.new_line_and_indent();
            visitor.write("#define ");
            visitor.write(lamb_name);
            visitor.write("_cap_destr NULL");
        }

    }
    visitor.new_line_and_indent();
    accept_func_return_with_name(visitor, lamb, chem::string_view(lamb_name.data(), lamb_name.size()), true);
    aliases[lamb] = lamb_name;
    write('(');
    unsigned i = 0;
    // writing the captured struct as a parameter
    if(lamb->isCapturing()) {
        auto self_param = lamb->get_self_param();
        if(self_param) {
            visitor.visit(self_param);
            visitor.write(", ");
            i++;
        }
        visitor.write("void*");
        visitor.write(" this");
        if(lamb->params.size() > (self_param ? 1 : 0)) {
            visitor.write(',');
        }
    }
    func_type_params(visitor, lamb, i);
    write(')');
    auto prev_destroy_scope = visitor.destructor->destroy_current_scope;
    visitor.destructor->destroy_current_scope = true;
    auto previous_destruct_jobs = std::move(visitor.destructor->destruct_jobs);
    auto prev_func_type = visitor.current_func_type;
    visitor.current_func_type = lamb;
    scope(visitor, lamb->scope, lamb);
    visitor.current_func_type = prev_func_type;
    visitor.destructor->destruct_jobs = std::move(previous_destruct_jobs);
    visitor.destructor->destroy_current_scope = prev_destroy_scope;
}

void CValueDeclarationVisitor::VisitReturnStmt(ReturnStatement *stmt) {

//    const auto val = stmt->value;
//    if(val) {
//        const auto func_type = stmt->func_type;
//        // replace value with call to implicit constructor if there is one
//        const auto func = func_type->as_function();
//        if (!(func && func->is_constructor_fn())) {
//            const auto implicit = func_type->returnType->implicit_constructor_for(visitor.allocator, val);
//            if (implicit && implicit != func_type && implicit->parent_node != func_type->parent()) {
//                stmt->value = call_with_arg(implicit, val, func_type->returnType, visitor.allocator, visitor);
//            }
//        }
//    }

    RecursiveValueVisitor::VisitReturnStmt(stmt);

}


void CValueDeclarationVisitor::VisitFunctionCall(FunctionCall *call) {
    // replace all values that call implicit constructor with actual calls
//    const auto size = call->values.size();
//    auto func_type = call->function_type(visitor.allocator);
//    if(func_type) {
//        unsigned i = 0;
//        while (i < size) {
//            auto& value = call->values[i];
//            auto func_param = func_type->func_param_for_arg_at(i);
//            // TODO the func_param is nullptr sometimes, for which the arg is given
//            auto constructor = func_param->type->implicit_constructor_for(visitor.allocator, value);
//            if (constructor) {
//                value = call_with_arg(constructor, value, func_param->type, visitor.allocator, visitor);
//            }
//            i++;
//        }
//    }
    RecursiveValueVisitor::VisitFunctionCall(call);
}

void CValueDeclarationVisitor::VisitArrayValue(ArrayValue *arrayVal) {

//    // replace all values that call implicit constructors with actual calls
//    const auto elem_type = arrayVal->element_type(visitor.allocator);
//    const auto def = elem_type->linked_struct_def();
//    if(def) {
//        for (auto& value : arrayVal->values) {
//            const auto implicit = def->implicit_constructor_func(visitor.allocator, value);
//            if (implicit) {
//                value = call_with_arg(implicit, value, elem_type, visitor.allocator, visitor);
//            }
//        }
//    }

    RecursiveValueVisitor::VisitArrayValue(arrayVal);

}

void CValueDeclarationVisitor::VisitStructValue(StructValue *structValue) {

//    const auto container = structValue->variables();
//    for(auto& value : structValue->values) {
//        auto& value_ptr = value.second.value;
//        auto variable = container->variable_type_index(value.first);
//        auto implicit = variable.second->implicit_constructor_for(visitor.allocator, value_ptr);
//        if(implicit) {
//            value_ptr = call_with_arg(implicit, value_ptr, variable.second, visitor.allocator, visitor);
//        }
//    }

    RecursiveValueVisitor::VisitStructValue(structValue);

}

void declare_params(CValueDeclarationVisitor* value_visitor, std::vector<FunctionParam*>& params) {
    for(const auto param : params) {
        if(param->type->kind() == BaseTypeKind::Function && param->type->as_function_type()->isCapturing()) {
            // do not declare capturing function types
            continue;
        }
        if(param->type->kind() != BaseTypeKind::Function) {
            value_visitor->visit(param);
        }
    }
}

void func_ret_func_proto_after_l_paren(ToCAstVisitor& visitor, FunctionDeclaration* decl, FunctionType* retFunc, unsigned declFuncParamStart = 0, unsigned retFuncParamStart = 0) {
    visitor.write('*');
    visitor.mangle(decl);
    visitor.write('(');
    func_type_params(visitor, decl, declFuncParamStart);
    visitor.write("))(");
    func_type_params(visitor, retFunc, retFuncParamStart);
    visitor.write(')');
}

void func_that_returns_func_proto(ToCAstVisitor& visitor, FunctionDeclaration* decl, FunctionType* retFunc) {
    if(decl->body.has_value()) {
        visitor.write("static ");
    }
    accept_func_return(visitor, retFunc->returnType);
    visitor.write("(");
    func_ret_func_proto_after_l_paren(visitor, decl, retFunc);
}

void declare_func_with_return(ToCAstVisitor& visitor, FunctionDeclaration* decl) {
    if(decl->is_comptime()) {
        return;
    }
    const auto ret_func = decl->returnType->as_function_type();
    if(ret_func && !ret_func->isCapturing()) {
        func_that_returns_func_proto(visitor, decl, ret_func);
    } else {
        const auto ret_kind = decl->returnType->kind();
        accept_func_return_with_name(visitor, decl, decl->body.has_value() && !decl->is_exported_fast());
        visitor.write('(');
        func_type_params(visitor, decl);
        visitor.write(')');
    }
}

void early_declare_node(ToCAstVisitor& visitor, ASTNode* node);

inline void declare_inherited(ToCAstVisitor& visitor, VariablesContainer* def) {
    // declare inherited types
    for(auto& inherit : def->inherited) {
        auto in_node = inherit.type->get_direct_linked_node();
        if(in_node) {
            early_declare_node(visitor, in_node);
        }
    }
}

void early_declare_container(ToCAstVisitor& visitor, VariablesContainer* def) {
    declare_inherited(visitor, def);
    // declare sub variables
    for(const auto var : def->variables()) {
        const auto known_t = var->known_type();
        if(known_t) {
            auto sub_node = known_t->get_direct_linked_node();
            if (sub_node) {
                early_declare_node(visitor, sub_node);
            }
        }
    }
}

void early_declare_container(ToCAstVisitor& visitor, VariantDefinition* def) {
    declare_inherited(visitor, def);
    // declare sub variables
    for(const auto var : def->variables()) {
        early_declare_node(visitor, var->as_variant_member_unsafe());
    }
}

void early_declare_struct(ToCAstVisitor& visitor, StructDefinition* def) {
    early_declare_container(visitor, def);
    visitor.tld.early_declare_struct_def(def);
}

void early_declare_variant(ToCAstVisitor& visitor, VariantDefinition* def) {
    early_declare_container(visitor, def);
    visitor.tld.early_declare_variant_def(def);
}

void early_declare_union(ToCAstVisitor& visitor, UnionDef* def) {
    early_declare_container(visitor, def);
    visitor.tld.early_declare_union_def(def);
}

void early_declare_node(ToCAstVisitor& c_visitor, ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::TypealiasStmt:
            c_visitor.tld.VisitTypealiasStmt(node->as_typealias_unsafe());
            return;
        case ASTNodeKind::StructDecl:
            early_declare_struct(c_visitor, node->as_struct_def_unsafe());
            return;
        case ASTNodeKind::VariantDecl:
            early_declare_variant(c_visitor, node->as_variant_def_unsafe());
            return;
        case ASTNodeKind::UnionDecl:
            early_declare_union(c_visitor, node->as_union_def_unsafe());
            return;
        default:
            return;
    }
}

void early_declare_composed_variables(ToCAstVisitor& visitor, VariablesContainer& container) {
    for(const auto variable : container.variables()) {
        auto t = variable->known_type();
        const auto node = t->get_direct_linked_node();
        if(node) {
            early_declare_node(visitor, node);
        }
    }
    for(auto& inh : container.inherited) {
        const auto node = inh.type->get_direct_linked_node();
        if(node) {
            early_declare_node(visitor, node);
        }
    }
}

void early_declare_type(ToCAstVisitor& visitor, BaseType* type) {
    switch(type->kind()) {
//        case BaseTypeKind::Reference:
//            early_declare_type(visitor, type->as_reference_type_unsafe()->type);
//            return;
//        case BaseTypeKind::Pointer:
//            early_declare_type(visitor, type->as_pointer_type_unsafe()->type);
//            return;
        case BaseTypeKind::Linked:
            early_declare_node(visitor, type->as_linked_type_unsafe()->linked);
            return;
        case BaseTypeKind::Dynamic:
            early_declare_type(visitor, type->as_dynamic_type_unsafe()->referenced);
            return;
        case BaseTypeKind::Array:
            early_declare_type(visitor, type->as_array_type_unsafe()->elem_type);
            return;
        case BaseTypeKind::Generic:
            early_declare_node(visitor, type->as_generic_type_unsafe()->referenced->linked);
            for(const auto ty : type->as_generic_type_unsafe()->types) {
                early_declare_type(visitor, ty);
            }
            return;
        default:
            return;
    }
}

void declare_by_name(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl) {
    if(decl->is_comptime()) {
        return;
    }
    declare_params(tld->value_visitor, decl->params);
    if(decl->returnType->as_function_type() == nullptr) {
        tld->value_visitor->visit(decl->returnType);
    }
    tld->visitor.new_line_and_indent();
    declare_func_with_return(tld->visitor, decl);
    tld->visitor.write(';');
}

// when a function is inside struct / interface
void declare_contained_func(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, bool overrides, StructDefinition* overridden = nullptr) {
    if(decl->is_comptime()) {
        return;
    }
    declare_params(tld->value_visitor, decl->params);
    if(decl->returnType->as_function_type() == nullptr) {
        tld->value_visitor->visit(decl->returnType);
    }
    tld->visitor.new_line_and_indent();
    FunctionParam* param = !decl->params.empty() ? decl->params[0] : nullptr;
    unsigned i = 0;
    auto write_self_param_now = [decl, tld, param, &i, overrides, overridden]() {
        if(param && should_void_pointer_to_self(param->type, param->name, 0, overrides)) {
            if(overridden) {
                tld->write("struct ");
                node_name(tld->visitor, overridden);
                tld->write('*');
                tld->space();
            } else {
                tld->write("void* ");
            }
            tld->write(param->name);
            if(decl->params.size() > 1) {
                tld->write(", ");
            }
            i = 1;
        }
    };
    const auto func_parent = decl->parent();
    const auto func_parent_kind = func_parent->kind();
    const auto is_func_parent_public = func_parent->specifier() == AccessSpecifier::Public;
    auto is_parent_interface = func_parent_kind == ASTNodeKind::InterfaceDecl;
    const auto decl_return_func_type = decl->returnType->as_function_type();
    if(decl_return_func_type != nullptr && !decl_return_func_type->isCapturing()) {
        tld->value_visitor->write("static ");
        accept_func_return(tld->visitor, decl_return_func_type->returnType);
        tld->write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(tld->visitor, decl, decl_return_func_type, i);
    } else {
        accept_func_return_with_name(tld->visitor, decl, (is_parent_interface || decl->body.has_value()) && !decl->is_exported_fast() && !is_func_parent_public);
        tld->write('(');
        write_self_param_now();
        func_type_params(tld->visitor, decl, i);
        tld->write(')');
    }
    if(is_parent_interface) {
        tld->write(" __attribute__((weak))");
    }
    tld->write(';');
}

void CTopLevelDeclarationVisitor::VisitVarInitStmt(VarInitStatement *init) {
    if(!init->is_top_level()) return;
    const auto init_type = init->type ? init->type : init->value->create_type(visitor.allocator);
    early_declare_type(visitor, init_type);
    visitor.new_line_and_indent();
    const auto is_exported = init->is_exported();
    var_init_top_level(visitor, init, init_type, !is_exported, false, is_exported && external_module);
}

void CTopLevelDeclarationVisitor::VisitIfStmt(IfStatement* stmt) {
    if(!stmt->is_top_level()) return;
    if(stmt->computed_scope.has_value()) {
        auto scope = stmt->computed_scope.value();
        if(scope) {
            for(const auto node : scope->nodes) {
                visit(node);
            }
        }
        return;
    }
}

void CTopLevelDeclarationVisitor::declare_func(FunctionDeclaration* decl) {
    // TODO we will fix capturing lambda types when introducing generics and unions
//    if(decl->returnType->function_type() && decl->returnType->function_type()->isCapturing) {
//        visitor->error("Function name " + decl->name + " returns a capturing lambda, which is not supported");
//    }
    declare_by_name(this, decl);
}

void CTopLevelDeclarationVisitor::VisitFunctionDecl(FunctionDeclaration *decl) {
    declare_func(decl);
}

void CTopLevelDeclarationVisitor::VisitGenericFuncDecl(GenericFuncDecl* node) {
    for(const auto impl : node->instantiations) {
        declare_func(impl);
    }
}

void CValueDeclarationVisitor::VisitFunctionDecl(FunctionDeclaration *decl) {
    if(decl->body.has_value() && !decl->is_comptime()) {
        visit(&decl->body.value());
    }
}

void type_def_stmt(ToCAstVisitor& visitor, TypealiasStatement* stmt) {
    early_declare_type(visitor, stmt->actual_type);
    visitor.new_line_and_indent();
    visitor.write("typedef ");
    const auto kind = stmt->actual_type->kind();
    if(kind == BaseTypeKind::Function) {
        const auto func_type = stmt->actual_type->as_function_type_unsafe();
        // TODO: do not store htis
        const auto mangled_name = visitor.mangler.mangle(stmt);
        func_type_with_id(visitor, func_type, chem::string_view(mangled_name));
    } else {
        visitor.visit(stmt->actual_type);
        visitor.write(' ');
        visitor.mangle(stmt);
    }
    visitor.write(';');
}

void CTopLevelDeclarationVisitor::VisitTypealiasStmt(TypealiasStatement *stmt) {
    if(!has_declared(stmt)) {
        set_declared(stmt);
        type_def_stmt(visitor, stmt);
    }
}

void CTopLevelDeclarationVisitor::VisitUnionDecl(UnionDef *def) {
    declare_union_iterations(def);
}

void CTopLevelDeclarationVisitor::VisitNamespaceDecl(Namespace *ns) {
    if(ns->is_comptime()) return;
    for(const auto node : ns->nodes) {
        visit(node);
    }
}

void CTopLevelDeclarationVisitor::declare_struct_def_only(StructDefinition* def) {
    for(const auto mem : def->variables()) {
        value_visitor->visit(mem);
    }
    // before we declare this struct, we must early declare any direct struct type variables
    // inside this struct, because some structs get used inside which are present in other modules
    // will be declared later, so C responds with incomplete type
    early_declare_composed_variables(visitor, *def);
    visitor.new_line_and_indent();
    write("struct ");
    visitor.mangle(def);
    write(" {");
    visitor.indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def) {
            visitor.new_line_and_indent();
            visitor.write("struct ");
            visitor.mangle(struct_def);
            visitor.space();
            visitor.write(struct_def->name_view());
            visitor.write(';');
        }
    }
    for(const auto var : def->variables()) {
        visitor.new_line_and_indent();
        visitor.visit(var);
    }
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    write("};");
}

void CTopLevelDeclarationVisitor::declare_struct_functions(StructDefinition* def) {
    for(auto& func : def->instantiated_functions()) {
        if(def->get_overriding_interface(func) == nullptr) {
            declare_contained_func(this, func, false);
        }
    }
}

static void contained_struct_functions(ToCAstVisitor& visitor, StructDefinition* def);

void CTopLevelDeclarationVisitor::early_declare_struct_def(StructDefinition* def) {
    if(!has_declared(def)) {
        set_declared(def);
        declare_struct_def_only(def);
    }
}

void CTopLevelDeclarationVisitor::declare_union_def_only(UnionDef* def) {
    for(const auto mem : def->variables()) {
        value_visitor->visit(mem);
    }
    // before we declare this struct, we must early declare any direct struct type variables
    // inside this struct, because some structs get used inside which are present in other modules
    // will be declared later, so C responds with incomplete type
    early_declare_composed_variables(visitor, *def);
    visitor.new_line_and_indent();
    write("union ");
    visitor.mangle(def);
    write(" {");
    visitor.indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def) {
            visitor.new_line_and_indent();
            visitor.write("struct ");
            visitor.mangle(struct_def);
            visitor.space();
            visitor.write(struct_def->name_view());
            visitor.write(';');
        }
    }
    for(const auto var : def->variables()) {
        visitor.new_line_and_indent();
        visitor.visit(var);
    }
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    write("};");
}

void CTopLevelDeclarationVisitor::declare_union_functions(UnionDef* def) {
    for(auto& func : def->instantiated_functions()) {
        declare_contained_func(this, func, false);
    }
}

void CTopLevelDeclarationVisitor::early_declare_union_def(UnionDef* def) {
    if(!has_declared(def)) {
        set_declared(def);
        declare_union_def_only(def);
    }
}

void CTopLevelDeclarationVisitor::VisitStructDecl(StructDefinition* def) {
    declare_struct_iterations(def);
}

void CTopLevelDeclarationVisitor::VisitGenericTypeDecl(GenericTypeDecl* node) {
    for(const auto impl : node->instantiations) {
        VisitTypealiasStmt(impl);
    }
}

void CTopLevelDeclarationVisitor::VisitGenericStructDecl(GenericStructDecl* node) {
    for(const auto impl : node->instantiations) {
        VisitStructDecl(impl);
    }
}

void CTopLevelDeclarationVisitor::VisitGenericUnionDecl(GenericUnionDecl* node) {
    for(const auto impl : node->instantiations) {
        VisitUnionDecl(impl);
    }
}

void CTopLevelDeclarationVisitor::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    for(const auto impl : node->instantiations) {
        VisitInterfaceDecl(impl);
    }
}

void CTopLevelDeclarationVisitor::VisitGenericVariantDecl(GenericVariantDecl* node) {
    for(const auto impl : node->instantiations) {
        VisitVariantDecl(impl);
    }
}

void CTopLevelDeclarationVisitor::declare_variant_def_only(VariantDefinition* def) {
    for(const auto mem : def->variables()) {
        value_visitor->visit(mem);
    }
    visitor.new_line_and_indent();
    write("struct ");
    visitor.mangle(def);
    write(" {");
    visitor.indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def) {
            visitor.new_line_and_indent();
            visitor.write("struct ");
            visitor.mangle(struct_def);
            visitor.space();
            visitor.write(struct_def->name_view());
            visitor.write(';');
        }
    }
    new_line_and_indent();
    write("int ");
    write(variant_type_variant_name);
    write(';');
    new_line_and_indent();
    write("union {");
    visitor.indentation_level += 1;
    for(const auto var : def->variables()) {
        visitor.new_line_and_indent();
        const auto member = var->as_variant_member();

        visitor.write("struct ");
        visitor.write('{');
        visitor.indentation_level += 1;
        for(auto& value : member->values) {
            visitor.new_line_and_indent();
            visitor.visit(value.second->type);
            visitor.space();
            visitor.write(value.second->name);
            visitor.write(';');
        }
        visitor.indentation_level -= 1;
        visitor.new_line_and_indent();
        visitor.write("} ");
        visitor.write(member->name);
        visitor.write(';');

    }
    visitor.indentation_level -= 1;
    new_line_and_indent();
    write("};");
    visitor.indentation_level-=1;
    new_line_and_indent();
    write("};");
}

void CTopLevelDeclarationVisitor::declare_variant_functions(VariantDefinition* def) {
    for(auto& func : def->instantiated_functions()) {
        declare_contained_func(this, func, false);
    }
}

void generate_contained_functions(ToCAstVisitor& visitor, VariantDefinition* def);

void CTopLevelDeclarationVisitor::early_declare_variant_def(VariantDefinition* def) {
    if(!has_declared(def)) {
        set_declared(def);
        declare_variant_def_only(def);
    }
}

void CTopLevelDeclarationVisitor::VisitVariantDecl(VariantDefinition *def) {
    declare_variant_iterations(def);
}

void create_v_table(ToCAstVisitor& visitor, InterfaceDefinition* interface, StructDefinition* definition, bool declare_only) {
    visitor.new_line_and_indent();
    if(declare_only) {
        visitor.write("extern ");
    }
    visitor.write("const");
    visitor.write(' ');
    visitor.write("struct");
    visitor.space();
    visitor.write('{');
    visitor.indentation_level += 1;

    const auto prev_active = interface->active_user;
    interface->active_user = definition;

    // type pointers
    for(auto& func : interface->instantiated_functions()) {
        auto self_param = func->get_self_param();
        if(self_param) {
            visitor.new_line_and_indent();
            func_type_with_id_no_params(visitor, func, func->name_view());
            visitor.write("struct ");
            node_name(visitor, definition);
            visitor.write('*');
            visitor.space();
            visitor.write(self_param->name);
            func_type_params(visitor, func, 1, true);
            visitor.write(')');
            visitor.write(';');
        }
    }

    visitor.indentation_level -=1;
    visitor.new_line_and_indent();
    visitor.write('}');
    visitor.space();
    vtable_name(visitor, interface, definition);
    if(!declare_only) {
        visitor.write(" = ");
        visitor.write('{');
        visitor.indentation_level += 1;

        // func pointer values
        for (auto& func: interface->instantiated_functions()) {
            if (func->has_self_param()) {
                visitor.new_line_and_indent();
                visitor.mangle(func);
                visitor.write(',');
            }
        }

        visitor.indentation_level -= 1;
        visitor.new_line_and_indent();
        visitor.write('}');
    }

    interface->active_user = prev_active;

    visitor.write(';');
}

void contained_func_decl(ToCAstVisitor& visitor, FunctionDeclaration* decl, bool overrides, ExtendableMembersContainerNode* def);

static void contained_interface_functions(ToCAstVisitor& visitor, InterfaceDefinition* def) {
    for(auto& func : def->instantiated_functions()) {
        const auto interface = def->get_overriding_interface(func);
        contained_func_decl(visitor, func, interface != nullptr, def);
    }
}

void CTopLevelDeclarationVisitor::declare_interface(InterfaceDefinition* def) {
    const auto is_static = def->is_static();
    for (auto& func: def->instantiated_functions()) {
        if(is_static || !func->has_self_param()) {
            declare_contained_func(this, func, false);
        }
    }
#ifdef COMPILER_BUILD
    if(!external_module && !def->vtable_pointers.empty()) {
        // this only occurs because when we generate interface with both backends without re-parsing, this module
        visitor.error("interface has already vtable pointers generated, suspected interface being used in both backends", def);
#ifdef DEBUG
        throw std::runtime_error("interface being used in both backends c and llvm");
#endif
    }
#endif
    if(!is_static) {
        for (auto& use: def->users) {
            def->active_user = use.first;
            for (auto& func: def->instantiated_functions()) {
                if (func->has_self_param()) {
                    declare_contained_func(this, func, false, use.first);
                }
            }
        }
        def->active_user = nullptr;
        // either create or declare the vtable, depending on whether it has been declared before
        for(auto& user : def->users) {
            const auto linked_struct = user.first;
            if(linked_struct) {
#ifdef COMPILER_BUILD
                auto vtable_done = def->vtable_pointers.find(linked_struct);
                const auto has_vtable_impl = vtable_done != def->vtable_pointers.end();
#else
                const auto has_vtable_impl = user.second;
#endif
                create_v_table(visitor, def, linked_struct, has_vtable_impl);
#ifdef COMPILER_BUILD
                if(!has_vtable_impl) {
                    def->vtable_pointers[linked_struct] = nullptr;
                }
#else
                if(!has_vtable_impl) {
                    def->users[linked_struct] = true;
                }
#endif
            }
        }
    }
}

void CTopLevelDeclarationVisitor::VisitInterfaceDecl(InterfaceDefinition *def) {
    // forward declaring the structs of users, because currently we only need to use them
    // as pointers, even if user returns a struct, the function only takes a pointer (to memcpy)
    for(auto& use : def->users) {
        new_line_and_indent();
        write("struct ");
        node_name(visitor, use.first);
        write(';');
    }
    declare_interface(def);
}

void CTopLevelDeclarationVisitor::VisitImplDecl(ImplDefinition *def) {

}

void CTopLevelDeclarationVisitor::reset() {
    declared.clear();
}

void CValueDeclarationVisitor::VisitFunctionType(FunctionType *type) {
    // TODO remove this method
}

void CValueDeclarationVisitor::VisitStructMember(StructMember *member) {
    if(member->type->kind() != BaseTypeKind::Function) {
        RecursiveValueVisitor::VisitStructMember(member);
    }
}

void CValueDeclarationVisitor::VisitIfStmt(IfStatement *stmt) {
    if(stmt->computed_scope.has_value()) {
        auto scope = stmt->computed_scope.value();
        if(scope) {
            visit(scope);
        }
        return;
    }
    RecursiveValueVisitor::VisitIfStmt(stmt);
}

void declare_fat_pointer(ToCAstVisitor& visitor) {
    visitor.fat_pointer_type = "__chemical_fat_pointer__";
    visitor.write("typedef struct {");
    visitor.indentation_level+=1;
    visitor.new_line_and_indent();
    visitor.write("void* first;");
    visitor.new_line_and_indent();
    visitor.write("void* second;");
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    visitor.write("} ");
    visitor.write(visitor.fat_pointer_type);
    visitor.write(';');
}

void ToCAstVisitor::prepare_translate() {
    write("#include <stdbool.h>\n");
    write("#include <stddef.h>\n");
    // declaring a fat pointer
    declare_fat_pointer(*this);
    // declaring malloc function
    new_line_and_indent();
    if(comptime_scope.target_data.is_win64) {
        write("extern void* malloc(unsigned long long size);");
    } else {
        write("extern void* malloc(unsigned long size);");
    }
}

void ToCAstVisitor::reset() {
    local_allocated.clear();
    evaluated_func_calls.clear();
    destructible_refs.clear();
    current_func_type = nullptr;
    current_members_container = nullptr;
    indentation_level = 0;
    local_temp_var_i = 0;
    nested_value = false;
    return_redirect_block = "";
    declarer->reset();
    tld.reset();
    before_stmt->reset();
    after_stmt->reset();
    destructor->reset();
    tld.reset();
}

ToCAstVisitor::~ToCAstVisitor() = default;

void ToCAstVisitor::write(char value) {
    output->put(value);
}

void ToCAstVisitor::indent() {
    unsigned start = 0;
    while(start < indentation_level) {
        write('\t');
        start++;
    }
}

std::string ToCAstVisitor::get_local_temp_var_name() {
    std::string name;
    name.append("__chx__lv__");
    name.append(std::to_string(local_temp_var_i++));
    return name;
}

void ToCAstVisitor::write(std::string& value) {
    output->write(value.c_str(), (std::streamsize) value.size());
}

void ToCAstVisitor::write_str(const std::string& value) {
    output->write(value.c_str(), (std::streamsize) value.size());
}

void ToCAstVisitor::write(chem::string_view& str) {
    output->write(str.data(), (std::streamsize) str.size());
}

void ToCAstVisitor::write(std::string_view& str) {
    output->write(str.data(), (std::streamsize) str.size());
}

void ToCAstVisitor::write(const chem::string_view& str) {
    output->write(str.data(), (std::streamsize) str.size());
}

std::string ToCAstVisitor::string_accept(Value* any) {
    std::ostringstream stream;
    auto curr_out = output;
    output = &stream;
    visit(any);
    output = curr_out;
    return stream.str();
}

void ToCAstVisitor::VisitVarInitStmt(VarInitStatement *init) {
    if(init->is_top_level()) {
        const auto init_type = init->type ? init->type : init->value->create_type(allocator);
        new_line_and_indent();
        const auto is_exported = init->is_exported();
        var_init_top_level(*this, init, init_type, !is_exported, true, false);
        return;
    }
    auto init_type = init->type ? init->type : init->value->create_type(allocator);
    var_init(*this, init, init_type);
    destructor->VisitVarInitStmt(init);
}

void ToCAstVisitor::VisitAssignmentStmt(AssignStatement *assign) {
    assign_statement(*this, assign);
    write(';');
}

void write_assignable(ToCAstVisitor& visitor, ASTNode* node) {
    const auto k = node->kind();
    switch(k) {
        case ASTNodeKind::VarInitStmt:
            visitor.write(node->as_var_init()->name_view());
            return;
        case ASTNodeKind::AssignmentStmt:
            visitor.visit(node->as_assignment()->lhs);
            return;
        default:
            const auto p = node->parent();
            if(p) {
                write_assignable(visitor, p);
            } else {
                visitor.write("[UnknownAssignable]");
            }
            return;
    }
}

void ToCAstVisitor::VisitBreakStmt(BreakStatement *node) {
    if(node->value) {
        auto val_kind = node->value->val_kind();
        if(val_kind != ValueKind::SwitchValue && val_kind != ValueKind::IfValue && val_kind != ValueKind::LoopValue) {
            write_assignable(*this, node->parent());
            write(" = ");
        }
        auto prev = nested_value;
        nested_value = true;
        visit(node->value);
        nested_value = prev;
        write(';');
        new_line_and_indent();
    }
    write("break;");
}

void ToCAstVisitor::VisitContinueStmt(ContinueStatement *continueStatement) {
    write("continue;");
}

void ToCAstVisitor::VisitUnreachableStmt(UnreachableStmt *stmt) {
    write("unreachable();");
}

void ToCAstVisitor::VisitUnsafeBlock(UnsafeBlock *block) {
    visit(&block->scope);
}

void ToCAstVisitor::VisitImportStmt(ImportStatement *importStatement) {
    // leave imports alone
}

void struct_initialize_inside_braces(ToCAstVisitor& visitor, StructValue* val) {
    visitor.write("(struct ");
    visitor.write(val->linked_name_view());
    visitor.write(")");
    visitor.visit(val);
}

bool ToCAstVisitor::requires_return(Value* val) {
    if(val->as_struct_value()) {
        if (pass_structs_to_initialize) {
            return false;
        } else {
            return true;
        }
    } else {
        const auto valType = val->create_type(allocator);
        if(valType->isStructLikeType()) {
            return false;
        } else {
            return true;
        }
    }
}

void ToCAstVisitor::return_value(Value* val, BaseType* non_canon_type) {
    const auto type = non_canon_type->canonical();
    const auto imp_cons = type->implicit_constructor_for(allocator, val);
    if(imp_cons) {
        if(imp_cons->is_comptime()) {
            const auto imp_call = call_with_arg(imp_cons, val, type, allocator, *this);
            const auto eval = evaluated_func_val_proper(*this, imp_cons, imp_call);
            val = eval;
        } else {
            call_implicit_constructor_no_alloc(*this, imp_cons, val, struct_passed_param_name, true);
            return;
        }
    }
    const auto struct_val = val->as_struct_value();
    const auto val_type = val->create_type(allocator);
    if(struct_val) {
        if(implicit_mutate_value_default(*this, type, val)) {
           return;
        }
        if(pass_structs_to_initialize) {
            auto size = struct_val->values.size();
            unsigned i = 0;
            for(const auto& mem : struct_val->values) {
                auto child_member = struct_val->child_member(mem.first);
                write(struct_passed_param_name);
                write("->");
                write(mem.first);
                write(" = ");
                accept_mutating_value(child_member ? child_member->known_type() : nullptr, mem.second.value, false);
                if(i != size - 1){
                    write(';');
                    new_line_and_indent();
                }
                i++;
            }
        } else {
            struct_initialize_inside_braces(*this, (StructValue*) val);
        }
    } else if(val->kind() == ValueKind::LambdaFunc && type->kind() == BaseTypeKind::CapturingFunction) {
        write('*');
        write(struct_passed_param_name);
        write(" = ");
        accept_mutating_value_explicit(type, val, false);
    } else if(val_type->isStructLikeType()) {
        write('*');
        write(struct_passed_param_name);
        write(" = ");
        if(!implicit_mutate_value_default(*this, type, val)) {
            if(is_value_param_pointer_like(val)) {
                write('*');
            }
            visit(val);
        }
    } else {
        accept_mutating_value_explicit(type, val, false);
    }
}

void ToCAstVisitor::writeReturnStmtFor(Value* returnValue) {
    const auto val = returnValue;
    const auto return_type = current_func_type->returnType;
    std::string saved_into_temp_var;
    const auto has_struct_like_return = return_type->isStructLikeType();
    if(val && has_struct_like_return) {
        return_value(val, return_type);
        write(';');
        new_line_and_indent();
    } else if(val && BaseType::isPrimitiveType(return_type->pure_type(allocator)->kind()) && !destructor->destruct_jobs.empty()) {
        saved_into_temp_var = get_local_temp_var_name();
        write("const ");
        auto type = val->create_type(allocator);
        visit(type);
        space();
        write(saved_into_temp_var);
        write(" = ");
        return_value(val, return_type);
        write(';');
        new_line_and_indent();
    }
    int i = ((int) destructor->destruct_jobs.size()) - 1;
    auto new_line_prev = destructor->new_line_before;
    destructor->new_line_before = false;
    auto current_return = returnValue ? returnValue : nullptr;
    while(i >= 0) {
        destructor->destruct(destructor->destruct_jobs[i], current_return);
        i--;
    }
    destructor->new_line_before = new_line_prev;
    destructor->destroy_current_scope = false;
    if(returnValue) {
//        if(handle_return_after) {
            write("return");
            if(!saved_into_temp_var.empty()) {
                write(' ');
                write(saved_into_temp_var);
            } else if(!has_struct_like_return) {
                write(' ');
                accept_mutating_value_explicit(return_type, val, false);
            }
            write(';');
//        } else {
//            write("return;");
//        }
    } else {
        if(return_redirect_block.empty()) {
            write("return;");
        } else {
            write("goto ");
            write(return_redirect_block);
            write(';');
        }
    }
}

void ToCAstVisitor::VisitReturnStmt(ReturnStatement *returnStatement) {
    writeReturnStmtFor(returnStatement->value);
}

void ToCAstVisitor::VisitDoWhileLoopStmt(DoWhileLoop *doWhileLoop) {
    write("do ");
    scope(*this, doWhileLoop->body);
    write(" while(");
    visit(doWhileLoop->condition);
    write(");");
}

void ToCAstVisitor::VisitEnumDecl(EnumDeclaration *enumDecl) {

}

void ToCAstVisitor::VisitForLoopStmt(ForLoop *forLoop) {
    write("for(");
    visit(forLoop->initializer);
    visit(forLoop->conditionExpr);
    write(';');
    const auto inc_kind = forLoop->incrementerExpr->kind();
    if(inc_kind == ASTNodeKind::AssignmentStmt) {
        assign_statement(*this, forLoop->incrementerExpr->as_assignment_unsafe());
    } else if(inc_kind == ASTNodeKind::ValueWrapper) {
        visit(forLoop->incrementerExpr->as_value_wrapper_unsafe()->value);
    } else {
        visit(forLoop->incrementerExpr);
    }
    write(')');
    scope(*this, forLoop->body);
}

void ToCAstVisitor::VisitFunctionParam(FunctionParam *param) {
    param_type_with_id(*this, param->type, param->name);
}

void func_decl_with_name(ToCAstVisitor& visitor, FunctionDeclaration* decl) {
    if(!decl->body.has_value() || decl->is_comptime()) {
        return;
    }
    auto prev_func_decl = visitor.current_func_type;
    visitor.current_func_type = decl;
    visitor.new_line_and_indent();
    const auto decl_ret_func = decl->returnType->as_function_type();
    if(decl_ret_func && !decl_ret_func->isCapturing()) {
        func_that_returns_func_proto(visitor, decl, decl_ret_func);
    } else {
        declare_func_with_return(visitor, decl);
    }
    // before generating function's body, it's very important we clear the cached comptime calls
    // because multiple generic functions must re-evaluate the comptime function call
    visitor.evaluated_func_calls.clear();
    scope(visitor, decl->body.value(), decl);
    visitor.current_func_type = prev_func_decl;
}

void call_variant_param_func(
    ToCAstVisitor& visitor,
    VariantMember* member,
    VariantMemberParam* mem_param,
    FunctionDeclaration*(*choose_func)(MembersContainer*)
) {
    auto mem_type = mem_param->type;
    const auto mem_def = mem_type->get_members_container();
    if(!mem_def) {
        return;
    }
    auto func = choose_func(mem_def);
    if (!func) {
        return;
    }
    visitor.new_line_and_indent();
    visitor.mangle(func);
    visitor.write('(');
    if (func->has_self_param()) {
        visitor.write("&self->");
        visitor.write(member->name);
        visitor.write('.');
        visitor.write(mem_param->name);
    }
    visitor.write(')');
    visitor.write(';');
}

inline void call_variant_member_fn(ToCAstVisitor& visitor, VariantMember* member, FunctionDeclaration*(*choose_func)(MembersContainer*)) {
    for(auto& mem_param : member->values) {
        call_variant_param_func(visitor, member, mem_param.second, choose_func);
    }
}

void call_variant_member_delete_fn(ToCAstVisitor& visitor, VariantMember* member) {
    call_variant_member_fn(visitor, member, [](MembersContainer* container)-> FunctionDeclaration* {
        return container->destructor_func();
    });
}

void variant_member_pre_move_fn_gen(
    ToCAstVisitor& visitor,
    VariantMember* member,
    FunctionDeclaration* func,
    MembersContainer* mem_def,
    VariantMemberParam* mem_param
) {
    // copy func call
    visitor.new_line_and_indent();
    visitor.mangle(func);
    visitor.write('(');

    // writing the self arg
    visitor.write("&self->");
    visitor.write(member->name);
    visitor.write('.');
    visitor.write(mem_param->name);
    visitor.write(", ");

    // writing the other arg
    visitor.write('&');
    visitor.write(visitor.current_func_type->params[1]->name);
    visitor.write("->");
    visitor.write(member->name);
    visitor.write('.');
    visitor.write(mem_param->name);

    visitor.write(')');
    visitor.write(';');
}

typedef void(VariantMemberProcessFn)(
        ToCAstVisitor& visitor,
        VariantMember* member,
        FunctionDeclaration* func,
        MembersContainer* mem_def,
        VariantMemberParam* mem_param
);

typedef FunctionDeclaration*(VariantMemberFuncSelFn)(MembersContainer* container);

void process_variant_member_using(
    ToCAstVisitor& visitor,
    VariantMember* member,
    VariantMemberFuncSelFn* func_sel_fn,
    VariantMemberProcessFn* variant_member_process_fn
) {
    for(auto& mem_param_pair : member->values) {
        auto mem_param = mem_param_pair.second;
        auto mem_type = mem_param->type;
        const auto linked = mem_type->linked_node();
        auto mem_def = linked->as_members_container();
        auto func = func_sel_fn(mem_def);
        if (!func) {
            return;
        }
        variant_member_process_fn(visitor, member, func, mem_def, mem_param);
    }
}

void call_variant_member_copy_fn(
    ToCAstVisitor& visitor,
    VariantMember* member
) {
    process_variant_member_using(visitor, member, [](MembersContainer* container) -> FunctionDeclaration* {
        return container->copy_func();
    },variant_member_pre_move_fn_gen);
}

void process_variant_members_using(
    ToCAstVisitor& visitor,
    ExtendableMembersContainerNode* def,
    void(*process_member)(ToCAstVisitor& visitor, VariantMember* member)
) {
    visitor.new_line_and_indent();
    visitor.write("switch(self->");
    visitor.write(variant_type_variant_name);
    visitor.write(") {");
    visitor.indentation_level += 1;
    unsigned index = 0;
    for (const auto var : def->variables()) {
        visitor.new_line_and_indent();
        visitor.write("case ");
        *visitor.output << index;
        visitor.write(':');
        const auto member = var->as_variant_member();
        process_member(visitor, member);
        visitor.new_line_and_indent();
        visitor.write("break;");
        index++;
    }
    visitor.indentation_level -= 1;
    visitor.new_line_and_indent();
    visitor.write('}');
}

void write_type_assignment_in_variant_copy(ToCAstVisitor& visitor, FunctionDeclaration* func) {
    // setting type to the other param
    // self->__chx__vt_621827 = other->__chx__vt_621827
    visitor.new_line_and_indent();
    visitor.write(func->params[0]->name);
    visitor.write("->");
    visitor.write(variant_type_variant_name);
    visitor.write(" = ");
    visitor.write(func->params[1]->name);
    visitor.write("->");
    visitor.write(variant_type_variant_name);
    visitor.write(';');
}

void call_struct_members_pre_move_fn(
        ToCAstVisitor& visitor,
        MembersContainer* mem_def,
        FunctionDeclaration* func,
        const chem::string_view& member_name
) {
    visitor.new_line_and_indent();
    visitor.mangle(func);
    visitor.write('(');
    // writing the self arg
    visitor.write("&self->");
    visitor.write(member_name);
    visitor.write(", ");
    // writing the other arg
    visitor.write('&');
    visitor.write(visitor.current_func_type->params[1]->name);
    visitor.write("->");
    visitor.write(member_name);
    visitor.write(')');
    visitor.write(';');
}

void call_struct_members_copy_fn(
        ToCAstVisitor& visitor,
        BaseType* mem_type,
        const chem::string_view& member_name
) {
    if (mem_type->isStructLikeType()) {
        const auto linked = mem_type->linked_node();
        auto mem_def = linked->as_members_container();
        auto func = mem_def->copy_func();
        if (!func) {
            return;
        }
        call_struct_members_pre_move_fn(visitor, mem_def, func, member_name);
    }
}

void process_struct_members_using(
    ToCAstVisitor& visitor,
    ExtendableMembersContainerNode* def,
    void(*process_member)(ToCAstVisitor& visitor, BaseType* member_type, const chem::string_view& member_name)
) {
    for(auto& inherits : def->inherited) {
        auto linked = inherits.type->linked_struct_def();
        if(linked) {
            process_member(visitor, inherits.type, linked->name_view());
        }
    }
    for (const auto var : def->variables()) {
        auto value_type = var->known_type();
        process_member(visitor, value_type, var->name);
    }
}

void initialize_def_struct_values_constructor(ToCAstVisitor& visitor, FunctionDeclaration* decl) {
    auto parent = decl->parent();
    if(!parent) return;
    if(!decl->is_constructor_fn()) {
        return;
    }
    const auto struct_def = parent->as_struct_def();
    if(!struct_def) return;
    std::unordered_map<chem::string_view, InitBlockInitializerValue>* initializers = nullptr;
    if(decl->body.has_value() && !decl->body->nodes.empty()) {
        auto block = decl->body->nodes.front()->as_init_block();
        if(block) {
            initializers = &block->initializers;
        }
    }
    for(auto& inh : struct_def->inherited) {
        auto& var = inh;
        const auto var_type = var.type->pure_type(visitor.allocator);
        const auto def = var_type->get_direct_linked_struct();
        if(def) {
            auto has_initializer = initializers && initializers->find(def->name_view()) != initializers->end();
            if(has_initializer) {
                continue;
            }
            const auto defConstructor = def->default_constructor_func();
            if(defConstructor) {
                visitor.new_line_and_indent();
                visitor.mangle(defConstructor);
                visitor.write("(&this->");
                visitor.write(def->name_view());
                visitor.write(");");
            }
        }
    }
    for(const auto var : struct_def->variables()) {
        const auto defValue = var->default_value();
        auto has_initializer = initializers && initializers->find(var->name) != initializers->end();
        // TODO currently we check if the value has a initializer in the init block
        // TODO we should check whether the value has an assignment in the function as well (or use that to initialize it)
        if(has_initializer) {
            continue;
        }
        if(!defValue) {
            // since default value doesn't exist, however the variable maybe of type struct and have a default constructor
            // we must call the default non argument constructor automatically
            const auto mem_type = var->known_type();
            const auto mem_pure = mem_type->pure_type(visitor.allocator);
            const auto def = mem_pure->get_direct_linked_struct();
            if(def) {
                const auto defConstructor = def->default_constructor_func();
                if(defConstructor) {
                    visitor.new_line_and_indent();
                    visitor.mangle(defConstructor);
                    visitor.write("(&this->");
                    visitor.write(var->name);
                    visitor.write(");");
                } else {
                    // struct has no default value
                    // struct has no default constructor
                    // if struct doesn't have an initializer block, we should generate an error
                    // TODO cannot generate an error, as some members are being initialized via equal, as the initializer block is not good enough
                    // visitor.error("member doesn't have a default constructor, default value or initializer", var.second);
                }
            }
            continue;
        }
        visitor.new_line_and_indent();
        visitor.write("this->");
        visitor.write(var->name);
        visitor.write(" = ");
        visitor.visit(defValue);
        visitor.write(';');
    }
}

void contained_func_decl(ToCAstVisitor& visitor, FunctionDeclaration* decl, bool overrides, ExtendableMembersContainerNode* def) {
    if(!decl->body.has_value() || decl->is_comptime()) {
        return;
    }
    auto prev_func_decl = visitor.current_func_type;
    visitor.current_func_type = decl;
    visitor.new_line_and_indent();
//    std::string self_pointer_name;
    FunctionParam* param = !decl->params.empty() ? decl->params[0] : nullptr;
    unsigned i = 0;
    const auto interface = def && overrides ? def->get_overriding_interface(decl) : nullptr;
    auto write_self_param_now = [decl, &visitor, param, &i, overrides, def, interface]() {
        if(param && should_void_pointer_to_self(param->type, param->name, 0, overrides)) {
//            self_pointer_name = "__ch_self_pointer_329283";
//            visitor->write("void* ");
            if(interface && interface->is_static()) {
                visitor.write("void* ");
                visitor.write(static_interface_passed_param_name);
            } else {
                visitor.write("struct ");
                node_name(visitor, def);
                visitor.write('*');
                visitor.space();
                visitor.write(param->name);
            }
//            visitor->write(self_pointer_name);
            if(decl->params.size() > 1) {
                visitor.write(", ");
            }
            i = 1;
        }
    };
    const auto decl_ret_func = decl->returnType->as_function_type();
    if(decl_ret_func != nullptr && !decl_ret_func->isCapturing()) {
        visitor.write("static ");
        accept_func_return(visitor, decl_ret_func->returnType);
        visitor.write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(visitor, decl, decl_ret_func, i);
    } else {
        accept_func_return_with_name(visitor, decl, decl->body.has_value() && !decl->is_exported_fast());
        visitor.write('(');
        write_self_param_now();
        func_type_params(visitor, decl, i);
        visitor.write(')');
    }
    visitor.write('{');
    visitor.indentation_level+=1;
    if(interface && interface->is_static()) {
        visitor.new_line_and_indent();
        visitor.write("struct ");
        visitor.mangle(def);
        visitor.write('*');
        visitor.space();
        visitor.write("self = ");
        visitor.write(static_interface_passed_param_name);
        visitor.write(';');
    }
    const auto is_destructor = decl->is_delete_fn();
    const auto is_copy_fn = decl->is_copy_fn();
    const bool has_cleanup_block = is_destructor;
    std::string cleanup_block_name;
    if(has_cleanup_block) {
        cleanup_block_name = "__chx__dstctr_clnup_blk__";
        visitor.return_redirect_block = cleanup_block_name;
    }
    const auto struc_def = def ? def->as_struct_def() : nullptr;
    const auto variant_def = def ? def->as_variant_def() : nullptr;
    unsigned begin = visitor.destructor->destruct_jobs.size();
    visitor.destructor->queue_destruct_decl_params(decl);
    if(is_copy_fn) {
        if(struc_def) {
            process_struct_members_using(visitor, struc_def, call_struct_members_copy_fn);
        } else if(variant_def) {
            write_type_assignment_in_variant_copy(visitor, decl);
            process_variant_members_using(visitor, variant_def, call_variant_member_copy_fn);
        }
    }
    initialize_def_struct_values_constructor(visitor, decl);
    // before generating function's body, it's very important we clear the cached comptime calls
    // because multiple generic functions must re-evaluate the comptime function call
    visitor.evaluated_func_calls.clear();
    visitor.visit_scope(&decl->body.value(), (int) begin);
    if(has_cleanup_block) {
        visitor.new_line_and_indent();
        visitor.write(cleanup_block_name);
        visitor.write(":{");
        visitor.indentation_level++;
        if(is_destructor) {
            if (struc_def) {
                process_struct_members_using(visitor, def, call_struct_member_delete_fn);
            } else if (variant_def) {
                process_variant_members_using(visitor, def, call_variant_member_delete_fn);
            }
        }
        visitor.indentation_level--;
        visitor.new_line_and_indent();
        visitor.write("}");
        visitor.return_redirect_block = "";
    }
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    visitor.write('}');
    visitor.current_func_type = prev_func_decl;
}

void ToCAstVisitor::VisitFunctionDecl(FunctionDeclaration *decl) {
    func_decl_with_name(*this, decl);
}

void ToCAstVisitor::VisitGenericFuncDecl(GenericFuncDecl* node) {
    auto& i = node->total_bodied_instantiations;
    const auto total = node->instantiations.size();
    while(i < total) {
        func_decl_with_name(*this, node->instantiations[i]);
        i++;
    }
}

void do_patt_mat_expr(ToCAstVisitor& visitor, PatternMatchExpr* value) {
    const auto elseKind = value->elseExpression.kind;
    const auto type = value->expression->create_type(visitor.allocator);
    visitor.visit(type);
    visitor.write('*');
    auto varName = visitor.get_local_temp_var_name();
    visitor.write(' ');
    visitor.write(varName);
    visitor.write(" = ");
    if(!is_value_param_hidden_pointer(value->expression)) {
        visitor.write('&');
    }
    visitor.visit(value->expression);
    visitor.write(';');
    visitor.local_allocated[value] = varName;
}

void do_patt_mat_expr_cond(ToCAstVisitor& visitor, PatternMatchExpr* value) {
    auto found = visitor.local_allocated.find(value);
#ifdef DEBUG
    if(found == visitor.local_allocated.end()) {
        throw std::runtime_error("this shouldn't fail");
    }
#endif
    visitor.write(found->second);
    visitor.write("->");
    visitor.write(variant_type_variant_name);
    visitor.write(" == ");
    const auto mem = value->member;
    const auto def = mem->parent();
    *visitor.output << def->direct_child_index(mem->name);
}

void ToCAstVisitor::VisitIfStmt(IfStatement *decl) {
    // generating code for compile time if statements
    if(decl->computed_scope.has_value()) {
        auto scope = decl->computed_scope.value();
        if(scope) {
            visit(scope);
        }
        return;
    } else if(decl->is_computable) {
        auto scope = decl->resolve_evaluated_scope(comptime_scope, *this);
        if(scope.has_value()) {
           if(scope.value()) {
               visit(scope.value());
           }
           return;
        }
    }
    if(decl->condition->kind() == ValueKind::PatternMatchExpr) {
        do_patt_mat_expr(*this, decl->condition->as_pattern_match_expr_unsafe());
        new_line_and_indent();
    }
    // generating code for normal if statements
    write("if(");
    nested_value = true;
    if(decl->condition->kind() == ValueKind::PatternMatchExpr) {
        do_patt_mat_expr_cond(*this, decl->condition->as_pattern_match_expr_unsafe());
    } else {
        visit(decl->condition);
    }
    nested_value = false;
    write(')');
    scope(*this, decl->ifBody);
    unsigned i = 0;
    while(i < decl->elseIfs.size()) {
        auto& elif = decl->elseIfs[i];
        write("else if(");
        visit(elif.first);
        write(')');
        scope(*this, elif.second);
        i++;
    }
    if(decl->elseBody.has_value()) {
        write(" else ");
        scope(*this, decl->elseBody.value());
    }
}

void ToCAstVisitor::VisitImplDecl(ImplDefinition *def) {
    const auto overrides = def->struct_type != nullptr;
    const auto linked_interface = def->interface_type->linked_node()->as_interface_def();
    const auto linked_struct = def->struct_type ? def->struct_type->linked_struct_def() : nullptr;
    for(auto& func : def->instantiated_functions()) {
        contained_func_decl(*this, func, overrides,linked_struct);
    }
}

void ToCAstVisitor::VisitInterfaceDecl(InterfaceDefinition *def) {

}

void ToCAstVisitor::visit_scope(Scope *scope, unsigned destruct_begin) {
    auto prev = top_level_node;
    top_level_node = false;
    for(const auto node : scope->nodes) {
        new_line_and_indent();
        before_stmt->visit(node);
        visit(node);
        after_stmt->visit(node);
    }
    if(destructor->destroy_current_scope) {
        destructor->dispatch_jobs_from_no_clean((int) destruct_begin);
    } else {
        destructor->destroy_current_scope = true;
    }
    auto itr = destructor->destruct_jobs.begin() + destruct_begin;
    destructor->destruct_jobs.erase(itr, destructor->destruct_jobs.end());
    top_level_node = prev;
}

void ToCAstVisitor::VisitScope(Scope *scope) {
    visit_scope(scope, destructor->destruct_jobs.size());
}

void ToCAstVisitor::VisitUnnamedUnion(UnnamedUnion *def) {
    write("union ");
    write('{');
    indentation_level+=1;
    for(const auto var : def->variables()) {
        new_line_and_indent();
        visit(var);
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
    space();
    write(def->name);
    write(';');
}

void ToCAstVisitor::VisitUnnamedStruct(UnnamedStruct *def) {
    write("struct ");
    write('{');
    indentation_level+=1;
    for(const auto var : def->variables()) {
        new_line_and_indent();
        visit(var);
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
    space();
    write(def->name);
    write(';');
}

static void contained_struct_functions(ToCAstVisitor& visitor, StructDefinition* def) {
    for(auto& func : def->instantiated_functions()) {
        const auto overriding = def->get_func_overriding_info(func);
        contained_func_decl(visitor, func, overriding.base_func != nullptr, def);
    }
}

static void contained_union_functions(ToCAstVisitor& visitor, UnionDef* def) {
    for(auto& func : def->instantiated_functions()) {
        const auto overriding = def->get_func_overriding_info(func);
        contained_func_decl(visitor, func, overriding.base_func != nullptr, def);
    }
}

void ToCAstVisitor::VisitStructDecl(StructDefinition *def) {
    auto prev_members_container = current_members_container;
    current_members_container = def;
    for (auto& inherits: def->inherited) {
        const auto overridden = inherits.type->linked_node()->as_interface_def();
        if (overridden) {
            for (auto& func: overridden->instantiated_functions()) {
                if (!def->contains_func(func->name_view())) {
                    contained_func_decl(*this, func, false, def);
                }
            }
        }
    }
    contained_struct_functions(*this, def);
    current_members_container = prev_members_container;
}

void ToCAstVisitor::VisitGenericStructDecl(GenericStructDecl* node) {
    auto& i = node->total_bodied_instantiations;
    const auto total = node->instantiations.size();
    while(i < total) {
        VisitStructDecl(node->instantiations[i]);
        i++;
    }
}

void ToCAstVisitor::VisitGenericUnionDecl(GenericUnionDecl* node) {
    auto& i = node->total_bodied_instantiations;
    const auto total = node->instantiations.size();
    while(i < total) {
        VisitUnionDecl(node->instantiations[i]);
        i++;
    }
}

void ToCAstVisitor::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    auto& i = node->total_bodied_instantiations;
    const auto total = node->instantiations.size();
    while(i < total) {
        VisitInterfaceDecl(node->instantiations[i]);
        i++;
    }
}

void ToCAstVisitor::VisitGenericVariantDecl(GenericVariantDecl* node) {
    auto& i = node->total_bodied_instantiations;
    const auto total = node->instantiations.size();
    while(i < total) {
        VisitVariantDecl(node->instantiations[i]);
        i++;
    }
}

void generate_contained_functions(ToCAstVisitor& visitor, VariantDefinition* def) {
    for(auto& func : def->instantiated_functions()) {
        contained_func_decl(visitor, func, false, def);
    }
}

void ToCAstVisitor::VisitVariantDecl(VariantDefinition* def) {
    if(def->iterations_body_done == 0) {
        generate_contained_functions(*this, def);
        def->iterations_body_done = 1;
    }
}

void ToCAstVisitor::VisitUnionDecl(UnionDef *def) {
    auto prev_members_container = current_members_container;
    current_members_container = def;
    for (auto& inherits: def->inherited) {
        const auto overridden = inherits.type->linked_node()->as_interface_def();
        if (overridden) {
            for (auto& func: overridden->instantiated_functions()) {
                if (!def->contains_func(func->name_view())) {
                    contained_func_decl(*this, func, false, def);
                }
            }
        }
    }
    contained_union_functions(*this, def);
    current_members_container = prev_members_container;
}

void ToCAstVisitor::VisitNamespaceDecl(Namespace *ns) {
    for(const auto node : ns->nodes) {
        visit(node);
    }
}

void ToCAstVisitor::VisitDeleteStmt(DestructStmt *stmt) {

    bool new_line_before = true;

    auto data = stmt->get_data(allocator);

    if(data.destructor_func == nullptr) {
        return;
    }

    auto prev_nested = nested_value;
    nested_value = true;
    auto self_name = string_accept(stmt->identifier);
    nested_value = prev_nested;
    UBigIntValue siz_val(data.array_size, ZERO_LOC);

    if(stmt->is_array) {
        destructor->destruct_arr_ptr(chem::string_view(self_name.data(), self_name.size()), data.array_size != 0 ? &siz_val : stmt->array_value, data.parent_node, data.destructor_func);
    } else {
        destructor->destruct(chem::string_view(self_name.data(), self_name.size()), data.parent_node, data.destructor_func, true);
    }

}

void ToCAstVisitor::VisitIsValue(IsValue *isValue) {
    bool result = false;
    auto comp_time = isValue->get_comp_time_result();
    if(comp_time.has_value()) {
        result = comp_time.value();
    } else {
        const auto linked = isValue->type->get_direct_linked_node();
        if(linked->kind() == ASTNodeKind::VariantMember) {
            const auto mem = linked->as_variant_member_unsafe();
            const auto var = mem->parent();
            // turn on the active iteration of the variant
            write('(');
            visit(isValue->value);
            write_accessor(*this, isValue->value, nullptr);
            write(variant_type_variant_name);
            if(isValue->is_negating) {
                write(" != ");
            } else {
                write(" == ");
            }
            *output << var->variable_index(mem->name, false);
            write(')');
            return;
        }
    }
    write(result ? '1' : '0');
}

void ToCAstVisitor::VisitNewTypedValue(NewTypedValue *value) {
    write("malloc(sizeof(");
    visit(value->type);
    write("))");
}

void ToCAstVisitor::VisitNewValue(NewValue *value) {
    const auto value_kind = value->value->val_kind();
    if(value_kind == ValueKind::StructValue || value_kind == ValueKind::AccessChain) {
        auto value_type = value->value->create_type(allocator);
        write("({ ");
        visit(value_type);
        write("* ");
        auto temp_name = get_local_temp_var_name();
        write(temp_name);
        write(" = ");
        write("malloc(sizeof(");
        visit(value_type);
        write(")); *");
        write(temp_name);
        write(" = ");
        visit(value->value);
        write("; ");
        write(temp_name);
        write("; })");
    } else {
        error("unknown value", value);
    }
}

void ToCAstVisitor::VisitPlacementNewValue(PlacementNewValue *value) {
    const auto value_kind = value->value->val_kind();
    if(value_kind == ValueKind::StructValue || value_kind == ValueKind::AccessChain) {
        auto value_type = value->value->create_type(allocator);
        write("({ ");
        visit(value_type);
        write("* ");
        auto temp_name = get_local_temp_var_name();
        write(temp_name);
        write(" = ");
        visit(value->pointer);
        write("; *");
        write(temp_name);
        write(" = ");
        visit(value->value);
        write("; ");
        write(temp_name);
        write("; })");
    } else {
        error("unknown value", value);
    }
}

void ToCAstVisitor::VisitWhileLoopStmt(WhileLoop *whileLoop) {
    write("while(");
    visit(whileLoop->condition);
    write(") ");
    scope(*this, whileLoop->body);
}

void ToCAstVisitor::VisitLoopBlock(LoopBlock *loop) {
    write("while(1)");
    scope(*this, loop->body);
}

void ToCAstVisitor::VisitVariantCase(VariantCase *variant_case) {
    const auto member = variant_case->member;
    *output << member->parent()->direct_child_index(member->name);
}

void ToCAstVisitor::VisitIncDecValue(IncDecValue *value) {
    if(!value->post) {
        write(value->increment ? "++" : "--");
    }
    const auto type = value->value->create_type(allocator);
    if(type && type->pure_type(allocator)->getLoadableReferredType() != nullptr) {
        if(value->post) {
            write('(');
        }
        write('*');
        visit(value->value);
        if(value->post) {
            write(')');
            write(value->increment ? "++" : "--");
        }
        return;
    }
    visit(value->value);
    if(value->post) {
        write(value->increment ? "++" : "--");
    }
}

void capture_call(ToCAstVisitor& visitor, FunctionType* type, FunctionCall* func_call) {
    visitor.write('(');
    visitor.write('(');
    visitor.write('(');
    func_type_with_id(visitor, type, "");
    visitor.write(") ");
    // TODO func_call parent value is being accepted twice - 1
    visitor.visit(func_call->parent_val);
    visitor.write("->first");
    visitor.write(')');
    visitor.write('(');
    if(type->has_self_param()) {
        visitor.write('&');
        auto parent_val = get_parent_from(func_call->parent_val);
        visitor.visit(parent_val);
        visitor.write(", ");
    }
    // TODO func_call parent value is being accepted twice - 2
    visitor.visit(func_call->parent_val);
    visitor.write("->second");
    if(!func_call->values.empty()) {
        visitor.write(',');
    }
    func_call_args(visitor, func_call, type);
    visitor.write(')');
    visitor.write(')');
}

//void func_call(ToCAstVisitor& visitor, FunctionType* type, std::unique_ptr<Value>& current, std::unique_ptr<Value>& next, unsigned int& i) {
//    if(type->isCapturing() && current->as_func_call() == nullptr) {
//        capture_call(visitor, type, next->as_func_call(), [&current, &visitor](){
//            current->accept(&visitor);
//        }, [&current, &visitor] {
//            visitor.write("NO_SELF_REF_FOR_CAP_LAMB");
//        });
//        i++;
//    } else {
//        current->accept(&visitor);
//        func_call(visitor, next->as_func_call(), type);
//        i++;
//    }
//}

// this automatically determines which parent to pass through
void func_container_name(ToCAstVisitor& visitor, FunctionDeclaration* func_node) {
    const auto parent = func_node->ASTNode::parent();
    if(parent) {
        const auto struct_parent = parent->as_struct_def();
        const auto impl_parent = parent->as_impl_def();
        if(impl_parent) {
            const auto interface_def = impl_parent->interface_type->linked_node()->as_interface_def();
            node_parent_name(visitor, struct_parent);
            visitor.write(interface_def->name_view());
            func_name(visitor, func_node);
            return;
        }
        if(struct_parent) {
            if(func_node->has_self_param()) {
                visitor.mangle(struct_parent);
                func_name(visitor, func_node);
                return;
            }
            const auto interface = struct_parent->get_overriding_interface(func_node);
            if(interface) {
                node_parent_name(visitor, struct_parent);
                visitor.write(interface->name_view());
                func_name(visitor, func_node);
                return;
            } else {
                visitor.mangle(struct_parent);
                func_name(visitor, func_node);
                return;
            }
        }
    }
    node_parent_name(visitor, func_node);
    func_name(visitor, func_node);
}

void write_path_to_child(ToCAstVisitor& visitor, std::vector<int>& path, ExtendableMembersContainerNode* def) {
    int i = 0;
    int last = (((int) path.size()) - 1);
    while(i < last) {
        const auto seg = path[i];
        auto& base = def->inherited[seg];
        def = base.type->linked_struct_def();
        visitor.write(def->name_view());
        visitor.write('.');
        i++;
    }
}

void write_path_to_member(ToCAstVisitor& visitor, ExtendableMembersContainerNode* member_def, BaseDefMember* member) {
    // current member name member->name;
    const auto mem = member_def->direct_variable(member->name);
    if(!mem) {
        std::vector<int> path;
        path.reserve(5);
        auto found = member_def->build_path_to_child(path, member->name);
        if(found) {
            write_path_to_child(visitor, path, member_def);
        }
    }
}

void accept_opt_nestable(ToCAstVisitor& visitor, ChainValue* value, bool is_nested) {
    if(is_nested) {
        const auto prev_nested = visitor.nested_value;
        visitor.nested_value = true;
        visitor.visit(value);
        visitor.nested_value = prev_nested;
    } else {
        visitor.visit(value);
    }
}

void chain_value_accept(ToCAstVisitor& visitor, ChainValue* previous, ChainValue* value, ChainValue* next) {
//    const auto linked = value->linked_node();
    const auto value_kind = value->val_kind();
    if (value_kind == ValueKind::Identifier) {
        const auto id = value->as_identifier_unsafe();
        const auto member = id->linked_node()->as_base_def_member();
        if (member) {
            if(previous) {
                // user wrote 'self.' before the member access
                const auto prev_type = previous->get_pure_type(visitor.allocator);
                const auto previous_def = prev_type->linked_struct_def();
                if (previous_def) {
                    write_path_to_member(visitor, previous_def, member);
                }
            } else {
                // does not have 'self.', since 'a' is an identifier
                // when it is accepted it visits the identifier, which will determine the path to member and write it
//                    auto self_param = visitor.current_func_type->get_self_param();
//                    if(self_param) {
//                        visitor.write(self_param->name);
//                        visitor.write("->");
//                    }
//                    const auto parent = member->parent();
//                    if(parent) {
//                        const auto def = parent->create_val_type()->get_direct_linked_struct();
//                        if(def) {
//                            write_path_to_member(visitor, def, member);
//                        }
//                    }
            }
        }
        visitor.write_identifier(id, previous == nullptr);
        return;
    }
//    if(previous != nullptr && linked && linked->as_variant_case_var()) {
//        const auto var = linked->as_variant_case_var();
//        Value* expr = var->switch_statement->expression;
//        const auto var_mem = var->parent_val->linked_node()->as_variant_member();
//        expr->accept(&visitor);
//        write_accessor(visitor, expr, nullptr);
//        visitor.write(var_mem->name);
//        visitor.write('.');
//    }
    accept_opt_nestable(visitor, value, next != nullptr);
}

void write_enum(ToCAstVisitor& visitor, EnumMember* member) {
    if(member->init_value) {
        visitor.visit(member->init_value);
    } else {
        *visitor.output << member->get_default_index();
    }
}

void access_chain(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, const unsigned start, const unsigned end);

// this function is called, with start index to the chain value which is definitely a function call
// we check if it's a function call to a struct which has a destructor
// if it does have a destructor, we store the accessed value and destruct the struct afterwards
bool write_destructible_call_chain_values(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned int start, unsigned int end) {
    // a function is called on an object that's returned from a function (the object is destructible)
    // for example give_destructible().call_on_it()
    // we find this by checking if last value is a function decl
    const auto last = values[end-1];
    const auto is_call_to_func = last->kind() == ValueKind::Identifier && last->as_identifier_unsafe()->linked->kind() == ASTNodeKind::FunctionDecl;
    if(is_call_to_func) {
        return false;
    }
    // user is making a function call
    // and there's a next value meaning call().next <-- next identifier is accessed from returned struct of the function call
    // we need to check if the function returns a struct that has a destructor so we can generate code to destruct it properly
    const auto func_call = values[start]->as_func_call_unsafe();
    const auto func_type = func_call->function_type(visitor.allocator);
    if(func_type) {
        const auto pure_return = func_type->returnType->pure_type(visitor.allocator);
        const auto memContainer = pure_return->get_members_container();
        if(memContainer) {
            const auto destructorFn = memContainer->destructor_func();
            if (destructorFn) {

                visitor.write("({ ");
                visitor.visit(memContainer->known_type());
                visitor.write("* ");

                // the pointer to constructed struct
                const auto temp_struct_ptr = visitor.get_local_temp_var_name();
                visitor.write_str(temp_struct_ptr);
                visitor.write(" = &");
                accept_opt_nestable(visitor, func_call, true);
                visitor.write("; ");

                // saving the accessed thing pointer
                // the pointer to saved variable so we can access it after destruction
                const auto temp_saved_var = visitor.get_local_temp_var_name();
                const auto last_type = values[end - 1]->create_type(visitor.allocator);
                visitor.visit(last_type);
                visitor.write(' ');
                visitor.write_str(temp_saved_var);
                visitor.write(" = ");
                visitor.write_str(temp_struct_ptr);
                visitor.write("->");
                access_chain(visitor, values, start + 1, end);
                visitor.write("; ");

                // destructing the struct which was accessed
                visitor.mangle(destructorFn);
                visitor.write('(');
                if(destructorFn->has_self_param()) {
                    visitor.write_str(temp_struct_ptr);
                }
                visitor.write("); ");

                // returning the saved temporary variable
                visitor.write_str(temp_saved_var);
                visitor.write("; })");

                return true;
            }
        }
    }
    return false;
}

void chain_after_func(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, const unsigned start, const unsigned end) {
    const auto total_size = values.size();
    unsigned index = start;
    while(index < end) {
        const auto previous = index >= 1 ? values[index - 1] : nullptr;
        const auto current = values[index];
        const auto next = index + 1 < total_size ? values[index + 1] : nullptr;
        chain_value_accept(visitor, previous, current, next);
        if(next) {
            write_accessor(visitor, current, next);
        }
        index++;
    }
}

// the end index is exclusive
void call_any_function_above(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, const int start, int end) {
    end--;
    while(end >= start) {
        const auto value = values[end];
        if(value->val_kind() == ValueKind::FunctionCall) {
            visitor.visit(value);
            visitor.write(", ");
            return;
        }
        end--;
    }
}

void access_chain(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, const unsigned start, const unsigned end) {
    if(end == start) {
        return;
    } else if(end - start == 1) {
        chain_value_accept(visitor, nullptr, values[start], nullptr);
        return;
    }
    // from here, there's more than one value in access chain
    const auto first = values[start];
    if(first->val_kind() == ValueKind::FunctionCall && write_destructible_call_chain_values(visitor, values, start, end)) {
        return;
    }
    const auto last = values[end - 1];
    if(last->kind() == ValueKind::Identifier) {
        const auto linked = last->linked_node();
        if (linked) {
            const auto lastKind = linked->kind();
            if (lastKind == ASTNodeKind::FunctionDecl) {
                if (!linked->as_function_unsafe()->has_self_param()) {
                    // TODO calling functions above without destructing the structs
                    call_any_function_above(visitor, values, (int) start, (int) end - 1);
                }
                visitor.mangle(linked);
                return;
            } else if (lastKind == ASTNodeKind::EnumMember) {
                write_enum(visitor, linked->as_enum_member_unsafe());
                return;
            }
        }
    }
    chain_after_func(visitor, values, start, end);
}

void ToCAstVisitor::VisitAccessChain(AccessChain *chain) {
    if(chain->is_moved()) {
        auto found = local_allocated.find(chain);
        if(found != local_allocated.end()) {
            write(found->second);
            return;
        }
    }
    const auto size = chain->values.size();
    access_chain(*this, chain->values, 0, size);
}

void writeStructReturningFunctionCall(ToCAstVisitor& visitor, FunctionCall* call, ASTNode* return_linked, FunctionType* func_type) {
    visitor.write("(*({ ");
    auto found = visitor.local_allocated.find(call);
    if(found != visitor.local_allocated.end()) {
        auto temp_name = chem::string_view(found->second);
        // write function name
        visitor.visit(call->parent_val);
        visitor.write("(&");
        visitor.write(temp_name);
        write_implicit_args(visitor, func_type, call, false);
        func_call_args(visitor, call, func_type);
        visitor.write("); &");
        visitor.write(temp_name);
        visitor.write("; }))");
    } else {
        // TODO we'll remove this block, and generate an error
        const auto temp_name_str = visitor.get_local_temp_var_name();
        const auto temp_name = chem::string_view(temp_name_str);
        allocate_struct_by_name_no_init(visitor, return_linked, temp_name);
        visitor.write("; ");
        // write function name
        visitor.visit(call->parent_val);
        visitor.write("(&");
        visitor.write(temp_name);
        write_implicit_args(visitor, func_type, call, false);
        func_call_args(visitor, call, func_type);
        visitor.write("); &");
        visitor.write(temp_name);
        visitor.write("; }))");
    }
}

void ToCAstVisitor::VisitFunctionCall(FunctionCall *call) {

    const auto linked = call->parent_val->linked_node();
    // you can't call an enum member, so we're using it as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;

    // handle calls to variant
    if(linked_kind == ASTNodeKind::VariantMember) {
        write_variant_call(*this, call);
        return;
    }

    const auto func_decl = ASTNode::isFunctionDecl(linked_kind) ? linked->as_function_unsafe() : nullptr;
    const auto parent_type = call->parent_val->create_type(allocator);
    const auto canonical_parent = parent_type->canonical();
    const auto func_type = call->func_type_from_parent_type(allocator, canonical_parent);

    // handling comptime functions
    if(func_decl && func_decl->is_comptime()) {
        const auto value = evaluated_func_val(*this, func_decl, call);
        visit(value);
        return;
    }

    if(canonical_parent->kind() == BaseTypeKind::CapturingFunction) {
        const auto capType = canonical_parent->as_capturing_func_type_unsafe();
        auto temp_var = get_local_temp_var_name();
        write("({ ");
        VisitCapturingFunctionType(capType);
        write("* ");
        write(temp_var);
        write(" = ");
        if(!is_value_param_hidden_pointer(call->parent_val)) {
            write('&');
        }
        visit(call->parent_val);
        write("; ");
        write("(");
        write("(");
        func_type_with_id_no_params(*this, func_type, "");
        write("void*");
        func_type_params(*this, func_type, 0, true);
        write(")");
        write(")");
        write(temp_var);
        write("->fn_pointer");
        write(")");
        write('(');
        write(temp_var);
        write("->fn_data_ptr");
        write_implicit_args(*this, func_type, call, false);
        func_call_args(*this, call, func_type);
        write(')');
        write("; })");
        return;
    }

    // handling capturing lambdas
    if(func_type->isCapturing()) {
        capture_call(*this, func_type, call);
        return;
    }

    // handling dynamic dispatch
    const auto grandpa = get_parent_from(call->parent_val);
    if(grandpa) {
        auto grandpaType = grandpa->create_type(allocator);
        if(grandpaType) {
            auto pure_grandpa = grandpaType->pure_type(allocator);
            if (pure_grandpa && pure_grandpa->kind() == BaseTypeKind::Dynamic) {
                const auto interface = pure_grandpa->linked_node()->as_interface_def();
                if (interface && !interface->users.empty()) {
                    // Dynamic dispatch
                    // ((typeof(PhoneSmartPhone)*) phone.second)->call(phone.first);
                    auto first = interface->users.begin();
                    auto first_def = first->first;
                    write('(');
                    write('(');
                    write("typeof");
                    write('(');
                    vtable_name(*this, interface, first_def);
                    write(')');
                    write('*');
                    write(')');
                    space();
                    const auto grandpa_chain = build_parent_chain(call->parent_val, allocator);
                    visit(grandpa_chain);
                    write('.');
                    write("second");
                    write(')');
                    write("->");
                    func_name(*this, call->parent_val, func_decl);
                    write('(');
                    if (write_self_arg_bool_no_pointer(*this, func_type, grandpa_chain, call)) {
                        write('.');
                        write("first");
                        if (!call->values.empty()) {
                            write(", ");
                        }
                    }
                    func_call_args(*this, call, func_type);
                    write(')');
                } else {
                    if (!interface) {
                        write("[Dynamic Dispatch used with type other than interface]");
                        error("Dynamic Dispatch used with a type other than interface", pure_grandpa->linked_node());
                    } else {
                        write("[Dynamic Dispatch Interface has no known users]");
                    }
                }
                return;
            }
        }
    }

    if(!func_decl || !func_decl->is_comptime()) {
        const auto returnType = func_type->returnType->pure_type(allocator);
        const auto returnTypeKind = returnType->kind();
        // handling functions that return dynamic objects
        if (returnTypeKind == BaseTypeKind::Dynamic) {
            write("(*({ __chemical_fat_pointer__ ");
            const auto temp_name = get_local_temp_var_name();
            write_str(temp_name);
            write("; ");
            // write function name
            visit(call->parent_val);
            write("(&");
            write_str(temp_name);
            write_implicit_args(*this, func_type, call, false);
            func_call_args(*this, call, func_type);
            write("); &");
            write_str(temp_name);
            write("; }))");
            if(!nested_value) {
                write(';');
            }
            return;
        } else if(returnTypeKind == BaseTypeKind::CapturingFunction) {
            const auto instanceType = returnType->as_capturing_func_type_unsafe()->instance_type;
            const auto return_linked = instanceType->get_direct_linked_canonical_node();
            writeStructReturningFunctionCall(*this, call, return_linked, func_type);
            return;
        } else {
            // functions that return struct like (struct, variants, unions) are handled in this block of code
            const auto return_linked = returnType->get_direct_linked_node();
            if (return_linked) {
                const auto returnKind = return_linked->kind();
                if (returnKind == ASTNodeKind::StructDecl || returnKind == ASTNodeKind::VariantDecl || returnKind == ASTNodeKind::UnionDecl) {
                    writeStructReturningFunctionCall(*this, call, return_linked, func_type);
                    return;
                }
            }
        }
    }

    // normal functions
    visit(call->parent_val);
    write('(');
    write_implicit_args(*this, func_type, call);
    func_call_args(*this, call, func_type);
    write(')');

}

void ToCAstVisitor::VisitGenericTypeDecl(GenericTypeDecl* node) {
    for(const auto inst : node->instantiations) {
        visit(inst);
    }
}

void ToCAstVisitor::VisitInitBlock(InitBlock *initBlock) {
    const auto container = initBlock->getContainer();
    auto& initializers = initBlock->initializers;
    auto is_union = container->kind() == ASTNodeKind::UnionDecl;
    for(auto& init : initializers) {
        auto value = init.second.value;
        auto variable = container->variable_type_index(init.first, true);
        if(container->is_one_of_inherited_type(variable.second)) {
            auto chain = value->as_access_chain_unsafe();
            auto val = chain->values.back();
            auto call = val->as_func_call();
            auto called_struct = call->parent_val->linked_node();
            if(call->values.size() == 1) {
                auto struc_val = call->values[0]->as_struct_value();
                if(struc_val && struc_val->linked_node() == called_struct) {
                    // initializing directly using a struct
                    write("this->");
                    write(init.first);
                    write(" = ");
                    visit(struc_val);
                    write(';');
                    continue;
                }
            }
            local_allocated[call] = "this->" + init.first.str();
            before_stmt->visit(chain);
        } else {
            write("this->");
            write(init.first);
            write(" = ");
            visit(init.second.value);
            write(';');
        }
    }
}

void ToCAstVisitor::VisitStructMember(StructMember *member) {
    if(member->type->kind() == BaseTypeKind::Function) {
        const auto func_type = member->type->as_function_type();
        if(func_type->isCapturing()) {
            write(fat_pointer_type);
            write('*');
            space();
            write(member->name);
        } else {
            func_type_with_id(*this, func_type, member->name);
        }
    } else {
        type_with_id(*this, member->type, member->name);
    }
    write(';');
}

void ToCAstVisitor::VisitProvideStmt(ProvideStmt *stmt) {
    stmt->put_in(implicit_args, stmt->value, this, [](ProvideStmt* stmt, void* data) {
        const auto v = (ToCAstVisitor*) data;
        v->visit(&stmt->body);
    });
}

void ToCAstVisitor::VisitComptimeBlock(ComptimeBlock *block) {
    comptime_scope.interpret(&block->body);
}

void ToCAstVisitor::VisitTypealiasStmt(TypealiasStatement *stmt) {
    if(stmt->is_top_level()) return;
    type_def_stmt(*this, stmt);
}

void write_variant_call_id_index(ToCAstVisitor& visitor, VariableIdentifier* value) {
    const auto member = value->linked->as_variant_member();
    if(member) {
        *visitor.output << member->parent()->variable_index(member->name, false);
    } else {
        visitor.write("-1");
    }
}

void write_variant_call_call_index(ToCAstVisitor& visitor, FunctionCall* value) {
    const auto member = value->parent_val->linked_node()->as_variant_member();
    if(member) {
        *visitor.output << member->parent()->variable_index(member->name, false);
    } else {
        visitor.write("-1");
    }
}

void write_variant_call_index(ToCAstVisitor& visitor, Value* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            write_variant_call_id_index(visitor, value->as_identifier_unsafe());
            return;
        case ValueKind::FunctionCall:
            write_variant_call_call_index(visitor, value->as_func_call_unsafe());
            return;
        case ValueKind::AccessChain: {
            const auto chain = value->as_access_chain_unsafe();
            if(chain) {
                write_variant_call_index(visitor, chain->values.back());
            } else {
                visitor.write("-1");
            }
            return;
        }
        default:
            visitor.write("-1");
            return;
    }
}

void switch_expr(ToCAstVisitor& visitor, Value* expr, BaseType* type) {
    // automatic dereference
    if(type->pure_type(visitor.allocator)->getLoadableReferredType() != nullptr) {
        visitor.write('*');
    }
    visitor.visit(expr);
}

void ToCAstVisitor::VisitSwitchStmt(SwitchStatement *statement) {
    write("switch(");
    VariantDefinition* variant = nullptr;
    const auto known_t = statement->expression->create_type(allocator);
    if(known_t) {
        const auto linked = known_t->linked_node();
        if(linked) {
            const auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::VariantDecl) {
                variant = linked->as_variant_def_unsafe();
            } else if(linked_kind == ASTNodeKind::VariantMember) {
                variant = linked->as_variant_member_unsafe()->parent();
            }
            if (variant) {
                // turn on the active iteration of the variant
                visit(statement->expression);
                write_accessor(*this, statement->expression, nullptr);
                write(variant_type_variant_name);
            } else {
                switch_expr(*this, statement->expression, known_t);
            }
        } else {
            switch_expr(*this, statement->expression, known_t);
        }
    } else {
        visit(statement->expression);
    }
    write(") {");
    unsigned i = 0;
    indentation_level += 1;
    while(i < statement->scopes.size()) {
        auto& scope = statement->scopes[i];
        new_line_and_indent();

        unsigned case_ind = 0;
        const auto size = statement->cases.size();
        bool has_line_before = true;
        while(case_ind < size) {
            auto& switch_case = statement->cases[case_ind];
            if(switch_case.second == i) {
                if(!has_line_before) {
                    new_line_and_indent();
                }
                write("case ");

                visit(switch_case.first);

                write(':');
                has_line_before = false;
            }
            case_ind++;
        }
        if(statement->defScopeInd == i) {
            new_line_and_indent();
            write("default:");
        }

        indentation_level += 1;
        ::scope(*this, scope);
        new_line_and_indent();
        write("break;");
        indentation_level -= 1;
        i++;
    }
    indentation_level -= 1;
    new_line_and_indent();
    write('}');
}

void ToCAstVisitor::VisitTryStmt(TryCatch *statement) {
    write("[TryCatch_UNIMPLEMENTED]");
}

void ToCAstVisitor::VisitIntValue(IntValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitBigIntValue(BigIntValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitLongValue(LongValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitShortValue(ShortValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitUBigIntValue(UBigIntValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitUIntValue(UIntValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitULongValue(ULongValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitUShortValue(UShortValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitInt128Value(Int128Value *val) {
    *output << val->get_num_value();
}

void ToCAstVisitor::VisitUInt128Value(UInt128Value *val) {
    *output << val->get_num_value();
}

void ToCAstVisitor::VisitNumberValue(NumberValue *numValue) {
    *output << numValue->get_num_value();
}

void ToCAstVisitor::VisitFloatValue(FloatValue *val) {
    // we must convert to a string before writing because
    // well if value is 4.0, it doesn't write the point zero part and writes just 4
    // after writing 4 we can't write 'f' after it, C expects 4.0f not 4f
    write_str(std::to_string(val->value));
    write('f');
}

void ToCAstVisitor::VisitDoubleValue(DoubleValue *val) {
    *output << val->value;
}

void ToCAstVisitor::VisitCharValue(CharValue *val) {
    write('\'');
    write_escape_encoded(*output, val->value);
    write('\'');
}

void ToCAstVisitor::VisitUCharValue(UCharValue *val) {
    write('\'');
    write_escape_encoded(*output, (char) val->value);
    write('\'');
}

void ToCAstVisitor::VisitStringValue(StringValue *val) {
    write('"');
    write_encoded(*this, val->value);
    write('"');
}

void ToCAstVisitor::VisitBoolValue(BoolValue *boolVal) {
    if(boolVal->value) {
        write('1');
    } else {
        write('0');
    }
}

void ToCAstVisitor::VisitArrayValue(ArrayValue *arr) {
    write('{');
    unsigned i = 0;
    auto prev = nested_value;
    nested_value = true;
    const auto elem_type = arr->element_type(allocator);
    while(i < arr->values.size()) {
        const auto value = arr->values[i];
        if(value->is_ref_moved()) {
            // since we're moving the value here
            // what we must do is set the drop flag to false
            write("({ ");
            set_moved_ref_drop_flag(*this, value);
            space();
            accept_mutating_value(elem_type, value, false);
            write("; })");
        } else {
            accept_mutating_value(elem_type, value, false);
        }
        if(i != arr->values.size() - 1) {
            write(',');
        }
        i++;
    }
    nested_value = prev;
    write('}');
}

void ToCAstVisitor::VisitStructValue(StructValue *val) {
    auto linked = val->linked_node();
    auto linked_kind = linked->kind();
    const auto runName = val->runtime_name_str(mangler);
    if(!runName.empty()) {
        write('(');
        if (linked_kind == ASTNodeKind::UnionDecl) {
            write("union ");
        } else {
            write("struct ");
        }
        *output << runName;
        write(')');
    }
    write('{');
    auto prev = nested_value;
    nested_value = true;
    bool has_value_before = false;
    indentation_level += 1;
    for(auto& value : val->values) {
        if(has_value_before) {
            write(", ");
        } else {
            has_value_before = true;
        }
        new_line_and_indent();
        // we are only getting direct / inherited members, not inherited structs here
        const auto member = val->child_member(value.first);
        write('.');
        write(value.first);
        write(" = ");
        if(value.second.value->is_ref_moved()) {
            // since we're moving the value here
            // what we must do is set the drop flag to false
            write("({ ");
            set_moved_ref_drop_flag(*this, value.second.value);
            space();
            accept_mutating_value(member ? member->known_type() : nullptr, value.second.value, false);
            write("; })");
        } else {
            accept_mutating_value(member ? member->known_type() : nullptr, value.second.value, false);
        }
    }
    auto& variables = val->variables()->variables();
    for(const auto var : variables) {
        auto found = val->values.find(var->name);
        if(found == val->values.end()) {
            auto defValue = var->default_value();
            const auto member = val->child_member(var->name);
            if(has_value_before) {
                write(", ");
            } else {
                has_value_before = true;
            }
            new_line_and_indent();
            if(defValue) {
                write('.');
                write(var->name);
                write(" = ");
                accept_mutating_value(member ? member->known_type() : nullptr, defValue, false);
            } else if(!val->is_union()) {
                error(val) << "no default value present for '" << var->name << "' in struct value";
            }
        }
    }
    indentation_level -= 1;
    nested_value = prev;
    new_line_and_indent();
    write('}');
}

//void deref_id(ToCAstVisitor& visitor, VariableIdentifier* identifier) {
//    visitor.write('(');
//    visitor.write('*');
//    visitor.write(identifier->value);
//    visitor.write(')');
//}
//
//bool should_deref_node(ASTNode* node) {
//    return node && ASTNode::isStoredStructDecl(node->kind());
//}
//
//bool write_id_accessor(ToCAstVisitor& visitor, VariableIdentifier* identifier, ASTNode* node) {
//    if (should_deref_node(node)) {
//        deref_id(visitor, identifier);
//        return true;
//    } else {
//        return false;
//    }
//}

void ToCAstVisitor::write_identifier(VariableIdentifier *identifier, bool is_first) {
    if(identifier->is_moved) {
        auto found = local_allocated.find(identifier);
        if(found != local_allocated.end()) {
            write(found->second);
            return;
        }
    }
    const auto linked = identifier->linked_node();
    const auto linked_kind = linked->kind();
    if(ASTNode::isAnyStructMember(linked_kind)) {
        if(is_first) {
            const auto func = current_func_type->as_function();
            if (func && func->is_constructor_fn()) {
                write("this->");
            }
            else if(func && func->parent()) {
                auto self_param = func->get_self_param();
                if(self_param && ASTNode::isMembersContainer(func->parent()->kind())) {
                    write(self_param->name);
                    write("->");
                }
                auto ext_node = func->parent()->as_extendable_members_container_node();
                if(ext_node) {
                    write_path_to_member(*this, ext_node, linked->as_base_def_member());
                }
            }
        }
    } else {
        switch(linked_kind) {
            case ASTNodeKind::FunctionDecl:
                mangle(linked->as_function_unsafe());
                return;
            case ASTNodeKind::VarInitStmt: {
                const auto init = linked->as_var_init_unsafe();
                if (init->is_comptime()) {
                    visit(init->value);
                    return;
                } else if(init->is_top_level()) {
                    mangle(init);
                    return;
                }
                break;
            }
            case ASTNodeKind::CapturedVariable:{
                auto found = declarer->aliases.find(linked->as_captured_var_unsafe());
                if(found == declarer->aliases.end()) {
                    write("this->");
                } else {
                    write("((struct ");
                    write(found->second);
                    write("*) this)->");
                }
                break;
            }
            case ASTNodeKind::VariantCaseVariable:{
                const auto var = linked->as_variant_case_var_unsafe();
                const auto expr = var->parent()->expression;
                const auto var_mem = var->member_param->parent();
                visit(expr);
                write_accessor(*this, expr, identifier);
                write(var_mem->name);
                write('.');
                break;
            }
            case ASTNodeKind::PatternMatchId: {
                const auto id = linked->as_patt_match_id_unsafe();
                const auto matchExpr = id->matchExpr;
                auto allocated = local_allocated.find(matchExpr);
                const auto elseKind = matchExpr->elseExpression.kind;
                if(elseKind == PatternElseExprKind::DefValue) {
                    if (allocated != local_allocated.end()) {
                        write(allocated->second);
                        return;
                    } else {
                        //TODO handle this
                    };
                } else {
                    if (allocated != local_allocated.end()) {
                        write(allocated->second);
                        write("->");
                        write(matchExpr->member_name);
                        write('.');
                        write(id->identifier);
                        return;
                    } else {
                        //TODO handle this
                    };
                }
                break;
            }
            case ASTNodeKind::EnumMember: {
                write_enum(*this, linked->as_enum_member_unsafe());
                return;
            }
            default:
                break;
        }
    }
    write(identifier->value);
}

void ToCAstVisitor::VisitVariableIdentifier(VariableIdentifier *identifier) {
    write_identifier(identifier, true);
}

void ToCAstVisitor::VisitSizeOfValue(SizeOfValue *size_of) {
    write("sizeof");
    write('(');
    const auto pure_t = size_of->for_type->pure_type(allocator);
    if(pure_t->kind() == BaseTypeKind::Reference) {
        visit(pure_t->as_reference_type_unsafe()->type);
    } else {
        visit(size_of->for_type);
    }
    write(')');
}

void ToCAstVisitor::VisitAlignOfValue(AlignOfValue *align_of) {
    write("_Alignof");
    write('(');
    visit(align_of->for_type->removeReferenceFromType());
    write(')');
}

void ToCAstVisitor::VisitExpression(Expression *expr) {
    write('(');
    auto prev_nested = nested_value;
    nested_value = true;

    // automatic dereferencing the first value
    const auto first_type = expr->firstValue->create_type(allocator);
    const auto first_pure = first_type->pure_type(allocator);
    if(first_pure->getLoadableReferredType() != nullptr) {
        write('*');
    }

    visit(expr->firstValue);

    space();
    write_str(to_string(expr->operation));
    space();

    // automatic dereferencing the second value
    const auto second_type = expr->secondValue->create_type(allocator);
    const auto second_pure = second_type->pure_type(allocator);
    if(second_pure->getLoadableReferredType() != nullptr) {
        write('*');
    }

    visit(expr->secondValue);

    nested_value = prev_nested;
    write(')');
}

void ToCAstVisitor::VisitComptimeValue(ComptimeValue* value) {
    const auto evaluated = value->evaluate(allocator, &comptime_scope);
    if(evaluated) {
        visit(evaluated);
    } else {
        error(value) << "couldn't evaluate comptime value";
    }
}

void ToCAstVisitor::VisitCastedValue(CastedValue *casted) {
    write('(');
    write('(');
    visit(casted->type);
    write(')');
    write(' ');
    auto prev_nested = nested_value;
    nested_value = true;
    visit(casted->value);
    nested_value = prev_nested;
    write(')');
}

void ToCAstVisitor::VisitAddrOfValue(AddrOfValue *value) {
    if(!is_value_param_hidden_pointer(value)) {
        write('&');
    }
    visit(value->value);
}

void ToCAstVisitor::VisitPatternMatchExpr(PatternMatchExpr* value) {

    const auto elseKind = value->elseExpression.kind;
    if(elseKind == PatternElseExprKind::Unreachable || elseKind == PatternElseExprKind::Return) {
        const auto type = value->expression->create_type(allocator);
        visit(type);
        write('*');
    } else if(elseKind == PatternElseExprKind::DefValue) {
        visit(value->param_names[0]->member_param->type);
    }
    auto varName = get_local_temp_var_name();
    write(' ');
    write(varName);
    write(" = ");

    if(elseKind == PatternElseExprKind::Unreachable || elseKind == PatternElseExprKind::Return) {
        if(!is_value_param_hidden_pointer(value->expression)) {
            write('&');
        }
        visit(value->expression);
    } else if(elseKind == PatternElseExprKind::DefValue) {

        const auto type = value->expression->create_type(allocator);
        const auto memberId = value->param_names[0];
        const auto member = memberId->member_param->parent();
        const auto def = member->parent();

        auto varName2 = get_local_temp_var_name();
        write("({ ");
        visit(type);
        write('*');
        write(' ');
        write(varName2);
        write(" = ");
        if(!is_value_param_hidden_pointer(value->expression)) {
            write('&');
        }
        visit(value->expression);
        write("; ");
        write(varName2);
        write("->");
        write(variant_type_variant_name);
        write(" == ");

        *output << def->direct_child_index(member->name);
        write(" ? ");
        write(varName2);
        write("->");
        write(member->name);
        write('.');
        write(memberId->identifier);
        write(" : ");
        visit(value->elseExpression.value);
        write("; })");
    }

    if(elseKind == PatternElseExprKind::Return) {

        const auto member = value->member;
        const auto def = member->parent();

        write(';');
        new_line_and_indent();
        write("if(");
        write(varName);
        write("->");
        write(variant_type_variant_name);
        write(" != ");
        *output << def->direct_child_index(member->name);
        write(") {");
        indentation_level += 1;
        new_line_and_indent();
        writeReturnStmtFor(value->elseExpression.value);
        indentation_level -= 1;
        new_line_and_indent();
        write('}');

    }


    local_allocated[value] = varName;
}

void ToCAstVisitor::VisitRetStructParamValue(RetStructParamValue *paramVal) {
    write(struct_passed_param_name);
}

void ToCAstVisitor::VisitDereferenceValue(DereferenceValue *casted) {
//    const auto known = casted->value->known_type();
    write('*');
    visit(casted->value);
}

void ToCAstVisitor::VisitIndexOperator(IndexOperator *op) {
    visit(op->parent_val);
    unsigned i = 0;
    while(i < op->values.size()) {
        write('[');
        auto& val = op->values[i];
        visit(val);
        write(']');
        i++;
    }
}

void ToCAstVisitor::VisitBlockValue(BlockValue *blockVal) {
    write("({");
    new_line_and_indent();
    scope_no_parens(*this, blockVal->scope);
    new_line_and_indent();
    write("})");
}

void ToCAstVisitor::VisitNegativeValue(NegativeValue *negValue) {
    write('-');
    visit(negValue->value);
}

void ToCAstVisitor::VisitNotValue(NotValue *notValue) {
    write('!');
    visit(notValue->value);
}

void ToCAstVisitor::VisitNullValue(NullValue *nullValue) {
    write("NULL");
}

void ToCAstVisitor::VisitExtractionValue(ExtractionValue* value) {

    const auto src = value->value;
    auto& aliases = declarer->aliases;
    auto found = declarer->aliases.find(src);
    if(found == aliases.end()) return;

    switch(value->extractionKind) {
        case ExtractionKind::LambdaFnPtr:
            write(found->second);
            write("_pair->first");
            break;
        case ExtractionKind::LambdaCapturedPtr:
            write(found->second);
            write("_pair->second");
            break;
        case ExtractionKind::LambdaCapturedDestructor:{
            write(found->second);
            write("_cap_destr");
            break;
        }
        case ExtractionKind::SizeOfLambdaCaptured: {
            write("sizeof(");
            write("struct ");
            write(found->second);
            write("_cap");
            write(')');
            break;
        }
        case ExtractionKind::AlignOfLambdaCaptured: {
            write("_Alignof(");
            write("struct ");
            write(found->second);
            write("_cap");
            write(')');
            break;
        }
    }
}

void ToCAstVisitor::VisitValueNode(ValueNode *node) {
    auto val_kind = node->value->val_kind();
    if(val_kind != ValueKind::SwitchValue && val_kind != ValueKind::IfValue && val_kind != ValueKind::LoopValue) {
        write_assignable(*this, node->parent());
        write(" = ");
    }
    auto prev = nested_value;
    nested_value = true;
//    if(val_kind == ValueKind::AccessChain) {
//
//    } else if(val_kind == ValueKind::FunctionCall) {
//
//    } else {
        visit(node->value);
//    }
    nested_value = prev;
    write(';');
}

void ToCAstVisitor::VisitValueWrapper(ValueWrapperNode *node) {
    const auto val_type = node->value->create_type(allocator);
    if(val_type->isStructLikeType()) {
        const auto destr = val_type->get_destructor();
        if (destr != nullptr) {
            const auto temp_name = get_local_temp_var_name_view();
            visit(val_type);
            write(' ');
            write(temp_name);
            write(" = ");
            visit(node->value);
            write(';');
            destructor->queue_destruct(temp_name, node, destr->parent()->as_extendable_members_container_node(), false, false);
            return;
        }
    }
    visit(node->value);
    write(';');
}

void write_captured_struct(ToCAstVisitor& visitor, LambdaFunction* func, const std::string& lamb_name) {
    visitor.write("(struct ");
    visitor.write_str(lamb_name);
    visitor.write("_cap");
    visitor.write(')');
    visitor.write('{');
    unsigned i = 0;
    while (i < func->captureList.size()) {
        auto& cap = func->captureList[i];
        if (cap->capture_by_ref) {
            visitor.write('&');
        }
        visitor.write(cap->name);
        if (i != func->captureList.size() - 1) {
            visitor.write(',');
        }
        i++;
    }
    visitor.write('}');
}

void ToCAstVisitor::VisitLambdaFunction(LambdaFunction *func) {
    auto found = declarer->aliases.find(func);
    if(found != declarer->aliases.end()) {
        if(func->isCapturing()) {
            write('(');
            write('&');
            write('(');
            write(fat_pointer_type);
            write(')');
            write('{');
            write(found->second);
            write(',');
            if(func->captureList.empty()) {
                write("NULL");
            } else {
//                 if(does_live_on_stack) {
                    write("(&");
                    write_captured_struct(*this, func, found->second);
                    write(')');
//                } else {
//                    auto temp_var = get_local_temp_var_name();
//                    write("({ struct ");
//                    write(found->second);
//                    write("_cap");
//                    write("* ");
//                    write(temp_var);
//                    write(" = ");
//                    write("malloc(sizeof(struct ");
//                    write(found->second);
//                    write("_cap)); *");
//                    write(temp_var);
//                    write(" = ");
//                    write_captured_struct(*this, func, found->second);
//                    write("; ");
//                    write(temp_var);
//                    write("; })");
//                }
            }
            write('}');
            write(')');
        } else {
            write(found->second);
        }
    } else {
        write("[LambdaFunction_NOT_FOUND]");
    }
}

void ToCAstVisitor::VisitAnyType(AnyType *any_type) {
    write("void");
}

void ToCAstVisitor::VisitLiteralType(LiteralType *literal) {
    visit(literal->underlying);
}

void ToCAstVisitor::VisitArrayType(ArrayType *type) {
    visit(type->elem_type);
}

void ToCAstVisitor::VisitBoolType(BoolType *func) {
    if(cpp_like) {
        write("bool");
    } else {
        write("_Bool");
    }
}

void ToCAstVisitor::VisitDoubleType(DoubleType *func) {
    write("double");
}

void ToCAstVisitor::VisitFloatType(FloatType *func) {
    write("float");
}

void ToCAstVisitor::VisitFloat128Type(Float128Type* func) {
    write("__float128");
}

void ToCAstVisitor::VisitLongDoubleType(LongDoubleType *type) {
    write("long double");
}

void ToCAstVisitor::VisitComplexType(ComplexType *type) {
    write("_Complex ");
    visit(type->elem_type);
}

void ToCAstVisitor::VisitFunctionType(FunctionType *type) {
    if(type->isCapturing()) {
        write(fat_pointer_type);
        write('*');
        return;
    }
    func_type_with_id(*this, type, "NOT_FOUND");
}

void ToCAstVisitor::VisitCapturingFunctionType(CapturingFunctionType* type) {
    visit(type->instance_type);
}

void ToCAstVisitor::VisitGenericType(GenericType *gen_type) {
    const auto gen_struct = gen_type->referenced->linked->as_members_container();
    visit(gen_type->referenced);
}

void ToCAstVisitor::VisitIntNType(IntNType *type) {
    switch(type->IntNKind()) {
        case IntNTypeKind::Char:
            write("char");
            return;
        case IntNTypeKind::Short:
            write("short");
            return;
        case IntNTypeKind::Int:
            write("int");
            return;
        case IntNTypeKind::Long:
            write("long");
            return;
        case IntNTypeKind::BigInt:
            write("long long");
            return;
        case IntNTypeKind::Int128:
            write("__int128");
            return;
        case IntNTypeKind::UChar:
            write("unsigned char");
            return;
        case IntNTypeKind::UShort:
            write("unsigned short");
            return;
        case IntNTypeKind::UInt:
            write("unsigned int");
            return;
        case IntNTypeKind::ULong:
            write("unsigned long");
            return;
        case IntNTypeKind::UBigInt:
            write("unsigned long long");
            return;
        case IntNTypeKind::UInt128:
            write("unsigned __int128");
            return;
    }

}

void ToCAstVisitor::VisitPointerType(PointerType *func) {
    visit(func->type);
    if(func->is_mutable) {
        write('*');
    } else {
        write(" const*");
    }
}

void ToCAstVisitor::VisitReferenceType(ReferenceType* func) {
    visit(func->type);
    if(func->is_mutable) {
        write('*');
    } else {
        write("*const");
    }
}

void ToCAstVisitor::VisitLinkedType(LinkedType *type) {
    auto& linked = *type->linked;
    const auto kind = linked.kind();
    switch(kind) {
        case ASTNodeKind::InterfaceDecl:
            if(linked.as_interface_def_unsafe()->active_user) {
                write("struct ");
                mangle(linked.as_interface_def_unsafe()->active_user);
            } else {
                write("void*");
            }
            return;
        case ASTNodeKind::EnumDecl:
            write("int");
            return;
        case ASTNodeKind::VariantMember:
            write("struct ");
            mangle(linked.as_variant_member_unsafe()->parent());
            return;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
            write("struct ");
            mangle(&linked);
            return;
        case ASTNodeKind::UnionDecl:
            write("union ");
            mangle(&linked);
            return;
        case ASTNodeKind::GenericTypeParam:
            visit(linked.known_type());
            return;
        case ASTNodeKind::TypealiasStmt: {
            auto alias = declarer->aliases.find(&linked);
            if (alias != declarer->aliases.end()) {
                write(alias->second);
                return;
            }
            break;
        };
        default:
            break;
    }
    mangle(&linked);
}

//void ToCAstVisitor::VisitLinkedValueType(LinkedValueType *ref_type) {
//    write("[ref_value_type]");
//}

void ToCAstVisitor::VisitDynamicType(DynamicType *type) {
    write(fat_pointer_type);
}

void ToCAstVisitor::VisitStringType(StringType *func) {
    write("char*");
}

void ToCAstVisitor::VisitStructType(StructType *val) {
    write("struct ");
    if(!val->name.empty()) {
        write(val->name);
        write(' ');
    }
    write("{ ");
    for(const auto variable : val->variables()) {
        visit(variable);
        write(' ');
    }
    write("}");
}

void ToCAstVisitor::VisitUnionType(UnionType *val) {
    write("union ");
    if(!val->name.empty()) {
        write(val->name);
        write(' ');
    }
    write("{ ");
    for(const auto variable : val->variables()) {
        visit(variable);
        write(' ');
    }
    write("}");
}

void ToCAstVisitor::VisitVoidType(VoidType *func) {
    write("void");
}

void ToCAstVisitor::VisitNullPtrType(NullPtrType* type) {
    write("void*");
}

bool ToCBackendContext::forget(ASTNode* targetNode) {
    auto& jobs = visitor->destructor->destruct_jobs;
    auto it = std::find_if(
            jobs.begin(),
            jobs.end(),
            [targetNode](DestructionJob const& p){
                return p.initializer == targetNode;
            });
    if (it == jobs.end()) return false;
    jobs.erase(it);
    return true;
}

void ToCBackendContext::mem_copy(Value *lhs, Value *rhs) {
    visitor->new_line_and_indent();
    visitor->visit(lhs);
    visitor->write(" = ");
    visitor->visit(rhs);
    visitor->write(';');
}