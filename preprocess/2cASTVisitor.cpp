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
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/MacroValueStatement.h"
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
#include "ast/structures/VariantMember.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/If.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/LoopScope.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/LoopBlock.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/PointerType.h"
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
#include "ast/types/ULongType.h"
#include "ast/types/UShortType.h"
#include "ast/types/VoidType.h"
#include "ast/values/UShortValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IntValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
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
#include "ast/values/TernaryValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/IsValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ULongValue.h"
#include "ast/utils/CommonVisitor.h"
#include "utils/RepresentationUtils.h"
#include "ast/utils/ASTUtils.h"
#include <sstream>

ToCAstVisitor::ToCAstVisitor(std::ostream *output) : output(output), ASTDiagnoser() {
    declarer = std::make_unique<CValueDeclarationVisitor>(this);
    tld = std::make_unique<CTopLevelDeclarationVisitor>(this, declarer.get());
    before_stmt = std::make_unique<CBeforeStmtVisitor>(this);
    after_stmt = std::make_unique<CAfterStmtVisitor>(this);
    destructor = std::make_unique<CDestructionVisitor>(this);
}

class ToCAstVisitor;

// will write a scope to visitor
void scope(ToCAstVisitor* visitor, Scope& scope) {
    visitor->write('{');
    visitor->indentation_level+=1;
    scope.accept(visitor);
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
}

void write_encoded(ToCAstVisitor* visitor, const std::string& value) {
    for(char c : value) {
        visitor->write(escape_encode(c));
    }
}

void write_type_post_id(ToCAstVisitor* visitor, BaseType* type) {
    if(type->kind() == BaseTypeKind::Array) {
        visitor->write('[');
        auto arrType = ((ArrayType*) type);
        if(arrType->array_size != -1) {
            visitor->write(std::to_string(arrType->array_size));
        }
        visitor->write(']');
        if(arrType->elem_type->kind() == BaseTypeKind::Array) {
            write_type_post_id(visitor, arrType->elem_type.get());
        }
    }
}

void assign_statement(ToCAstVisitor* visitor, AssignStatement* assign) {
    assign->lhs->accept(visitor);
    visitor->write(' ');
    if(assign->assOp != Operation::Assignment) {
        visitor->write(to_string(assign->assOp));
    }
    visitor->write('=');
    visitor->write(' ');
    visitor->accept_mutating_value(assign->lhs->known_type(), assign->value.get());
}

#define struct_passed_param_name "__chx_struct_ret_param_xx"
#define fn_call_struct_var_name "chx_fn_cl_struct"

static std::string struct_name_str(ToCAstVisitor* visitor, ExtendableMembersContainerNode* def) {
    if(def->generic_params.empty()) {
        return def->name;
    } else {
        return def->name + "__cgs__" + std::to_string(def->active_iteration);
    }
}

// without the parent node name
static void struct_name(ToCAstVisitor* visitor, ExtendableMembersContainerNode* def) {
    visitor->write(struct_name_str(visitor, def));
}

// nodes inside namespaces for example namespace name is written before their name
void node_name(ToCAstVisitor* visitor, ASTNode* node) {
    if(!node) return;
    std::string name;
    auto parent = node;
    while(parent) {
        if(parent->as_namespace()) {
            name = parent->as_namespace()->name + name;
        } else if(parent->as_struct_def()) {
            name = struct_name_str(visitor, parent->as_struct_def()) + name;
        } else if(parent->as_union_def()) {
            name = parent->as_union_def()->name + name;
        } else if(parent->as_interface_def()) {
            name = parent->as_interface_def()->name + name;
        } else {
            name = "[UNKNOWN_PARENT_NAME]" + name;
        }
        parent = parent->parent();
    }
    visitor->write(name);
}

// nodes inside namespaces for example namespace name is written before their name
void node_parent_name(ToCAstVisitor* visitor, ASTNode* node, bool take_parent = true) {
    if(!node) return;
    std::string name;
    auto parent = take_parent ? node->parent() : node;
    while(parent) {
        if(parent->as_namespace()) {
            name = parent->as_namespace()->name + name;
        } else if(parent->as_struct_def()) {
            name = struct_name_str(visitor, parent->as_struct_def()) + name;
        } else if(parent->as_union_def()) {
            name = parent->as_union_def()->name + name;
        } else if(parent->as_interface_def()) {
            name = parent->as_interface_def()->name + name;
        } else {
            name = "[UNKNOWN_PARENT_NAME]" + name;
        }
        parent = parent->parent();
    }
    visitor->write(name);
}

void func_type_with_id(ToCAstVisitor* visitor, FunctionType* type, const std::string& id);

void type_with_id(ToCAstVisitor* visitor, BaseType* type, const std::string& id) {
    if(visitor->inline_fn_types_in_params && type->function_type() != nullptr && !type->function_type()->is_capturing()) {
        func_type_with_id(visitor, type->function_type(), id);
    } else {
        type->accept(visitor);
        visitor->space();
        visitor->write(id);
        write_type_post_id(visitor, type);
    }
}

void param_type_with_id(ToCAstVisitor* visitor, BaseType* type, const std::string& id) {
    const auto node = type->get_direct_ref_node();
    if(node && (node->as_struct_def() || node->as_variant_def())) {
        PointerType ptr_type(hybrid_ptr<BaseType>{ type, false }, nullptr);
        type_with_id(visitor, &ptr_type, id);
    } else {
        type_with_id(visitor, type, id);
    }
}

void write_struct_return_param(ToCAstVisitor* visitor, FunctionType* decl) {
    decl->returnType->accept(visitor);
    visitor->write("* ");
    const auto func = decl->as_function();
    if(func && func->has_annotation(AnnotationKind::Constructor)) {
        visitor->write("this");
    } else {
        visitor->write(struct_passed_param_name);
    }
}

// don't know what to call '(' struct 'name' ')'
void write_struct_def_value_call(ToCAstVisitor* visitor, StructDefinition* def) {
    visitor->write('(');
    visitor->write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    visitor->write(')');
}

void extension_func_param(ToCAstVisitor* visitor, ExtensionFunction* extension) {
    extension->receiver.type->accept(visitor);
    visitor->space();
    visitor->write(extension->receiver.name);
}

void func_type_params(ToCAstVisitor* visitor, FunctionType* decl, unsigned i = 0, bool has_params_before = false) {
    auto is_struct_return = visitor->pass_structs_to_initialize && decl->returnType->value_type() == ValueType::Struct;
    auto extension = decl->as_extension_func();
    if(is_struct_return) {
        if(has_params_before) {
            visitor->write(", ");
        }
        write_struct_return_param(visitor, decl);
        has_params_before = true;
    }
    if(extension) {
        if(has_params_before) {
            visitor->write(", ");
        }
        extension_func_param(visitor, extension);
        has_params_before = true;
    }
    FunctionParam* param;
    auto size = decl->isVariadic ? decl->params.size() - 1 : decl->params.size();
    while(i < size) {
        param = decl->params[i].get();
        if(has_params_before) {
            visitor->write(", ");
        }
        param_type_with_id(visitor, param->type.get(), param->name);
        has_params_before = true;
        i++;
    }
    if(decl->isVariadic) {
        if(has_params_before) {
            visitor->write(", ");
        }
        visitor->write("...");
    }
}

void accept_func_return(ToCAstVisitor* visitor, BaseType* type) {
    if(visitor->pass_structs_to_initialize && type->value_type() == ValueType::Struct) {
        visitor->write("void");
    } else {
        type->accept(visitor);
    }
}

// func_type is declared with it's return type and name
// the last take_parent allows to skip appending one direct parent name, useful
// when the interface name is to be used, so interface appends the name in given name parameter
// take_parent is true, so this function skips direct parent but grandparents and other names are appended
void accept_func_return_with_name(ToCAstVisitor* visitor, FunctionType* func_type, const std::string& name, bool take_parent, bool is_static) {
    auto func_decl = func_type->as_function();
    if(func_decl && func_decl->has_annotation(AnnotationKind::Extern)) {
        visitor->write("extern ");
    }
    if(is_static) {
        visitor->write("static ");
    }
    accept_func_return(visitor, func_type->returnType.get());
    visitor->space();
    node_parent_name(visitor, take_parent ? func_type->parent() : func_type->as_function());
    visitor->write(name);
    if(func_decl) {
        if(func_decl->multi_func_index != 0) {
            visitor->write("__cmf_");
            visitor->write(std::to_string(func_decl->multi_func_index));
        }
        if(func_decl->active_iteration != 0) {
            visitor->write("__cgf_");
            visitor->write(std::to_string(func_decl->active_iteration));
        }
    }
}

void func_type_with_id_no_params(ToCAstVisitor* visitor, FunctionType* type, const std::string& id) {
    accept_func_return(visitor, type->returnType.get());
    visitor->write('(');
    visitor->write('*');
    visitor->write(id);
    visitor->write(")(");
    if(type->isCapturing) {
        visitor->write("void*");
        if(!type->params.empty()) {
            visitor->write(',');
        }
    }
//    if(type->params.empty()) {
//        if(!type->isCapturing) {
//            visitor->write("void");
//        }
//    }
}

void func_type_with_id(ToCAstVisitor* visitor, FunctionType* type, const std::string& id) {
    func_type_with_id_no_params(visitor, type, id);
    func_type_params(visitor, type);
    visitor->write(")");
}

bool should_void_pointer_to_self(BaseType* type, const std::string& id, unsigned index, bool overrides) {
    if(index == 0 && type->kind() == BaseTypeKind::Pointer && ((PointerType*) type)->type->kind() == BaseTypeKind::Referenced && (id == "self" || id == "this")) {
        if(((PointerType*) type)->type->linked_node()->as_interface_def() || (((PointerType*) type)->type->linked_node()->as_struct_def() && overrides)) {
            return true;
        }
    }
    return false;
}

//void param_type_with_id(ToCAstVisitor* visitor, BaseType* type, const std::string& id, unsigned index, bool overrides) {
//    if(should_void_pointer_to_self(type, id, index, overrides)) {
//        visitor->write("void* ");
//        visitor->write(id);
//        return;
//    }
//    type_with_id(visitor, type, id);
//}

ASTNode* get_func_param_ref_node(ASTNode* node) {
    if(!node) return nullptr;
    auto param = node->as_func_param();
    if(!param) return nullptr;
    return param->type->get_direct_ref_node();
}

StructDefinition* get_func_param_ref_struct(ASTNode* node) {
    if(!node) return nullptr;
    auto param = node->as_func_param();
    if(!param) return nullptr;
    return param->type->get_direct_ref_struct();
}

void vtable_name(ToCAstVisitor* visitor, InterfaceDefinition* interface, StructDefinition* definition) {
    visitor->write(interface->name);
    visitor->write(definition->name);
}

std::pair<InterfaceDefinition*, StructDefinition*> get_dyn_obj_impl(BaseType* type, Value* value) {
    if(!type) return {nullptr, nullptr};
    auto pure_type = type->pure_type();
    if(pure_type->kind() == BaseTypeKind::Dynamic) {
        const auto dyn_type = ((DynamicType*) pure_type);
        const auto interface = dyn_type->linked_node()->as_interface_def();
        if(interface && value->value_type() == ValueType::Struct) {
            const auto linked = value->known_type();
            const auto def = linked->linked_struct_def();
            if(def) {
                return { interface, def };
            } else {
#ifdef DEBUG
                throw std::runtime_error("linked node not StructDefinition");
#endif
            }
        }
    }
    return {nullptr, nullptr};
}

bool implicit_mutate_value_for_dyn_obj(ToCAstVisitor* visitor, BaseType* type, Value* value, void(*value_visit)(ToCAstVisitor* visitor, Value* value)) {
//    (__chemical_fat_pointer__){ &sm, (void*) &PhoneSmartPhone }
    auto dyn_obj = get_dyn_obj_impl(type, value);
    if(!dyn_obj.first) return false;
    visitor->write('(');
    visitor->write(visitor->fat_pointer_type);
    visitor->write(')');
    visitor->write('{');
    visitor->space();
    if(!(value->linked_node()->as_func_param() && value->known_type()->kind() == BaseTypeKind::Referenced)) {
        visitor->write('&');
    }
    value_visit(visitor, value);
    visitor->write(',');
    visitor->write("(void*) &");
    vtable_name(visitor, dyn_obj.first, dyn_obj.second);
    visitor->space();
    visitor->write('}');
    return true;
}

bool implicit_mutate_value(ToCAstVisitor* visitor, BaseType* type, Value* value, void(*value_visit)(ToCAstVisitor* visitor, Value* value)) {
    return implicit_mutate_value_for_dyn_obj(visitor, type, value, value_visit);
}

bool implicit_mutate_value_default(ToCAstVisitor* visitor, BaseType* type, Value* value) {
    return implicit_mutate_value_for_dyn_obj(visitor, type, value, [](ToCAstVisitor* visitor, Value* value) -> void {
        const auto as_struct = value->as_struct();
        if(as_struct) {
            visitor->write('(');
            visitor->write("struct");
            visitor->space();
            node_name(visitor, as_struct->definition);
            visitor->write(')');
            value->accept(visitor);
        } else {
            value->accept(visitor);
        }
    });
}

void ToCAstVisitor::accept_mutating_value(BaseType* type, Value* value) {
    if(!implicit_mutate_value_default(this, type, value)) {
        value->accept(this);
    }
}

