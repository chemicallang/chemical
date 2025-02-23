// Copyright (c) Qinetik 2024.

#include "2cASTVisitor.h"
#include <memory>
#include <ostream>
#include <random>
#include <ranges>
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
#include "ast/structures/ExtensionFunction.h"
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
#include "ast/values/FloatValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/Int128Value.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/VariantCall.h"
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
#include "ast/values/UInt128Value.h"
#include "ast/values/IsValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ULongValue.h"
#include "ast/utils/CommonVisitor.h"
#include "preprocess/utils/RepresentationUtils.h"
#include "ast/utils/ASTUtils.h"
#include <sstream>
#include "CValueDeclVisitor.h"
#include "CBeforeStmtVisitor.h"
#include "CAfterStmtVisitor.h"
#include "ast/structures/InitBlock.h"

ToCAstVisitor::ToCAstVisitor(
    GlobalInterpretScope& scope,
    std::ostream *output,
    ASTAllocator& allocator,
    LocationManager& manager,
    std::vector<std::string>* compiler_interfaces
) : comptime_scope(scope), output(output), allocator(allocator), declarer(new CValueDeclarationVisitor(*this)),
    tld(*this, declarer.get()), ASTDiagnoser(manager), compiler_interfaces(compiler_interfaces)
{
    before_stmt = std::make_unique<CBeforeStmtVisitor>(*this);
    after_stmt = std::make_unique<CAfterStmtVisitor>(*this);
    destructor = std::make_unique<CDestructionVisitor>(*this);
}

void ToCAstVisitor::declare_before_translation(std::vector<ASTNode*>& nodes) {
    // declare the top level things with this visitor
    for(const auto node : nodes) {
        node->accept((Visitor*) &tld);
    }
}

void ToCAstVisitor::translate_after_declaration(std::vector<ASTNode*>& nodes) {
    // take out values like lambda from within functions
    for(const auto node : nodes) {
        node->accept((Visitor*) declarer.get());
    }
    // writing
    for(const auto node : nodes) {
        visit(node);
    }
}

void ToCAstVisitor::external_declare(std::vector<ASTNode*>& nodes) {
    auto& vis = tld;
    auto prev = vis.redefining;
    vis.redefining = true;
    // declare the top level things with this visitor
    for(auto& node : nodes) {
        node->accept(&vis);
    }
    vis.redefining = prev;
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
    def->runtime_name_no_parent(*visitor.output);
}

void node_name(ToCAstVisitor& visitor, ASTNode* node) {
    if(!node) return;
    node->runtime_name(*visitor.output);
}

