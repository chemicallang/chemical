// Copyright (c) Qinetik 2024.

#include "2cASTVisitor.h"
#include <memory>
#include <ostream>
#include <random>
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Break.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/MacroValueStatement.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/If.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/LoopScope.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/FunctionType.h"
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
#include "ast/values/NullValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/TernaryValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ULongValue.h"
#include "ast/utils/CommonVisitor.h"
#include "utils/RepresentationUtils.h"
#include "ast/utils/ASTUtils.h"

ToCAstVisitor::ToCAstVisitor(std::ostream *output, const std::string& path) : output(output), ASTDiagnoser(path) {
    declarer = std::make_unique<CValueDeclarationVisitor>(this);
    tld = std::make_unique<CTopLevelDeclarationVisitor>(this, declarer.get());
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
    assign->value->accept(visitor);
}

#define struct_passed_param_name "__chx_struct_ret_param_xx"
#define fn_call_struct_var_name "chx_fn_cl_struct"

void func_type_with_id(ToCAstVisitor* visitor, FunctionType* type, const std::string& id);

void type_with_id(ToCAstVisitor* visitor, BaseType* type, const std::string& id) {
    if(visitor->inline_fn_types_in_params && type->function_type() != nullptr && !type->function_type()->isCapturing) {
        func_type_with_id(visitor, type->function_type(), id);
    } else {
        type->accept(visitor);
        visitor->space();
        visitor->write(id);
        write_type_post_id(visitor, type);
    }
}

void write_struct_return_param(ToCAstVisitor* visitor, BaseFunctionType* decl) {
    decl->returnType->accept(visitor);
    visitor->write("* ");
    visitor->write(struct_passed_param_name);
}

void extension_func_param(ToCAstVisitor* visitor, ExtensionFunction* extension) {
    extension->receiver.type->accept(visitor);
    visitor->space();
    visitor->write(extension->receiver.name);
}

