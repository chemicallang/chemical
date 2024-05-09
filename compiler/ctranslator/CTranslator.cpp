// Copyright (c) Qinetik 2024.

#include "CTranslator.h"
#include "ast/types/IntType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/LongType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"

CTranslator::CTranslator() {
    init_type_makers();
}

void CTranslator::init_type_makers() {
//    type_makers["int"] = [](const std::string& type_str) -> BaseType* {
//        return new IntType();
//    };
//    type_makers["char"] = [](const std::string& type_str) -> BaseType* {
//        return new CharType();
//    };
//    type_makers["float"] = [](const std::string& type_str) -> BaseType* {
//        return new FloatType();
//    };
//    type_makers["double"] = [](const std::string& type_str) -> BaseType* {
//        return new DoubleType();
//    };
//    type_makers["long"] = [](const std::string& type_str) -> BaseType* {
//        return new LongType(sizeof(long) == 8);
//    };
//    type_makers["long long"] = [](const std::string& type_str) -> BaseType* {
//        return new BigIntType();
//    };
//    type_makers["void"] = [](const std::string& type_str) -> BaseType* {
//        return new VoidType();
//    };
}

void CTranslator::error(const std::string& err) {
#ifdef DEBUG
    std::cerr << "error :" << err << std::endl;
#endif
    errors.emplace_back(
        err
    );
}