// Copyright (c) Qinetik 2024.

#include "cst/structures/FunctionCST.h"
#include "lexer/model/tokens/AbstractStringToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "ast/structures/FunctionDeclaration.h"
#include "cst/statements/VarInitCST.h"
#include "ast/statements/VarInit.h"
#include "ast/types/Int32Type.h"
#include "ast/types/StringType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/VoidType.h"
#include "ast/types/AnyType.h"
#include "ast/types/DoubleType.h"
#include "lexer/model/tokens/TypeToken.h"
#include "ast/types/ReferencedType.h"
#include "cst/types/ArrayTypeCST.h"
#include "cst/types/GenericTypeCST.h"
#include "cst/statements/AssignmentCST.h"
#include "cst/values/NegativeCST.h"
#include "cst/values/NotCST.h"
#include "ast/statements/Assignment.h"
#include "cst/types/FunctionTypeCST.h"
#include "cst/structures/BodyCST.h"
#include "cst/values/AccessChainCST.h"
#include "cst/values/ExpressionCST.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/Expression.h"
#include "ast/values/BoolValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/IntValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "lexer/model/tokens/StringToken.h"
#include "lexer/model/tokens/CharToken.h"
#include "ast/values/VariableIdentifier.h"
#include "lexer/model/tokens/OperationToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "lexer/model/tokens/BoolToken.h"

inline std::string str_token(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int index) {
    return static_cast<AbstractStringToken *>(tokens[index].get())->value;
}

inline char char_op(CSTToken *token) {
    return static_cast<CharOperatorToken *>(token)->op;
}

bool VarInitCST::is_const() {
    return str_token(tokens, 0) == "const";
}

std::string VarInitCST::identifier() {
    return str_token(tokens, 1);
}

std::string FunctionCST::func_name() {
    return str_token(tokens, 1);
}

CSTConverter::CSTConverter() {
    primitive_type_map["any"] = []() -> BaseType * { return new AnyType(); };
    primitive_type_map["bool"] = []() -> BaseType * { return new BoolType(); };
    primitive_type_map["char"] = []() -> BaseType * { return new CharType(); };
    primitive_type_map["double"] = []() -> BaseType * { return new DoubleType(); };
    primitive_type_map["float"] = []() -> BaseType * { return new FloatType(); };
    primitive_type_map["int"] = []() -> BaseType * { return new Int32Type(); };
    primitive_type_map["string"] = []() -> BaseType * { return new StringType(); };
    primitive_type_map["void"] = []() -> BaseType * { return new VoidType(); };
}

void CSTConverter::visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start, unsigned int end) {
    while (start < end) {
        tokens[start]->accept(this);
        start++;
    }
}

std::unique_ptr<Value> CSTConverter::value() {
    auto value = std::move(values.back());
    values.pop_back();
    return value;
}

std::unique_ptr<BaseType> CSTConverter::type() {
    auto type = std::move(types.back());
    types.pop_back();
    return type;
}

std::optional<std::unique_ptr<Value>> CSTConverter::opt_value() {
    if (values.empty()) return std::nullopt;
    return value();
}

std::optional<std::unique_ptr<BaseType>> CSTConverter::opt_type() {
    if (types.empty()) return std::nullopt;
    return type();
}

void CSTConverter::error(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
    diagnostics.emplace_back(
            Range{
                    start->start_token()->position,
                    end->end_token()->position
            },
            severity,
            std::nullopt,
            message
    );
}

void CSTConverter::error(const std::string &message, CSTToken *inside, DiagSeverity severity) {
    error(message, inside->start_token(), inside->end_token(), severity);
}

void CSTConverter::visit(FunctionParamCST *param) {
    auto identifier = str_token(param->tokens, 0);
    visit(param->tokens, 2);
    auto lastToken = param->tokens[param->tokens.size() - 1].get();
    auto isVariadic = !lastToken->compound() && lastToken->start_token()->is_abs_string() &&
                      ((AbstractStringToken *) lastToken)->value == "...";
    nodes.emplace_back(std::make_unique<FunctionParam>(identifier, type(), param_index, isVariadic, opt_value()));
    param_index++;
}

