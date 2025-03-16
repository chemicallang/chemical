// Copyright (c) Chemical Language Foundation 2025.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"
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

void TypealiasStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), false);
}

void TypealiasStatement::link_signature(SymbolResolver &linker) {
    actual_type->link(linker);
}

BaseType* TypealiasStatement::known_type() {
    return actual_type;
}

uint64_t TypealiasStatement::byte_size(bool is64Bit) {
    return actual_type->byte_size(is64Bit);
}