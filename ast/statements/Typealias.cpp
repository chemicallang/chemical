// Copyright (c) Qinetik 2024.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/WrapperType.h"
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
    const auto evalVal = provider->evaluated_value(scope);
    if(!evalVal) {
        scope.error("expected value to return a value containing type", provider);
        return;
    }
    if(evalVal->val_kind() != ValueKind::ValueContainingType) {
        scope.error("expected value to return a value containing type", provider);
        return;
    }
    // get the type from the evaluated value
    const auto evaluated_type = evalVal->as_value_containing_type_unsafe()->type;
    // set the current evaluated type to wrapper type
    const auto type = actual_type->as_wrapper_type_unsafe();
    type->actual_type = evaluated_type;
    // when the scope dies, we reset scope of the type containing value back to null pointer
    // this way type containing value can check, whether the scope lives
    scope.add_destructor(type, [](void* data) {
        const auto type = ((WrapperType*) data);
        type->actual_type = nullptr;
    });
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