void func_call_args(ToCAstVisitor* visitor, FunctionCall* call, FunctionType* func_type) {
    auto prev_value = visitor->nested_value;
    visitor->nested_value = true;
    unsigned i = 0;
    FunctionParam* param;
    while(i < call->values.size()) {
        param = func_type->func_param_for_arg_at(i);
        auto& val = call->values[i];
        const auto param_ref_node = param->type->get_direct_ref_node();
        const auto val_ref_node = get_func_param_ref_node(val->linked_node());
        const auto is_struct_param = (param_ref_node && param_ref_node->as_struct_def() && (!val_ref_node || !val_ref_node->as_struct_def()));
        const auto is_variant_param = (param_ref_node && param_ref_node->as_variant_def() && (!val_ref_node || !val_ref_node->as_variant_def()));
        if(is_struct_param || is_variant_param) {
            auto allocated = visitor->local_allocated.find(val.get());
            visitor->write('&');
            if(allocated != visitor->local_allocated.end()) {
                visitor->write(allocated->second);
            } else {
                visitor->debug_comment("this struct (returned or literal) wasn't allocated by before stmt visitor", false);
                // TODO this struct isn't allocated, if it has a destructor it won't be called, fix this
                val->accept(visitor);
            }
            if (i != call->values.size() - 1) {
                visitor->write(", ");
            }
            i++;
            continue;
        } else if(!val->reference() && val->value_type() == ValueType::Array) {
            visitor->write('(');
            auto base_type = val->get_base_type();
            base_type->accept(visitor);
            visitor->write("[]");
            visitor->write(')');
        }
        visitor->accept_mutating_value(param->type.get(), val.get());
        if (i != call->values.size() - 1) {
            visitor->write(", ");
        }
        i++;
    }
    visitor->nested_value = prev_value;
}

void access_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, unsigned end, unsigned size);

void func_container_name(ToCAstVisitor* visitor, FunctionDeclaration* func_node);

void func_container_name(ToCAstVisitor* visitor, ASTNode* parent_node, ASTNode* linked_node);

void func_name_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, unsigned end);

void access_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, unsigned end) {
    access_chain(visitor, values, start, end, values.size());
}

#define variant_type_variant_name "__chx__vt_621827"

void value_alloca(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::unique_ptr<Value>& value) {
    type_with_id(visitor, type, identifier);
    const auto var = type->get_direct_ref_variant();
    if(var) {
        visitor->write(" = ");
        visitor->write("{ .");
        visitor->write(variant_type_variant_name);
        visitor->write(" = ");
        visitor->write(std::to_string(var->variables.size()));
        visitor->space();
        visitor->write('}');
    }
    visitor->write(';');
}

void write_accessor(ToCAstVisitor* visitor, Value* current, Value* next) {
    if(next && next->as_index_op()) return;
    auto linked = current->linked_node();
    if(linked && linked->as_namespace()) {
        return;
    }
    if(linked && linked->as_base_func_param()){
        const auto node = linked->as_base_func_param()->type->get_direct_ref_node();
        if(node && (node->as_struct_def() || node->as_variant_def())) {
            visitor->write("->");
            return;
        }
    }
    if (current->type_kind() == BaseTypeKind::Pointer) {
        visitor->write("->");
    } else {
        visitor->write('.');
    }
}

void write_accessor(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned int index) {
    write_accessor(visitor, values[index].get(), index + 1 < values.size() ? values[index + 1].get() : nullptr);
}

bool write_self_arg_bool(ToCAstVisitor* visitor, FunctionType* func_type, std::vector<std::unique_ptr<ChainValue>>& values, unsigned int grandpa_index, FunctionCall* call, bool force_no_pointer) {
    if(func_type->has_self_param()) {
        auto grandpa = values[grandpa_index].get();
        if(!grandpa->is_pointer() && !force_no_pointer) {
            visitor->write('&');
        }
        unsigned index = 0;
        while(index <= grandpa_index) {
            values[index]->accept(visitor);
            if(index < grandpa_index) {
                write_accessor(visitor, values, index);
            }
            index++;
        }
        return true;
    } else {
        return false;
    }
}

void write_self_arg(ToCAstVisitor* visitor, FunctionType* func_type, std::vector<std::unique_ptr<ChainValue>>& values, unsigned int grandpa_index, FunctionCall* call) {
    if(write_self_arg_bool(visitor, func_type, values, grandpa_index, call, false)) {
        if (!call->values.empty()) {
            visitor->write(',');
        }
    }
}

Value* evaluate_comptime_func(
        ToCAstVisitor* visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call
) {
    Value* eval;
    auto value = std::unique_ptr<Value>(func_decl->call(&visitor->comptime_scope, call, nullptr, false));
    if(!value) {
        visitor->error("comptime function call didn't return anything", call);
        return nullptr;
    }
    auto eval_call = value->create_evaluated_value(visitor->comptime_scope);
    if(eval_call) {
        eval = eval_call.release();
    } else {
        eval = value.release();
    }
    visitor->evaluated_func_calls[call] = std::unique_ptr<Value>(eval);
    return eval;
}

Value* evaluated_func_val(
        ToCAstVisitor* visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call
) {
    Value* eval;
    auto found_eval = visitor->evaluated_func_calls.find(call);
    if(found_eval == visitor->evaluated_func_calls.end()) {
        eval = evaluate_comptime_func(visitor, func_decl, call);
    } else {
        eval = found_eval->second.get();
    }
    return eval;
}

void visit_evaluated_func_val(
        ToCAstVisitor* visitor,
        Visitor* actual_visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call,
        Value* eval,
        const std::string& assign_id = ""
) {
    auto returns_struct = func_decl->returnType->value_type() == ValueType::Struct;
    FunctionCall* remove_alloc = nullptr;
    bool write_semi = false;
    if(!assign_id.empty() && returns_struct) {
        if(eval->as_struct()) {
            auto struc = eval->as_struct();
            visitor->write(assign_id);
            visitor->write(" = ");
            write_struct_def_value_call(visitor, struc->definition);
            write_semi = true;
        } else if(eval->as_access_chain()) {
            auto& chain = eval->as_access_chain()->values;
            auto last = chain[chain.size() - 1].get();
            if(last->as_func_call()) {
                visitor->local_allocated[last] = assign_id;
                remove_alloc = last->as_func_call();
            }
        }
    }
    eval->accept(actual_visitor);
    if(remove_alloc) {
        auto found = visitor->local_allocated.find(remove_alloc);
        if(found != visitor->local_allocated.end()) {
            visitor->local_allocated.erase(found);
        }
    }
    if(write_semi) {
        visitor->write(';');
    }
}

Value* evaluate_func(
        ToCAstVisitor* visitor,
        Visitor* actual_visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call,
        const std::string& assign_id = ""
) {
    Value* eval = evaluated_func_val(visitor, func_decl, call);
    visit_evaluated_func_val(visitor, actual_visitor, func_decl, call, eval, assign_id);
    return eval;
}

void value_assign_default(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, Value* value) {
    if(value->as_access_chain()) {
        auto chain = value->as_access_chain();
        auto func_call = chain->values.back()->as_func_call();
        if(func_call) {
            auto linked_func = func_call->safe_linked_func();
            if(linked_func && linked_func->has_annotation(AnnotationKind::CompTime)) {

                Value* eval;
                auto found_eval = visitor->evaluated_func_calls.find(func_call);
                bool not_found = found_eval == visitor->evaluated_func_calls.end();
                if(not_found) {
                    eval = evaluate_comptime_func(visitor, linked_func, func_call);
                } else {
                    eval = found_eval->second.get();
                }
                if(eval->primitive()) {
                    visitor->write(identifier);
                    visitor->write(" = ");
                }
                if(not_found) {
                    eval->accept((Visitor*) visitor->before_stmt.get());
                }
                visit_evaluated_func_val(visitor, visitor, linked_func, func_call, eval, identifier);
                if(!eval->reference()) {
                    visitor->write(';');
                }
                return;
            }
        }
        if(func_call && func_call->create_type()->value_type() == ValueType::Struct) {
            return;
//            auto func_decl = func_call->safe_linked_func();
//            auto parent_type = func_call->parent_val->create_type();
//            auto func_type = parent_type->function_type();
//            auto end = chain->values.size();
//            auto grandpa = ((int) end - 3) >= 0 ? chain->values[end - 3].get() : nullptr;
//            visitor->nested_value = true;
//            if(func_decl) {
//                func_container_name(visitor, func_decl);
//            } else {
//                func_name_chain(visitor, chain->values, 0, chain->values.size() - 1);
//            }
//            visitor->nested_value = false;
//            visitor->write('(');
//            visitor->write('&');
//            visitor->write(identifier);
//            if(!chain->values.back()->as_func_call()->values.empty() || func_type->has_self_param()){
//                visitor->write(", ");
//            }
//            if(grandpa) write_self_arg(visitor, func_type, chain->values, ((int) ((int) end - 3)), func_call);
//            func_call_args(visitor, chain->values.back()->as_func_call(), func_type);
//            visitor->write(");");
//            return;
        }
    }
    visitor->write(identifier);
    write_type_post_id(visitor, type);
    visitor->write(" = ");
    visitor->accept_mutating_value(type, value);
    visitor->write(';');
}

void value_init_default(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, Value* value) {
    const auto struct_value = value->as_struct();
    int16_t prev_itr = 0;
    const auto is_generic = struct_value && !struct_value->definition->generic_params.empty();
    if(struct_value && is_generic) {
        prev_itr = struct_value->definition->active_iteration;
        struct_value->definition->set_active_iteration(struct_value->generic_iteration);
    }
    type->accept(visitor);
    visitor->space();
    value_assign_default(visitor, identifier, type, value);
    if(struct_value && is_generic) struct_value->definition->set_active_iteration(prev_itr);
}

//void value_store_default(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
//    visitor->new_line_and_indent();
//    value_assign_default(visitor, identifier, type, value->get());
//}
//
//void value_store(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
//    if(value.has_value()) {
//        value_store_default(visitor, identifier, type, value);
//    }
//}

void value_alloca_store(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::unique_ptr<Value>& value) {
    if(value) {
        auto value_kind = value->val_kind();
        if(value_kind == ValueKind::IfValue || value_kind == ValueKind::SwitchValue || value_kind == ValueKind::LoopValue) {
            value_alloca(visitor, identifier, type, value);
            visitor->new_line_and_indent();
            value->accept(visitor);
            return;
        }
        if(type->value_type() == ValueType::Struct && value->as_access_chain()) {
            // struct instantiation is done in 2 instructions -> declaration and assignment
//            value_alloca(visitor, identifier, type, value);
//            visitor->new_line_and_indent();
            value_assign_default(visitor, identifier, type, value.get());
        } else {
            value_init_default(visitor, identifier, type, value.get());
        }
    } else {
        value_alloca(visitor, identifier, type, value);
    }
}

void var_init(ToCAstVisitor* visitor, VarInitStatement* init, bool is_static) {
    visitor->debug_comment("var_init defining the value");
    if(is_static) {
        visitor->write("static ");
    }
    if (!init->type) {
        init->type.reset(init->value->create_type().release());
    }
    value_alloca_store(visitor, init->identifier, init->type.get(), init->value);
}

class SubVisitor {
public:

    /**
     * c visitor
     */
    ToCAstVisitor* visitor;

    /**
     * constructor
     */
    SubVisitor(ToCAstVisitor* visitor) : visitor(visitor) {

    };

    /**
     * space fn using visitor
     */
    inline void space() const {
        visitor->space();
    }

    /**
     * write fn using visitor
     */
    inline void write(char value) const {
        visitor->write(value);
    }

    /**
     * write fn using visitor
     */
    inline void write(const std::string& value) const {
        visitor->write(value);
    }

    /**
     * new line and indent to current indentation level
     */
    inline void new_line_and_indent() {
        visitor->new_line_and_indent();
    }

    /**
     * reset the visitor, to translate another set of nodes
     */
    virtual void reset() {
        // does nothing
    }

};

class CBeforeStmtVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    void visit(FunctionCall *call) override;

    void visit(ArrayValue *arrayVal) override;

    void visit(StructValue *structValue) override;

    void visit(AccessChain *chain) override;

    void visit(VariantCall *call) override;

    void process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const std::string& identifier);

    void process_init_value(Value* value, const std::string& identifier);

    void visit(VarInitStatement *init) override;

    void visit(ReturnStatement *stmt) override;

    void visit(Scope *scope) override {
        // do nothing
    }

    void visit(LambdaFunction *func) override {
        // do nothing
    }

};

class CAfterStmtVisitor : public CommonVisitor, public SubVisitor {

    using SubVisitor::SubVisitor;

    void visit(AccessChain *chain) override;

    void visit(FunctionCall *call) override;

    void destruct_chain(AccessChain *chain, bool destruct_last);

    void visit(LambdaFunction *func) override {
        // do nothing
    }

    void visit(Scope *scope) override {
        // do nothing
    }

};

void allocate_struct_by_name(ToCAstVisitor* visitor, ExtendableMembersContainerNode* def, const std::string& name, Value* initializer = nullptr) {
    visitor->write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    visitor->write(' ');
    visitor->write(name);
    if(initializer) {
        visitor->write(" = ");
        initializer->accept(visitor);
    }
    visitor->write(';');
    visitor->new_line_and_indent();
}

void allocate_fat_pointer_by_name(ToCAstVisitor* visitor, const std::string& name, Value* initializer = nullptr) {
    visitor->write(visitor->fat_pointer_type);
    visitor->write(' ');
    visitor->write(name);
    if(initializer) {
        visitor->write(" = ");
        initializer->accept(visitor);
    }
    visitor->write(';');
    visitor->new_line_and_indent();
}

void allocate_fat_pointer_for_value(ToCAstVisitor* visitor, Value* value, const std::string& name, Value* initializer) {
    auto found = visitor->local_allocated.find(value);
    if(found != visitor->local_allocated.end()) {
        // already allocated
        return;
    }
    visitor->local_allocated[value] = name;
    allocate_fat_pointer_by_name(visitor, name, initializer);
}

void allocate_struct_for_value(ToCAstVisitor* visitor, ExtendableMembersContainerNode* def, Value* value, const std::string& name, Value* initializer) {
    auto found = visitor->local_allocated.find(value);
    if(found != visitor->local_allocated.end()) {
        // already allocated
        return;
    }
    visitor->local_allocated[value] = name;
    allocate_struct_by_name(visitor, def, name, initializer);
}

void allocate_struct_for_struct_value(ToCAstVisitor* visitor, ExtendableMembersContainerNode* def, StructValue* value, const std::string& name, Value* initializer = nullptr) {
    if(def->generic_params.empty()) {
        allocate_struct_for_value(visitor, def, value, name, initializer);
    } else {
        auto prev_itr = def->active_iteration;
        def->set_active_iteration(value->generic_iteration);
        allocate_struct_for_value(visitor, def, value, name, initializer);
        def->set_active_iteration(prev_itr);
    }
}

