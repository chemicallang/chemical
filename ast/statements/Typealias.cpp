// Copyright (c) Qinetik 2024.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"

TypealiasStatement::TypealiasStatement(
        std::string identifier,
        BaseType* actual_type,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier
) : identifier(std::move(identifier)), actual_type(actual_type), parent_node(parent_node), token(token), specifier(specifier) {

}

void TypealiasStatement::interpret(InterpretScope &scope) {

}

void TypealiasStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(identifier, this, specifier, false);
}

void TypealiasStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
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

ValueType TypealiasStatement::value_type() const {
    return actual_type->value_type();
}