// nodes inside namespaces for example namespace name is written before their name
void node_parent_name(ToCAstVisitor& visitor, ASTNode* node, bool take_parent = true) {
    auto current = take_parent ? (node ? node->parent() : nullptr) : node;
    if(current) {
        current->runtime_name(*visitor.output);
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
    if(type->isStructLikeType() && type->kind() != BaseTypeKind::Dynamic) {
        PointerType ptr_type(type, ZERO_LOC, true);
        type_with_id(visitor, &ptr_type, id);
    } else {
        type_with_id(visitor, type, id);
    }
}

void write_struct_return_param(ToCAstVisitor& visitor, FunctionType* decl) {
    visitor.visit(decl->returnType);
    visitor.write("* ");
    const auto func = decl->as_function();
    if(func && func->is_constructor_fn()) {
        visitor.write("this");
    } else {
        visitor.write(struct_passed_param_name);
    }
}

// don't know what to call '(' struct 'name' ')'
void write_struct_def_value_call(ToCAstVisitor& visitor, StructDefinition* def) {
    visitor.write('(');
    visitor.write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    visitor.write(')');
}

void extension_func_param(ToCAstVisitor& visitor, ExtensionFunction* extension) {
    visitor.visit(extension->receiver.type);
    visitor.space();
    visitor.write(extension->receiver.name);
}

void func_type_params(ToCAstVisitor& visitor, FunctionType* decl, unsigned i = 0, bool has_params_before = false) {
    auto is_struct_return = visitor.pass_structs_to_initialize && decl->returnType->isStructLikeType();
    auto func = decl->as_function();
    auto extension = decl->as_extension_func();
    if(is_struct_return && !(func && (func->is_copy_fn() || func->is_move_fn()))) {
        if(has_params_before) {
            visitor.write(", ");
        }
        write_struct_return_param(visitor, decl);
        has_params_before = true;
    }
    if(extension) {
        if(has_params_before) {
            visitor.write(", ");
        }
        extension_func_param(visitor, extension);
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
    func_decl->runtime_name(*visitor.output);
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
    def->runtime_name(*visitor.output);
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

void vtable_name(ToCAstVisitor& visitor, InterfaceDefinition* interface, StructDefinition* definition) {
    visitor.write(interface->name_view());
    visitor.write(definition->name_view());
}

std::pair<InterfaceDefinition*, StructDefinition*> get_dyn_obj_impl(BaseType* type, Value* value) {
    if(!type) return {nullptr, nullptr};
    auto pure_type = type->pure_type();
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
bool is_value_passed_pointer_like(Value* value) {
    const auto linked = value->linked_node();
    if(linked) {
        switch(linked->kind()) {
            case ASTNodeKind::FunctionParam:
            case ASTNodeKind::ExtensionFuncReceiver:{
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

bool implicit_mutate_value_for_dyn_obj(ToCAstVisitor& visitor, BaseType* type, Value* value, void(*value_visit)(ToCAstVisitor& visitor, Value* value)) {
//    (__chemical_fat_pointer__){ &sm, (void*) &PhoneSmartPhone }
    auto dyn_obj = get_dyn_obj_impl(type, value);
    if(!dyn_obj.first) return false;
    visitor.write('(');
    visitor.write(visitor.fat_pointer_type);
    visitor.write(')');
    visitor.write('{');
    visitor.space();
    if(!is_value_passed_pointer_like(value)) {
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
    } else {
        visitor.write('&');
        return false;
    }
}

void ToCAstVisitor::accept_mutating_value_explicit(BaseType* type, Value* value, bool assigning_value) {
    // automatically passing address to a reference type
    if(!assigning_value && type && type->kind() == BaseTypeKind::Reference && !value->is_ref(allocator)) {
        const auto ref_type = type->as_reference_type_unsafe();
        if(write_value_for_ref_type(*this, value, ref_type)) {
            return;
        }
    }
    // automatic dereference
    if(type) {
        if (type->get_direct_linked_node() != nullptr && is_value_passed_pointer_like(value)) {
            write('*');
        } else {
            const auto value_type = value->create_type(allocator);
            const auto derefType = value_type->getAutoDerefType(type);
            if (derefType) {
                write('*');
            }
        }
    }
    // mutating value
    if(!implicit_mutate_value_default(*this, type, value)) {
        visit(value);
    }
}

void call_implicit_constructor_no_alloc(ToCAstVisitor& visitor, FunctionDeclaration* imp_constructor, Value* value, const chem::string_view& var_name, bool is_var_ptr) {
    const auto new_expected_type = imp_constructor->params[0]->type;
    imp_constructor->runtime_name(*visitor.output);
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
    allocate_struct_by_name_no_init(visitor, imp_constructor->parent_node, var_name);
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

Value* evaluate_comptime_func(
        ToCAstVisitor& visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call
) {
    auto value = func_decl->call(&visitor.comptime_scope, visitor.allocator, call, nullptr, false);
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

Value* evaluated_func_val_proper(ToCAstVisitor& visitor, FunctionDeclaration* func_decl, FunctionCall* call) {
    if(call->generic_iteration == -1) {
        call->fix_generic_iteration(visitor, nullptr);
    }
    const auto prev_iteration = call->set_curr_itr_on_decl();
    auto eval = evaluated_func_val(visitor, func_decl, call);
    const auto eval_kind = eval->val_kind();
    if (eval_kind == ValueKind::AccessChain) {
        const auto chain = eval->as_access_chain_unsafe();
        chain->fix_generic_iteration(visitor, func_decl->returnType);
    } else if(eval_kind == ValueKind::FunctionCall) {
        eval->as_func_call_unsafe()->fix_generic_iteration(visitor, func_decl->returnType);
    }
    return eval;
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

void func_call_args(ToCAstVisitor& visitor, FunctionCall* call, FunctionType* func_type, unsigned i = 0) {
    auto prev_value = visitor.nested_value;
    visitor.nested_value = true;
    bool has_value_before = false;
    std::string temp_struct_name;
    while(i < call->values.size()) {
        auto param = func_type->func_param_for_arg_at(i);
        auto val = call->values[i];
        if (has_value_before) {
            visitor.write(", ");
        } else {
            has_value_before = true;
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
        bool is_memcpy_ref_str = val->requires_memcpy_ref_struct(param->type);
        if(is_memcpy_ref_str) {
            const auto linked = param->type->get_direct_linked_node();
            visitor.write("({ ");
            temp_struct_name = visitor.get_local_temp_var_name();
            auto t = val->create_type(visitor.allocator);
            visitor.visit(t);
            visitor.write(' ');
            visitor.write(temp_struct_name);
            visitor.write(" = ");
            if(is_value_passed_pointer_like(val)) {
                visitor.write('*');
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
        if(isStructLikeTypePtr || (is_param_type_ref && !val->is_stored_ptr_or_ref(visitor.allocator))) {
            auto allocated = visitor.local_allocated.find(val);
            if(allocated != visitor.local_allocated.end()) {
                visitor.write(allocated->second);
            } else if(accept_value) {
                visitor.visit(val);
            }
        } else if(!val->reference() && base_type->pure_type()->kind() == BaseTypeKind::Array) {
            visitor.write('(');
            visitor.visit(base_type);
            visitor.write("[]");
            visitor.write(')');
            visitor.accept_mutating_value_explicit(param->type, val, false);
        } else {
            visitor.accept_mutating_value_explicit(param->type, val, false);
        }
        if(is_memcpy_ref_str) {
            visitor.write(';');
            visitor.write(' ');
            visitor.write('&');
            visitor.write(temp_struct_name);
            visitor.write(';');
            visitor.write(" })");
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
                visitor.error("function param '" + param->name.str() + "' doesn't have a default value, however no argument exists for it", call);
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

void func_container_name(ToCAstVisitor& visitor, ASTNode* parent_node, ASTNode* linked_node);

#define variant_type_variant_name "__chx__vt_621827"

void value_alloca(ToCAstVisitor& visitor, const chem::string_view& identifier, BaseType* type, Value* value) {
    type_with_id(visitor, type, identifier);
    const auto var = type->get_direct_linked_variant();
    if(var) {
        visitor.write(" = ");
        visitor.write("{ .");
        visitor.write(variant_type_variant_name);
        visitor.write(" = ");
        visitor.write_str(std::to_string(var->variables.size()));
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
    if(is_value_passed_pointer_like(current)) {
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
    const auto pure_type = type->pure_type();
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
    if(!grandpa->is_pointer() && !is_value_passed_pointer_like(grandpa)) {
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
//    if(value->val_kind() == ValueKind::AccessChain) {
//        auto chain = value->as_access_chain_unsafe();
//        auto func_call = chain->values.back()->as_func_call();
//        if(func_call) {
//            auto linked_func = func_call->safe_linked_func();
//            if(linked_func && linked_func->is_comptime()) {
//                Value* eval;
//                func_call->set_curr_itr_on_decl();
//                auto found_eval = visitor.evaluated_func_calls.find(func_call);
//                bool not_found = found_eval == visitor.evaluated_func_calls.end();
//                if(not_found) {
//                    eval = evaluate_comptime_func(visitor, linked_func, func_call);
//                } else {
//                    eval = found_eval->second;
//                }
//                bool declared = is_declared_with_initializer(visitor.allocator, eval);
//                if(declared) {
//                    if(type->kind() != BaseTypeKind::Function) {
//                        visitor.write(identifier);
//                        visitor.write(' ');
//                    }
//                    visitor.write("= ");
//                }
//                if(not_found) {
//                    eval->accept((Visitor*) visitor.before_stmt.get());
//                }
//                visit_evaluated_func_val(visitor, &visitor, linked_func, func_call, eval, identifier);
//                if(declared) {
//                    visitor.write(';');
//                }
//                return;
//            }
//        }
//        if(func_call) {
//            const auto parent = func_call->parent_val->linked_node();
//            if((!parent || !ASTNode::isVariantMember(parent->kind())) && func_call->create_type(visitor.allocator)->isStructLikeType()) {
//                visitor.accept_mutating_value(type, value, true);
//                return;
//            }
//        }
//    }
    if(write_id) {
        visitor.write(identifier);
        write_type_post_id(visitor, type);
    }
    visitor.write(" = ");
    visitor.accept_mutating_value(type, value, true);
    visitor.write(';');
}

void value_init_default(ToCAstVisitor& visitor, const chem::string_view& identifier, BaseType* type, Value* value) {
    const auto struct_value = value->as_struct_value();
    int16_t prev_itr = 0;
    const auto is_generic = struct_value && struct_value->is_generic();
    if(struct_value && is_generic) {
        prev_itr = struct_value->get_active_iteration();
        struct_value->set_active_iteration(struct_value->generic_iteration);
    }
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
    if(struct_value && is_generic) struct_value->set_active_iteration(prev_itr);
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

void var_init(ToCAstVisitor& visitor, VarInitStatement* init, bool is_static, bool initialize = true, bool is_extern = false) {
    if(init->is_comptime()) {
        return;
    }
    visitor.debug_comment("var_init defining the value");
    if(is_static) {
        visitor.write("static ");
    } else if(is_extern) {
        visitor.write("extern ");
    }
    auto init_type = init->type ? init->type : init->value->create_type(visitor.allocator);
    value_alloca_store(visitor, init->name_view(), init_type, initialize ? init->value : nullptr);
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

//void allocate_struct_for_struct_value(ToCAstVisitor& visitor, ExtendableMembersContainerNode* def, StructValue* value, const std::string& name, Value* initializer = nullptr) {
//    if(def->generic_params.empty()) {
//        allocate_struct_for_value(visitor, def, value, name, initializer);
//    } else {
//        auto prev_itr = def->active_iteration;
//        def->set_active_iteration(value->generic_iteration);
//        allocate_struct_for_value(visitor, def, value, name, initializer);
//        def->set_active_iteration(prev_itr);
//    }
//}

void allocate_struct_for_func_call(ToCAstVisitor& visitor, ExtendableMembersContainerNode* def, FunctionCall* call, FunctionType* func_type, const chem::string_view& name, Value* initializer = nullptr) {
    if(func_type->returnType->kind() == BaseTypeKind::Generic) {
        auto prev_itr = def->active_iteration;
        def->set_active_iteration(func_type->returnType->get_generic_iteration());
        allocate_struct_for_value(visitor, def, call, name, initializer);
        def->set_active_iteration(prev_itr);
    } else {
        allocate_struct_for_value(visitor, def, call, name, initializer);
    }
}

std::string allocate_temp_struct(ToCAstVisitor& visitor, ASTNode* def_node, Value* initializer) {
    auto struct_name = visitor.get_local_temp_var_name();
    allocate_struct_by_name(visitor, def_node, chem::string_view(struct_name.data(), struct_name.size()));
    return struct_name;
}

void moved_value_call(ToCAstVisitor& visitor, Value* value) {
    auto known_t = value->pure_type_ptr();
    auto linked_node = known_t->get_direct_linked_node();
    if(!linked_node) {
        // probably an int, in generic where generic type is moved, but being used with int, so can't be moved
        return;
    }
    const auto linked_node_kind = linked_node->kind();
    if(!linked_node->isStoredStructType(linked_node_kind)) {
        // we can pass directly, as the node is not a stored struct type, what could it be except typealias
        return;
    }
    if(!ASTNode::isStoredStructType(linked_node_kind)) {
        // non struct types are just non movable
        return;
    }
    auto movable = linked_node->as_members_container();
    if(!movable) {
        // if there's no call to move / clear func call, we don't need to allocate a temporary struct, C would perform memcpy automatically
        return;
    }
    const auto move_func = movable->pre_move_func();
    const auto clear_func = movable->clear_func();
    if(!move_func && !clear_func) {
        // since pre move and clear both do not exist, direct reference to struct value will create automatically a std memcpy in C
        return;
    }
    // allocating temporary struct
    auto struct_name = allocate_temp_struct(visitor, linked_node, clear_func ? value : nullptr);
    visitor.new_line_and_indent();
    func_container_name(visitor, clear_func ? clear_func : move_func);
    visitor.write('(');
    if (move_func) {
        visitor.write('&');
        visitor.write(struct_name);
        visitor.write(", ");
    }
    visitor.write('&');
    visitor.visit(value);
    visitor.write(')');
    visitor.write(';');
    visitor.local_allocated[value] = struct_name;
}

void move_identifier(ToCAstVisitor& visitor, VariableIdentifier* id) {
    if(!id->is_moved) return;
    const auto linked = id->linked;
    const auto kind = linked->kind();
    if(kind != ASTNodeKind::VarInitStmt && kind != ASTNodeKind::FunctionParam) {
        moved_value_call(visitor, id);
    }
}

void move_chain(ToCAstVisitor& visitor, AccessChain* chain) {
    if(chain->values.size() == 1) {
        auto identifier = chain->values.back()->as_identifier();
        if(identifier) {
            if (chain->is_moved()) {
                identifier->is_moved = true;
            }
            move_identifier(visitor, identifier);
        }
        return;
    }
    if(chain->is_moved()) {
        moved_value_call(visitor, chain);
    }
}

// will call clear function on given value
void move_value(ToCAstVisitor& visitor, Value* value) {
    switch(value->val_kind()) {
        case ValueKind::AccessChain:
            move_chain(visitor, value->as_access_chain_unsafe());
            return;
        case ValueKind::Identifier:
            move_identifier(visitor, value->as_identifier_unsafe());
            return;
        default:
            return;
    }
}

// will cal clear functions on moved arguments in function call
void move_func_call(ToCAstVisitor& visitor, FunctionCall* call) {
    for(auto& value : call->values) {
        move_value(visitor, value);
    }
}

// will call clear functions as required on the access chain
//void move_access_chain(ToCAstVisitor& visitor, AccessChain* chain) {
//    for(auto& value : chain->values) {
//        const auto func_call = value->as_func_call();
//        if(func_call) {
//            move_func_call(visitor, func_call);
//        } else {
//            const auto nested = value->as_access_chain();
//            if(nested) {
//                move_access_chain(visitor, nested);
//            }
//        }
//    }
//}

void write_implicit_args(ToCAstVisitor& visitor, FunctionType* func_type, FunctionCall* call, bool has_comma_before);

//void func_call_that_returns_struct(ToCAstVisitor& visitor, FunctionType* func_type, FunctionCall* call) {
//    auto last = call;
//    auto func_decl = last->safe_linked_func();
//    auto parent = call->parent_val;
//    auto grandpa = get_grandpa_from(call->parent_val);
//    bool is_lambda = (parent->linked_node() != nullptr && parent->linked_node()->as_struct_member() != nullptr);
//    if (visitor.pass_structs_to_initialize && func_type->returnType->isStructLikeType()) {
//        // functions that return struct
//        auto allocated = visitor.local_allocated.find(last);
//        if (allocated == visitor.local_allocated.end()) {
//            visitor.write("[NotFoundAllocated in func_call_that_returns_struct]");
//            return;
//        }
//        if(func_decl) {
//            if(func_decl->parent_node) {
//                const auto container = func_decl->parent_node->as_members_container();
//                if(!container->generic_params.empty()) {
//                    container->set_active_iteration(last->generic_iteration);
//                }
//            }
//            func_container_name(visitor, func_decl);
//        } else {
//            auto prev_nested = visitor.nested_value;
//            visitor.nested_value = true;
////            const auto name_val = build_parent_chain(call->parent_val, visitor.allocator);
//            call->parent_val->accept(&visitor);
//            visitor.nested_value = prev_nested;
//        }
//        if (grandpa && !is_lambda) {
//            auto grandpaType = grandpa->create_type(visitor.allocator);
////            func_container_name(visitor, grandpaType->linked_node(), parent->linked_node());
////            func_name(visitor, parent, func_decl);
//            visitor.write('(');
//        } else {
////            if (func_decl && func_decl->has_annotation(AnnotationKind::Constructor)) {
////                // struct name for the constructor, getting through return type
////                auto linked = func_decl->returnType->linked_node();
////                if (linked && linked->as_struct_def()) {
////                    visitor->write(linked->as_struct_def()->name);
////                }
////            }
////            access_chain(visitor, values, start, end - 1);
//            visitor.write('(');
//        }
//        visitor.write('&');
//        visitor.write(allocated->second);
////        if (!last->values.empty() || func_type->has_self_param()) {
////            visitor.write(", ");
////        }
//        write_implicit_args(visitor, func_type, call, false);
//        func_call_args(visitor, last, func_type);
//        visitor.write(')');
//        if(!visitor.nested_value) {
//            visitor.write(';');
//        }
//        visitor.new_line_and_indent();
//    }
//}

void CBeforeStmtVisitor::visit(FunctionCall *call) {

    const auto linked = call->parent_val->linked_node();
    // enum member can't be called, we're using it as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;
    const auto decl = ASTNode::isFunctionDecl(linked_kind) ? linked->as_function_unsafe() : nullptr;

    // visit the values
    CommonVisitor::visit(call);

    // handling non variant function
    if(linked_kind != ASTNodeKind::VariantMember) {

//        auto func_type = call->function_type(visitor.allocator);
//        if (decl) {
//            if(call->generic_iteration == -1) {
//                call->fix_generic_iteration(visitor, nullptr);
//            }
//            // TODO comptime functions should be shifted from CBeforeStmtVisitor
//            if (decl->is_comptime()) {
//                const auto prev_iteration = call->set_curr_itr_on_decl();
//                auto eval = evaluated_func_val(visitor, decl, call);
//                const auto eval_kind = eval->val_kind();
//                if (eval_kind == ValueKind::AccessChain) {
//                    const auto chain = eval->as_access_chain_unsafe();
//                    chain->fix_generic_iteration(visitor, visitor.current_func_type->returnType);
//                }
//                auto identifier = visitor.get_local_temp_var_name();
//                if (eval_kind == ValueKind::StructValue) {
//                    allocate_struct_by_name(visitor, eval->as_struct_value_unsafe()->linked_extendable(), chem::string_view(identifier.data(), identifier.size()));
//                    visitor.local_allocated[eval] = identifier;
//                }
//                process_init_value(eval, chem::string_view(identifier.data(), identifier.size()));
//                eval->accept(this);
//                if(decl && decl->is_generic()) {
//                    decl->set_active_iteration(prev_iteration);
//                }
//                return;
//            }
//        }

//        // if function returns struct, we allocate that struct here
//        if(func_type->returnType->isStructLikeType()) {
//            auto returnLinked = func_type->returnType->linked_node();
//            const auto returnLinkedType = returnLinked->kind();
//            if(returnLinkedType == ASTNodeKind::StructDecl) {
//                const auto temp_name = visitor.get_local_temp_var_name();
//                allocate_struct_for_func_call(visitor, returnLinked->as_struct_def_unsafe(), call, func_type, chem::string_view(temp_name.data(), temp_name.size()));
//            } else if(returnLinkedType == ASTNodeKind::VariantDecl) {
//                const auto temp_name = visitor.get_local_temp_var_name();
//                allocate_struct_for_func_call(visitor, returnLinked->as_variant_def_unsafe(), call, func_type, chem::string_view(temp_name.data(), temp_name.size()));
//            } else if(returnLinkedType == ASTNodeKind::InterfaceDecl && func_type->returnType->pure_type()->kind() == BaseTypeKind::Dynamic) {
//                const auto temp_name = visitor.get_local_temp_var_name();
//                allocate_fat_pointer_for_value(visitor, call, chem::string_view(temp_name.data(), temp_name.size()), nullptr);
//            }
//        }
//
//        // functions that are constructors / return struct
//        // we call these functions beforehand and prepare
//        if(!decl || !decl->is_comptime()) {
//            const auto returnType = func_type->returnType->pure_type();
//            const auto returnTypeKind = returnType->kind();;
//            if (returnTypeKind == BaseTypeKind::Dynamic) {
//                func_call_that_returns_struct(visitor, func_type, call);
//                return;
//            } else {
//                const auto return_linked = returnType->get_direct_linked_node(returnTypeKind);
//                if(return_linked) {
//                    const auto returnKind = return_linked->kind();
//                    if (returnKind == ASTNodeKind::StructDecl || returnKind == ASTNodeKind::VariantDecl || returnKind == ASTNodeKind::UnionDecl) {
//                        func_call_that_returns_struct(visitor, func_type, call);
//                        return;
//                    }
//                }
//            }
//        }

    }

}

void chain_after_func(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned start, const unsigned end, const unsigned total_size);

void func_name(ToCAstVisitor& visitor, Value* ref, FunctionDeclaration* func_decl) {
    func_decl->runtime_name_no_parent_fast(*visitor.output);
}

void func_name(ToCAstVisitor& visitor, FunctionDeclaration* func_decl) {
    func_decl->runtime_name_no_parent_fast(*visitor.output);
}

void write_implicit_args(ToCAstVisitor& visitor, FunctionType* func_type, FunctionCall* call, bool has_comma_before = true) {
    for(const auto param : func_type->params) {
        if(param->is_implicit) {
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
                        visitor.error("couldn't find implicit argument with name '" + param->name.str() + "'", call);
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

void CBeforeStmtVisitor::visit(VariableIdentifier *identifier) {
    if(identifier->is_moved) {
        move_identifier(visitor, identifier);
    }
}

void CBeforeStmtVisitor::visit(AccessChain *chain) {

    CommonVisitor::visit(chain);

    move_chain(visitor, chain);

//    const auto start = 0;
//    const auto end = chain->values.size();
//    auto& values = chain->values;
//    unsigned i = start;
//    // function call would be processed recursively
//    {
//        int j = end - 1;
//        while(j >= 0) {
//            auto& current = values[j];
//            auto call = current->as_func_call();
//            if(call) {
//                func_call_that_returns_struct(visitor, this, values, start, j + 1);
//            }
//            j--;
//        }
//    }

}

void write_variant_call(ToCAstVisitor& visitor, FunctionCall* call) {

    const auto member = call->parent_val->linked_node()->as_variant_member();
    const auto linked = member->parent_node;
    const auto index = linked->direct_child_index(member->name);

    int16_t prev_itr;
    if(!linked->generic_params.empty()) {
        prev_itr = linked->get_active_iteration();
        linked->set_active_iteration(call->generic_iteration);
    }

    visitor.write("(struct ");
    node_parent_name(visitor, linked);
    struct_name(visitor, linked);
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
        const auto& val = call->values[i];
        visitor.accept_mutating_value(value.second->type, val, false);
        visitor.write(", ");
        i++;
    }
    visitor.nested_value = prev_nested;
    visitor.write('}');

    if(!linked->generic_params.empty()) {
        linked->set_active_iteration(prev_itr);
    }
}

void CBeforeStmtVisitor::visit(VariantCall *call) {

    CommonVisitor::visit(call);
    const auto member = call->parent_val->linked_node()->as_variant_member();
    const auto linked = member->parent_node;
    const auto index = linked->direct_child_index(member->name);

    int16_t prev_itr;
    if(!linked->generic_params.empty()) {
        prev_itr = linked->get_active_iteration();
        linked->set_active_iteration(call->generic_iteration);
    }

    visitor.write("struct ");
    node_parent_name(visitor, linked);
    struct_name(visitor, linked);
    visitor.write(' ');
    const auto local = visitor.get_local_temp_var_name();
    visitor.write_str(local);
    visitor.local_allocated[call] = local;
    visitor.write(" = ");
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
        const auto& val = call->values[i];
        visitor.accept_mutating_value(value.second->type, val, false);
        visitor.write(", ");
        i++;
    }
    visitor.nested_value = prev_nested;
    visitor.write('}');
    visitor.write(';');
    visitor.new_line_and_indent();

    if(!linked->generic_params.empty()) {
        linked->set_active_iteration(prev_itr);
    }

}

void CBeforeStmtVisitor::process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const chem::string_view& identifier) {
    call->set_curr_itr_on_decl();
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
                } else if (linked_kind == ASTNodeKind::InterfaceDecl && func_type->returnType->pure_type()->kind() == BaseTypeKind::Dynamic) {
                    allocate_fat_pointer_for_value(visitor, &call, identifier, nullptr);
                }
            }
        }
    }
}

void CBeforeStmtVisitor::visit(VarInitStatement *init) {
//    if (!init->type) {
//        init->type = init->value->create_type(visitor.allocator);
//    }
    CommonVisitor::visit(init);
}

CTopLevelDeclarationVisitor::CTopLevelDeclarationVisitor(
    ToCAstVisitor &visitor,
    CValueDeclarationVisitor *value_visitor
) : SubVisitor(visitor), value_visitor(value_visitor) {

}

enum class DestructionJobType {
    Default,
    Array
};

struct DestructionJob {
    DestructionJobType type;
    chem::string_view self_name;
    ASTNode* initializer;
    union {
        struct {
            ExtendableMembersContainerNode* parent_node;
            int16_t generic_iteration;
            FunctionDeclaration* destructor;
            bool is_pointer;
        } default_job;
        struct {
            int array_size;
            ExtendableMembersContainerNode* linked;
            int16_t generic_iteration;
            FunctionDeclaration* destructorFunc;
        } array_job;
    };
};

class CDestructionVisitor : public Visitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    bool destroy_current_scope = true;

    bool new_line_before = true;

    std::vector<DestructionJob> destruct_jobs;

    void destruct_no_gen(
            const chem::string_view& self_name,
            ExtendableMembersContainerNode* linked,
            int16_t generic_iteration,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void destruct(
            const chem::string_view& self_name,
            ExtendableMembersContainerNode* linked,
            int16_t generic_iteration,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void queue_destruct(
            const chem::string_view& self_name,
            ASTNode* initializer,
            int16_t generic_iteration,
            ExtendableMembersContainerNode* linked,
            bool is_pointer = false
    );

    void queue_destruct(const chem::string_view& self_name, ASTNode* initializer, FunctionCall* call);

    void destruct_arr_ptr(const chem::string_view& self_name, Value* array_size, ExtendableMembersContainerNode* linked, int16_t generic_iteration, FunctionDeclaration* destructor);

    void destruct_arr(const chem::string_view& self_name, int array_size, ExtendableMembersContainerNode* linked, int16_t generic_iteration, FunctionDeclaration* destructor) {
        IntValue siz(array_size, ZERO_LOC);
        destruct_arr_ptr(self_name, &siz, linked, generic_iteration, destructor);
    }

    void destruct(const DestructionJob& job, Value* current_return);

    bool queue_destruct_arr(const chem::string_view& self_name, ASTNode* initializer, BaseType* elem_type, int array_size);

    void visit(VarInitStatement *init) final;

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

void assign_statement(ToCAstVisitor& visitor, AssignStatement* assign) {
    auto type = assign->lhs->create_type(visitor.allocator)->pure_type();
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
    visitor.accept_mutating_value(type, assign->value, true);
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
                        eval->second->accept(this);
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
                    int16_t generic_iteration = 0;
                    if(!struct_def->generic_params.empty()) {
                        generic_iteration = func_type->returnType->get_generic_iteration();
                    }
                    auto destructor = struct_def->destructor_func();
                    if(destructor) {
                        auto destructible = visitor.local_allocated.find(call);
                        if (destructible != visitor.local_allocated.end()) {
                            visitor.destructor->destruct(
                                    chem::string_view(destructible->second.data(), destructible->second.size()),
                                    struct_def,
                                    generic_iteration,
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

void CAfterStmtVisitor::visit(AccessChain *chain) {
    CommonVisitor::visit(chain);
//    destruct_chain(chain, chain->is_node());
}

void CAfterStmtVisitor::visit(FunctionCall *call) {
//    auto decl = call->safe_linked_func();
//    if(decl && decl->is_comptime()) {
//        auto eval = visitor->evaluated_func_calls.find(call);
//        if(eval != visitor->evaluated_func_calls.end()) {
//            eval->second->accept(this);
//            return;
//        } else {
//            std::cerr << "[2c] warn: evaluated function call value not found in after statement visitor" << std::endl;
//        }
//    }
    for(auto& val : call->values) {
        const auto chain = val->as_access_chain();
        if(chain) {
            // if we ever pass struct as a reference, where struct is created at call time
            // we can set destruct_last to true, to destruct the struct after this call
//            destruct_chain(chain, false);
        } else {
            // if we ever pass struct as a reference, where struct is created at call time
            // we can set destruct_last to true, to destruct the struct after this call
//            const auto struc = val->as_struct();
//            if(struc) {
//                auto found = visitor->local_allocated.find(struc);
//                if(found != visitor->local_allocated.end()) {
//                    visitor->destructor->queue_destruct(
//                            found->second,
//                            nullptr,
//                            struc->generic_iteration,
//                            struc->definition
//                    );
//                } else {
//                    val->accept(this);
//                }
//            } else {
                // don't need to destruct, value is accepting this visitor
                // if it's access chain, it will always trigger function calls present in access chain, which will again trigger this
                // if it's identifier, we have a method for identifier on this visitor
                val->accept(this);
//            }
        }
    }
}

void CDestructionVisitor::destruct_no_gen(const chem::string_view& self_name, ExtendableMembersContainerNode* parent_node, int16_t generic_iteration, FunctionDeclaration* destructor, bool is_pointer) {
    if(new_line_before) {
        visitor.new_line_and_indent();
    }
    func_container_name(visitor, parent_node, destructor);
    visitor.write(destructor->name_view());
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

void CDestructionVisitor::destruct(const chem::string_view& self_name, ExtendableMembersContainerNode* parent_node, int16_t generic_iteration, FunctionDeclaration* destructor, bool is_pointer) {
    int16_t prev_itr;
    if(!parent_node->generic_params.empty()) {
        prev_itr = parent_node->active_iteration;
        parent_node->set_active_iteration(generic_iteration);
    }
    destruct_no_gen(self_name, parent_node, generic_iteration, destructor, is_pointer);
    if(!parent_node->generic_params.empty()) {
        parent_node->set_active_iteration(prev_itr);
    }
}

void CDestructionVisitor::queue_destruct(const chem::string_view& self_name, ASTNode* initializer, int16_t generic_iteration, ExtendableMembersContainerNode* linked, bool is_pointer) {
    if(!linked) return;
    auto destructorFunc = linked->destructor_func();
    if(destructorFunc) {
        destruct_jobs.emplace_back(DestructionJob{
                .type = DestructionJobType::Default,
                .self_name = self_name,
                .initializer = initializer,
                .default_job = {
                        linked,
                        generic_iteration,
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
            const auto variant = member->parent_node;
            queue_destruct(self_name, initializer, return_type->get_generic_iteration(), variant);
            return;
        }
        queue_destruct(self_name, initializer, return_type->get_generic_iteration(), linked->as_extendable_members_container_node());
    }
}

void CDestructionVisitor::destruct_arr_ptr(const chem::string_view &self_name, Value* array_size, ExtendableMembersContainerNode* parent_node, int16_t generic_iteration, FunctionDeclaration* destructorFunc) {

    int16_t prev_itr;
    const auto is_generic = parent_node->is_generic();
    if(is_generic) {
        prev_itr = parent_node->active_iteration;
        parent_node->set_active_iteration(generic_iteration);
    }

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
    destruct_no_gen(chem::string_view(name.data(), name.size()), parent_node, generic_iteration, destructorFunc, false);
    visitor.indentation_level--;
    visitor.new_line_and_indent();
    visitor.write('}');

    if(is_generic) {
        parent_node->set_active_iteration(prev_itr);
    }
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
            destruct(job.self_name, job.default_job.parent_node, job.default_job.generic_iteration, job.default_job.destructor, job.default_job.is_pointer);
            break;
        case DestructionJobType::Array:
            destruct_arr(job.self_name, job.array_job.array_size, job.array_job.linked, job.array_job.generic_iteration, job.array_job.destructorFunc);
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
        if(d_param->get_has_moved()) continue;
        auto linked = d_param->type->get_direct_linked_node();
        if(linked) {
            const auto members_container = linked->as_members_container();
            if(members_container) {
                const auto destructor_func = members_container->destructor_func();
                if (destructor_func) {
                    queue_destruct(d_param->name, d_param, d_param->type->get_generic_iteration(),
                                   linked->as_extendable_members_container_node(), true);
                }
            }
        }
    }
}

bool CDestructionVisitor::queue_destruct_arr(const chem::string_view& self_name, ASTNode* initializer, BaseType *elem_type, int array_size) {
    if(elem_type->isStructLikeType()) {
        auto linked = elem_type->linked_node();
        FunctionDeclaration* destructorFunc;
        const auto struc_def = linked->as_extendable_members_container_node();
        if(struc_def) {
            destructorFunc = struc_def->destructor_func();
            if (!destructorFunc) {
                return false;
            }
            int16_t generic_iteration = 0;
            if(struc_def->is_generic()) {
                auto type = initializer->create_value_type(visitor.allocator);
                generic_iteration = type->get_generic_iteration();
            }
            destruct_jobs.emplace_back(DestructionJob{
                .type = DestructionJobType::Array,
                .self_name = self_name,
                .initializer = initializer,
                .array_job = {
                    array_size,
                    struc_def,
                    generic_iteration,
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
    auto chain = init_value->as_access_chain_unsafe();
    if(init_val_kind == ValueKind::AccessChain) {
        const auto last_func_call = chain->values.back()->as_func_call();
        if(last_func_call) {
            queue_destruct(init->name_view(), init, last_func_call);
            return;
        } else {
            if(chain->is_moved()) {
                auto init_type = init->create_value_type(visitor.allocator);
                auto linked = init_type->linked_node();
                if(!linked) {
                    return;
                }
                queue_destruct(init->name_view(), init, init_type->get_generic_iteration(), linked->as_extendable_members_container_node());
            }
            return;
        }
    } else if(init_val_kind == ValueKind::FunctionCall) {
        queue_destruct(init->name_view(), init, init_value->as_func_call_unsafe());
        return;
    }
    auto id = init_value->as_identifier_unsafe();
    if(init_val_kind == ValueKind::Identifier && id->is_moved) {
        auto init_type = init->create_value_type(visitor.allocator);
        auto linked = init_type->linked_node();
        if(!linked) {
            visitor.error("couldn't destruct var init", init);
            return;
        }
        queue_destruct(init->name_view(), init, init_type->get_generic_iteration(), linked->as_extendable_members_container_node());
        return;
    }
    auto array_val = init_value->as_array_value_unsafe();
    if(init_val_kind == ValueKind::ArrayValue) {
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
    auto struct_val = init_value->as_struct_value_unsafe();
    if(init_val_kind == ValueKind::StructValue) {
        queue_destruct(init->name_view(), init, struct_val->generic_iteration, struct_val->linked_struct());
    }
}

void CDestructionVisitor::visit(VarInitStatement *init) {
    // do not destruct pointers, references or moved objects
    if(init->get_has_moved()) {
        return;
    }
    const auto pure_t = init->create_value_type(visitor.allocator)->pure_type();
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
            auto linked = init->type->linked_node();
            if (linked)
                queue_destruct(init->name_view(), init, init->type->get_generic_iteration(),
                               linked->as_struct_def());
        } else if(init->type->kind() == BaseTypeKind::Array) {
            auto type = (ArrayType*) init->type;
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
    visitor.destructor->queue_destruct_decl_params(decl);
    visitor.indentation_level+=1;
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

std::string func_type_alias(ToCAstVisitor& visitor, FunctionType* type) {
    std::string alias = "__chx_functype_";
    alias += std::to_string(random(100,999)) + "_";
    alias += std::to_string(visitor.declarer->func_type_num++);
    func_type_with_id(visitor, type, chem::string_view(alias.data(), alias.size()));
    visitor.declarer->aliases[type] = alias;
    return alias;
}

std::string typedef_func_type(ToCAstVisitor& visitor, FunctionType* type) {
    visitor.new_line_and_indent();
    visitor.write("typedef ");
    auto alia = func_type_alias(visitor, type);
    visitor.write(';');
    return alia;
}

//void func_call(ToCAstVisitor& visitor, FunctionCall* call, FunctionType* func_type) {
//    visitor.write('(');
//    func_call_args(visitor, call, func_type);
//    visitor.write(')');
////    if(!visitor->nested_value) {
////        visitor->write(';');
////    }
//}

void CValueDeclarationVisitor::visit(LambdaFunction *lamb) {
    CommonVisitor::visit(lamb);
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
                PointerType pointer(var->linked->create_value_type(visitor.allocator), ZERO_LOC);
                visitor.visit(&pointer);
            } else {
                visitor.visit(var->linked->create_value_type(visitor.allocator));
            }
            visitor.space();
            visitor.write(var->name);
            visitor.write(';');
        }
        visitor.indentation_level -= 1;
        visitor.new_line_and_indent();
        visitor.write("};");
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

void CValueDeclarationVisitor::visit(ReturnStatement *stmt) {

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

    CommonVisitor::visit(stmt);

}


void CValueDeclarationVisitor::visit(FunctionCall *call) {
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
    CommonVisitor::visit(call);
}

void CValueDeclarationVisitor::visit(VariantCall *call) {
//    auto itr = call->get_member()->values.begin();
//    for(auto& value_ptr : call->values) {
//        // replace calls to implicit constructor with actual calls
//        auto implicit_constructor = itr->second->type->implicit_constructor_for(visitor.allocator, value_ptr);
//        if (implicit_constructor) {
//            value_ptr = call_with_arg(implicit_constructor, value_ptr, itr->second->type, visitor.allocator, visitor);
//        }
//        itr++;
//    }
    CommonVisitor::visit(call);
}

void CValueDeclarationVisitor::visit(ArrayValue *arrayVal) {

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

    CommonVisitor::visit(arrayVal);

}

void CValueDeclarationVisitor::visit(StructValue *structValue) {

//    const auto container = structValue->variables();
//    for(auto& value : structValue->values) {
//        auto& value_ptr = value.second.value;
//        auto variable = container->variable_type_index(value.first);
//        auto implicit = variable.second->implicit_constructor_for(visitor.allocator, value_ptr);
//        if(implicit) {
//            value_ptr = call_with_arg(implicit, value_ptr, variable.second, visitor.allocator, visitor);
//        }
//    }

    CommonVisitor::visit(structValue);

}

void declare_params(CValueDeclarationVisitor* value_visitor, std::vector<FunctionParam*>& params) {
    for(auto& param : params) {
        if(param->type->kind() == BaseTypeKind::Function && param->type->as_function_type()->isCapturing()) {
            // do not declare capturing function types
            continue;
        }
        if(param->type->kind() != BaseTypeKind::Function) {
            param->accept(value_visitor);
        }
    }
}

void func_ret_func_proto_after_l_paren(ToCAstVisitor& visitor, FunctionDeclaration* decl, FunctionType* retFunc, unsigned declFuncParamStart = 0, unsigned retFuncParamStart = 0) {
    visitor.write('*');
    decl->runtime_name(*visitor.output);
    visitor.write('(');
    func_type_params(visitor, decl, declFuncParamStart);
    visitor.write("))(");
    func_type_params(visitor, retFunc, retFuncParamStart);
    visitor.write(')');
}

void func_that_returns_func_proto(ToCAstVisitor& visitor, FunctionDeclaration* decl, const chem::string_view& name, FunctionType* retFunc) {
    if(decl->body.has_value()) {
        visitor.write("static ");
    }
    accept_func_return(visitor, retFunc->returnType);
    visitor.write("(");
    func_ret_func_proto_after_l_paren(visitor, decl, retFunc);
}

void declare_func_with_return(ToCAstVisitor& visitor, FunctionDeclaration* decl, const chem::string_view& name) {
    if(decl->is_comptime()) {
        return;
    }
    const auto ret_func = decl->returnType->as_function_type();
    if(ret_func && !ret_func->isCapturing()) {
        func_that_returns_func_proto(visitor, decl, name, ret_func);
    } else {
        const auto ret_kind = decl->returnType->kind();
        accept_func_return_with_name(visitor, decl, decl->body.has_value() && !decl->is_exported_fast());
        visitor.write('(');
        func_type_params(visitor, decl);
        visitor.write(')');
    }
}

void declare_by_name(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, const chem::string_view& name) {
    if(decl->is_comptime()) {
        return;
    }
    declare_params(tld->value_visitor, decl->params);
    if(decl->returnType->as_function_type() == nullptr) {
        decl->returnType->accept(tld->value_visitor);
    }
    tld->visitor.new_line_and_indent();
    declare_func_with_return(tld->visitor, decl, name);
    tld->visitor.write(';');
}

// when a function is inside struct / interface
void declare_contained_func(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, bool overrides, StructDefinition* overridden = nullptr) {
    if(decl->is_comptime()) {
        return;
    }
    declare_params(tld->value_visitor, decl->params);
    if(decl->returnType->as_function_type() == nullptr) {
        decl->returnType->accept(tld->value_visitor);
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
    const auto func_parent_kind = decl->parent_node->kind();
    auto is_parent_interface = func_parent_kind == ASTNodeKind::InterfaceDecl;
    const auto decl_return_func_type = decl->returnType->as_function_type();
    if(decl_return_func_type != nullptr && !decl_return_func_type->isCapturing()) {
        tld->value_visitor->write("static ");
        accept_func_return(tld->visitor, decl_return_func_type->returnType);
        tld->write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(tld->visitor, decl, decl_return_func_type, i);
    } else {
        accept_func_return_with_name(tld->visitor, decl, (is_parent_interface || decl->body.has_value()) && !decl->is_exported_fast());
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

void CTopLevelDeclarationVisitor::visit(VarInitStatement *init) {
    if(!init->is_top_level()) return;
    visitor.new_line_and_indent();
    const auto is_exported = init->is_exported();
    var_init(visitor, init, !is_exported, !redefining, is_exported && redefining);
}

void early_declare_node(CTopLevelDeclarationVisitor& visitor, ASTNode* node) {
    const auto node_kind = node->kind();
    if (node_kind == ASTNodeKind::StructDecl) {
        const auto def = node->as_struct_def_unsafe();
        if(visitor.redefining || def->iterations_declared < def->total_generic_iterations()) {
            // declare inherited types
            for(auto& inherit : def->inherited) {
                auto in_node = inherit.type->get_direct_linked_node();
                if(in_node) {
                    early_declare_node(visitor, in_node);
                }
            }
            // declare sub variables
            for(auto& var : def->variables) {
                auto sub_node = var.second->known_type()->get_direct_linked_node();
                if(sub_node) {
                    early_declare_node(visitor, sub_node);
                }
            }
            visitor.declare_struct_iterations(def);
        }
    } else if (node_kind == ASTNodeKind::VariantDecl) {
        const auto def = node->as_variant_def_unsafe();
        if(visitor.redefining || def->iterations_declared < def->total_generic_iterations()) {
            visitor.declare_variant_iterations(def);
        }
    } else if (node_kind == ASTNodeKind::UnionDecl) {
        // TODO this
    }
}

void early_declare_gen_arg_structs(CTopLevelDeclarationVisitor& visitor, std::vector<GenericTypeParameter*>& gen_params) {
    for(auto& param : gen_params) {
        auto t = param->known_type();
        const auto node = t->get_direct_linked_node();
        if(node) {
            early_declare_node(visitor, node);
        }
    }
}

void early_declare_composed_variables(CTopLevelDeclarationVisitor& visitor, VariablesContainer& container) {
    for(auto& variable : container.variables) {
        auto t = variable.second->create_value_type(visitor.visitor.allocator);
        const auto node = t->get_direct_linked_node();
        if(node) {
            early_declare_node(visitor, node);
        }
    }
}

void func_decl_with_name(ToCAstVisitor& visitor, FunctionDeclaration* decl, const chem::string_view& name);

// just generates the remaining functions
void gen_generic_functions(ToCAstVisitor& visitor, FunctionDeclaration* decl) {
    int16_t itr = decl->bodies_gen_index;
    int16_t total = decl->total_generic_iterations();
    while(itr < total) {
        decl->set_active_iteration(itr);
        func_decl_with_name(visitor, decl, decl->name_view());
        itr++;
    }
    // set to -1, so whoever tries to access generic parameters types, they receive an error if active iteration is not set again
    decl->set_active_iteration(-1);
    decl->bodies_gen_index = total;
}

void CTopLevelDeclarationVisitor::declare_func(FunctionDeclaration* decl) {
    // TODO we will fix capturing lambda types when introducing generics and unions
//    if(decl->returnType->function_type() && decl->returnType->function_type()->isCapturing) {
//        visitor->error("Function name " + decl->name + " returns a capturing lambda, which is not supported");
//    }
    if(decl->generic_params.empty()) {
        declare_by_name(this, decl, decl->name_view());
    } else {
        auto size = decl->total_generic_iterations();
        int16_t i = 0;
        while(i < size) {
            decl->set_active_iteration(i);
            early_declare_gen_arg_structs(*this, decl->generic_params);
            declare_by_name(this, decl, decl->name_view());
            i++;
        }
        // set the active iteration to -1, so whoever accesses it without setting it to zero receives an error
        decl->set_active_iteration(-1);
        // generate remaining functions
        gen_generic_functions(visitor, decl);
    }
}

void CTopLevelDeclarationVisitor::visit(FunctionDeclaration *decl) {
    declare_func(decl);
}

void CTopLevelDeclarationVisitor::visit(ExtensionFunction *decl) {
    declare_func(decl);
}

void CValueDeclarationVisitor::visit(FunctionDeclaration *decl) {
    if(decl->body.has_value() && !decl->is_comptime()) {
        decl->body.value().accept(this);
    }
}

void CValueDeclarationVisitor::visit(ExtensionFunction *decl) {
    if(decl->body.has_value() && !decl->is_comptime()) {
        decl->body.value().accept(this);
    }
}

void CValueDeclarationVisitor::visit(EnumDeclaration *enumDecl) {
    if(visitor.inline_enum_member_access) return;
    unsigned i = 0;
    for(auto& mem : enumDecl->members) {
        visitor.new_line_and_indent();
        std::string value = ("__CHENUM_");
        value += (std::to_string(enum_num++));
        value += ('_');
        value += (enumDecl->name_view().view());
        value += (std::to_string(random(100, 999)));
        value += ("_");
        value += (mem.first.view());
        write("#define ");
        visitor.write_str(value);
        write(' ');
        *visitor.output << mem.second->get_default_index();
        aliases[mem.second] = value;
        i++;
    }
}

void CTopLevelDeclarationVisitor::visit(TypealiasStatement *stmt) {
    visitor.new_line_and_indent();
    write("typedef ");
    const auto kind = stmt->actual_type->kind();
    if(kind == BaseTypeKind::Function) {
        const auto func_type = stmt->actual_type->as_function_type_unsafe();
        func_type_with_id(visitor, func_type, stmt->name_view());
    } else {
        visitor.visit(stmt->actual_type);
        write(' ');
        node_parent_name(visitor, stmt);
        write(stmt->name_view());
    }
    write(';');
}

void CTopLevelDeclarationVisitor::visit(UnionDef *def) {
    visitor.new_line_and_indent();
    write("union ");
    node_parent_name(visitor, def);
    write(def->name_view());
    write(" {");
    visitor.indentation_level+=1;
    for(auto& var : def->variables) {
        visitor.new_line_and_indent();
        visitor.visit(var.second);
    }
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    write("};");
    for(auto& func : def->functions()) {
        declare_contained_func(this, func, false);
    }
}

void CTopLevelDeclarationVisitor::visit(Namespace *ns) {
    if(ns->is_comptime()) return;
    for(const auto node : ns->nodes) {
        node->accept(this);
    }
}

void CTopLevelDeclarationVisitor::declare_struct_def_only(StructDefinition* def) {
    for(auto& mem : def->variables) {
        mem.second->accept(value_visitor);
    }
    // before we declare this struct, we must early declare any direct struct type variables
    // inside this struct, because some structs get used inside which are present in other modules
    // will be declared later, so C responds with incomplete type
    early_declare_composed_variables(*this, *def);
    visitor.new_line_and_indent();
    write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    write(" {");
    visitor.indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def) {
            visitor.new_line_and_indent();
            visitor.write("struct ");
            struct_name(visitor, struct_def);
            visitor.space();
            visitor.write(struct_def->name_view());
            visitor.write(';');
        }
    }
    for(auto& var : def->variables) {
        visitor.new_line_and_indent();
        visitor.visit(var.second);
    }
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    write("};");
}

void CTopLevelDeclarationVisitor::declare_struct(StructDefinition* def) {
    declare_struct_def_only(def);
    for(auto& func : def->functions()) {
        if(def->get_overriding_interface(func) == nullptr) {
            declare_contained_func(this, func, false);
        }
    }
}

static void contained_struct_functions(ToCAstVisitor& visitor, StructDefinition* def);

// just generates any remaining struct functions
void gen_generic_struct_functions(ToCAstVisitor& visitor, StructDefinition* def) {
    const auto total_itr = def->total_generic_iterations();
    if(total_itr == 0) return; // generic type never used (yet)
    int16_t itr = def->iterations_body_done;
    if(itr >= total_itr) return;
    auto prev = def->active_iteration;
    while (itr < total_itr) {
        def->set_active_iteration(itr);
        contained_struct_functions(visitor, def);
        itr++;
    }
    def->iterations_body_done = total_itr;
    def->set_active_iteration(prev);
}

void CTopLevelDeclarationVisitor::declare_struct_iterations(StructDefinition* def) {
    if(def->generic_params.empty()) {
        if(redefining || def->iterations_declared == 0) {
            declare_struct(def);
        }
        def->iterations_declared = 1;
    } else {
        // when redefining (struct imported from other module), we declare all iterations, otherwise begin where left off
        int16_t itr = redefining ? (int16_t) 0 : def->iterations_declared;
        const auto total = def->total_generic_iterations();
        while(itr < total) {
            def->set_active_iteration(itr);
            // early declare structs that are generic arguments
            early_declare_gen_arg_structs(*this, def->generic_params);
            declare_struct(def);
            itr++;
        }
        def->iterations_declared = total;
        def->set_active_iteration(-1);
        // generate any remaining functions that haven't been generated
        if(redefining) {
            gen_generic_struct_functions(visitor, def);
        }
    }
}

void CTopLevelDeclarationVisitor::visit(StructDefinition* def) {
    if(visitor.compiler_interfaces && def->is_compiler_interface()) {
        auto& interfaces = *visitor.compiler_interfaces;
        interfaces.emplace_back(def->name_view().str());
    }
    declare_struct_iterations(def);
}

void CTopLevelDeclarationVisitor::declare_variant(VariantDefinition* def) {
    for(auto& mem : def->variables) {
        mem.second->accept(value_visitor);
    }
    visitor.new_line_and_indent();
    write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    write(" {");
    visitor.indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def) {
            visitor.new_line_and_indent();
            visitor.write("struct ");
            struct_name(visitor, struct_def);
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
    for(auto& var : def->variables) {
        visitor.new_line_and_indent();
        const auto member = var.second->as_variant_member();

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
    for(auto& func : def->functions()) {
//        if(def->get_overriding_interface(func.get()) == nullptr) {
            declare_contained_func(this, func, false);
//        }
    }
}

void generate_contained_functions(ToCAstVisitor& visitor, VariantDefinition* def);

// just generates the remaining functions of the generic variant
void gen_variant_functions(ToCAstVisitor& visitor, VariantDefinition* def) {
    const auto total_itr = def->total_generic_iterations();
    if(total_itr == 0) return; // generic type never used (yet)
    int16_t itr = def->iterations_body_done;
    if(itr >= total_itr) return;
    auto prev = def->active_iteration;
    while (itr < total_itr) {
        def->set_active_iteration(itr);
        generate_contained_functions(visitor, def);
        itr++;
    }
    def->set_active_iteration(prev);
    def->iterations_body_done = total_itr;
}

void CTopLevelDeclarationVisitor::declare_variant_iterations(VariantDefinition* def) {
    if(def->generic_params.empty()) {
        if(redefining) {
            declare_variant(def);
        } else if(def->iterations_declared == 0) {
            declare_variant(def);
        }
        def->iterations_declared = 1;
    } else {
        // when redefining (struct imported from other module), we declare all iterations, otherwise begin where left off
        int16_t itr = redefining ? (int16_t) 0 : def->iterations_declared;
        const auto total = def->total_generic_iterations();
        while(itr < total) {
            def->set_active_iteration(itr);
            // early declare structs that are generic arguments
            early_declare_gen_arg_structs(*this, def->generic_params);
            declare_variant(def);
            itr++;
        }
        def->iterations_declared = total;
        // generating remaining functions of the variant (bodies)
        gen_variant_functions(visitor, def);
    }
}

void CTopLevelDeclarationVisitor::visit(VariantDefinition *def) {
    declare_variant_iterations(def);
}

void create_v_table(ToCAstVisitor& visitor, InterfaceDefinition* interface, StructDefinition* definition) {
    visitor.new_line_and_indent();
    visitor.write("const");
    visitor.write(' ');
    visitor.write("struct");
    visitor.space();
    visitor.write('{');
    visitor.indentation_level += 1;

    // type pointers
    for(auto& func : interface->functions()) {
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
    visitor.write(" = ");
    visitor.write('{');
    visitor.indentation_level += 1;

    // func pointer values
    for(auto& func : interface->functions()) {
        if(func->has_self_param()) {
            visitor.new_line_and_indent();
            visitor.write(definition->name_view());
            visitor.write(func->name_view());
            visitor.write(',');
        }
    }

    visitor.indentation_level -=1;
    visitor.new_line_and_indent();
    visitor.write('}');
    visitor.write(';');
}

void contained_func_decl(ToCAstVisitor& visitor, FunctionDeclaration* decl, bool overrides, ExtendableMembersContainerNode* def);

static void contained_interface_functions(ToCAstVisitor& visitor, InterfaceDefinition* def) {
    for(auto& func : def->functions()) {
        if(func->is_generic()) {
            visitor.write("[GENERIC_FUNCTIONS inside a interface not supported YET]");
        } else {
            // active iteration is probably going to zero here since function is non generic
            // it get's the parent iteration and figures out which functions generic calls
            // are calling inside the body of the function and activates it
            func->activate_gen_call_iterations(func->active_iteration);
            const auto interface = def->get_overriding_interface(func);
            contained_func_decl(visitor, func, interface != nullptr, def);
        }
    }
}

void gen_generic_interface_functions(ToCAstVisitor& visitor, InterfaceDefinition* def) {
    const auto total_itr = def->total_generic_iterations();
    if(total_itr == 0) return; // generic type never used (yet)
    int16_t itr = def->iterations_body_done;
    if(itr >= total_itr) return;
    auto prev = def->active_iteration;
    while (itr < total_itr) {
        def->set_active_iteration(itr);
        contained_interface_functions(visitor, def);
        itr++;
    }
    def->iterations_body_done = total_itr;
    def->set_active_iteration(prev);
}

void CTopLevelDeclarationVisitor::declare_interface(InterfaceDefinition* def) {
    const auto is_static = def->is_static();
    const auto is_generic = def->is_generic();
    for (auto& func: def->functions()) {
        if(is_static || !func->has_self_param()) {
            declare_contained_func(this, func, false);
        }
    }
    if(!is_static) {
        for (auto& use: def->users) {
            def->active_user = use.first;
            for (auto& func: def->functions()) {
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
                create_v_table(visitor, def, linked_struct);
            }
        }
    }
}

void CTopLevelDeclarationVisitor::declare_interface_iterations(InterfaceDefinition* def) {
    if(def->generic_params.empty()) {
        if(redefining || def->iterations_declared == 0) {
            declare_interface(def);
        }
        def->iterations_declared = 1;
    } else {
        // when redefining (struct imported from other module), we declare all iterations, otherwise begin where left off
        int16_t itr = redefining ? (int16_t) 0 : def->iterations_declared;
        const auto total = def->total_generic_iterations();
        while(itr < total) {
            def->set_active_iteration(itr);
            // early declare structs that are generic arguments
            early_declare_gen_arg_structs(*this, def->generic_params);
            declare_interface(def);
            itr++;
        }
        def->iterations_declared = total;
        def->set_active_iteration(-1);
        // generate any remaining functions that haven't been generated
        if(redefining) {
            gen_generic_interface_functions(visitor, def);
        }
    }
}

void CTopLevelDeclarationVisitor::visit(InterfaceDefinition *def) {
    // forward declaring the structs of users, because currently we only need to use them
    // as pointers, even if user returns a struct, the function only takes a pointer (to memcpy)
    for(auto& use : def->users) {
        new_line_and_indent();
        write("struct ");
        node_name(visitor, use.first);
        write(';');
    }
    declare_interface_iterations(def);
}

void CTopLevelDeclarationVisitor::visit(ImplDefinition *def) {

}

void CTopLevelDeclarationVisitor::reset() {
//    declared_nodes.clear();
}

void CValueDeclarationVisitor::visit(TypealiasStatement *stmt) {
    if(is_top_level_node) return;
    visitor.new_line_and_indent();
    write("typedef ");
    visitor.visit(stmt->actual_type);
    write(' ');
    std::string alias = "__chalias_";
    alias += std::to_string(random(100,999)) + "_";
    alias += std::to_string(alias_num++);
    visitor.write_str(alias);
    aliases[stmt] = alias;
    write(';');
}

void CValueDeclarationVisitor::visit(FunctionType *type) {
    // TODO remove this method
}

void CValueDeclarationVisitor::visit(StructMember *member) {
    if(member->type->kind() != BaseTypeKind::Function) {
        CommonVisitor::visit(member);
    }
}

void CValueDeclarationVisitor::visit(IfStatement *stmt) {
    if(stmt->computed_scope.has_value()) {
        auto scope = stmt->computed_scope.value();
        if(scope) {
            scope->accept(this);
        }
        return;
    }
    CommonVisitor::visit(stmt);
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
}

void ToCAstVisitor::reset() {
    local_allocated.clear();
    evaluated_func_calls.clear();
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
    if(init->is_top_level()) return;
    var_init(*this, init, false);
    init->accept(destructor.get());
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
            write_assignable(*this, node->parent_node);
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

void ToCAstVisitor::VisitCommentStmt(Comment *comment) {
    // leave comments alone
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
        if(pass_structs_to_initialize) {
            return false;
        } else {
            return true;
        }
    } else if(val->create_type(allocator)->isStructLikeType()) {
        return false;
    } else {
        return true;
    }
}

void ToCAstVisitor::return_value(Value* val, BaseType* type) {
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
    } else if(val_type->isStructLikeType()) {
//        auto refType = val->create_type();
//        auto structType = refType->linked_node()->as_struct_def();
//        auto size = structType->variables.size();
        write('*');
        write(struct_passed_param_name);
        write(" = ");
        if(!implicit_mutate_value_default(*this, type, val)) {
            if(is_value_passed_pointer_like(val)) {
                write('*');
            }
            visit(val);
        }
    } else {
        accept_mutating_value_explicit(type, val, false);
    }
}

void ToCAstVisitor::VisitReturnStmt(ReturnStatement *returnStatement) {
    const auto val = returnStatement->value;
    const auto return_type = returnStatement->func_type->returnType;
    std::string saved_into_temp_var;
    const auto has_struct_like_return = return_type->isStructLikeType();
    if(val && has_struct_like_return) {
        return_value(val, return_type);
        write(';');
        new_line_and_indent();
    } else if(val && BaseType::isPrimitiveType(return_type->pure_type(allocator)->kind()) && !destructor->destruct_jobs.empty()) {
        saved_into_temp_var = "__chx_ret_val_res";
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
    auto current_return = returnStatement->value ? returnStatement->value : nullptr;
    while(i >= 0) {
        destructor->destruct(destructor->destruct_jobs[i], current_return);
        i--;
    }
    destructor->new_line_before = new_line_prev;
    destructor->destroy_current_scope = false;
    if(returnStatement->value) {
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

void func_decl_with_name(ToCAstVisitor& visitor, FunctionDeclaration* decl, const chem::string_view& name) {
    if(!decl->body.has_value() || decl->is_comptime()) {
        return;
    }
    auto prev_func_decl = visitor.current_func_type;
    visitor.current_func_type = decl;
    visitor.new_line_and_indent();
    const auto decl_ret_func = decl->returnType->as_function_type();
    if(decl_ret_func && !decl_ret_func->isCapturing()) {
        func_that_returns_func_proto(visitor, decl, name, decl_ret_func);
    } else {
        declare_func_with_return(visitor, decl, name);
    }
    // before generating function's body, it's very important we clear the cached comptime calls
    // because multiple generic functions must re-evaluate the comptime function call
    visitor.evaluated_func_calls.clear();
    scope(visitor, decl->body.value(), decl);
    visitor.current_func_type = prev_func_decl;
}

void func_decl_with_name(ToCAstVisitor& visitor, FunctionDeclaration* decl) {
    if(!decl->generic_params.empty()) {
        gen_generic_functions(visitor, decl);
    } else {
        func_decl_with_name(visitor, decl, decl->name_view());
    }
}

void call_variant_param_func(
    ToCAstVisitor& visitor,
    VariantMember* member,
    VariantMemberParam* mem_param,
    FunctionDeclaration*(*choose_func)(MembersContainer*)
) {
    auto mem_type = mem_param->type;
    const auto linked = mem_type->linked_node();
    auto mem_def = linked->as_members_container();
    auto func = choose_func(mem_def);
    if (!func) {
        return;
    }
    visitor.new_line_and_indent();
    func_container_name(visitor, mem_def, func);
    visitor.write(func->name_view());
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

void call_variant_member_clear_fn(ToCAstVisitor& visitor, VariantMember* member) {
    call_variant_member_fn(visitor, member, [](MembersContainer* container)-> FunctionDeclaration* {
        return container->clear_func();
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
    func_container_name(visitor, mem_def, func);
    visitor.write(func->name_view());
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
        const auto prev_itr = mem_def->set_active_itr_ret_prev(mem_type->get_generic_iteration());
        variant_member_process_fn(visitor, member, func, mem_def, mem_param);
        mem_def->set_active_iteration(prev_itr);
    }
}

void call_variant_member_move_fn(ToCAstVisitor& visitor, VariantMember* member) {
    process_variant_member_using(visitor, member, [](MembersContainer* container) -> FunctionDeclaration* {
        return container->pre_move_func();
    },variant_member_pre_move_fn_gen);
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
    for (auto& var: def->variables) {
        visitor.new_line_and_indent();
        visitor.write("case ");
        *visitor.output << index;
        visitor.write(':');
        const auto member = var.second->as_variant_member();
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

void call_struct_member_fn(
        ToCAstVisitor& visitor,
        BaseType* mem_type,
        const chem::string_view& member_name,
        FunctionDeclaration*(*choose_func)(MembersContainer* container)
) {
    const auto linked = mem_type->linked_node();
    auto mem_def = linked->as_members_container();
    if(!mem_def) {
        return;
    }
    auto func = choose_func(mem_def);
    if (!func) {
        return;
    }
    visitor.new_line_and_indent();
    func_container_name(visitor, mem_def, func);
    visitor.write(func->name_view());
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

void call_struct_member_clear_fn(ToCAstVisitor& visitor, BaseType* mem_type, const chem::string_view& mem_name) {
    if (mem_type->isStructLikeType()) {
        call_struct_member_fn(visitor, mem_type, mem_name, [](MembersContainer* def) -> FunctionDeclaration* {
            return def->clear_func();
        });
    }
}

void call_struct_members_pre_move_fn(
        ToCAstVisitor& visitor,
        MembersContainer* mem_def,
        FunctionDeclaration* func,
        const chem::string_view& member_name
) {
    visitor.new_line_and_indent();
    func_container_name(visitor, mem_def, func);
    visitor.write(func->name_view());
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

void call_struct_member_move_fn(
        ToCAstVisitor& visitor,
        BaseType* mem_type,
        const chem::string_view& member_name
) {
    if (mem_type->isStructLikeType()) {
        const auto linked = mem_type->linked_node();
        auto mem_def = linked->as_members_container();
        auto func = mem_def->pre_move_func();
        if (!func) {
            return;
        }
        call_struct_members_pre_move_fn(visitor, mem_def, func, member_name);
    }
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
            const auto prev_itr = inherits.type->set_generic_iteration(inherits.type->get_generic_iteration());
            process_member(visitor, inherits.type, linked->name_view());
            inherits.type->set_generic_iteration(prev_itr);
        }
    }
    for (auto& var: def->variables) {
        auto value_type = var.second->create_value_type(visitor.allocator);
        const auto prev_itr = value_type->set_generic_iteration(value_type->get_generic_iteration());
        process_member(visitor, value_type, var.second->name);
        value_type->set_generic_iteration(prev_itr);
    }
}

void initialize_def_struct_values_constructor(ToCAstVisitor& visitor, FunctionDeclaration* decl) {
    auto parent = decl->parent_node;
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
                defConstructor->runtime_name(*visitor.output);
                visitor.write("(&this->");
                visitor.write(def->name_view());
                visitor.write(");");
            }
        }
    }
    for(auto& var : struct_def->variables) {
        const auto defValue = var.second->default_value();
        auto has_initializer = initializers && initializers->find(var.first) != initializers->end();
        // TODO currently we check if the value has a initializer in the init block
        // TODO we should check whether the value has an assignment in the function as well (or use that to initialize it)
        if(has_initializer) {
            continue;
        }
        if(!defValue) {
            // since default value doesn't exist, however the variable maybe of type struct and have a default constructor
            // we must call the default non argument constructor automatically
            const auto mem_type = var.second->create_value_type(visitor.allocator);
            const auto mem_pure = mem_type->pure_type(visitor.allocator);
            const auto def = mem_pure->get_direct_linked_struct();
            if(def) {
                const auto defConstructor = def->default_constructor_func();
                if(defConstructor) {
                    visitor.new_line_and_indent();
                    defConstructor->runtime_name(*visitor.output);
                    visitor.write("(&this->");
                    visitor.write(var.first);
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
        visitor.write(var.first);
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
        struct_name(visitor, def);
        visitor.write('*');
        visitor.space();
        visitor.write("self = ");
        visitor.write(static_interface_passed_param_name);
        visitor.write(';');
    }
    const auto is_destructor = decl->is_delete_fn();
    const auto is_clear_fn = decl->is_clear_fn();
    const auto is_copy_fn = decl->is_copy_fn();
    const auto is_move_fn = decl->is_move_fn();
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
    } else if(is_clear_fn) {
        if(struc_def) {
            process_struct_members_using(visitor, def, call_struct_member_clear_fn);
        } else if(variant_def) {
            process_variant_members_using(visitor, variant_def, call_variant_member_clear_fn);
        }
    } else if(is_move_fn) {
        if(struc_def) {
            process_struct_members_using(visitor, def, call_struct_member_move_fn);
        } else if(variant_def) {
            write_type_assignment_in_variant_copy(visitor, decl);
            process_variant_members_using(visitor, variant_def, call_variant_member_move_fn);
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

void ToCAstVisitor::VisitExtensionFunctionDecl(ExtensionFunction *decl) {
    func_decl_with_name(*this, decl);
}

void ToCAstVisitor::VisitIfStmt(IfStatement *decl) {
    // generating code for compile time if statements
    if(decl->computed_scope.has_value()) {
        auto scope = decl->computed_scope.value();
        if(scope) {
            visit(scope);
        }
        return;
    }
    // generating code for normal if statements
    write("if(");
    nested_value = true;
    visit(decl->condition);
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
    for(auto& func : def->functions()) {
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
        node->accept(before_stmt.get());
        visit(node);
        node->accept(after_stmt.get());
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
    for(auto& var : def->variables) {
        new_line_and_indent();
        visit(var.second);
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
    for(auto& var : def->variables) {
        new_line_and_indent();
        visit(var.second);
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
    space();
    write(def->name);
    write(';');
}

static void contained_struct_functions(ToCAstVisitor& visitor, StructDefinition* def) {
    for(auto& func : def->functions()) {
        if(func->is_generic()) {
            visitor.write("[GENERIC_FUNCTIONS inside a struct not supported YET]");
        } else {
            // active iteration is probably going to zero here since function is non generic
            // it get's the parent iteration and figures out which functions generic calls
            // are calling inside the body of the function and activates it
            func->activate_gen_call_iterations(func->active_iteration);
            const auto overriding = def->get_func_overriding_info(func);
            if(overriding.type) {
                const auto type = overriding.type->type;
                const auto prev_gen_itr = overriding.base_container->set_active_itr_ret_prev(type->get_generic_iteration());
                contained_func_decl(visitor, func, overriding.base_func != nullptr, def);
                overriding.base_container->set_active_iteration(prev_gen_itr);
            } else {
                contained_func_decl(visitor, func, overriding.base_func != nullptr, def);
            }
        }
    }
}

void ToCAstVisitor::VisitStructDecl(StructDefinition *def) {
    auto prev_members_container = current_members_container;
    current_members_container = def;
    for (auto& inherits: def->inherited) {
        const auto overridden = inherits.type->linked_node()->as_interface_def();
        if (overridden) {
            for (auto& func: overridden->functions()) {
                if (!def->contains_func(func->name_view())) {
                    contained_func_decl(*this, func, false, def);
                }
            }
        }
    }
    if(def->generic_params.empty()) {
        if(def->iterations_body_done == 0) {
            contained_struct_functions(*this, def);
            def->iterations_body_done = 1;
        }
    } else {
        gen_generic_struct_functions(*this, def);
    }
    current_members_container = prev_members_container;
}

void generate_contained_functions(ToCAstVisitor& visitor, VariantDefinition* def) {
    for(auto& func : def->functions()) {
        contained_func_decl(visitor, func, false, def);
    }
}

void ToCAstVisitor::VisitVariantDecl(VariantDefinition* def) {
    if(def->generic_params.empty()) {
        if(def->iterations_body_done == 0) {
            generate_contained_functions(*this, def);
            def->iterations_body_done = 1;
        }
    } else {
        gen_variant_functions(*this, def);
    }

}

void ToCAstVisitor::VisitUnionDecl(UnionDef *def) {
    for(auto& func : def->functions()) {
        contained_func_decl(*this, func, false, def);
    }
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
        destructor->destruct_arr_ptr(chem::string_view(self_name.data(), self_name.size()), data.array_size != 0 ? &siz_val : stmt->array_value, data.parent_node, 0, data.destructor_func);
    } else {
        destructor->destruct(chem::string_view(self_name.data(), self_name.size()), data.parent_node, 0, data.destructor_func, true);
    }
}

void ToCAstVisitor::VisitIsValue(IsValue *isValue) {
    bool result = false;
    auto comp_time = isValue->get_comp_time_result();
    if(comp_time.has_value()) {
        result = comp_time.value();
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
    const auto member = variant_case->parent_val->linked_node()->as_variant_member();
    *output << member->parent_node->direct_child_index(member->name);
}

void ToCAstVisitor::VisitVariantCall(VariantCall *call) {
    auto found = local_allocated.find(call);
    if(found == local_allocated.end()) {
        write("[VariantCallNotAllocated]");
    } else {
        write(found->second);
    }
}

inline bool isLoadableReference(BaseType* type) {
    return type->getLoadableReferredType() != nullptr;
}

void ToCAstVisitor::VisitIncDecValue(IncDecValue *value) {
    if(!value->post) {
        write(value->increment ? "++" : "--");
    }
    const auto type = value->value->create_type(allocator);
    if(type && isLoadableReference(type)) {
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
    const auto parent = func_node->parent();
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
                node_parent_name(visitor, struct_parent);
                struct_name(visitor, struct_parent);
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
                node_parent_name(visitor, struct_parent);
                struct_name(visitor, struct_parent);
                func_name(visitor, func_node);
                return;
            }
        }
    }
    node_parent_name(visitor, func_node);
    func_name(visitor, func_node);
}

// the parent_node is the parent of the function node
// linked_node is the actual function node
void func_container_name(ToCAstVisitor& visitor, ASTNode* parent_node, ASTNode* linked_node) {
    node_parent_name(visitor, parent_node);
    if(linked_node && linked_node->as_extension_func()) {
        return;
    }
    if(!parent_node) return;
    const auto func_parent = linked_node->parent();
    const auto impl_def = func_parent->as_impl_def();
    if(impl_def) {
        if(impl_def->struct_type) {
            visitor.write(impl_def->struct_type->linked_node()->as_struct_def()->name_view());
        } else {
            visitor.write(impl_def->interface_type->linked_node()->as_interface_def()->name_view());
        }
    } else if(parent_node->as_interface_def()) {
//        const auto func_node = linked_node->as_function();
        visitor.write(parent_node->as_interface_def()->name_view());
    } else if(parent_node->as_variant_def()) {
        struct_name(visitor, parent_node->as_variant_def());
    } else if(parent_node->as_struct_def()) {
        const auto info = parent_node->as_struct_def()->get_overriding_info(linked_node->as_function());
        if (info.first) {
            const auto func_node = linked_node->as_function();
            if(info.first->as_interface_def() && func_node && func_node->has_self_param()) {
                struct_name(visitor, parent_node->as_struct_def());
            } else {
                node_name(visitor, info.first);
            }
        } else {
            struct_name(visitor, parent_node->as_struct_def());
        }
    }
}

//class FuncGenericItrResetter {
//public:
//    FunctionDeclaration* decl;
//    int16_t iteration;
//    ~FuncGenericItrResetter() {
//        if(decl) {
//            decl->set_active_iteration(iteration);
//        }
//    }
//};

//void func_call(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned start, unsigned end) {
//    auto last = values[end - 1]->as_func_call();
//    auto func_decl = last->safe_linked_func();
//    auto parent = values[end - 2];
//    if(func_decl && func_decl->is_comptime()) {
//        evaluate_func(visitor, &visitor, func_decl, last);
//        return;
//    }
//    FuncGenericItrResetter resetter{};
//    if(func_decl && !func_decl->generic_params.empty()) {
//        resetter.decl = func_decl;
//        resetter.iteration = func_decl->active_iteration;
//        func_decl->set_active_iteration(last->generic_iteration);
//    }
//    auto grandpa = ((int) end) - 3 >= 0 ? values[end - 3] : nullptr;
//    auto parent_type = parent->create_type(visitor.allocator);
//    auto func_type = parent_type->function_type();
//    bool is_lambda = (parent->linked_node() != nullptr && parent->linked_node()->as_struct_member() != nullptr);
//    if(visitor.pass_structs_to_initialize && func_type->returnType->value_type() == ValueType::Struct) {
//        // functions that return struct
//        auto allocated = visitor.local_allocated.find(last);
//        if(allocated == visitor.local_allocated.end()) {
//            visitor.write("[NotFoundAllocated in func_call]");
//            return;
//        }
//        visitor.write(allocated->second);
//    } else if(func_type->isCapturing()) {
//        // function calls to capturing lambdas
//        capture_call(visitor, func_type, last, [&](){
//            auto prev = visitor.nested_value;
//            visitor.nested_value = true;
//            access_chain(visitor, values, start, end - 1);
//            visitor.nested_value = prev;
//        }, [&]() {
//            auto prev = visitor.nested_value;
//            visitor.nested_value = true;
//            if(!values[end - 2]->is_stored_ptr_or_ref(visitor.allocator)) {
//                visitor.write('&');
//            }
//            access_chain(visitor, values, start, end - 2);
//            visitor.nested_value = prev;
//        });
//    } else if(grandpa && !grandpa->linked_node()->as_namespace()) {
//        auto grandpaType = grandpa->create_type(visitor.allocator);
//        auto pure_grandpa = grandpaType->pure_type();
//        if(pure_grandpa && pure_grandpa->kind() == BaseTypeKind::Dynamic) {
//            const auto interface = pure_grandpa->linked_node()->as_interface_def();
//            if(interface && !interface->users.empty()) {
//                // Dynamic dispatch
//                // ((typeof(PhoneSmartPhone)*) phone.second)->call(phone.first);
//                auto first = interface->users.begin();
//                auto first_def = first->first;
//                visitor.write('(');
//                visitor.write('(');
//                visitor.write("typeof");
//                visitor.write('(');
//                vtable_name(visitor, interface, first_def);
//                visitor.write(')');
//                visitor.write('*');
//                visitor.write(')');
//                visitor.space();
//                func_name_chain(visitor, values, start, end - 2);
//                if(end - 2 - start == 1) {
//                    visitor.write('.');
//                }
//                visitor.write("second");
//                visitor.write(')');
//                visitor.write("->");
//                func_name(visitor, parent, func_decl);
//                visitor.write('(');
//                if(write_self_arg_bool(visitor, func_type, values, (((int) end) - 3), last, true)) {
//                    visitor.write('.');
//                    visitor.write("first");
//                    if (!last->values.empty()) {
//                        visitor.write(", ");
//                    }
//                }
//                func_call_args(visitor, last, func_type);
//                visitor.write(')');
//            } else {
//                if(!interface) {
//                    visitor.write("[Dynamic Dispatch used with type other than interface]");
//                    visitor.error("Dynamic Dispatch used with a type other than interface", pure_grandpa->linked_node());
//                } else {
//                    visitor.write("[Dynamic Dispatch Interface has no known users]");
//                }
//            }
//        } else if(grandpa->linked_node() && (grandpa->linked_node()->as_interface_def() || grandpa->linked_node()->as_struct_def())) {
//            // direct functions on interfaces and structs
//            func_container_name(visitor, grandpa->linked_node(), parent->linked_node());
//            func_name(visitor, parent, func_decl);
//            visitor.write('(');
//            if(func_decl->has_self_param() && grandpa->as_func_call()) {
//                auto found = visitor.local_allocated.find(grandpa);
//                if(found != visitor.local_allocated.end()) {
//                    visitor.write('&');
//                    visitor.write(found->second);
//                    if(!last->values.empty()) {
//                        visitor.write(", ");
//                    }
//                }
//            }
//            func_call_args(visitor, last, func_type);
//            visitor.write(')');
//        } else if(pure_grandpa->linked_struct_def()) {
//            if(parent->linked_node()->as_struct_member()) {
//                goto normal_functions;
//            }
//            auto generic_struct = grandpaType->get_generic_struct();
//            int16_t prev_iteration;
//            if(generic_struct) {
//                prev_iteration = generic_struct->active_iteration;
//                generic_struct->set_active_iteration(grandpaType->get_generic_iteration());
//            }
//            // functions on struct values
//            func_container_name(visitor, pure_grandpa->linked_node(), parent->linked_node());
//            func_name(visitor, parent, func_decl);
//            visitor.write('(');
//            write_implicit_args(visitor, func_type, values, end, last);
//            func_call_args(visitor, last, func_type);
//            visitor.write(')');
//            if(generic_struct) {
//                generic_struct->set_active_iteration(prev_iteration);
//            }
//        } else {
//            goto normal_functions;
//        }
//    } else {
//        goto normal_functions;
//    }
//    return;
//    normal_functions: {
//        // normal functions
//        auto linked_node = parent->linked_node();
//        auto as_func_decl = linked_node ? linked_node->as_function() : nullptr;
//        if(func_decl) {
//            func_container_name(visitor, func_decl);
//        } else {
//            func_name_chain(visitor, values, start, end - 1);
//        }
//        visitor.write('(');
//        write_implicit_args(visitor, func_type, values, end, last);
//        func_call_args(visitor, last, func_type);
//        visitor.write(')');
////        if(!visitor->nested_value) {
////            visitor->write(';');
////        }
//    };
//}

//void func_name_chain(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned start, unsigned end) {
//    access_chain(visitor, values, start, end, values.size());
//}

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
//                        const auto def = parent->create_value_type(visitor.allocator)->get_direct_linked_struct();
//                        if(def) {
//                            write_path_to_member(visitor, def, member);
//                        }
//                    }
            }
        }
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
    if(visitor.inline_enum_member_access) {
        if(member->init_value) {
            visitor.visit(member->init_value);
        } else {
            *visitor.output << member->get_default_index();
        }
    } else {
        auto found = visitor.declarer->aliases.find(member);
        if (found != visitor.declarer->aliases.end()) {
            visitor.write(found->second);
        } else {
            visitor.error("couldn't write the enum", member);
        }
    }
}

void access_chain(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, const unsigned start, const unsigned end);

// this function is called, with start index to the chain value which is definitely a function call
// we check if it's a function call to a struct which has a destructor
// if it does have a destructor, we store the accessed value and destruct the struct afterwards
bool write_destructible_call_chain_values(ToCAstVisitor& visitor, std::vector<ChainValue*>& values, unsigned int start, unsigned int end) {
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
                destructorFn->runtime_name(*visitor.output);
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
    const auto linked = last->linked_node();
    if(linked) {
        const auto lastKind = linked->kind();
        if(lastKind == ASTNodeKind::FunctionDecl || lastKind == ASTNodeKind::ExtensionFunctionDecl) {
            if(!linked->as_function_unsafe()->has_self_param()) {
                // TODO calling functions above without destructing the structs
                call_any_function_above(visitor, values, (int) start, (int) end - 1);
            }
            linked->runtime_name(*visitor.output);
            return;
        } else if(lastKind == ASTNodeKind::EnumMember) {
            write_enum(visitor, linked->as_enum_member_unsafe());
            return;
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
    std::vector<int16_t> active;
    chain->set_generic_iteration(active, allocator);
    access_chain(*this, chain->values, 0, size);
    chain->restore_generic_iteration(active, allocator);
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
    const auto func_type = call->function_type(allocator);

    // handling comptime functions
    if(func_decl && func_decl->is_comptime()) {
        call->set_curr_itr_on_decl();
        const auto value = evaluated_func_val(*this, func_decl, call);
        visit(value);
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
            auto pure_grandpa = grandpaType->pure_type();
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
                        error("Dynamic Dispatch used with a type other than interface",
                              pure_grandpa->linked_node());
                    } else {
                        write("[Dynamic Dispatch Interface has no known users]");
                    }
                }
                return;
            }
        }
    }

    // functions that return struct are handled in this block of code
    if(!func_decl || !func_decl->is_comptime()) {
        const auto returnType = func_type->returnType->pure_type();
        const auto returnTypeKind = returnType->kind();
        if (returnTypeKind == BaseTypeKind::Dynamic) {
            call->set_curr_itr_on_decl();
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
        } else {
            const auto return_linked = returnType->get_direct_linked_node();
            if (return_linked) {
                const auto returnKind = return_linked->kind();
                if (returnKind == ASTNodeKind::StructDecl || returnKind == ASTNodeKind::VariantDecl || returnKind == ASTNodeKind::UnionDecl) {
                    call->set_curr_itr_on_decl();
                    write("(*({ ");
                    const auto temp_name = get_local_temp_var_name();
                    const auto temp_name_view = chem::string_view(temp_name.data(), temp_name.size());
                    allocate_struct_by_name_no_init(*this, return_linked, temp_name_view);
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
                }
            }
        }
    }

    // normal functions
    call->set_curr_itr_on_decl();
    visit(call->parent_val);
    write('(');
    write_implicit_args(*this, func_type, call);
    func_call_args(*this, call, func_type);
    write(')');

//load_from_allocated:
//    auto found = local_allocated.find(call);
//    if(found != local_allocated.end()) {
//        write(found->second);
//        if(!nested_value) {
//            write(';');
//        }
//    } else {
//        write("[FunctionAllocationFailure: function that returns struct, hasn't been allocated in the before stmt visitor]");
//    }

}

void ToCAstVisitor::VisitInitBlock(InitBlock *initBlock) {
    auto& container = initBlock->container;
    auto& initializers = initBlock->initializers;
    auto is_union = container->kind() == ASTNodeKind::UnionDecl;
    for(auto& init : initializers) {
        auto value = init.second.value;
        auto variable = container->variable_type_index(init.first, init.second.is_inherited_type);
        if(init.second.is_inherited_type) {
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
            chain->accept(before_stmt.get());
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
    block->body.interpret(comptime_scope);
}

void ToCAstVisitor::VisitTypealiasStmt(TypealiasStatement *stmt) {
    // declared above
}

void write_variant_call_id_index(ToCAstVisitor& visitor, VariantDefinition* variant, VariableIdentifier* value) {
    const auto member = value->linked->as_variant_member();
    if(member) {
        *visitor.output << variant->variable_index(member->name, false);
    } else {
        visitor.write("-1");
    }
}

void write_variant_call_call_index(ToCAstVisitor& visitor, VariantDefinition* variant, FunctionCall* value) {
    const auto member = value->parent_val->linked_node()->as_variant_member();
    if(member) {
        *visitor.output << variant->variable_index(member->name, false);
    } else {
        visitor.write("-1");
    }
}

void write_variant_call_index(ToCAstVisitor& visitor, VariantDefinition* variant, Value* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            write_variant_call_id_index(visitor, variant, value->as_identifier_unsafe());
            return;
        case ValueKind::FunctionCall:
            write_variant_call_call_index(visitor, variant, value->as_func_call_unsafe());
            return;
        case ValueKind::AccessChain: {
            const auto chain = value->as_access_chain_unsafe();
            if(chain) {
                write_variant_call_index(visitor, variant, chain->values.back());
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
    if(isLoadableReference(type)) {
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
                variant = linked->as_variant_member_unsafe()->parent_node;
            }
            if (variant) {
                // turn on the active iteration of the variant
                variant->set_active_iteration(known_t->get_generic_iteration());
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

                if(variant) {
                    write_variant_call_index(*this, variant, switch_case.first);
                } else {
                    visit(switch_case.first);
                }

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
        accept_mutating_value(elem_type, arr->values[i], false);
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
    int16_t prev_itr;
    bool is_generic = val->is_generic();
    if(is_generic) {
        prev_itr = val->get_active_iteration();
        val->set_active_iteration(val->generic_iteration);
    }
    const auto runName = val->runtime_name_str();
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
    for(auto& value : val->values) {
        if(has_value_before) {
            write(", ");
        } else {
            has_value_before = true;
        }
//        if(value.second->as_access_chain()) {
//            auto chain = value.second->as_access_chain();
//            auto call = chain->values.back()->as_func_call();
//            if(call && call->create_type()->value_type() == ValueType::Struct) {
//                continue;
//            }
//        }
        // we are only getting direct / inherited members, not inherited structs here
        const auto member = val->child_member(value.first);
        write('.');
        write(value.first);
        write(" = ");
        accept_mutating_value(member ? member->known_type() : nullptr, value.second.value, false);
    }
    auto& variables = val->variables()->variables;
    for(auto& var : variables) {
        auto found = val->values.find(var.first);
        if(found == val->values.end()) {
            auto defValue = var.second->default_value();
            const auto member = val->child_member(var.first);
            if(has_value_before) {
                write(", ");
            } else {
                has_value_before = true;
            }
            if(defValue) {
                write('.');
                write(var.first);
                write(" = ");
                accept_mutating_value(member ? member->known_type() : nullptr, defValue, false);
            } else if(!val->is_union()) {
                error("no default value present for '" + var.first.str() + "' in struct value", val);
            }
        }
    }
    nested_value = prev;
    write('}');
    if(is_generic) {
        val->set_active_iteration(prev_itr);
    }
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

void ToCAstVisitor::VisitVariableIdentifier(VariableIdentifier *identifier) {
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
        if(identifier->parent_val == nullptr) {
            const auto func = current_func_type->as_function();
            if (func && func->is_constructor_fn()) {
                write("this->");
            }
            else if(func->parent_node) {
                auto self_param = func->get_self_param();
                if(self_param && ASTNode::isMembersContainer(func->parent_node->kind())) {
                    write(self_param->name);
                    write("->");
                }
                auto ext_node = func->parent_node->as_extendable_members_container_node();
                if(ext_node) {
                    write_path_to_member(*this, ext_node, linked->as_base_def_member());
                }
            }
        }
    } else {
        switch(linked_kind) {
            case ASTNodeKind::FunctionDecl:
                linked->as_function_unsafe()->runtime_name(*output);
                return;
            case ASTNodeKind::VarInitStmt: {
                const auto init = linked->as_var_init_unsafe();
                if (init->is_comptime()) {
                    visit(init->value);
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
                Value* expr = var->switch_statement->expression;
                const auto var_mem = var->parent_val->linked_node()->as_variant_member();
                visit(expr);
                write_accessor(*this, expr, identifier);
                write(var_mem->name);
                write('.');
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
//    else if(linked_kind == ASTNodeKind::FunctionParam || linked_kind == ASTNodeKind::ExtensionFuncReceiver) {
//        auto& type = *linked->as_func_param_unsafe()->type;
//        const auto type_kind = type.kind();
//        if(type_kind == BaseTypeKind::Reference) {
//            // const auto d_linked = ((ReferenceType&) type).type->get_direct_linked_node();
////            if(should_deref_node(d_linked)) {
//                deref_id(*this, identifier);
//                return;
////            }
//        } else {
//            const auto d_linked = type.get_direct_linked_node(type_kind);
//            if(write_id_accessor(*this, identifier, d_linked)) {
//                return;
//            }
//        }
//    }
    write(identifier->value);
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
    visit(align_of->for_type->removeReferenceFromType(allocator));
    write(')');
}

void ToCAstVisitor::VisitExpression(Expression *expr) {
    write('(');
    auto prev_nested = nested_value;
    nested_value = true;

    // automatic dereferencing the first value
    const auto first_type = expr->firstValue->create_type(allocator);
    const auto first_pure = first_type->pure_type();
    if(isLoadableReference(first_pure)) {
        write('*');
    }

    visit(expr->firstValue);

    space();
    write_str(to_string(expr->operation));
    space();

    // automatic dereferencing the second value
    const auto second_type = expr->secondValue->create_type(allocator);
    const auto second_pure = second_type->pure_type();
    if(isLoadableReference(second_pure)) {
        write('*');
    }

    visit(expr->secondValue);

    nested_value = prev_nested;
    write(')');
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
    if(!is_value_passed_pointer_like(value)) {
        write('&');
    }
    visit(value->value);
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
    write(")}");
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

void ToCAstVisitor::VisitValueNode(ValueNode *node) {
    auto val_kind = node->value->val_kind();
    if(val_kind != ValueKind::SwitchValue && val_kind != ValueKind::IfValue && val_kind != ValueKind::LoopValue) {
        write_assignable(*this, node->parent_node);
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
    visit(node->value);
    write(';');
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
                write("(&(struct ");
                write(found->second);
                write("_cap");
                write(')');
                write('{');
                unsigned i = 0;
                while(i < func->captureList.size()) {
                    auto& cap = func->captureList[i];
                    if(cap->capture_by_ref) {
                        write('&');
                    }
                    write(cap->name);
                    if(i != func->captureList.size() - 1) {
                        write(',');
                    }
                    i++;
                }
                write('}');
                write(')');
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

void ToCAstVisitor::VisitGenericType(GenericType *gen_type) {
    const auto gen_struct = gen_type->referenced->linked->as_members_container();
    const auto prev_itr = gen_struct->active_iteration;
    gen_struct->set_active_iteration(gen_type->generic_iteration);
    visit(gen_type->referenced);
    gen_struct->set_active_iteration(prev_itr);
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
            if(is64Bit) {
                write("long long");
            } else {
                write("long");
            }
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
            write("void");
            return;
        case ASTNodeKind::EnumDecl:
            write("int");
            return;
        case ASTNodeKind::VariantMember:
            write("struct ");
            linked.as_variant_member_unsafe()->parent_node->runtime_name(*output);
            return;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
            write("struct ");
            linked.runtime_name(*output);
            return;
        case ASTNodeKind::UnionDecl:
            write("union ");
            linked.runtime_name(*output);
            return;
        case ASTNodeKind::GenericTypeParam:
            visit(linked.get_value_type(allocator));
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
    node_parent_name(*this, &linked);
    if(type->type.empty()) {
        type->linked->runtime_name_no_parent(*output);
    } else {
        write(type->type);
    }
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
    for(auto& variable : val->variables) {
        visit(variable.second);
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
    for(auto& variable : val->variables) {
        visit(variable.second);
        write(' ');
    }
    write("}");
}

void ToCAstVisitor::VisitVoidType(VoidType *func) {
    write("void");
}

void ToCBackendContext::mem_copy(Value *lhs, Value *rhs) {
    visitor->new_line_and_indent();
    visitor->visit(lhs);
    visitor->write(" = ");
    visitor->visit(rhs);
    visitor->write(';');
}