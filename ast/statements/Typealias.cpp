// Copyright (c) Qinetik 2024.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/TypeContainingValue.h"
#include "ast/values/ValueContainingType.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"

void TypealiasStatement::interpret(InterpretScope &scope) {

}

//FunctionCall* get_call(Value* value) {
//    switch(value->val_kind()) {
//        case ValueKind::FunctionCall:
//            return value->as_func_call_unsafe();
//        default:
//            return nullptr;
//        case ValueKind::AccessChain:
//            return get_call(value->as_access_chain_unsafe()->values.front());
//    }
//}

void ValueTypealiasStmt::interpret(InterpretScope& scope) {
    const auto eval = value->evaluated_value(scope);
    if(!eval) {
        scope.error("comptime value didn't return anything", actual_type);
        return;
    }
    if(eval->val_kind() != ValueKind::ValueContainingType) {
        scope.error("comptime value didn't return a type containing value", actual_type);
        return;
    }
    const auto value_type = (ValueContainingType*) eval;
    actual_type = value_type->type->copy(scope.allocator);
}

void TypealiasStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), false);
}

void TypealiasStatement::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    actual_type->link(linker);
}

BaseType* TypealiasStatement::create_value_type(ASTAllocator& allocator) {
    return actual_type->copy(allocator);
}

//hybrid_ptr<BaseType> TypealiasStatement::get_value_type() {
//    return hybrid_ptr<BaseType> { actual_type, false };
//}

BaseType* TypealiasStatement::known_type() {
    return actual_type;
}

uint64_t TypealiasStatement::byte_size(bool is64Bit) {
    return actual_type->byte_size(is64Bit);
}

void TypealiasStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

BaseTypeKind TypealiasStatement::type_kind() const {
    return actual_type->kind();
}