void allocate_struct_for_func_call(ToCAstVisitor* visitor, ExtendableMembersContainerNode* def, FunctionCall* call, FunctionType* func_type, const std::string& name, Value* initializer = nullptr) {
    if(func_type->returnType->kind() == BaseTypeKind::Generic) {
        auto prev_itr = def->active_iteration;
        def->set_active_iteration(func_type->returnType->get_generic_iteration());
        allocate_struct_for_value(visitor, def, call, name, initializer);
        def->set_active_iteration(prev_itr);
    } else {
        allocate_struct_for_value(visitor, def, call, name, initializer);
    }
}

void CBeforeStmtVisitor::visit(FunctionCall *call) {
    auto func_type = call->create_function_type();
    auto decl = call->safe_linked_func();
    int16_t prev_iteration;
    if(decl) {
        if(!decl->generic_params.empty()) {
            prev_iteration = decl->active_iteration;
            decl->set_active_iteration(call->generic_iteration);
        }
        if(decl->has_annotation(AnnotationKind::CompTime)) {
            auto eval = evaluated_func_val(visitor, decl, call);
            auto identifier = visitor->get_local_temp_var_name();
            if(eval->as_struct()) {
                allocate_struct_by_name(visitor, eval->as_struct()->definition, identifier);
                visitor->local_allocated[eval] = identifier;
            }
            process_init_value(eval, identifier);
            eval->accept(this);
            return;
        }
    }
    CommonVisitor::visit(call);
    for(auto& value : call->values) {
        const auto struct_val = value->as_struct();
        if(struct_val) {
            allocate_struct_for_struct_value(visitor, struct_val->definition, struct_val, visitor->get_local_temp_var_name(), struct_val);
        }
    }
    if(func_type->returnType->value_type() == ValueType::Struct) {
        auto linked = func_type->returnType->linked_node();
        if(linked->as_struct_def()) {
            allocate_struct_for_func_call(visitor, linked->as_struct_def(), call, func_type.get(), visitor->get_local_temp_var_name());
        } else if(linked->as_variant_def()) {
            allocate_struct_for_func_call(visitor, linked->as_variant_def(), call, func_type.get(), visitor->get_local_temp_var_name());
        } else if(func_type->returnType->pure_type()->kind() == BaseTypeKind::Dynamic && linked->as_interface_def()) {
            allocate_fat_pointer_for_value(visitor, call, visitor->get_local_temp_var_name(), nullptr);
        }
    }
    if(decl && !decl->generic_params.empty()) {
        decl->set_active_iteration(prev_iteration);
    }
}

void CBeforeStmtVisitor::visit(ArrayValue *arrayVal) {
    arrayVal->call_implicit_constructors();
    CommonVisitor::visit(arrayVal);
}

void CBeforeStmtVisitor::visit(StructValue *structValue) {
    structValue->call_implicit_constructors();
    CommonVisitor::visit(structValue);
}

void chain_after_func(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, const unsigned end, const unsigned total_size);

void func_name(ToCAstVisitor* visitor, Value* ref, FunctionDeclaration* func_decl) {
    ref->accept(visitor); // function name
    if(func_decl->multi_func_index != 0) {
        visitor->write("__cmf_");
        visitor->write(std::to_string(func_decl->multi_func_index));
    }
}

void func_name(ToCAstVisitor* visitor, FunctionDeclaration* func_decl) {
    visitor->write(func_decl->name);
    if(func_decl->multi_func_index != 0) {
        visitor->write("__cmf_");
        visitor->write(std::to_string(func_decl->multi_func_index));
    }
    if(func_decl->active_iteration != 0) {
        visitor->write("__cgf_");
        visitor->write(std::to_string(func_decl->active_iteration));
    }
}

void func_call_that_returns_struct(ToCAstVisitor* visitor, CBeforeStmtVisitor* actual_visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, unsigned end) {
    auto last = values[end - 1]->as_func_call();
    auto func_decl = last->safe_linked_func();
    auto parent = values[end - 2].get();
    if (func_decl && func_decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    auto grandpa = ((int) end) - 3 >= 0 ? values[end - 3].get() : nullptr;
    auto parent_type = parent->create_type();
    auto func_type = parent_type->function_type();
    bool is_lambda = (parent->linked_node() != nullptr && parent->linked_node()->as_struct_member() != nullptr);
    if (visitor->pass_structs_to_initialize && func_type->returnType->value_type() == ValueType::Struct) {
        visitor->debug_comment("function call being taken out that returns struct");
        // functions that return struct
        auto allocated = visitor->local_allocated.find(last);
        if (allocated == visitor->local_allocated.end()) {
            visitor->write("[NotFoundAllocated in func_call_that_returns_struct]");
            return;
        }
        if(func_decl) {
            func_container_name(visitor, func_decl);
        } else {
            auto prev_nested = visitor->nested_value;
            visitor->nested_value = true;
            func_name_chain(visitor, values, start, end - 1);
            visitor->nested_value = prev_nested;
        }
        if (grandpa && !is_lambda) {
            auto grandpaType = grandpa->create_type();
//            func_container_name(visitor, grandpaType->linked_node(), parent->linked_node());
//            func_name(visitor, parent, func_decl);
            visitor->write('(');
        } else {
//            if (func_decl && func_decl->has_annotation(AnnotationKind::Constructor)) {
//                // struct name for the constructor, getting through return type
//                auto linked = func_decl->returnType->linked_node();
//                if (linked && linked->as_struct_def()) {
//                    visitor->write(linked->as_struct_def()->name);
//                }
//            }
//            access_chain(visitor, values, start, end - 1);
            visitor->write('(');
        }
        visitor->write('&');
        visitor->write(allocated->second);
        if (!last->values.empty() || func_type->has_self_param()) {
            visitor->write(", ");
        }
        if(func_type->has_self_param()) {
            if(end >= 3) {
                if (write_self_arg_bool(visitor, func_type, values, (((int) end) - 3), last, false)) {
                    if (!last->values.empty()) {
                        visitor->write(", ");
                    }
                }
            } else if(visitor->current_func_type) {
                auto self_param = visitor->current_func_type->get_self_param();
                if(self_param) {
                    visitor->write(self_param->name);
                    visitor->write(", ");
                } else {
//                visitor->error("No self param can be passed to a function, because current function doesn't take a self arg");
                }
            }
        }
//        if(grandpa) write_self_arg(visitor, func_type, values, ((int) ((int) end - 3)), last);
        func_call_args(visitor, last->as_func_call(), func_type);
        visitor->write(')');
        if(!visitor->nested_value) {
            visitor->write(';');
        }
        visitor->new_line_and_indent();
    }
}

void CBeforeStmtVisitor::visit(AccessChain *chain) {

    CommonVisitor::visit(chain);

    const auto start = 0;
    const auto end = chain->values.size();
    auto& values = chain->values;
    unsigned i = start;
    // function call would be processed recursively
    {
        int j = end - 1;
        while(j >= 0) {
            auto& current = values[j];
            auto call = current->as_func_call();
            if(call) {
                func_call_that_returns_struct(visitor, this, values, start, j + 1);
            }
            j--;
        }
    }

}

void CBeforeStmtVisitor::visit(VariantCall *call) {

    CommonVisitor::visit(call);
    const auto member = call->chain->linked_node()->as_variant_member();
    const auto linked = member->parent_node;
    const auto index = linked->direct_child_index(member->name);

    int16_t prev_itr;
    if(!linked->generic_params.empty()) {
        prev_itr = linked->get_active_iteration();
        linked->set_active_iteration(call->generic_iteration);
    }

    visitor->write("struct ");
    node_parent_name(visitor, linked);
    struct_name(visitor, linked);
    visitor->write(' ');
    const auto local = visitor->get_local_temp_var_name();
    visitor->write(local);
    visitor->local_allocated[call] = local;
    visitor->write(" = ");
    visitor->write("{ ");
    visitor->write(std::to_string(index));
    visitor->write(", ");
    unsigned i = 0;
    auto prev_nested = visitor->nested_value;
    visitor->nested_value = true;
    for(auto& value : member->values) {
        visitor->write('.');
        visitor->write(member->name);
        visitor->write('.');
        visitor->write(value.second->name);
        visitor->write(" = ");
        const auto& val = call->values[i];
        val->accept(visitor);
        visitor->write(", ");
        i++;
    }
    visitor->nested_value = prev_nested;
    visitor->write('}');
    visitor->write(';');
    visitor->new_line_and_indent();

    if(!linked->generic_params.empty()) {
        linked->set_active_iteration(prev_itr);
    }

}

void CBeforeStmtVisitor::process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const std::string& identifier) {
    auto eval = evaluated_func_val(visitor, decl, call);
    if(eval->as_struct()) {
        allocate_struct_by_name(visitor, eval->as_struct()->definition, identifier);
        visitor->local_allocated[eval] = identifier;
    }
    process_init_value(eval, identifier);
}

void CBeforeStmtVisitor::process_init_value(Value* value, const std::string& identifier) {
    if(value && value->value_type() == ValueType::Struct) {
        auto chain = value->as_access_chain();
        if(!chain) return;
        auto call = chain->values[chain->values.size() - 1]->as_func_call();
        if(call) {
            auto decl = call->safe_linked_func();
            auto func_type = call->create_function_type();
            auto linked = func_type->returnType->linked_node();
            if(decl && decl->has_annotation(AnnotationKind::CompTime)) {
                process_comp_time_call(decl, call, identifier);
                return;
            } else if(linked->as_struct_def()) {
                allocate_struct_for_func_call(visitor, linked->as_struct_def(), call, func_type.get(), identifier);
            } else if(linked->as_variant_def()) {
                allocate_struct_for_func_call(visitor, linked->as_variant_def(), call, func_type.get(), identifier);
            } else if(func_type->returnType->pure_type()->kind() == BaseTypeKind::Dynamic && linked->as_interface_def()) {
                allocate_fat_pointer_for_value(visitor, call, identifier, nullptr);
            }
        }
    }
}

void CBeforeStmtVisitor::visit(VarInitStatement *init) {
    if (!init->type) {
        init->type.reset(init->value->create_type().release());
    }
    visitor->debug_comment("visiting var init in before");
    if(init->value) {
        process_init_value(init->value.get(), init->identifier);
    }
    CommonVisitor::visit(init);
}

void CBeforeStmtVisitor::visit(ReturnStatement* returnStmt) {
    if(returnStmt->value) {
        returnStmt->call_implicit_constructor(*visitor);
    }
    CommonVisitor::visit(returnStmt);
}

struct DeclaredNodeData {
    union {
        // total iterations that are done
        int16_t iterations_done;
        bool declared_struct;
    } struct_def;
    union {
        // total iterations that are done
        int16_t iterations_done;
    } variant_def;
};

class CTopLevelDeclarationVisitor : public Visitor, public SubVisitor {
public:

    CValueDeclarationVisitor* value_visitor;

    CTopLevelDeclarationVisitor(
            ToCAstVisitor* visitor,
            CValueDeclarationVisitor* value_visitor
    );

    /**
     * nodes can be declared early if present in generics
     */
    std::unordered_map<ASTNode*, DeclaredNodeData> declared_nodes;

    // this will not declare it's contained functions
    void declare_struct_def_only(StructDefinition* def, bool check_declared);

    void declare_struct(StructDefinition* structDef, bool check_declared);

    void declare_variant(VariantDefinition* structDef);

    void visit(TypealiasStatement *statement) override;

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(VariantDefinition *variant_def) override;

    void visit(Namespace *ns) override;

    void visit(UnionDef *def) override;

    void visit(InterfaceDefinition *interfaceDefinition) override;

    void visit(ImplDefinition *implDefinition) override;

    void reset();

};

CTopLevelDeclarationVisitor::CTopLevelDeclarationVisitor(
    ToCAstVisitor *visitor,
    CValueDeclarationVisitor *value_visitor
) : SubVisitor(visitor), value_visitor(value_visitor) {

}

class CValueDeclarationVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    std::unordered_map<void*, std::string> aliases;

    unsigned lambda_num = 0;

    unsigned func_type_num = 0;

    unsigned alias_num = 0;

    unsigned enum_num = 0;

    void visit(VarInitStatement *init) override;

    void visit(LambdaFunction *func) override;

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(EnumDeclaration *enumDeclaration) override;

    void visit(TypealiasStatement *statement) override;

    void visit(FunctionType *func) override;

    void visit(StructMember *member) override;

    void reset() override {
        aliases.clear();
    }

};

enum class DestructionJobType {
    Default,
    Array
};

struct DestructionJob {
    DestructionJobType type;
    std::string self_name;
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

    void destruct(
            const std::string& self_name,
            ExtendableMembersContainerNode* linked,
            int16_t generic_iteration,
            FunctionDeclaration* destructor,
            bool is_pointer
    );

    void queue_destruct(
            const std::string& self_name,
            ASTNode* initializer,
            int16_t generic_iteration,
            ExtendableMembersContainerNode* linked,
            bool is_pointer = false
    );

    void queue_destruct(const std::string& self_name, ASTNode* initializer, FunctionCall* call);

    void destruct_arr_ptr(const std::string& self_name, Value* array_size, ExtendableMembersContainerNode* linked, int16_t generic_iteration, FunctionDeclaration* destructor);

    void destruct_arr(const std::string& self_name, int array_size, ExtendableMembersContainerNode* linked, int16_t generic_iteration, FunctionDeclaration* destructor) {
        IntValue siz(array_size, nullptr);
        destruct_arr_ptr(self_name, &siz, linked, generic_iteration, destructor);
    }

    void destruct(const DestructionJob& job, Value* current_return);

    bool queue_destruct_arr(const std::string& self_name, ASTNode* initializer, BaseType* elem_type, int array_size);

    void visit(VarInitStatement *init) override;

    void dispatch_jobs_from_no_clean(int begin);

    void dispatch_jobs_from(int begin);

    void queue_destruct_decl_params(FunctionType* decl);

    void process_init_value(VarInitStatement *init, Value* value);

    void reset() override {
        destroy_current_scope = true;
        new_line_before = true;
        destruct_jobs.clear();
    }

};