void CSTConverter::visit(FunctionCST *function) {

    param_index = 0;

    auto isVariadic = false;
    func_params params;
    unsigned i = 3;
    while (i < function->tokens.size()) {
        if (function->tokens[i]->compound()) {
            function->tokens[i]->accept(this);
            param_index++;
            auto param = (FunctionParam *) nodes.back().release();
            params.emplace_back(param);
            nodes.pop_back();
            if (param->isVariadic) {
                isVariadic = true;
                break;
            }
        } else if (function->tokens[i]->start_token()->type() == LexTokenType::CharOperator &&
                   char_op(function->tokens[i].get()) == ',') {
            // do nothing
        } else {
            break;
        }
        i++;
    }

    if(char_op(function->tokens[i + 1].get()) == ':') {
        function->tokens[i + 2]->accept(this);
        i += 2;
    }

    auto returnType = opt_type();
    if (!returnType.has_value()) {
        returnType.emplace(std::make_unique<VoidType>());
    }

    auto prev_nodes = std::move(nodes);
    visit(function->tokens, i);
    auto body = LoopScope(std::move(nodes));
    nodes = std::move(prev_nodes);

    nodes.emplace_back(std::make_unique<FunctionDeclaration>(function->func_name(), std::move(params),
                                                             std::move(returnType.value()), isVariadic,
                                                             std::move(body)));
}

void CSTConverter::visit(VarInitCST *varInit) {
    visit(varInit->tokens, 2);
    nodes.emplace_back(std::make_unique<VarInitStatement>(
            varInit->is_const(),
            varInit->identifier(),
            opt_type(),
            opt_value())
    );
}

void CSTConverter::visit(AssignmentCST *assignment) {
    visit(assignment->tokens, 0);
    auto val = value();
    auto chain = value().release();
    nodes.emplace_back(std::make_unique<AssignStatement>(
            std::unique_ptr<Value>(chain),
            std::move(val),
            (assignment->tokens[1]->type() == LexTokenType::Operation)
            ? ((OperationToken *) assignment->tokens[1].get())->op : Operation::Assignment
    ));
}

void CSTConverter::visit(TypeToken *token) {
    auto primitive = primitive_type_map.find(token->value);
    if (primitive == primitive_type_map.end()) {
        types.emplace_back(std::make_unique<ReferencedType>(token->value));
    } else {
        types.emplace_back(std::unique_ptr<BaseType>(primitive->second()));
    }
}

void CSTConverter::visit(BodyCST *bodyCst) {
    visit(bodyCst->tokens, 0);
}

void CSTConverter::visit(GenericTypeCST *genericType) {
    // TODO
}

void CSTConverter::visit(ArrayTypeCST *arrayType) {
    convert(arrayType->tokens);
    auto val = opt_value();
    auto arraySize = (val.has_value() && val.value()->value_type() == ValueType::Int) ? val.value()->as_int() : -1;
    types.emplace_back(std::make_unique<ArrayType>(std::move(type()), arraySize));
}

void CSTConverter::visit(FunctionTypeCST *functionType) {
    // TODO
}


void CSTConverter::visit(StringToken *token) {
    values.emplace_back(std::make_unique<StringValue>(token->value));
}

void CSTConverter::visit(CharToken *token) {
    values.emplace_back(std::make_unique<CharValue>(token->value));
}

void CSTConverter::visit(NumberToken *token) {
    try {
        if (token->has_dot()) {
            if (token->is_float()) {
                values.emplace_back(std::make_unique<FloatValue>(std::stof(token->value)));
            } else {
                values.emplace_back(std::make_unique<DoubleValue>(std::stod(token->value)));
            }
        } else {
            values.emplace_back(std::make_unique<IntValue>(std::stoi(token->value)));
        }
    } catch (...) {
        error("invalid number given", token);
    }
}

void CSTConverter::visit(StructValueCST *structValueCst) {

}

void CSTConverter::visit(ArrayValueCST *arrayValue) {

}

void CSTConverter::visit(AccessChainCST *chain) {
    auto prev_values = std::move(values);
    visit(chain->tokens, 0);
    auto ret_chain = std::make_unique<AccessChain>(std::move(values));
    values = std::move(prev_values);
    values.push_back(std::move(ret_chain));
}

void CSTConverter::visit(ExpressionCST *expr) {
    visit(expr->tokens, 0);
    auto second = value();
    auto first = value();
    values.emplace_back(std::make_unique<Expression>(std::move(first), std::move(second), ((OperationToken *) expr->tokens[1].get())->op));
}

void CSTConverter::visit(VariableToken *token) {
    values.emplace_back(std::make_unique<VariableIdentifier>(token->value));
}

void CSTConverter::visit(BoolToken *token) {
    values.emplace_back(std::make_unique<BoolValue>(token->value));
}

void CSTConverter::visit(NegativeCST *neg) {
    visit(neg->tokens, 0);
    values.emplace_back(std::make_unique<NegativeValue>(value()));
}

void CSTConverter::visit(NotCST *notCst) {
    visit(notCst->tokens, 0);
    values.emplace_back(std::make_unique<NotValue>(value()));
}