void func_type_params(ToCAstVisitor* visitor, BaseFunctionType* decl, unsigned i = 0) {
    auto is_struct_return = visitor->pass_structs_to_initialize && decl->returnType->value_type() == ValueType::Struct;
    auto extension = decl->as_extension_func();
    if(extension) {
        extension_func_param(visitor, extension);
        if(is_struct_return || !extension->params.empty()) {
            visitor->write(", ");
        }
    }
    if(is_struct_return) {
        write_struct_return_param(visitor, decl);
        if(!decl->params.empty()) {
            visitor->write(", ");
        }
    }
    FunctionParam* param;
    auto size = decl->isVariadic ? decl->params.size() - 1 : decl->params.size();
    while(i < size) {
        param = decl->params[i].get();
        type_with_id(visitor, param->type.get(), param->name);
        if(i != decl->params.size() - 1) {
            visitor->write(", ");
        }
        i++;
    }
    if(decl->isVariadic) {
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

void accept_func_return_with_name(ToCAstVisitor* visitor, BaseType* type, const std::string& name) {
    accept_func_return(visitor, type);
    visitor->space();
    visitor->write(name);
}

void func_type_with_id(ToCAstVisitor* visitor, FunctionType* type, const std::string& id) {
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

void param_type_with_id(ToCAstVisitor* visitor, BaseType* type, const std::string& id, unsigned index, bool overrides) {
    if(should_void_pointer_to_self(type, id, index, overrides)) {
        visitor->write("void* ");
        visitor->write(id);
        return;
    }
    type_with_id(visitor, type, id);
}

void func_call_args(ToCAstVisitor* visitor, FunctionCall* call) {
    unsigned i = 0;
    while(i < call->values.size()) {
        auto& val = call->values[i];
        val->accept(visitor);
        if(i != call->values.size() - 1) {
            visitor->write(", ");
        }
        i++;
    }
}

void access_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<Value>>& values, unsigned start, unsigned end, unsigned size);

void access_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<Value>>& values, unsigned start, unsigned end) {
    access_chain(visitor, values, start, end, values.size());
}

void value_alloca(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
    type_with_id(visitor, type, identifier);
    visitor->write(';');
}

bool write_self_arg_bool(ToCAstVisitor* visitor, BaseFunctionType* func_type, Value* grandpa, FunctionCall* call) {
    if(func_type->has_self_param()) {
        visitor->write('&');
        grandpa->accept(visitor);
        return true;
    } else {
        return false;
    }
}

void write_self_arg(ToCAstVisitor* visitor, BaseFunctionType* func_type, Value* grandpa, FunctionCall* call) {
    if(write_self_arg_bool(visitor, func_type, grandpa, call)) {
        if (!call->values.empty()) {
            visitor->write(',');
        }
    }
}

void value_assign_default(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, Value* value) {
    if(value->as_access_chain()) {
        auto chain = value->as_access_chain();
        auto func_call = chain->values.back()->as_func_call();
        if(func_call && func_call->create_type()->value_type() == ValueType::Struct) {
            auto parent_type = func_call->parent_val->create_type();
            auto func_type = parent_type->function_type();
            auto end = chain->values.size();
            auto grandpa = end - 3 < end ? chain->values[end - 3].get() : nullptr;
            visitor->nested_value = true;
            access_chain(visitor, chain->values, 0, chain->values.size() - 1);
            visitor->nested_value = false;
            visitor->write('(');
            visitor->write('&');
            visitor->write(identifier);
            if(!chain->values.back()->as_func_call()->values.empty()){
                visitor->write(", ");
            }
            if(grandpa) write_self_arg(visitor, func_type, grandpa, func_call);
            func_call_args(visitor, chain->values.back()->as_func_call());
            visitor->write(");");
            return;
        }
    }
    visitor->write(identifier);
    write_type_post_id(visitor, type);
    visitor->write(" = ");
    value->accept(visitor);
    visitor->write(';');
}

void value_init_default(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
    type->accept(visitor);
    visitor->space();
    value_assign_default(visitor, identifier, type, value->get());
}

void value_store_default(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
    visitor->new_line_and_indent();
    value_assign_default(visitor, identifier, type, value->get());
}

void value_store(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
    if(value.has_value()) {
        value_store_default(visitor, identifier, type, value);
    }
}

void value_alloca_store(ToCAstVisitor* visitor, const std::string& identifier, BaseType* type, std::optional<std::unique_ptr<Value>>& value) {
    if(value.has_value()) {
        if(type->value_type() == ValueType::Struct && value.value()->value_type() != ValueType::Struct) {
            // struct instantiation is done in 2 instructions -> declaration and assignment
            value_alloca(visitor, identifier, type, value);
            visitor->new_line_and_indent();
            value_assign_default(visitor, identifier, type, value->get());
        } else {
            value_init_default(visitor, identifier, type, value);
        }
    } else {
        value_alloca(visitor, identifier, type, value);
    }
}

void var_init(ToCAstVisitor* visitor, VarInitStatement* init) {
    if (!init->type.has_value()) {
        init->type.emplace(init->value.value()->create_type().release());
    }
    value_alloca_store(visitor, init->identifier, init->type.value().get(), init->value);
    if(init->value.has_value()) {
        init->type = std::nullopt;
    }
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

};

class CTopLevelDeclarationVisitor : public Visitor, public SubVisitor {
public:

    CValueDeclarationVisitor* value_visitor;

    CTopLevelDeclarationVisitor(
            ToCAstVisitor* visitor,
            CValueDeclarationVisitor* value_visitor
    );

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(InterfaceDefinition *interfaceDefinition) override;

    void visit(ImplDefinition *implDefinition) override;

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

};

class CDestructionVisitor : public Visitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    void destruct(const std::string& self_name, ASTNode* linked, FunctionDeclaration* destructor);

    void destruct(const std::string& self_name, ASTNode* linked);

    void visit(VarInitStatement *init) override;

};

void func_container_name(ToCAstVisitor* visitor, ASTNode* parent_node, ASTNode* linked_node);

void CDestructionVisitor::destruct(const std::string& self_name, ASTNode* parent_node, FunctionDeclaration* destructor) {
    visitor->new_line_and_indent();
    func_container_name(visitor, parent_node, destructor);
    visitor->write(destructor->name);
    visitor->write('(');
    if(destructor->has_self_param()) {
        visitor->write('&');
        visitor->write(self_name);
    }
    visitor->write(')');
    visitor->write(';');
}

void CDestructionVisitor::destruct(const std::string& self_name, ASTNode* linked) {
    if(linked->as_struct_def()) {
        auto destructorFunc = linked->as_struct_def()->destructor_func();
        if(destructorFunc) {
            destruct(self_name, linked, destructorFunc);
        }
    }
}

