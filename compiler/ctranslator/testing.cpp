#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include "ast/base/BaseType.h"
#include "CTranslator.h"
#include "ast/types/VoidType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/UIntType.h"
#include "ast/types/IntType.h"
#include "ast/types/PointerType.h"
#include "ast/types/FloatType.h"

BaseType* new_type_2(CTranslator* translator, clang::QualType *type) {
    auto ptr = type->getTypePtr();
    if(ptr->isBuiltinType()) {
        auto builtIn = static_cast<clang::BuiltinType*>(const_cast<clang::Type*>(ptr));
        if(builtIn->getKind()) {

        }
    }
    if(ptr->isVoidType()) {
        return new VoidType();
    } else if(ptr->isPointerType()) {
        auto point = ptr->getPointeeType();
        auto pointee = new_type_2(translator, &point);
        if(!pointee) return nullptr;
        return new PointerType(std::unique_ptr<BaseType>(pointee));
    } else if(ptr->isBooleanType()) {
        return new BoolType();
    } else if(ptr->isCharType()) {
        return new CharType();
    } else if(ptr->isFloatingType()) {
        return new FloatType();
    } else if(ptr->isIntegerType()) {
        auto scalar = ptr->getAsPlaceholderType();
        if(scalar->Long)
        if(ptr->isUnsignedIntegerType()) {
            return new UIntType();
        } else {
            return new IntType();
        }
    } else {
        return nullptr;
    }
}