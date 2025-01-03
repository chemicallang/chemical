// Copyright (c) Qinetik 2024.

#include "DereferenceValue.h"
#include "ast/types/ReferenceType.h"

DereferenceValue::DereferenceValue(
        Value* value,
        SourceLocation location
) : value(value), location(location) {

}

bool DereferenceValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    return value->link(linker, value);
}

BaseType* DereferenceValue::create_type(ASTAllocator& allocator) {
    auto addr = value->create_type(allocator);
    const auto addr_kind = addr->kind();
    if(addr_kind == BaseTypeKind::Pointer) {
        return ((PointerType*) addr)->type->copy(allocator);
    } else if(addr_kind == BaseTypeKind::Reference) {
        return ((ReferenceType*) addr)->type->copy(allocator);
    } else {
        // TODO cannot report error here, the type cannot be created because the linked type is not a pointer
        return nullptr;
    }
}

//hybrid_ptr<BaseType> DereferenceValue::get_base_type() {
//    auto addr = value->get_pure_type();
//    if(addr->kind() == BaseTypeKind::Pointer) {
//        if(addr.get_will_free()) {
//            return hybrid_ptr<BaseType> { ((PointerType*) (addr.get()))->type->copy() };
//        } else {
//            return hybrid_ptr<BaseType> { ((PointerType*) (addr.get()))->type.get(), false};
//        }
//    } else {
//        // TODO cannot report error here, the type cannot be created because the linked type is not a pointer
//        std::cout << "DereferenceValue returning nullptr, because de-referenced type is not a pointer" << std::endl;
//        return hybrid_ptr<BaseType> { nullptr, false };
//    }
//}

BaseType* DereferenceValue::known_type() {
    auto addr = value->known_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return ((PointerType*) addr)->type;
    } else {
        // TODO cannot report error here, the type cannot be created because the linked type is not a pointer
        std::cerr << "DereferenceValue returning nullptr, because de-referenced type is not a pointer" << std::endl;
        return nullptr;
    }
}

DereferenceValue *DereferenceValue::copy(ASTAllocator& allocator) {
    return new DereferenceValue(
            value->copy(allocator),
            location
    );
}