void CAfterStmtVisitor::destruct_chain(AccessChain *chain, bool destruct_last) {
    int index = ((int) chain->values.size()) - (destruct_last ? 1 : 2);
    FunctionCall* call;
    while(index >= 0) {
        call = chain->values[index]->as_func_call();
        if(call) {
            const auto decl = call->safe_linked_func();
            if(decl && decl->has_annotation(AnnotationKind::CompTime)) {
                auto eval = visitor->evaluated_func_calls.find(call);
                if(eval != visitor->evaluated_func_calls.end()) {
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
            auto func_type = call->create_function_type();
            if(func_type->returnType->value_type() == ValueType::Struct) {
                auto linked = func_type->returnType->linked_node();
                if(linked->as_struct_def()) {
                    const auto struct_def = linked->as_struct_def();
                    int16_t generic_iteration = 0;
                    if(!struct_def->generic_params.empty()) {
                        generic_iteration = func_type->returnType->get_generic_iteration();
                    }
                    auto destructor = struct_def->destructor_func();
                    if(destructor) {
                        auto destructible = visitor->local_allocated.find(call);
                        if (destructible != visitor->local_allocated.end()) {
                            visitor->destructor->destruct(
                                    destructible->second,
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
    destruct_chain(chain, chain->is_node);
}

void CAfterStmtVisitor::visit(FunctionCall *call) {
//    auto decl = call->safe_linked_func();
//    if(decl && decl->has_annotation(AnnotationKind::CompTime)) {
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
            destruct_chain(chain, false);
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
                val->accept(this);
//            }
        }
    }
}

void CDestructionVisitor::destruct(const std::string& self_name, ExtendableMembersContainerNode* parent_node, int16_t generic_iteration, FunctionDeclaration* destructor, bool is_pointer) {
    int16_t prev_itr;
    if(!parent_node->generic_params.empty()) {
        prev_itr = parent_node->active_iteration;
        parent_node->set_active_iteration(generic_iteration);
    }
    if(new_line_before) {
        visitor->new_line_and_indent();
    }
    func_container_name(visitor, parent_node, destructor);
    visitor->write(destructor->name);
    visitor->write('(');
    if(destructor->has_self_param()) {
        if(!is_pointer) {
            visitor->write('&');
        }
        visitor->write(self_name);
    }
    visitor->write(')');
    visitor->write(';');
    if(!new_line_before) {
        visitor->new_line_and_indent();
    }
    if(!parent_node->generic_params.empty()) {
        parent_node->set_active_iteration(prev_itr);
    }
}

void CDestructionVisitor::queue_destruct(const std::string& self_name, ASTNode* initializer, int16_t generic_iteration, ExtendableMembersContainerNode* linked, bool is_pointer) {
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

void CDestructionVisitor::queue_destruct(const std::string& self_name, ASTNode* initializer, FunctionCall* call) {
    auto return_type = call->create_type();
    auto linked = return_type->linked_node();
    if(linked) queue_destruct(self_name, initializer, return_type->get_generic_iteration(), linked->as_extendable_members_container_node());
}

void CDestructionVisitor::destruct_arr_ptr(const std::string &self_name, Value* array_size, ExtendableMembersContainerNode* linked, int16_t generic_iteration, FunctionDeclaration* destructorFunc) {
    std::string arr_val_itr_name = visitor->get_local_temp_var_name();
    visitor->new_line_and_indent();
    visitor->write("for(int ");
    visitor->write(arr_val_itr_name);
    visitor->write(" = ");
    array_size->accept(visitor);
    visitor->write(" - 1");
    visitor->write("; ");
    visitor->write(arr_val_itr_name);
    visitor->write(" >= 0;");
    visitor->write(arr_val_itr_name);
    visitor->write("--){");
    visitor->indentation_level++;
    destruct(self_name + "[" + arr_val_itr_name + "]", linked, generic_iteration, destructorFunc, false);
    visitor->indentation_level--;
    visitor->new_line_and_indent();
    visitor->write('}');
}

void CDestructionVisitor::destruct(const DestructionJob& job, Value* current_return) {
    if(current_return) {
        const auto id = current_return->as_identifier();
        if (id && id->linked == job.initializer) {
            return;
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
        auto linked = d_param->type->get_direct_ref_node();
        if(linked) {
            const auto members_container = linked->as_members_container();
            if(members_container) {
                const auto destructor_func = members_container->destructor_func();
                if (destructor_func) {
                    queue_destruct(d_param->name, d_param.get(), d_param->type->get_generic_iteration(),
                                   linked->as_extendable_members_container_node(), true);
                }
            }
        }
    }
}

bool CDestructionVisitor::queue_destruct_arr(const std::string& self_name, ASTNode* initializer, BaseType *elem_type, int array_size) {
    if(elem_type->value_type() == ValueType::Struct) {
        auto linked = elem_type->linked_node();
        FunctionDeclaration* destructorFunc;
        const auto struc_def = linked->as_extendable_members_container_node();
        if(struc_def) {
            destructorFunc = struc_def->destructor_func();
            if (!destructorFunc) {
                return false;
            }
            int16_t generic_iteration = 0;
            if(!struc_def->generic_params.empty()) {
                auto type = initializer->create_value_type();
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
    auto chain = init_value->as_access_chain();
    if(chain && chain->values.back()->as_func_call()) {
        queue_destruct(init->identifier, init, chain->values.back()->as_func_call());
        return;
    }
    auto array_val = init_value->as_array_value();
    if(array_val) {
        auto elem_type = array_val->element_type();
        queue_destruct_arr(init->identifier, init, elem_type.get(), array_val->array_size());
        return;
    }
    auto variant_call = init_value->as_variant_call();
    if(variant_call) {
        const auto def = variant_call->get_definition();
        queue_destruct(init->identifier, init, variant_call->generic_iteration, def);
        return;
    }
    auto struct_val = init_value->as_struct();
    if(struct_val) {
        auto linked = struct_val->definition;
        if(linked) queue_destruct(init->identifier, init, struct_val->generic_iteration, linked);
    }
}

void CDestructionVisitor::visit(VarInitStatement *init) {
    if(init->value_type() == ValueType::Pointer) {
        // do not destruct pointers
        return;
    }
    if(init->value) {
        auto init_value = init->value.get();
        auto chain = init_value->as_access_chain();
        if(chain && chain->values.back()->as_func_call()) {
            const auto func_call = chain->values.back()->as_func_call();
            auto decl = func_call->safe_linked_func();
            if(decl && decl->has_annotation(AnnotationKind::CompTime)) {
                auto found = visitor->evaluated_func_calls.find(func_call);
                if(found != visitor->evaluated_func_calls.end()) {
                    init_value = found->second.get();
                } else {
                    std::cerr << "[2c] warn: evaluated function call value not found in after statement visitor for " << func_call->representation() << std::endl;
                }
            }
        }
        process_init_value(init, init_value);
        return;
    } else {
        if(init->type->value_type() == ValueType::Struct) {
            auto linked = init->type->linked_node();
            if (linked)
                queue_destruct(init->identifier, init, init->type->get_generic_iteration(),
                               linked->as_struct_def());
        } else if(init->type->kind() == BaseTypeKind::Array) {
            auto type = (ArrayType*) init->type.get();
            if(type->array_size != -1) {
                queue_destruct_arr(init->identifier, init, type->elem_type.get(), type->array_size);
            } else {
                // cannot destruct array type without size
            }
        }
    }
}

// this will also destruct given function type's params at the end of scope
void scope(ToCAstVisitor* visitor, Scope& scope, FunctionType* decl) {
    unsigned begin = visitor->destructor->destruct_jobs.size();
    visitor->destructor->queue_destruct_decl_params(decl);
    visitor->write('{');
    visitor->indentation_level+=1;
    visitor->visit_scope(&scope, (int) begin);
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
}

std::string func_type_alias(ToCAstVisitor* visitor, FunctionType* type) {
    std::string alias = "__chx_functype_";
    alias += std::to_string(random(100,999)) + "_";
    alias += std::to_string(visitor->declarer->func_type_num++);
    func_type_with_id(visitor, type, alias);
    visitor->declarer->aliases[type] = alias;
    return alias;
}

std::string typedef_func_type(ToCAstVisitor* visitor, FunctionType* type) {
    visitor->new_line_and_indent();
    visitor->write("typedef ");
    auto alia = func_type_alias(visitor, type);
    visitor->write(';');
    return alia;
}

void func_call(ToCAstVisitor* visitor, FunctionCall* call, FunctionType* func_type) {
    visitor->write('(');
    func_call_args(visitor, call, func_type);
    visitor->write(')');
//    if(!visitor->nested_value) {
//        visitor->write(';');
//    }
}

void CValueDeclarationVisitor::visit(VarInitStatement *init) {
    if(!init->type) {
        // because it can contain function type, so we must emplace it
        // this function type creates a typedef, which is accessible by function type's pointer from aliases map
        init->type = init->value->create_type();
    }
    CommonVisitor::visit(init);
    if(!is_top_level_node) return;
    visitor->new_line_and_indent();
    var_init(visitor, init, true);
}

void CValueDeclarationVisitor::visit(LambdaFunction *lamb) {
    CommonVisitor::visit(lamb);
    std::string lamb_name = "__chemda_";
    lamb_name += std::to_string(random(100,999)) + "_";
    lamb_name += std::to_string(lambda_num++);
    if(!lamb->captureList.empty()) {
        visitor->new_line_and_indent();
        visitor->write("struct ");
        std::string capture_struct_name = lamb_name + "_cap";
        write(capture_struct_name);
        visitor->space();
        visitor->write('{');
        visitor->indentation_level += 1;
        for(auto& var : lamb->captureList) {
            aliases[var.get()] = capture_struct_name;
            visitor->new_line_and_indent();
            if(var->capture_by_ref) {
                PointerType pointer(var->linked->create_value_type(), nullptr);
                pointer.accept(visitor);
            } else {
                var->linked->create_value_type()->accept(visitor);
            }
            visitor->space();
            visitor->write(var->name);
            visitor->write(';');
        }
        visitor->indentation_level -= 1;
        visitor->new_line_and_indent();
        visitor->write("};");
    }
    visitor->new_line_and_indent();
    accept_func_return_with_name(visitor, lamb, lamb_name, false, true);
    aliases[lamb] = lamb_name;
    write('(');

    // writing the captured struct as a parameter
    if(lamb->isCapturing) {
        visitor->write("void*");
        visitor->write(" this");
        if(!lamb->params.empty()) {
            visitor->write(',');
        }
    }
    func_type_params(visitor, lamb);
    write(')');
    auto prev_destroy_scope = visitor->destructor->destroy_current_scope;
    visitor->destructor->destroy_current_scope = true;
    auto previous_destruct_jobs = std::move(visitor->destructor->destruct_jobs);
    scope(visitor, lamb->scope, lamb);
    visitor->destructor->destruct_jobs = std::move(previous_destruct_jobs);
    visitor->destructor->destroy_current_scope = prev_destroy_scope;
}

void declare_params(CValueDeclarationVisitor* value_visitor, std::vector<std::unique_ptr<FunctionParam>>& params) {
    for(auto& param : params) {
        if(param->type->kind() == BaseTypeKind::Function && param->type->function_type()->isCapturing) {
            // do not declare capturing function types
            continue;
        }
        if(!value_visitor->visitor->inline_fn_types_in_params || param->type->kind() != BaseTypeKind::Function) {
            param->accept(value_visitor);
        }
    }
}

void func_ret_func_proto_after_l_paren(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name, FunctionType* retFunc, unsigned declFuncParamStart = 0, unsigned retFuncParamStart = 0) {
    visitor->write('*');
    visitor->write(name);
    visitor->write('(');
    func_type_params(visitor, decl, declFuncParamStart);
    visitor->write("))(");
    func_type_params(visitor, retFunc, retFuncParamStart);
    visitor->write(')');
}

void func_that_returns_func_proto(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name, FunctionType* retFunc) {
    if(decl->body.has_value()) {
        visitor->write("static ");
    }
    accept_func_return(visitor, retFunc->returnType.get());
    visitor->write("(");
    func_ret_func_proto_after_l_paren(visitor, decl, name, retFunc);
}

void declare_func_with_return(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name) {
    if(decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(visitor->inline_fn_types_in_returns && decl->returnType->function_type() && !decl->returnType->function_type()->isCapturing) {
        func_that_returns_func_proto(visitor, decl, name, decl->returnType->function_type());
    } else {
        const auto ret_kind = decl->returnType->kind();
        if ((ret_kind == BaseTypeKind::Void || ret_kind == BaseTypeKind::IntN) && name == "main") {
            visitor->write("int main");
        } else {
            accept_func_return_with_name(visitor, decl, name, false, decl->body.has_value() && !decl->is_exported());
        }
        visitor->write('(');
        func_type_params(visitor, decl);
        visitor->write(')');
    }
}

void declare_by_name(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, const std::string& name) {
    if(decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    declare_params(tld->value_visitor, decl->params);
    if(!tld->visitor->inline_fn_types_in_returns || decl->returnType->function_type() == nullptr) {
        decl->returnType->accept(tld->value_visitor);
    }
    tld->visitor->new_line_and_indent();
    declare_func_with_return(tld->visitor, decl, name);
    tld->visitor->write(';');
}

// when a function is inside struct / interface
void declare_contained_func(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, const std::string& name, bool overrides, StructDefinition* overridden = nullptr) {
    if(decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    declare_params(tld->value_visitor, decl->params);
    if(!tld->visitor->inline_fn_types_in_returns || decl->returnType->function_type() == nullptr) {
        decl->returnType->accept(tld->value_visitor);
    }
    tld->visitor->new_line_and_indent();
    FunctionParam* param = !decl->params.empty() ? decl->params[0].get() : nullptr;
    unsigned i = 0;
    auto write_self_param_now = [decl, tld, param, &i, overrides, overridden]() {
        if(param && should_void_pointer_to_self(param->type.get(), param->name, 0, overrides)) {
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
    if(tld->visitor->inline_fn_types_in_returns && decl->returnType->function_type() != nullptr && !decl->returnType->function_type()->isCapturing) {
        tld->value_visitor->write("static ");
        accept_func_return(tld->visitor, decl->returnType->function_type()->returnType.get());
        tld->write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(tld->visitor, decl, name, decl->returnType->function_type(), i);
    } else {
        auto is_parent_interface = decl->parent_node->as_interface_def() != nullptr;
        accept_func_return_with_name(tld->visitor, decl, name, true, (is_parent_interface || decl->body.has_value()) && !decl->is_exported());
        tld->write('(');
        write_self_param_now();
        func_type_params(tld->visitor, decl, i);
        tld->write(')');
    }
    tld->write(';');
}

void CTopLevelDeclarationVisitor::visit(FunctionDeclaration *decl) {
    // TODO we will fix capturing lambda types when introducing generics and unions
//    if(decl->returnType->function_type() && decl->returnType->function_type()->isCapturing) {
//        visitor->error("Function name " + decl->name + " returns a capturing lambda, which is not supported");
//    }
    if(decl->generic_params.empty()) {
        declare_by_name(this, decl, decl->name);
    } else {
        auto size = decl->total_generic_iterations();
        int16_t i = 0;
        while(i < size) {
            decl->set_active_iteration(i);
            declare_by_name(this, decl, decl->name);
            i++;
        }
        // set the active iteration to -1, so whoever accesses it without setting it to zero receives an error
        decl->set_active_iteration(-1);
    }
}

void CTopLevelDeclarationVisitor::visit(ExtensionFunction *decl) {
    declare_by_name(this, decl, decl->name);
}

void CValueDeclarationVisitor::visit(FunctionDeclaration *decl) {
    if(decl->body.has_value() && !decl->has_annotation(AnnotationKind::CompTime)) {
        decl->body.value().accept(this);
    }
}

void CValueDeclarationVisitor::visit(ExtensionFunction *decl) {
    if(decl->body.has_value() && !decl->has_annotation(AnnotationKind::CompTime)) {
        decl->body.value().accept(this);
    }
}

void CValueDeclarationVisitor::visit(EnumDeclaration *enumDecl) {
    if(visitor->inline_enum_member_access) return;
    unsigned i = 0;
    for(auto& mem : enumDecl->members) {
        visitor->new_line_and_indent();
        std::string value = ("__CHENUM_");
        value += (std::to_string(enum_num++));
        value += ('_');
        value += (enumDecl->name);
        value += (std::to_string(random(100, 999)));
        value += ("_");
        value += (mem.first);
        write("#define ");
        write(value);
        write(' ');
        write(std::to_string(mem.second->index));
        aliases[mem.second.get()] = value;
        i++;
    }
}

void CTopLevelDeclarationVisitor::visit(TypealiasStatement *stmt) {
    if(stmt->has_annotation(AnnotationKind::NotInC)) {
        return;
    }
    visitor->new_line_and_indent();
    write("typedef ");
    stmt->actual_type->accept(visitor);
    write(' ');
    node_parent_name(visitor, stmt);
    write(stmt->identifier);
    write(';');
}

void CTopLevelDeclarationVisitor::visit(UnionDef *def) {
    visitor->new_line_and_indent();
    write("union ");
    node_parent_name(visitor, def);
    write(def->name);
    write(" {");
    visitor->indentation_level+=1;
    for(auto& var : def->variables) {
        visitor->new_line_and_indent();
        var.second->accept(visitor);
    }
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    write("};");
    for(auto& func : def->functions()) {
        declare_contained_func(this, func.get(), def->name + func->name, false);
    }
}

void CTopLevelDeclarationVisitor::visit(Namespace *ns) {
    if(ns->has_annotation(AnnotationKind::CompTime)) return;
    for(auto& node : ns->nodes) {
        node->accept(this);
    }
}

void CTopLevelDeclarationVisitor::declare_struct_def_only(StructDefinition* def, bool do_check) {
    if(do_check) {
        auto found = declared_nodes.find(def);
        if (found != declared_nodes.end()) {
            if(found->second.struct_def.declared_struct) {
                return;
            } else {
                found->second.struct_def.declared_struct = true;
            }
        } else {
            auto& c = declared_nodes[def];
            c.struct_def.iterations_done = 0;
            c.struct_def.declared_struct = true;
        }
    }
    for(auto& mem : def->variables) {
        mem.second->accept(value_visitor);
    }
    visitor->new_line_and_indent();
    write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    write(" {");
    visitor->indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits->type->linked_struct_def();
        if(struct_def) {
            visitor->new_line_and_indent();
            visitor->write("struct ");
            struct_name(visitor, struct_def);
            visitor->space();
            visitor->write(struct_def->name);
            visitor->write(';');
        }
    }
    for(auto& var : def->variables) {
        visitor->new_line_and_indent();
        var.second->accept(visitor);
    }
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    write("};");
}

void CTopLevelDeclarationVisitor::declare_struct(StructDefinition* def, bool check_declared) {
    // no need to forward declare struct when inlining function types
    if(!visitor->inline_struct_members_fn_types) {
        // forward declaring struct for function types that take a pointer to it
        // typedefing struct before usage
        //    visitor->new_line_and_indent();
        //    write("typedef struct ");
        //    node_parent_name(visitor, def);
        //    struct_name(visitor, def);
        //    write(' ');
        //    node_parent_name(visitor, def);
        //    struct_name(visitor, def);
        //    write(';');
    }
    declare_struct_def_only(def, check_declared);
    if(def->requires_destructor() && def->destructor_func() == nullptr) {
        auto decl = def->create_destructor();
        decl->ensure_destructor(def);
    }
    for(auto& func : def->functions()) {
        if(def->get_overriding_interface(func.get()) == nullptr) {
            declare_contained_func(this, func.get(), struct_name_str(visitor, def) + func->name, false);
        }
    }
}

void CTopLevelDeclarationVisitor::visit(StructDefinition* def) {
    if(def->generic_params.empty()) {
        declare_struct(def, true);
    } else {
        int16_t itr = 0;
        // if previously we generated definition, we must start at the definition not done yet
        // this can happen if generic was present in other file, and we generated all implementations
        // however in this file we found another implementation that hasn't been generated before
        auto found = declared_nodes.find(def);
        if(found != declared_nodes.end()) {
            itr = found->second.struct_def.iterations_done;
        }
        const auto total = def->total_generic_iterations();
        while(itr < total) {
            def->set_active_iteration(itr);
            declare_struct(def, false);
            itr++;
        }
    }
}

void CTopLevelDeclarationVisitor::declare_variant(VariantDefinition* def) {
    // no need to forward declare struct when inlining function types
    if(!visitor->inline_struct_members_fn_types) {
        // forward declaring struct for function types that take a pointer to it
        // typedefing struct before usage
        //    visitor->new_line_and_indent();
        //    write("typedef struct ");
        //    node_parent_name(visitor, def);
        //    struct_name(visitor, def);
        //    write(' ');
        //    node_parent_name(visitor, def);
        //    struct_name(visitor, def);
        //    write(';');
    }
    for(auto& mem : def->variables) {
        mem.second->accept(value_visitor);
    }
    visitor->new_line_and_indent();
    write("struct ");
    node_parent_name(visitor, def);
    struct_name(visitor, def);
    write(" {");
    visitor->indentation_level+=1;
    for(auto& inherits : def->inherited) {
        const auto struct_def = inherits->type->linked_struct_def();
        if(struct_def) {
            visitor->new_line_and_indent();
            visitor->write("struct ");
            struct_name(visitor, struct_def);
            visitor->space();
            visitor->write(struct_def->name);
            visitor->write(';');
        }
    }
    new_line_and_indent();
    write("int ");
    write(variant_type_variant_name);
    write(';');
    new_line_and_indent();
    write("union {");
    visitor->indentation_level += 1;
    for(auto& var : def->variables) {
        visitor->new_line_and_indent();
        const auto member = var.second->as_variant_member();

        visitor->write("struct ");
        visitor->write('{');
        visitor->indentation_level += 1;
        for(auto& value : member->values) {
            visitor->new_line_and_indent();
            value.second->type->accept(visitor);
            visitor->space();
            visitor->write(value.second->name);
            visitor->write(';');
        }
        visitor->indentation_level -= 1;
        visitor->new_line_and_indent();
        visitor->write("} ");
        visitor->write(member->name);
        visitor->write(';');

    }
    visitor->indentation_level -= 1;
    new_line_and_indent();
    write("};");
    visitor->indentation_level-=1;
    new_line_and_indent();
    write("};");
    if(def->requires_destructor() && def->destructor_func() == nullptr) {
        // TODO create destructor
//        auto decl = def->create_destructor();
//        decl->ensure_destructor(def);
    }
    for(auto& func : def->functions()) {
//        if(def->get_overriding_interface(func.get()) == nullptr) {
            declare_contained_func(this, func.get(), struct_name_str(visitor, def) + func->name, false);
//        }
    }
}

void CTopLevelDeclarationVisitor::visit(VariantDefinition *variant_def) {
    if(variant_def->generic_params.empty()) {
        declare_variant(variant_def);
    } else {
        int16_t itr = 0;
        // if previously we generated definition, we must start at the definition not done yet
        // this can happen if generic was present in other file and we generated all implementations
        // however in this file we found another implementation that hasn't been generated before
        auto found = declared_nodes.find(variant_def);
        if(found != declared_nodes.end()) {
            itr = found->second.variant_def.iterations_done;
        }
        const auto total = variant_def->total_generic_iterations();
        while(itr < total) {
            variant_def->set_active_iteration(itr);
            // early declare contained structs
            for(auto& mem : variant_def->variables) {
                auto& mem_vals = mem.second->as_variant_member()->values;
                for(auto& val : mem_vals) {
                    const auto str = val.second->type->pure_type()->get_direct_ref_struct();
                    if(str) {
                        declare_struct_def_only(str, true);
                    }
                }
            }
            declare_variant(variant_def);
            itr++;
        }
    }
}

void create_v_table(ToCAstVisitor* visitor, InterfaceDefinition* interface, StructDefinition* definition) {
    visitor->new_line_and_indent();
    visitor->write("const");
    visitor->write(' ');
    visitor->write("struct");
    visitor->space();
    visitor->write('{');
    visitor->indentation_level += 1;

    // type pointers
    for(auto& func : interface->functions()) {
        auto self_param = func->get_self_param();
        if(self_param) {
            visitor->new_line_and_indent();
            func_type_with_id_no_params(visitor, func.get(), func->name);
            visitor->write("struct ");
            node_name(visitor, definition);
            visitor->write('*');
            visitor->space();
            visitor->write(self_param->name);
            func_type_params(visitor, func.get(), 1, true);
            visitor->write(')');
            visitor->write(';');
        }
    }

    visitor->indentation_level -=1;
    visitor->new_line_and_indent();
    visitor->write('}');
    visitor->space();
    vtable_name(visitor, interface, definition);
    visitor->write(" = ");
    visitor->write('{');
    visitor->indentation_level += 1;

    // func pointer values
    for(auto& func : interface->functions()) {
        if(func->has_self_param()) {
            visitor->new_line_and_indent();
            visitor->write(definition->name);
            visitor->write(func->name);
            visitor->write(',');
        }
    }

    visitor->indentation_level -=1;
    visitor->new_line_and_indent();
    visitor->write('}');
    visitor->write(';');
}

void CTopLevelDeclarationVisitor::visit(InterfaceDefinition *def) {
    for(auto& use : def->users) {
        new_line_and_indent();
        write("struct ");
        node_name(visitor, use.first);
        write(';');
    }
    for (auto& func: def->functions()) {
        if(!func->has_self_param()) {
            declare_contained_func(this, func.get(), def->name + func->name, false);
        }
    }
    for(auto& use : def->users) {
        for (auto& func: def->functions()) {
            if(func->has_self_param()) {
                declare_contained_func(this, func.get(), use.first->name + func->name, false, use.first);
            }
        }
    }
    for(auto& user : def->users) {
        const auto linked_struct = user.first;
        if(linked_struct) {
            create_v_table(visitor, def, linked_struct);
        }
    }
}

void CTopLevelDeclarationVisitor::visit(ImplDefinition *def) {

}

void CTopLevelDeclarationVisitor::reset() {
//    declared_nodes.clear();
}

void CValueDeclarationVisitor::visit(TypealiasStatement *stmt) {
    if(is_top_level_node) return;
    visitor->new_line_and_indent();
    write("typedef ");
    stmt->actual_type->accept(visitor);
    write(' ');
    std::string alias = "__chalias_";
    alias += std::to_string(random(100,999)) + "_";
    alias += std::to_string(alias_num++);
    write(alias);
    aliases[stmt] = alias;
    write(';');
}

void CValueDeclarationVisitor::visit(FunctionType *type) {
    if(type->isCapturing) {
        return;
    }
    typedef_func_type(visitor, type);
}

void CValueDeclarationVisitor::visit(StructMember *member) {
    if(!visitor->inline_struct_members_fn_types || member->type->kind() != BaseTypeKind::Function) {
        CommonVisitor::visit(member);
    }
}

void declare_fat_pointer(ToCAstVisitor* visitor) {
    visitor->fat_pointer_type = "__chemical_fat_pointer__";
    visitor->write("typedef struct {");
    visitor->indentation_level+=1;
    visitor->new_line_and_indent();
    visitor->write("void* first;");
    visitor->new_line_and_indent();
    visitor->write("void* second;");
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write("} ");
    visitor->write(visitor->fat_pointer_type);
    visitor->write(';');
}

void ToCAstVisitor::prepare_translate() {
    write("#include <stdbool.h>\n");
    write("#include <stddef.h>\n");
    // declaring a fat pointer
    declare_fat_pointer(this);
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
    tld->reset();
    before_stmt->reset();
    after_stmt->reset();
    destructor->reset();
    tld->reset();
}

ToCAstVisitor::~ToCAstVisitor() = default;

void ToCAstVisitor::visitCommon(ASTNode *node) {
#ifdef DEBUG
    throw std::runtime_error("visitor common node called in 2c ASTVisitor");
#endif
}

void ToCAstVisitor::visitCommonValue(Value *value) {
#ifdef DEBUG
    throw std::runtime_error("visitor common value called in 2c ASTVisitor");
#endif
}

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

void ToCAstVisitor::write(const std::string& value) {
    output->write(value.c_str(), value.size());
}

std::string ToCAstVisitor::string_accept(ASTAny* any) {
    std::ostringstream stream;
    auto curr_out = output;
    output = &stream;
    any->accept(this);
    output = curr_out;
    return stream.str();
}

void ToCAstVisitor::visit(VarInitStatement *init) {
    if(top_level_node) return;
    var_init(this, init, false);
    init->accept(destructor.get());
}

void ToCAstVisitor::visit(AssignStatement *assign) {
    assign_statement(this, assign);
    write(';');
}

void write_assignable(ToCAstVisitor* visitor, ASTNode* node) {
    const auto k = node->kind();
    switch(k) {
        case ASTNodeKind::VarInitStmt:
            visitor->write(node->as_var_init()->identifier);
            return;
        case ASTNodeKind::AssignmentStmt:
            node->as_assignment()->lhs->accept(visitor);
            return;
        default:
            const auto p = node->parent();
            if(p) {
                write_assignable(visitor, p);
            } else {
                visitor->write("[UnknownAssignable]");
            }
            return;
    }
}

void ToCAstVisitor::visit(BreakStatement *node) {
    if(node->value) {
        auto val_kind = node->value->val_kind();
        if(val_kind != ValueKind::SwitchValue && val_kind != ValueKind::IfValue && val_kind != ValueKind::LoopValue) {
            write_assignable(this, node->parent_node);
            write(" = ");
        }
        auto prev = nested_value;
        nested_value = true;
        node->value->accept(this);
        nested_value = prev;
        write(';');
        new_line_and_indent();
    }
    write("break;");
}

void ToCAstVisitor::visit(Comment *comment) {
    // leave comments alone
}

void ToCAstVisitor::visit(ContinueStatement *continueStatement) {
    write("continue;");
}

void ToCAstVisitor::visit(ImportStatement *importStatement) {
    // leave imports alone
}

void struct_initialize_inside_braces(ToCAstVisitor* visitor, StructValue* val) {
    visitor->write("(struct ");
    visitor->write(val->definition->name);
    visitor->write(")");
    val->accept(visitor);
}

bool ToCAstVisitor::requires_return(Value* val) {
    if(val->as_struct()) {
        if(pass_structs_to_initialize) {
            return false;
        } else {
            return true;
        }
    } else if(val->value_type() == ValueType::Struct) {
        return false;
    } else {
        return true;
    }
}

void ToCAstVisitor::return_value(Value* val, BaseType* type) {
    if(val->as_struct()) {
        if(implicit_mutate_value_default(this, type, val)) {
           return;
        }
        if(pass_structs_to_initialize) {
            auto size = val->as_struct()->values.size();
            unsigned i = 0;
            for(const auto& mem : val->as_struct()->values) {
                write(struct_passed_param_name);
                write("->");
                write(mem.first);
                write(" = ");
                mem.second->accept(this);
                if(i != size - 1){
                    write(';');
                    new_line_and_indent();
                }
                i++;
            }
        } else {
            struct_initialize_inside_braces(this, (StructValue*) val);
        }
    } else if(val->value_type() == ValueType::Struct) {
//        auto refType = val->create_type();
//        auto structType = refType->linked_node()->as_struct_def();
//        auto size = structType->variables.size();
        write('*');
        write(struct_passed_param_name);
        write(" = ");
        if(!implicit_mutate_value_default(this, type, val)) {
            const auto id = val->as_identifier();
            if(id) {
                const auto func_param = id->linked->as_func_param();
                if(func_param && (func_param->type->get_direct_ref_struct() || func_param->type->get_direct_ref_variant())) {
                    write('*');
                }
            }
            val->accept(this);
        }
    } else {
        accept_mutating_value(type, val);
    }
}

void ToCAstVisitor::visit(ReturnStatement *returnStatement) {
    Value* val = nullptr;
    if(returnStatement->value) {
        val = returnStatement->value.get();
    }
    const auto return_type  =returnStatement->func_type->returnType.get();
    std::string saved_into_temp_var;
    bool handle_return_after = true;
    if(val && !requires_return(val)) {
        return_value(val, return_type);
        write(';');
        handle_return_after = false;
    } else if(val && !val->primitive() && !destructor->destruct_jobs.empty()) {
        saved_into_temp_var = "__chx_ret_val_res";
        write("const ");
        auto type = val->get_base_type();
        type->accept(this);
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
    auto current_return = returnStatement->value ? returnStatement->value.get() : nullptr;
    while(i >= 0) {
        destructor->destruct(destructor->destruct_jobs[i], current_return);
        i--;
    }
    destructor->new_line_before = new_line_prev;
    destructor->destroy_current_scope = false;
    if(returnStatement->value) {
        if(handle_return_after) {
            write("return ");
            if(!saved_into_temp_var.empty()) {
                write(saved_into_temp_var);
            } else {
                return_value(val, return_type);
            }
            write(';');
        }
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

void ToCAstVisitor::visit(DoWhileLoop *doWhileLoop) {
    write("do ");
    scope(this, doWhileLoop->body);
    write(" while(");
    doWhileLoop->condition->accept(this);
    write(");");
}

void ToCAstVisitor::visit(EnumDeclaration *enumDecl) {

}

void ToCAstVisitor::visit(ForLoop *forLoop) {
    write("for(");
    forLoop->initializer->accept(this);
    forLoop->conditionExpr->accept(this);
    write(';');
    if(forLoop->incrementerExpr->as_assignment() != nullptr) {
        assign_statement(this, forLoop->incrementerExpr->as_assignment());
    } else {
        forLoop->incrementerExpr->accept(this);
    }
    write(')');
    scope(this, forLoop->body);
}

void ToCAstVisitor::visit(FunctionParam *functionParam) {
    write("[FunctionParam_UNIMPLEMENTED]");
}

void func_decl_with_name(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name) {
    if(!decl->body.has_value() || decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    auto prev_func_decl = visitor->current_func_type;
    visitor->current_func_type = decl;
    visitor->new_line_and_indent();
    if(visitor->inline_fn_types_in_returns && decl->returnType->function_type() && !decl->returnType->function_type()->isCapturing) {
        func_that_returns_func_proto(visitor, decl, name, decl->returnType->function_type());
    } else {
        declare_func_with_return(visitor, decl, name);
    }
    scope(visitor, decl->body.value(), decl);
    visitor->current_func_type = prev_func_decl;
}

void func_decl_with_name(ToCAstVisitor* visitor, FunctionDeclaration* decl) {
    if(!decl->generic_params.empty()) {
        int16_t itr = 0;
        int16_t total = decl->total_generic_iterations();
        while(itr < total) {
            decl->set_active_iteration(itr);
            func_decl_with_name(visitor, decl, decl->name);
            itr++;
        }
        // set to -1, so whoever tries to access generic parameters types, they receive an error if active iteration is not set again
        decl->set_active_iteration(-1);
    } else {
        func_decl_with_name(visitor, decl, decl->name);
    }
}

void contained_func_decl(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name, bool overrides, ExtendableMembersContainerNode* def) {
    if(!decl->body.has_value() || decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    auto prev_func_decl = visitor->current_func_type;
    visitor->current_func_type = decl;
    visitor->new_line_and_indent();
//    std::string self_pointer_name;
    FunctionParam* param = !decl->params.empty() ? decl->params[0].get() : nullptr;
    unsigned i = 0;
    auto write_self_param_now = [decl, visitor, param, &i, overrides, def]() {
        if(param && should_void_pointer_to_self(param->type.get(), param->name, 0, overrides)) {
//            self_pointer_name = "__ch_self_pointer_329283";
//            visitor->write("void* ");
            visitor->write("struct ");
            node_name(visitor, def);
            visitor->write('*');
            visitor->space();
            visitor->write(param->name);
//            visitor->write(self_pointer_name);
            if(decl->params.size() > 1) {
                visitor->write(", ");
            }
            i = 1;
        }
    };
    if(visitor->inline_fn_types_in_returns && decl->returnType->function_type() != nullptr && !decl->returnType->function_type()->isCapturing) {
        visitor->write("static ");
        accept_func_return(visitor, decl->returnType->function_type()->returnType.get());
        visitor->write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(visitor, decl, name, decl->returnType->function_type(), i);
    } else {
        accept_func_return_with_name(visitor, decl, name, true, decl->body.has_value() && !decl->is_exported());
        visitor->write('(');
        write_self_param_now();
        func_type_params(visitor, decl, i);
        visitor->write(')');
    }
    visitor->write('{');
    visitor->indentation_level+=1;
//    if(!self_pointer_name.empty() && def) {
//        visitor->new_line_and_indent();
//        visitor->write("struct ");
//        struct_name(visitor, def);
//        visitor->write('*');
//        visitor->space();
//        visitor->write("self = ");
//        visitor->write(self_pointer_name);
//        visitor->write(';');
//    }
    auto is_destructor = decl->has_annotation(AnnotationKind::Destructor);
    std::string cleanup_block_name;
    if(is_destructor) {
        cleanup_block_name = "__chx__dstctr_clnup_blk__";
        visitor->return_redirect_block = cleanup_block_name;
    }
    unsigned begin = visitor->destructor->destruct_jobs.size();
    visitor->destructor->queue_destruct_decl_params(decl);
    visitor->visit_scope(&decl->body.value(), (int) begin);
    if(is_destructor) {
        visitor->new_line_and_indent();
        visitor->write(cleanup_block_name);
        visitor->write(":{");
        unsigned index = 0;
        visitor->indentation_level++;
        const auto struc_def = def->as_struct_def();
        const auto variant_def = def->as_variant_def();
        if(struc_def) {
            for (auto& var: def->variables) {
                if (var.second->value_type() == ValueType::Struct) {
                    auto mem_type = var.second->get_value_type();
                    const auto linked = mem_type->linked_node();
                    auto mem_def = linked->as_members_container();
                    auto destructor = mem_def->destructor_func();
                    if (!destructor) {
                        index++;
                        continue;
                    }
                    visitor->new_line_and_indent();
                    func_container_name(visitor, mem_def, destructor);
                    visitor->write(destructor->name);
                    visitor->write('(');
                    if (destructor->has_self_param()) {
                        visitor->write("&self->");
                        visitor->write(var.second->name);
                    }
                    visitor->write(')');
                    visitor->write(';');
                }
                index++;
            }
        } else if(variant_def) {
            visitor->new_line_and_indent();
            visitor->write("switch(self->");
            visitor->write(variant_type_variant_name);
            visitor->write(") {");
            visitor->indentation_level += 1;
            for (auto& var: def->variables) {
                visitor->new_line_and_indent();
                visitor->write("case ");
                visitor->write(std::to_string(index));
                visitor->write(':');
                const auto member = var.second->as_variant_member();
                for(auto& mem_param : member->values) {
                    auto mem_type = mem_param.second->type.get();
                    const auto linked = mem_type->linked_node();
                    auto mem_def = linked->as_members_container();
                    auto destructor = mem_def->destructor_func();
                    if (!destructor) {
                        index++;
                        continue;
                    }
                    visitor->new_line_and_indent();
                    func_container_name(visitor, mem_def, destructor);
                    visitor->write(destructor->name);
                    visitor->write('(');
                    if (destructor->has_self_param()) {
                        visitor->write("&self->");
                        visitor->write(var.second->name);
                        visitor->write('.');
                        visitor->write(mem_param.first);
                    }
                    visitor->write(')');
                    visitor->write(';');
                }
                visitor->new_line_and_indent();
                visitor->write("break;");
                index++;
            }
            visitor->indentation_level -= 1;
            visitor->new_line_and_indent();
            visitor->write('}');
        }
        visitor->indentation_level--;
        visitor->new_line_and_indent();
        visitor->write("}");
        visitor->return_redirect_block = "";
    }
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
    visitor->current_func_type = prev_func_decl;
}

void ToCAstVisitor::visit(FunctionDeclaration *decl) {
    func_decl_with_name(this, decl);
}

void ToCAstVisitor::visit(ExtensionFunction *decl) {
    func_decl_with_name(this, decl);
}

void ToCAstVisitor::visit(IfStatement *decl) {
    write("if(");
    nested_value = true;
    decl->condition->accept(this);
    nested_value = false;
    write(')');
    scope(this, decl->ifBody);
    unsigned i = 0;
    while(i < decl->elseIfs.size()) {
        auto& elif = decl->elseIfs[i];
        write("else if(");
        elif.first->accept(this);
        write(')');
        scope(this, elif.second);
        i++;
    }
    if(decl->elseBody.has_value()) {
        write(" else ");
        scope(this, decl->elseBody.value());
    }
}

void ToCAstVisitor::visit(ImplDefinition *def) {
    const auto& interface_name = def->interface_type->ref_name();
    const auto overrides = def->struct_type != nullptr;
    const auto& struct_or_interface_name = def->struct_type ? def->struct_type->ref_name() : def->interface_type->ref_name();
    const auto linked_interface = def->interface_type->linked_node()->as_interface_def();
    const auto linked_struct = def->struct_type ? def->struct_type->linked_struct_def() : nullptr;
    for(auto& func : def->functions()) {
        if(func->has_self_param()) {
            contained_func_decl(this, func.get(), struct_or_interface_name + func->name, overrides,linked_struct);
        } else {
            contained_func_decl(this, func.get(), interface_name + func->name, overrides,linked_struct);
        }
    }
}

void ToCAstVisitor::visit(InterfaceDefinition *def) {

}

void ToCAstVisitor::visit_scope(Scope *scope, unsigned destruct_begin) {
    auto prev = top_level_node;
    top_level_node = false;
    for(auto& node : scope->nodes) {
        new_line_and_indent();
        node->accept(before_stmt.get());
        node->accept(this);
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

void ToCAstVisitor::visit(Scope *scope) {
    visit_scope(scope, destructor->destruct_jobs.size());
}

void ToCAstVisitor::visit(UnnamedUnion *def) {
    write("union ");
    write('{');
    indentation_level+=1;
    for(auto& var : def->variables) {
        new_line_and_indent();
        var.second->accept(this);
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
    space();
    write(def->name);
    write(';');
}

void ToCAstVisitor::visit(UnnamedStruct *def) {
    write("struct ");
    write('{');
    indentation_level+=1;
    for(auto& var : def->variables) {
        new_line_and_indent();
        var.second->accept(this);
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
    space();
    write(def->name);
    write(';');
}

static void contained_struct_functions(ToCAstVisitor* visitor, StructDefinition* def) {
    for(auto& func : def->functions()) {
        const auto interface = def->get_overriding_interface(func.get());
        const auto& parent_name = func->has_self_param() ? struct_name_str(visitor, def) : (interface ? interface->name : struct_name_str(visitor, def));
        if(interface) {
            contained_func_decl(visitor, func.get(), parent_name + func->name, true, def);
        } else {
            // overridding a function in base struct <--- not yet supported
            contained_func_decl(visitor, func.get(), parent_name + func->name, false, def);
        }
    }
}

void ToCAstVisitor::visit(StructDefinition *def) {
    auto prev_members_container = current_members_container;
    current_members_container = def;
    for(auto& inherits : def->inherited) {
        const auto overridden = inherits->type->linked_node()->as_interface_def();
        if(overridden) {
            for (auto &func: overridden->functions()) {
                if (!def->contains_func(func->name)) {
                    const auto& parent_name = func->has_self_param() ? def->name : overridden->name;
                    contained_func_decl(this, func.get(), parent_name + func->name, false, def);
                }
            }
        }
    }
    if(def->generic_params.empty()) {
        contained_struct_functions(this, def);
    } else {
        const auto total_itr = def->total_generic_iterations();
        if(total_itr == 0) return; // generic type never used (yet)
        int16_t itr = 0;
        // if previously we generated function bodies, we must start at the functions not done yet
        // this can happen if generic was present in other file and we generated all implementations
        // however in this file we found another implementation that hasn't been generated before
        auto found = tld->declared_nodes.find(def);
        if(found != tld->declared_nodes.end()) {
            itr = found->second.struct_def.iterations_done;
        }
        auto prev = def->active_iteration;
        while (itr < total_itr) {
            def->set_active_iteration(itr);
            contained_struct_functions(this, def);
            itr++;
        }
        def->set_active_iteration(prev);
        tld->declared_nodes[def].struct_def.iterations_done = total_itr;
    }
    current_members_container = prev_members_container;
}

void generate_contained_functions(ToCAstVisitor* visitor, VariantDefinition* def) {
    for(auto& func : def->functions()) {
        contained_func_decl(visitor, func.get(), struct_name_str(visitor, def) + func->name, false, def);
    }
}

void ToCAstVisitor::visit(VariantDefinition* def) {
    if(def->generic_params.empty()) {
        generate_contained_functions(this, def);
    } else {
        const auto total_itr = def->total_generic_iterations();
        if(total_itr == 0) return; // generic type never used (yet)
        int16_t itr = 0;
        // if previously we generated bodies for this function, we must start at the function not done yet
        // this can happen if generic was present in other file and we generated all implementations
        // however in this file we found another implementation that hasn't been generated before
        auto found = tld->declared_nodes.find(def);
        if(found != tld->declared_nodes.end()) {
            itr = found->second.variant_def.iterations_done;
        }
        auto prev = def->active_iteration;
        while (itr < total_itr) {
            def->set_active_iteration(itr);
            generate_contained_functions(this, def);
            itr++;
        }
        def->set_active_iteration(prev);
        tld->declared_nodes[def].variant_def.iterations_done = total_itr;
    }

}

void ToCAstVisitor::visit(UnionDef *def) {
    for(auto& func : def->functions()) {
        contained_func_decl(this, func.get(), def->name + func->name, false, def);
    }
}

void ToCAstVisitor::visit(Namespace *ns) {
    for(auto& node : ns->nodes) {
        node->accept(this);
    }
}

void ToCAstVisitor::visit(DestructStmt *stmt) {

    bool new_line_before = true;

    auto data = stmt->get_data();

    auto self_name = string_accept(stmt->identifier.get());
    IntValue siz_val(data.array_size, nullptr);

    if(stmt->is_array) {
        destructor->destruct_arr_ptr(self_name, data.array_size != -1 ? &siz_val : stmt->array_value.get(), data.parent_node, 0, data.destructor_func);
    } else {
        destructor->destruct(self_name, data.parent_node, 0, data.destructor_func, true);
    }
}

void ToCAstVisitor::visit(IsValue *isValue) {
    bool result = false;
    auto comp_time = isValue->get_comp_time_result();
    if(comp_time.has_value()) {
        result = comp_time.value();
    }
    write(result ? '1' : '0');
}

void ToCAstVisitor::visit(WhileLoop *whileLoop) {
    write("while(");
    whileLoop->condition->accept(this);
    write(") ");
    scope(this, whileLoop->body);
}

void ToCAstVisitor::visit(LoopBlock *loop) {
    write("while(1)");
    scope(this, loop->body);
}

void ToCAstVisitor::visit(VariantCase *variant_case) {
    const auto member = variant_case->chain->linked_node()->as_variant_member();
    write(std::to_string(member->parent_node->direct_child_index(member->name)));
}

void ToCAstVisitor::visit(VariantCall *call) {
    auto found = local_allocated.find(call);
    if(found == local_allocated.end()) {
        write("[VariantCallNotAllocated]");
    } else {
        write(found->second);
    }
}

template<typename current_call>
void capture_call(ToCAstVisitor* visitor, FunctionType* type, current_call call, FunctionCall* func_call) {
    visitor->write('(');
    visitor->write('(');
    visitor->write('(');
    func_type_with_id(visitor, type, "");
    visitor->write(") ");
    call();
    visitor->write("->first");
    visitor->write(')');
    visitor->write('(');
    call();
    visitor->write("->second");
    if(!func_call->values.empty()) {
        visitor->write(',');
    }
    func_call_args(visitor, func_call, type);
    visitor->write(')');
    visitor->write(')');
}

void func_call(ToCAstVisitor* visitor, FunctionType* type, std::unique_ptr<Value>& current, std::unique_ptr<Value>& next, unsigned int& i) {
    if(type->isCapturing && current->as_func_call() == nullptr) {
        capture_call(visitor, type, [&current, visitor](){ current->accept(visitor); }, next->as_func_call());
        i++;
    } else {
        current->accept(visitor);
        func_call(visitor, next->as_func_call(), type);
        i++;
    }
}

// this automatically determines which parent to pass through
void func_container_name(ToCAstVisitor* visitor, FunctionDeclaration* func_node) {
    const auto parent = func_node->parent();
    if(parent) {
        const auto struct_parent = parent->as_struct_def();
        const auto impl_parent = parent->as_impl_def();
        if(impl_parent) {
            const auto interface_def = impl_parent->interface_type->linked_node()->as_interface_def();
            node_parent_name(visitor, struct_parent);
            visitor->write(interface_def->name);
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
                visitor->write(interface->name);
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
void func_container_name(ToCAstVisitor* visitor, ASTNode* parent_node, ASTNode* linked_node) {
    node_parent_name(visitor, parent_node);
    if(linked_node && linked_node->as_extension_func()) {
        return;
    }
    if(!parent_node) return;
    const auto func_parent = linked_node->parent();
    const auto impl_def = func_parent->as_impl_def();
    if(impl_def) {
        if(impl_def->struct_type) {
            visitor->write(impl_def->struct_type->linked_node()->as_struct_def()->name);
        } else {
            visitor->write(impl_def->interface_type->linked_node()->as_interface_def()->name);
        }
    } else if(parent_node->as_interface_def()) {
//        const auto func_node = linked_node->as_function();
        visitor->write(parent_node->as_interface_def()->name);
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

class FuncGenericItrResetter {
public:
    FunctionDeclaration* decl;
    int16_t iteration;
    ~FuncGenericItrResetter() {
        if(decl) {
            decl->set_active_iteration(iteration);
        }
    }
};

void func_call(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, unsigned end) {
    auto last = values[end - 1]->as_func_call();
    auto func_decl = last->safe_linked_func();
    auto parent = values[end - 2].get();
    if(func_decl && func_decl->has_annotation(AnnotationKind::CompTime)) {
        evaluate_func(visitor, visitor, func_decl, last);
        return;
    }
    FuncGenericItrResetter resetter{};
    if(func_decl && !func_decl->generic_params.empty()) {
        resetter.decl = func_decl;
        resetter.iteration = func_decl->active_iteration;
        func_decl->set_active_iteration(last->generic_iteration);
    }
    auto grandpa = ((int) end) - 3 >= 0 ? values[end - 3].get() : nullptr;
    auto parent_type = parent->create_type();
    auto func_type = parent_type->function_type();
    bool is_lambda = (parent->linked_node() != nullptr && parent->linked_node()->as_struct_member() != nullptr);
    if(visitor->pass_structs_to_initialize && func_type->returnType->value_type() == ValueType::Struct) {
        // functions that return struct
        auto allocated = visitor->local_allocated.find(last);
        if(allocated == visitor->local_allocated.end()) {
            visitor->write("[NotFoundAllocated in func_call]");
            return;
        }
        visitor->write(allocated->second);
    } else if(func_type->isCapturing) {
        // function calls to capturing lambdas
        capture_call(visitor, func_type, [&](){
            visitor->nested_value = true;
            access_chain(visitor, values, start, end - 1);
            visitor->nested_value = false;
        }, last);
    } else if(grandpa && !grandpa->linked_node()->as_namespace()) {
        auto grandpaType = grandpa->create_type();
        auto pure_grandpa = grandpaType->pure_type();
        if(pure_grandpa && pure_grandpa->kind() == BaseTypeKind::Dynamic) {
            const auto interface = pure_grandpa->linked_node()->as_interface_def();
            if(interface && !interface->users.empty()) {
                // Dynamic dispatch
                // ((typeof(PhoneSmartPhone)*) phone.second)->call(phone.first);
                auto first = interface->users.begin();
                auto first_def = first->first;
                visitor->write('(');
                visitor->write('(');
                visitor->write("typeof");
                visitor->write('(');
                vtable_name(visitor, interface, first_def);
                visitor->write(')');
                visitor->write('*');
                visitor->write(')');
                visitor->space();
                func_name_chain(visitor, values, start, end - 2);
                if(end - 2 - start == 1) {
                    visitor->write('.');
                }
                visitor->write("second");
                visitor->write(')');
                visitor->write("->");
                func_name(visitor, parent, func_decl);
                visitor->write('(');
                if(write_self_arg_bool(visitor, func_type, values, (((int) end) - 3), last, true)) {
                    visitor->write('.');
                    visitor->write("first");
                    if (!last->values.empty()) {
                        visitor->write(", ");
                    }
                }
                func_call_args(visitor, last->as_func_call(), func_type);
                visitor->write(')');
            } else {
                if(!interface) {
                    visitor->write("[Dynamic Dispatch used with type other than interface]");
                    visitor->error("Dynamic Dispatch used with a type other than interface", pure_grandpa->linked_node());
                } else {
                    visitor->write("[Dynamic Dispatch Interface has no known users]");
                }
            }
        } else if(grandpa->linked_node() && (grandpa->linked_node()->as_interface_def() || grandpa->linked_node()->as_struct_def())) {
            // direct functions on interfaces and structs
            func_container_name(visitor, grandpa->linked_node(), parent->linked_node());
            func_name(visitor, parent, func_decl);
            func_call(visitor, last->as_func_call(), func_type);
        } else if(grandpaType->linked_struct_def()) {
            if(parent->linked_node()->as_struct_member()) {
                goto normal_functions;
            }
            auto generic_struct = grandpaType->get_generic_struct();
            int16_t prev_iteration;
            if(generic_struct) {
                prev_iteration = generic_struct->active_iteration;
                generic_struct->set_active_iteration(grandpaType->get_generic_iteration());
            }
            // functions on struct values
            func_container_name(visitor, grandpaType->linked_node(), parent->linked_node());
            func_name(visitor, parent, func_decl);
            visitor->write('(');
            write_self_arg(visitor, func_type, values, (((int) end) - 3), last);
            func_call_args(visitor, last->as_func_call(), func_type);
            visitor->write(')');
            if(generic_struct) {
                generic_struct->set_active_iteration(prev_iteration);
            }
        } else {
            goto normal_functions;
        }
    } else {
        goto normal_functions;
    }
    return;
    normal_functions: {
        // normal functions
        auto linked_node = parent->linked_node();
        auto as_func_decl = linked_node ? linked_node->as_function() : nullptr;
        if(func_decl) {
            func_container_name(visitor, func_decl);
        } else {
            func_name_chain(visitor, values, start, end - 1);
        }
        visitor->write('(');
        if(func_type->has_self_param()) {
            if(grandpa) {
                if (chain_contains_func_call(values, start, end - 2)) {
                    visitor->error("Function call inside a access chain with lambda that requires self is not allowed", values[start].get());
                    return;
                }
                if (end - 3 >= 0 && values[end - 3]->value_type() != ValueType::Pointer) {
                    visitor->write('&');
                }
                access_chain(visitor, values, start, end - 2, end - 2);
            } else if(visitor->current_func_type && !is_lambda) {
                auto self_arg = visitor->current_func_type->get_self_param();
                if(self_arg) {
                    visitor->write(self_arg->name);
                } else {
                    visitor->error("couldn't pass self arg where current function has none.", values[start].get());
                }
            }
            if (!last->values.empty()) {
                visitor->write(',');
            }
        }
        func_call_args(visitor, last, func_type);
        visitor->write(')');
//        if(!visitor->nested_value) {
//            visitor->write(';');
//        }
    };
}

void func_name_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, unsigned end) {
    access_chain(visitor, values, start, end, values.size());
}

void write_path_to_child(ToCAstVisitor* visitor, std::vector<int>& path, StructDefinition* def) {
    int i = 0;
    int last = (((int) path.size()) - 1);
    while(i < last) {
        const auto seg = path[i];
        auto& base = def->inherited[seg];
        def = base->type->linked_struct_def();
        visitor->write(def->name);
        visitor->write('.');
        i++;
    }
}

void chain_value_accept(ToCAstVisitor* visitor, ChainValue* previous, ChainValue* value) {
    const auto linked = value->linked_node();
    if(previous) {
        const auto prev_type = previous->get_pure_type();
        const auto previous_def = prev_type->linked_struct_def();
        if(previous_def) {
            const auto id = value->as_identifier();
            if (id) {
                const auto member = id->linked_node()->as_base_def_member();
                if (member) {
                    // current member name member->name;
                    const auto mem = previous_def->direct_child_member(member->name);
                    if(!mem) {
                        std::vector<int> path;
                        path.reserve(5);
                        auto found = previous_def->build_path_to_child(path, member->name);
                        if(found) {
                            write_path_to_child(visitor, path, previous_def);
                        }
                    }
                }
            }
        }
    }
    if(previous != nullptr && linked && linked->as_variant_case_var()) {
        const auto var = linked->as_variant_case_var();
        Value* expr = var->variant_case->switch_statement->expression.get();
        const auto var_mem = var->variant_case->chain->linked_node()->as_variant_member();
        expr->accept(visitor);
        write_accessor(visitor, expr, nullptr);
        visitor->write(var_mem->name);
        visitor->write('.');
    }
    value->accept(visitor);
}

void chain_after_func(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, unsigned start, const unsigned end, const unsigned total_size) {
    ChainValue* previous = nullptr;
    ChainValue* current = nullptr;
    ChainValue* next = nullptr;
    while(start < end) {
        previous = current;
        current = values[start].get();
        if(start + 1 < total_size) {
            next = values[start + 1].get();
            if(next->as_func_call() || next->as_index_op()) {
                chain_value_accept(visitor, previous, current);
            } else {
                if(current->linked_node()->as_enum_decl() != nullptr) {
                    if(visitor->inline_enum_member_access) {
                        visitor->write(std::to_string(next->linked_node()->as_enum_member()->index));
                        start++;
                    } else {
                        auto found = visitor->declarer->aliases.find(next->linked_node()->as_enum_member());
                        if (found != visitor->declarer->aliases.end()) {
                            visitor->write(found->second);
                            start++;
                        } else {
                            visitor->write("[EnumAC_NOT_FOUND:" + current->representation() + "." + next->representation() +"]");
                        }
                    }
                } else {
                    chain_value_accept(visitor, previous, current);
                    write_accessor(visitor, current, next);
                }
            }
        } else {
            chain_value_accept(visitor, previous, current);
            next = nullptr;
        }
        start++;
    }
}

void access_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<ChainValue>>& values, const unsigned start, const unsigned end, const unsigned total_size) {
    auto diff = end - start;
    if(diff == 0) {
        return;
    } else if(diff == 1) {
        chain_value_accept(visitor, nullptr, values[start].get());
        return;
    }
    unsigned i = start;
    // function call would be processed recursively
    {
        int j = end - 1;
        while(j >= 0) {
            auto& current = values[j];
            if(current->as_func_call()) {
                func_call(visitor, values, start, j + 1);
                if(j + 1 < end) {
                    write_accessor(visitor, values, j);
                    i = j + 1;
                } else {
                    if(!visitor->nested_value) {
                        visitor->write(';');
                    }
                    return;
                }
            }
            j--;
        }
    }
    chain_after_func(visitor, values, i, end, total_size);
}

void ToCAstVisitor::visit(AccessChain *chain) {
    access_chain(this, chain->values, 0, chain->values.size());
}

void ToCAstVisitor::visit(MacroValueStatement *statement) {
    write("[MacroValueStatement_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(StructMember *member) {
    if(inline_struct_members_fn_types && member->type->kind() == BaseTypeKind::Function) {
        if(member->type->function_type()->isCapturing) {
            write(fat_pointer_type);
            write('*');
            space();
            write(member->name);
        } else {
            func_type_with_id(this, member->type->function_type(), member->name);
        }
    } else {
        type_with_id(this, member->type.get(), member->name);
    }
    write(';');
}

void ToCAstVisitor::visit(TypealiasStatement *stmt) {
    // declared above
}

void ToCAstVisitor::visit(SwitchStatement *statement) {
    write("switch(");
    const auto known_t = statement->expression->known_type();
    if(known_t) {
        const auto linked = known_t->linked_node();
        if(linked) {
            const auto variant = linked->as_variant_def();
            if (variant) {
                statement->expression->accept(this);
                write_accessor(this, statement->expression.get(), nullptr);
                write(variant_type_variant_name);
            } else {
                statement->expression->accept(this);
            }
        } else {
            statement->expression->accept(this);
        }
    } else {
        statement->expression->accept(this);
    }
    write(") {");
    unsigned i = 0;
    indentation_level += 1;
    while(i < statement->scopes.size()) {
        auto& scope = statement->scopes[i];
        new_line_and_indent();

        write("case ");
        scope.first->accept(this);
        write(':');

        indentation_level += 1;
        scope.second.accept(this);
        new_line_and_indent();
        write("break;");
        indentation_level -= 1;
        i++;
    }
    if(statement->defScope.has_value()) {
        new_line_and_indent();
        write("default:");
        indentation_level +=1;
        statement->defScope.value().accept(this);
        new_line_and_indent();
        write("break;");
        indentation_level -=1;
    }
    indentation_level -= 1;
    new_line_and_indent();
    write('}');
}

void ToCAstVisitor::visit(TryCatch *statement) {
    write("[TryCatch_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(IntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(BigIntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(LongValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(ShortValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(UBigIntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(UIntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(ULongValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(UShortValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(Int128Value *val) {
    write(std::to_string(val->get_num_value()));
}

void ToCAstVisitor::visit(UInt128Value *val) {
    write(std::to_string(val->get_num_value()));
}

void ToCAstVisitor::visit(NumberValue *numValue) {
    write(std::to_string(numValue->get_num_value()));
}

void ToCAstVisitor::visit(FloatValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(DoubleValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(CharValue *val) {
    write('\'');
    write(escape_encode(val->value));
    write('\'');
}

void ToCAstVisitor::visit(StringValue *val) {
    write('"');
    write_encoded(this, val->value);
    write('"');
}

void ToCAstVisitor::visit(BoolValue *boolVal) {
    if(boolVal->value) {
        write('1');
    } else {
        write('0');
    }
}

void ToCAstVisitor::visit(ArrayValue *arr) {
    write('{');
    unsigned i = 0;
    auto prev = nested_value;
    nested_value = true;
    const auto elem_type = arr->element_type();
    while(i < arr->values.size()) {
        accept_mutating_value(elem_type.get(), arr->values[i].get());
        if(i != arr->values.size() - 1) {
            write(',');
        }
        i++;
    }
    nested_value = prev;
    write('}');
}

void ToCAstVisitor::visit(StructValue *val) {
    write('{');
    auto prev = nested_value;
    nested_value = true;
    for(auto& value : val->values) {
//        if(value.second->as_access_chain()) {
//            auto chain = value.second->as_access_chain();
//            auto call = chain->values.back()->as_func_call();
//            if(call && call->create_type()->value_type() == ValueType::Struct) {
//                continue;
//            }
//        }
        // we are only getting direct / inherited members, not inherited structs here
        const auto member = val->definition->child_member(value.first);
        write('.');
        write(value.first);
        write(" = ");
        accept_mutating_value(member ? member->known_type() : nullptr, value.second.get());
        write(", ");
    }
    nested_value = prev;
    write('}');
}

void ToCAstVisitor::visit(VariableIdentifier *identifier) {
    const auto linked = identifier->linked_node();
    if(linked->isAnyStructMember()) {
        if(identifier->parent_val == nullptr) {
            const auto func = current_func_type->as_function();
            if (func && func->has_annotation(AnnotationKind::Constructor)) {
                write("this->");
            }
        }
    } else if(linked->isCapturedVariable()) {
        auto found = declarer->aliases.find(linked->as_captured_var_unsafe());
        if(found == declarer->aliases.end()) {
            write("this->");
        } else {
            write("((struct ");
            write(found->second);
            write("*) this)->");
        }
    } else if(linked->isVariantCaseVariable()) {
        const auto var = linked->as_variant_case_var_unsafe();
        Value* expr = var->variant_case->switch_statement->expression.get();
        const auto var_mem = var->variant_case->chain->linked_node()->as_variant_member();
        expr->accept(this);
        write_accessor(this, expr, identifier);
        write(var_mem->name);
        write('.');
    }
    write(identifier->value);
}

void ToCAstVisitor::visit(SizeOfValue *size_of) {
//    if(use_sizeof) {
        write("sizeof");
        write('(');
        size_of->for_type->accept(this);
        write(')');
//    } else {
//        size_of->calculate_size(is64Bit);
//        size_of->UBigIntValue::accept(this);
//    }
}

void ToCAstVisitor::visit(Expression *expr) {
    write('(');
    auto prev_nested = nested_value;
    nested_value = true;
    expr->firstValue->accept(this);
    space();
    write(to_string(expr->operation));
    space();
    expr->secondValue->accept(this);
    nested_value = prev_nested;
    write(')');
}

void ToCAstVisitor::visit(CastedValue *casted) {
    write('(');
    write('(');
    casted->type->accept(this);
    write(')');
    write(' ');
    auto prev_nested = nested_value;
    nested_value = true;
    casted->value->accept(this);
    nested_value = prev_nested;
    write(')');
}

void ToCAstVisitor::visit(AddrOfValue *casted) {
    write('&');
    casted->value->accept(this);
}

void ToCAstVisitor::visit(RetStructParamValue *paramVal) {
    write(struct_passed_param_name);
}

void ToCAstVisitor::visit(DereferenceValue *casted) {
    write('*');
    casted->value->accept(this);
}

void ToCAstVisitor::visit(FunctionCall *call) {
    write("[ERROR:FunctionCall is part of access chain and it must be generated there]");
}

void ToCAstVisitor::visit(IndexOperator *op) {
    unsigned i = 0;
    while(i < op->values.size()) {
        write('[');
        auto& val = op->values[i];
        val->accept(this);
        write(']');
        i++;
    }
}

void ToCAstVisitor::visit(NegativeValue *negValue) {
    write('-');
    negValue->value->accept(this);
}

void ToCAstVisitor::visit(NotValue *notValue) {
    write('!');
    notValue->value->accept(this);
}

void ToCAstVisitor::visit(NullValue *nullValue) {
    write("NULL");
}

void ToCAstVisitor::visit(ValueNode *node) {
    auto val_kind = node->value->val_kind();
    if(val_kind != ValueKind::SwitchValue && val_kind != ValueKind::IfValue && val_kind != ValueKind::LoopValue) {
        write_assignable(this, node->parent_node);
        write(" = ");
    }
    auto prev = nested_value;
    nested_value = true;
    node->value->accept(this);
    nested_value = prev;
    write(';');
}

void ToCAstVisitor::visit(TernaryValue *ternary) {

}

void ToCAstVisitor::visit(LambdaFunction *func) {
    auto found = declarer->aliases.find(func);
    if(found != declarer->aliases.end()) {
        if(func->isCapturing) {
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

void ToCAstVisitor::visit(AnyType *func) {

}

void ToCAstVisitor::visit(LiteralType *literal) {
    literal->underlying->accept(this);
}

void ToCAstVisitor::visit(ArrayType *type) {
    type->elem_type->accept(this);
}

void ToCAstVisitor::visit(BigIntType *func) {
    write("long long");
}

void ToCAstVisitor::visit(BoolType *func) {
    if(cpp_like) {
        write("bool");
    } else {
        write("_Bool");
    }
}

void ToCAstVisitor::visit(CharType *func) {
    write("char");
}

void ToCAstVisitor::visit(DoubleType *func) {
    write("double");
}

void ToCAstVisitor::visit(FloatType *func) {
    write("float");
}

void ToCAstVisitor::visit(FunctionType *type) {
    if(type->isCapturing) {
        write(fat_pointer_type);
        write('*');
        return;
    }
    auto found = declarer->aliases.find(type);
    if(found != declarer->aliases.end()) {
        write(found->second);
    } else {
        func_type_with_id(this, type, "NOT_FOUND");
    }
}

void ToCAstVisitor::visit(GenericType *gen_type) {
    const auto gen_struct = gen_type->referenced->linked->as_members_container();
    const auto prev_itr = gen_struct->active_iteration;
    gen_struct->set_active_iteration(gen_type->generic_iteration);
    gen_type->referenced->accept(this);
    gen_struct->set_active_iteration(prev_itr);
}

void ToCAstVisitor::visit(Int128Type *func) {
    write("[Int128Type_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(IntType *func) {
    write("int");
}

void ToCAstVisitor::visit(UCharType *uchar) {
    write("unsigned char");
}

void ToCAstVisitor::visit(LongType *func) {
    if(is64Bit) {
        write("long long");
    } else {
        write("long");
    }
}

void ToCAstVisitor::visit(PointerType *func) {
    func->type->accept(this);
    write('*');
}

void ToCAstVisitor::visit(ReferencedType *type) {
    if(type->linked->as_interface_def()) {
        write("void");
        return;
    }
    if(type->linked->as_enum_decl()){
        write("int");
        return;
    }
    std::string name = type->type;
    if(type->linked->as_struct_def()) {
        write("struct ");
        name = struct_name_str(this, type->linked->as_struct_def());
    } else if(type->linked->as_variant_def()) {
        write("struct ");
        name = struct_name_str(this, type->linked->as_variant_def());
    } else if(type->linked->as_union_def()) {
        write("union ");
        name = type->linked->as_union_def()->name;
    }
    if(type->linked->as_generic_type_param()) {
        const auto gen = type->linked->as_generic_type_param();
        gen->get_value_type()->accept(this);
        return;
    }
    if(type->linked->as_typealias() != nullptr) {
        auto alias = declarer->aliases.find(type->linked->as_typealias());
        if(alias != declarer->aliases.end()) {
            write(alias->second);
            return;
        }
    }
    node_parent_name(this, type->linked);
    write(name);
}

void ToCAstVisitor::visit(ReferencedValueType *ref_type) {
    write("[ref_value_type]");
}

void ToCAstVisitor::visit(DynamicType *type) {
    write(fat_pointer_type);
}

void ToCAstVisitor::visit(ShortType *func) {
    write("short");
}

void ToCAstVisitor::visit(StringType *func) {
    write("char*");
}

void ToCAstVisitor::visit(StructType *val) {
    write("[StructType_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(UBigIntType *func) {
    write("unsigned long long");
}

void ToCAstVisitor::visit(UInt128Type *func) {
    write("[UInt128Type_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(UIntType *func) {
    write("unsigned int");
}

void ToCAstVisitor::visit(ULongType *func) {
    write("unsigned long");
}

void ToCAstVisitor::visit(UShortType *func) {
    write("unsigned short");
}

void ToCAstVisitor::visit(VoidType *func) {
    write("void");
}