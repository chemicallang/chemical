// Copyright (c) Chemical Language Foundation 2025.

#include "DereferenceValue.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/StringValue.h"
#include "ast/values/PointerValue.h"
#include "ast/base/InterpretScope.h"

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

Value* DereferenceValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    const auto k = eval->val_kind();
    switch(k) {
        case ValueKind::String:{
            const auto val = eval->as_string_unsafe();
            return new (scope.allocate<CharValue>()) CharValue(val->value[0], encoded_location());
        }
        case ValueKind::PointerValue: {
            const auto val = (PointerValue*) eval;
            return val->deref(scope, encoded_location(), this);
        }
        default:
            scope.error("couldn't dereference value in comptime", this);
            return eval;
    }
}

//hybrid_ptr<BaseType> DereferenceValue::get_base_type() {
//    auto addr = value->get_pure_type(allocator);
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
            encoded_location()
    );
}