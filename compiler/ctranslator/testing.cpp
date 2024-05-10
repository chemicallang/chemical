#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include "ast/base/BaseType.h"
#include "CTranslator.h"
#include "ast/types/PointerType.h"

BaseType* new_type_2(CTranslator* translator, clang::QualType *type) {
    auto ptr = type->getTypePtr();
    if(ptr->isBuiltinType()) {
        auto builtIn = static_cast<clang::BuiltinType*>(const_cast<clang::Type*>(ptr));
        auto maker = translator->type_makers[builtIn->getKind()];
        return maker(builtIn);
    } else if(ptr->isPointerType()) {
        auto point = ptr->getPointeeType();
        auto pointee = new_type_2(translator, &point);
        if(!pointee) return nullptr;
        return new PointerType(std::unique_ptr<BaseType>(pointee));
    } else {
        return nullptr;
    };
}