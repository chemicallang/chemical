// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Parser.h"

#include "ast/types/Int32Type.h"
#include "ast/types/StringType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/VoidType.h"
#include "ast/types/AnyType.h"
#include "ast/types/DoubleType.h"

void Parser::init() {
    primitive_type_map["any"] = [&](Parser *parser) -> BaseType * { return new AnyType(); };
    primitive_type_map["bool"] = [&](Parser *parser) -> BaseType * { return new BoolType(); };
    primitive_type_map["char"] = [&](Parser *parser) -> BaseType * { return new CharType(); };
    primitive_type_map["double"] = [&](Parser *parser) -> BaseType * { return new DoubleType(); };
    primitive_type_map["float"] = [&](Parser *parser) -> BaseType * { return new FloatType(); };
    primitive_type_map["int"] = [&](Parser *parser) -> BaseType * { return new Int32Type(); };
    primitive_type_map["string"] = [&](Parser *parser) -> BaseType * { return new StringType(); };
    primitive_type_map["void"] = [&](Parser *parser) -> BaseType * { return new VoidType(); };
}

void Parser::parse() {
    parseMultipleStatements();
    nodes.shrink_to_fit();
}