void CDestructionVisitor::visit(VarInitStatement *init) {
    if(init->value.has_value()) {
        auto func_call = init->value.value()->as_func_call();
        if(func_call) {
            auto return_type = func_call->create_type();
            auto linked = return_type->linked_node();
            if(linked) destruct(init->identifier, linked);
        }
        auto struct_val = init->value.value()->as_struct();
        if(struct_val) {
            auto linked = struct_val->definition;
            if(linked) destruct(init->identifier, linked);
        }
    } else {
        auto linked = init->type.value()->linked_node();
        if(linked) destruct(init->identifier, linked);
    }
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

void func_call(ToCAstVisitor* visitor, FunctionCall* call) {
    visitor->write('(');
    func_call_args(visitor, call);
    visitor->write(')');
    if(!visitor->nested_value) {
        visitor->write(';');
    }
}

void CValueDeclarationVisitor::visit(VarInitStatement *init) {
    if(!init->type.has_value()) {
        // because it can contain function type, so we must emplace it
        // this function type creates a typedef, which is accessible by function type's pointer from aliases map
        init->type.emplace(init->value.value()->create_type());
    }
    CommonVisitor::visit(init);
    if(!is_top_level_node) return;
    visitor->new_line_and_indent();
    var_init(visitor, init);
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
                PointerType pointer(var->linked->create_value_type());
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
    accept_func_return_with_name(visitor, lamb->returnType.get(), lamb_name);
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
    scope(visitor, lamb->scope);
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
    accept_func_return(visitor, retFunc->returnType.get());
    visitor->write("(");
    func_ret_func_proto_after_l_paren(visitor, decl, name, retFunc);
}

void declare_func_with_return(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name) {
    if(visitor->inline_fn_types_in_returns && decl->returnType->function_type() && !decl->returnType->function_type()->isCapturing) {
        func_that_returns_func_proto(visitor, decl, name, decl->returnType->function_type());
    } else {
        if (decl->returnType->kind() == BaseTypeKind::Void && name == "main") {
            visitor->write("int main");
        } else {
            accept_func_return_with_name(visitor, decl->returnType.get(), name);
        }
        visitor->write('(');
        func_type_params(visitor, decl);
        visitor->write(')');
    }
}

void declare_by_name(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, const std::string& name) {
    declare_params(tld->value_visitor, decl->params);
    if(!tld->visitor->inline_fn_types_in_returns || decl->returnType->function_type() == nullptr) {
        decl->returnType->accept(tld->value_visitor);
    }
    tld->visitor->new_line_and_indent();
    declare_func_with_return(tld->visitor, decl, name);
    tld->visitor->write(';');
}

// when a function is inside struct / interface
void declare_contained_func(CTopLevelDeclarationVisitor* tld, FunctionDeclaration* decl, const std::string& name, bool overrides) {
    declare_params(tld->value_visitor, decl->params);
    if(!tld->visitor->inline_fn_types_in_returns || decl->returnType->function_type() == nullptr) {
        decl->returnType->accept(tld->value_visitor);
    }
    tld->visitor->new_line_and_indent();
    FunctionParam* param = !decl->params.empty() ? decl->params[0].get() : nullptr;
    unsigned i = 0;
    auto write_self_param_now = [decl, tld, param, &i, overrides]() {
        if(param && should_void_pointer_to_self(param->type.get(), param->name, 0, overrides)) {
            tld->write("void* ");
            tld->write(param->name);
            if(decl->params.size() > 1) {
                tld->write(", ");
            }
            i = 1;
        }
    };
    if(tld->visitor->inline_fn_types_in_returns && decl->returnType->function_type() != nullptr && !decl->returnType->function_type()->isCapturing) {
        accept_func_return(tld->visitor, decl->returnType->function_type()->returnType.get());
        tld->write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(tld->visitor, decl, name, decl->returnType->function_type(), i);
    } else {
        accept_func_return_with_name(tld->visitor, decl->returnType.get(), name);
        tld->write('(');
        write_self_param_now();
        func_type_params(tld->visitor, decl, i);
        tld->write(')');
    }
    tld->write(';');
}

void CTopLevelDeclarationVisitor::visit(FunctionDeclaration *decl) {
    declare_by_name(this, decl, decl->name);
}

void CTopLevelDeclarationVisitor::visit(ExtensionFunction *decl) {
    declare_by_name(this, decl, decl->name);
}

void CValueDeclarationVisitor::visit(FunctionDeclaration *decl) {
    if(decl->body.has_value()) {
        decl->body.value().accept(this);
    }
}

void CValueDeclarationVisitor::visit(ExtensionFunction *decl) {
    if(decl->body.has_value()) {
        decl->body.value().accept(this);
    }
}

void CValueDeclarationVisitor::visit(EnumDeclaration *enumDecl) {
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

void CTopLevelDeclarationVisitor::visit(StructDefinition *def) {
    // forward declaring struct for function types that take a pointer to it
    visitor->new_line_and_indent();
    write("struct ");
    write(def->name);
    write(';');
    for(auto& mem : def->variables) {
        mem.second->accept(value_visitor);
    }
    visitor->new_line_and_indent();
    write("struct ");
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
    InterfaceDefinition* overridden = def->overrides.has_value() ? def->overrides.value()->linked->as_interface_def() : nullptr;
    for(auto& func : def->functions) {
        if(!overridden || overridden->functions.find(func.second->name) == overridden->functions.end()) {
            declare_contained_func(this, func.second.get(), def->name + func.second->name, false);
        }
    }
}

void CTopLevelDeclarationVisitor::visit(InterfaceDefinition *def) {
    for(auto& func : def->functions) {
        declare_contained_func(this, func.second.get(), def->name + func.second->name, false);
    }
}

void CTopLevelDeclarationVisitor::visit(ImplDefinition *def) {

}

void CValueDeclarationVisitor::visit(TypealiasStatement *stmt) {
    visitor->new_line_and_indent();
    write("typedef ");
    stmt->to->accept(visitor);
    write(' ');
    if(is_top_level_node) {
        write(stmt->from);
    } else {
        std::string alias = "__chalias_";
        alias += std::to_string(random(100,999)) + "_";
        alias += std::to_string(alias_num++);
        write(alias);
        aliases[stmt] = alias;
    }
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
    visitor->write("void* lambda;");
    visitor->new_line_and_indent();
    visitor->write("void* captured;");
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

ToCAstVisitor::~ToCAstVisitor() = default;

void ToCAstVisitor::visitCommon(ASTNode *node) {
    throw std::runtime_error("visitor common node called in 2c ASTVisitor");
}

void ToCAstVisitor::visitCommonValue(Value *value) {
    throw std::runtime_error("visitor common value called in 2c ASTVisitor");
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

void ToCAstVisitor::write(const std::string& value) {
    output->write(value.c_str(), value.size());
}

void ToCAstVisitor::visit(VarInitStatement *init) {
    if(top_level_node) return;
    var_init(this, init);
}

void ToCAstVisitor::visit(AssignStatement *assign) {
    assign_statement(this, assign);
    write(';');
}

void ToCAstVisitor::visit(BreakStatement *breakStatement) {
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

void ToCAstVisitor::visit(ReturnStatement *returnStatement) {
    if(returnStatement->value.has_value()) {
        auto val = returnStatement->value.value().get();
        if(val->as_struct()) {
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
                write("return ");
                struct_initialize_inside_braces(this, (StructValue*) val);
            }
        } else if(val->value_type() == ValueType::Struct) {
            auto refType = val->create_type();
            auto structType = refType->linked_node()->as_struct_def();
            auto size = structType->variables.size();
            unsigned i = 0;
            for(const auto& mem : structType->variables) {
                write(struct_passed_param_name);
                write("->");
                write(mem.first);
                write(" = ");
                val->accept(this);
                write('.');
                write(mem.first);
                if(i != size - 1){
                    write(';');
                    new_line_and_indent();
                }
                i++;
            }
        } else {
            write("return ");
           val->accept(this);
        }
        write(';');
    } else {
        write("return;");
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
    if(!decl->body.has_value()) {
        return;
    }
    visitor->new_line_and_indent();
    if(visitor->inline_fn_types_in_returns && decl->returnType->function_type() && !decl->returnType->function_type()->isCapturing) {
        func_that_returns_func_proto(visitor, decl, name, decl->returnType->function_type());
    } else {
        declare_func_with_return(visitor, decl, name);
    }
    scope(visitor, decl->body.value());
}

void contained_func_decl(ToCAstVisitor* visitor, FunctionDeclaration* decl, const std::string& name, bool overrides, StructDefinition* def) {
    if(!decl->body.has_value()) {
        return;
    }
    visitor->new_line_and_indent();
    std::string self_pointer_name;
    FunctionParam* param = !decl->params.empty() ? decl->params[0].get() : nullptr;
    unsigned i = 0;
    auto write_self_param_now = [decl, visitor, param, &i, &self_pointer_name, overrides]() {
        if(param && should_void_pointer_to_self(param->type.get(), param->name, 0, overrides)) {
            self_pointer_name = "__ch_self_pointer_329283";
            visitor->write("void* ");
            visitor->write(self_pointer_name);
            if(decl->params.size() > 1) {
                visitor->write(", ");
            }
            i = 1;
        }
    };
    if(visitor->inline_fn_types_in_returns && decl->returnType->function_type() != nullptr && !decl->returnType->function_type()->isCapturing) {
        accept_func_return(visitor, decl->returnType->function_type()->returnType.get());
        visitor->write('(');
        write_self_param_now();
        func_ret_func_proto_after_l_paren(visitor, decl, name, decl->returnType->function_type(), i);
    } else {
        accept_func_return_with_name(visitor, decl->returnType.get(), name);
        visitor->write('(');
        write_self_param_now();
        func_type_params(visitor, decl, i);
        visitor->write(')');
    }
    visitor->write('{');
    visitor->indentation_level+=1;
    if(!self_pointer_name.empty() && def) {
        visitor->new_line_and_indent();
        visitor->write("struct ");
        visitor->write(def->name);
        visitor->write('*');
        visitor->space();
        visitor->write("self = ");
        visitor->write(self_pointer_name);
        visitor->write(';');
    }
    decl->body.value().accept(visitor);
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
}

void ToCAstVisitor::visit(FunctionDeclaration *decl) {
    func_decl_with_name(this, decl, decl->name);
}

void ToCAstVisitor::visit(ExtensionFunction *decl) {
    func_decl_with_name(this, decl, decl->name);
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
    for(auto& func : def->functions) {
        contained_func_decl(this, func.second.get(), def->interface_name + func.second->name, def->struct_name.has_value(), def->struct_linked);
    }
}

void ToCAstVisitor::visit(InterfaceDefinition *def) {

}

void ToCAstVisitor::visit(Scope *scope) {
    auto prev = top_level_node;
    top_level_node = false;
    for(auto& node : scope->nodes) {
        new_line_and_indent();
        node->accept(this);
    }
    for(auto& node : scope->nodes) {
        node->accept(destructor.get());
    }
    top_level_node = prev;
}

void ToCAstVisitor::visit(StructDefinition *def) {
    auto overridden = def->overrides.has_value() ? def->overrides.value()->linked->as_interface_def() : nullptr;
    if(overridden) {
        for(auto& func : overridden->functions) {
            if(def->functions.find(func.second->name) == def->functions.end()) {
                contained_func_decl(this, func.second.get(), overridden->name + func.second->name, false, def);
            }
        }
    }
    for(auto& func : def->functions) {
        if(overridden && overridden->functions.find(func.second->name) != overridden->functions.end()) {
            contained_func_decl(this, func.second.get(), overridden->name + func.second->name, true, def);
        } else {
            contained_func_decl(this, func.second.get(), def->name + func.second->name, false, def);
        }
    }
}

void ToCAstVisitor::visit(WhileLoop *whileLoop) {
    write("while(");
    whileLoop->condition->accept(this);
    write(") ");
    scope(this, whileLoop->body);

}

template<typename current_call>
void capture_call(ToCAstVisitor* visitor, FunctionType* type, current_call call, FunctionCall* func_call) {
    visitor->write('(');
    visitor->write('(');
    visitor->write('(');
    func_type_with_id(visitor, type, "");
    visitor->write(") ");
    call();
    visitor->write("->lambda");
    visitor->write(')');
    visitor->write('(');
    call();
    visitor->write("->captured");
    if(!func_call->values.empty()) {
        visitor->write(',');
    }
    func_call_args(visitor, func_call);
    visitor->write(')');
    visitor->write(')');
}

void func_call(ToCAstVisitor* visitor, FunctionType* type, std::unique_ptr<Value>& current, std::unique_ptr<Value>& next, unsigned int& i) {
    if(type->isCapturing && current->as_func_call() == nullptr) {
        capture_call(visitor, type, [&current, visitor](){ current->accept(visitor); }, next->as_func_call());
        i++;
    } else {
        current->accept(visitor);
        func_call(visitor, next->as_func_call());
        i++;
    }
}

void func_container_name(ToCAstVisitor* visitor, ASTNode* parent_node, ASTNode* linked_node) {
    if(linked_node->as_extension_func()) {
        return;
    }
    if(parent_node->as_interface_def()) {
        visitor->write(parent_node->as_interface_def()->name);
    } else if(parent_node->as_struct_def()) {
        if(parent_node->as_struct_def()->overrides.has_value()) {
            auto interface = parent_node->as_struct_def()->overrides.value()->linked_node()->as_interface_def();
            if(interface->functions.find(linked_node->as_function()->name) != interface->functions.end()) {
                visitor->write(interface->name);
            } else {
                visitor->write(parent_node->as_struct_def()->name);
            }
        } else {
            visitor->write(parent_node->as_struct_def()->name);
        }
    }
}

void write_accessor(ToCAstVisitor* visitor, Value* current) {
    if (current->type_kind() == BaseTypeKind::Pointer) {
        visitor->write("->");
    } else {
        visitor->write('.');
    }
}

void func_call(ToCAstVisitor* visitor, std::vector<std::unique_ptr<Value>>& values, unsigned start, unsigned end) {
    auto grandpa = ((int) end) - 3 >= 0 ? values[end - 3].get() : nullptr;
    auto parent = values[end - 2].get();
    auto parent_type = parent->create_type();
    auto func_type = parent_type->function_type();
    auto last = values[end - 1]->as_func_call();
    bool is_lambda = (parent->linked_node() != nullptr && parent->linked_node()->as_struct_member() != nullptr);
    if(visitor->pass_structs_to_initialize && func_type->returnType->value_type() == ValueType::Struct) {
        // functions that return struct
        visitor->write("({ ");
        func_type->returnType->accept(visitor);
        visitor->space();
        visitor->write("__chem_x_1__; ");
        if(grandpa && !is_lambda) {
            auto grandpaType = grandpa->create_type();
            func_container_name(visitor, grandpaType->linked_node(), parent->linked_node());
            parent->accept(visitor); // function name
            visitor->write('(');
            if(write_self_arg_bool(visitor, func_type, grandpa, last)) {
                visitor->write(", ");
            }
        } else {
            access_chain(visitor, values, start, end - 1);
            visitor->write('(');
        }
        visitor->write('&');
        visitor->write("__chem_x_1__");
        if(!last->as_func_call()->values.empty()){
            visitor->write(", ");
        }
        func_call_args(visitor, last->as_func_call());
        visitor->write("); __chem_x_1__; })");
    } else if(func_type->isCapturing) {
        // function calls to capturing lambdas
        capture_call(visitor, func_type, [&](){
            visitor->nested_value = true;
            access_chain(visitor, values, start, end - 1);
            visitor->nested_value = false;
        }, last);
    } else if(grandpa) {
        auto grandpaType = grandpa->create_type();
        if(grandpa->linked_node() && (grandpa->linked_node()->as_interface_def() || grandpa->linked_node()->as_struct_def())) {
            // direct functions on interfaces and structs
            func_container_name(visitor, grandpa->linked_node(), parent->linked_node());
            parent->accept(visitor);
            func_call(visitor, last->as_func_call());
        } else if(grandpa->value_type() == ValueType::Struct && grandpaType->kind() == BaseTypeKind::Referenced) {
            if(parent->linked_node()->as_struct_member()) {
                goto normal_functions;
            }
            // functions on struct values
            func_container_name(visitor, grandpaType->linked_node(), parent->linked_node());
            parent->accept(visitor); // function name
            visitor->write('(');
            write_self_arg(visitor, func_type, grandpa, last);
            func_call_args(visitor, last->as_func_call());
            visitor->write(')');
        } else {
            goto normal_functions;
        }
    } else {
        goto normal_functions;
    }
    return;
    normal_functions: {
        // normal functions
        access_chain(visitor, values, start, end - 1);
        visitor->write('(');
        if(grandpa && func_type->has_self_param()) {
            if(chain_contains_func_call(values, start, end - 2)) {
                visitor->error("Function call inside a access chain with lambda that requires self is not allowed");
                return;
            }
            if(end - 3 >= 0 && values[end - 3]->value_type() != ValueType::Pointer) {
                visitor->write('&');
            }
            access_chain(visitor, values, start, end - 2, end - 2);
            if (!last->values.empty()) {
                visitor->write(',');
            }
        }
        func_call_args(visitor, last);
        visitor->write(')');
        if(!visitor->nested_value) {
            visitor->write(';');
        }
    };
}

void access_chain(ToCAstVisitor* visitor, std::vector<std::unique_ptr<Value>>& values, unsigned start, unsigned end, unsigned total_size) {
    auto diff = end - start;
    if(diff == 0) {
        return;
    } else if(diff == 1) {
        values[start]->accept(visitor);
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
                    write_accessor(visitor, values[j].get());
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
    Value* current;
    Value* next;
    while(i < end) {
        current = values[i].get();
        if(i + 1 < total_size) {
            next = values[i + 1].get();
            if(next->as_func_call() || next->as_index_op()) {
                current->accept(visitor);
            } else {
                if(current->linked_node()->as_enum_decl() != nullptr) {
                    auto found = visitor->declarer->aliases.find(next->linked_node()->as_enum_member());
                    if(found != visitor->declarer->aliases.end()) {
                        visitor->write(found->second);
                        i++;
                    } else {
                        visitor->write("[EnumAC_NOT_FOUND:" + current->representation() + "." + next->representation() + "]");
                    }
                } else {
                    current->accept(visitor);
                    write_accessor(visitor, current);
                }
            }
        } else {
            current->accept(visitor);
            next = nullptr;
        }
        i++;
    }
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
    statement->expression->accept(this);
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
        indentation_level -= 1;
        i++;
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
    while(i < arr->values.size()) {
        arr->values[i]->accept(this);
        if(i != arr->values.size() - 1) {
            write(',');
        }
        i++;
    }
    write('}');
}

void ToCAstVisitor::visit(StructValue *val) {
    write('{');
    for(auto& value : val->values) {
        write('.');
        write(value.first);
        write(" = ");
        value.second->accept(this);
        write(", ");
    }
    write('}');
}

void ToCAstVisitor::visit(VariableIdentifier *identifier) {
    if(identifier->linked_node()->as_captured_var() != nullptr) {
        auto found = declarer->aliases.find(identifier->linked_node()->as_captured_var());
        if(found == declarer->aliases.end()) {
            write("this->");
        } else {
            write("((struct ");
            write(found->second);
            write("*) this)->");
        }

    }
    write(identifier->value);
}

void ToCAstVisitor::visit(Expression *expr) {
    write('(');
    nested_value = true;
    expr->firstValue->accept(this);
    space();
    write(to_string(expr->operation));
    space();
    expr->secondValue->accept(this);
    nested_value = false;
    write(')');
}

void ToCAstVisitor::visit(CastedValue *casted) {
    write('(');
    write('(');
    casted->type->accept(this);
    write(')');
    write(' ');
    casted->value->accept(this);
    write(')');
}

void ToCAstVisitor::visit(AddrOfValue *casted) {
    write('&');
    casted->value->accept(this);
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

void ToCAstVisitor::visit(GenericType *func) {
    write("[GenericType_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(Int128Type *func) {
    write("[Int128Type_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(IntType *func) {
    write("int");
}

void ToCAstVisitor::visit(LongType *func) {
    write("long");
}

void ToCAstVisitor::visit(PointerType *func) {
    func->type->accept(this);
    write('*');
}

void ToCAstVisitor::visit(ReferencedType *type) {
    if(type->linked->as_enum_decl()){
        write("int");
        return;
    }
    if(type->linked->as_struct_def()) {
        write("struct ");
    }
    if(type->linked->as_typealias() != nullptr) {
        auto alias = declarer->aliases.find(type->linked->as_typealias());
        if(alias != declarer->aliases.end()) {
            write(alias->second);
            return;
        }
    }
    write(type->type);
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