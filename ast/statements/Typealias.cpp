// Copyright (c) Qinetik 2024.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"

TypealiasStatement::TypealiasStatement(
        std::string identifier,
        std::unique_ptr<BaseType> actual_type,
        ASTNode* parent_node
) : identifier(std::move(identifier)), actual_type(std::move(actual_type)), parent_node(parent_node) {

}

void TypealiasStatement::interpret(InterpretScope &scope) {

}

void TypealiasStatement::declare_top_level(SymbolResolver &linker) {
    linker.declare(identifier, this);
}

void TypealiasStatement::declare_and_link(SymbolResolver &linker) {
    actual_type->link(linker, actual_type);
}

std::unique_ptr<BaseType> TypealiasStatement::create_value_type() {
    return std::unique_ptr<BaseType>(actual_type->copy());
}

hybrid_ptr<BaseType> TypealiasStatement::get_value_type() {
    return hybrid_ptr<BaseType> { actual_type.get(), false };
}

BaseType* TypealiasStatement::known_type() {
    return actual_type.get();
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

ValueType TypealiasStatement::value_type() const {
    return actual_type->value_type();
}