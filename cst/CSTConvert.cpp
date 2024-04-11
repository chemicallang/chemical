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
#include "ast/types/PointerType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/AnyType.h"
#include "ast/types/DoubleType.h"
#include "lexer/model/tokens/TypeToken.h"
#include "cst/types/PointerTypeCST.h"
#include "ast/types/ReferencedType.h"
#include "cst/types/ArrayTypeCST.h"
#include "cst/types/GenericTypeCST.h"
#include "cst/statements/AssignmentCST.h"
#include "cst/values/NegativeCST.h"
#include "cst/values/FunctionCallCST.h"
#include "cst/values/IndexOpCST.h"
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
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/IntValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "lexer/model/tokens/StringToken.h"
#include "lexer/model/tokens/CharToken.h"
#include "ast/values/VariableIdentifier.h"
#include "lexer/model/tokens/OperationToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "lexer/model/tokens/BoolToken.h"
#include "ast/statements/Import.h"
#include "cst/statements/ImportCST.h"
#include "ast/statements/Return.h"
#include "cst/statements/ReturnCST.h"
#include "ast/types/GenericType.h"
#include "ast/structures/ForLoop.h"
#include "cst/structures/ForLoopCST.h"
#include "ast/structures/WhileLoop.h"
#include "cst/structures/WhileCST.h"
#include "ast/structures/DoWhileLoop.h"
#include "cst/structures/DoWhileCST.h"
#include "ast/statements/Continue.h"
#include "cst/statements/ContinueCST.h"
#include "ast/statements/Break.h"
#include "cst/statements/BreakCST.h"
#include "cst/base/CSTConverter.h"

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

FunctionParamsResult CSTConverter::function_params(cst_tokens_ref_type tokens, unsigned start) {
    param_index = 0;
    auto isVariadic = false;
    func_params params;
    unsigned i = start;
    while (i < tokens.size()) {
        if (tokens[i]->compound()) {
            tokens[i]->accept(this);
            param_index++;
            auto param = (FunctionParam *) nodes.back().release();
            params.emplace_back(param);
            nodes.pop_back();
            if (param->isVariadic) {
                isVariadic = true;
                break;
            }
        } else if (tokens[i]->start_token()->type() == LexTokenType::CharOperator &&
                   char_op(tokens[i].get()) == ',') {
            // do nothing
        } else {
            break;
        }
        i++;
    }
    return {isVariadic, std::move(params), i};
}

void CSTConverter::visit(FunctionCST *function) {

    auto params = function_params(function->tokens, 3);

    auto i = params.index;

    if (char_op(function->tokens[i + 1].get()) == ':') {
        function->tokens[i + 2]->accept(this);
        i += 2;
    }

    auto returnType = opt_type();
    if (!returnType.has_value()) {
        returnType.emplace(std::make_unique<VoidType>());
    }

    auto funcDecl = new FunctionDeclaration(function->func_name(), std::move(params.params),
                                            std::move(returnType.value()), params.isVariadic,
                                            LoopScope{});

    nodes.emplace_back(std::unique_ptr<FunctionDeclaration>(funcDecl));

    auto prev_nodes = std::move(nodes);
    visit(function->tokens, i);
    current_func_decl = funcDecl;
    funcDecl->body->nodes = std::move(nodes);
    nodes = std::move(prev_nodes);


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

void CSTConverter::visit(ImportCST *cst) {
    std::vector<std::string> ids;
    nodes.emplace_back(std::make_unique<ImportStatement>(str_token(cst->tokens, 1), ids));
}

void CSTConverter::visit(ReturnCST *cst) {
    visit(cst->tokens, 1);
    nodes.emplace_back(std::make_unique<ReturnStatement>(opt_value(), current_func_decl));
}

void CSTConverter::visit(TypeToken *token) {
    auto primitive = primitive_type_map.find(token->value);
    if (primitive == primitive_type_map.end()) {
        types.emplace_back(std::make_unique<ReferencedType>(token->value));
    } else {
        types.emplace_back(std::unique_ptr<BaseType>(primitive->second()));
    }
}

void CSTConverter::visit(ContinueCST *continueCst) {
    nodes.emplace_back(std::make_unique<ContinueStatement>(current_loop_node));
}

void CSTConverter::visit(BreakCST *breakCST) {
    nodes.emplace_back(std::make_unique<BreakStatement>(current_loop_node));
}

void CSTConverter::visit(BodyCST *bodyCst) {
    visit(bodyCst->tokens, 0);
}

void CSTConverter::visit(ForLoopCST *forLoop) {

}

void CSTConverter::visit(WhileCST *whileCst) {
    // visiting the condition expression
    whileCst->tokens[2]->accept(this);
    // get it
    auto cond = value();
    // save current nodes
    auto previous = std::move(nodes);
    // construct a loop
    auto loop = new WhileLoop(std::move(cond), LoopScope{});
    // visit the body
    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    visit(whileCst->tokens, 3);
    current_loop_node = prevLoop;
    loop->body.nodes = std::move(nodes);
    // restore nodes
    nodes = std::move(previous);
    nodes.emplace_back(std::unique_ptr<ASTNode>(loop));
}

void CSTConverter::visit(DoWhileCST *doWhileCst) {

}

void CSTConverter::visit(PointerTypeCST *cst) {
    visit(cst->tokens, 0);
    types.emplace_back(std::make_unique<PointerType>(type()));
}

void CSTConverter::visit(GenericTypeCST *cst) {
    visit(cst->tokens, 0);
    types.emplace_back(std::make_unique<GenericType>(str_token(cst->tokens, 0), type()));
}

void CSTConverter::visit(ArrayTypeCST *arrayType) {
    convert(arrayType->tokens);
    auto val = opt_value();
    auto arraySize = (val.has_value() && val.value()->value_type() == ValueType::Int) ? val.value()->as_int() : -1;
    types.emplace_back(std::make_unique<ArrayType>(std::move(type()), arraySize));
}

void CSTConverter::visit(FunctionTypeCST *funcType) {
    auto params = function_params(funcType->tokens, 1);
    visit(funcType->tokens, params.index + 2);
    types.emplace_back(std::make_unique<FunctionType>(std::move(params.params), type(), params.isVariadic));
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

void CSTConverter::visit(FunctionCallCST *call) {
    visit(call->tokens, 1);
    values.emplace_back(std::make_unique<FunctionCall>(str_token(call->tokens, 0), std::move(values)));
}

void CSTConverter::visit(IndexOpCST *op) {
    visit(op->tokens, 1);
    values.emplace_back(std::make_unique<IndexOperator>(str_token(op->tokens, 0), value()));
}

void CSTConverter::visit(AccessChainCST *chain) {
    auto prev_values = std::move(values);
    visit(chain->tokens);
    auto ret_chain = std::make_unique<AccessChain>(std::move(values));
    values = std::move(prev_values);
    if (chain->is_node) {
        nodes.push_back(std::move(ret_chain));
    } else {
        values.push_back(std::move(ret_chain));
    }
}

void CSTConverter::visit(ExpressionCST *expr) {
    visit(expr->tokens);
    auto second = value();
    auto first = value();
    values.emplace_back(std::make_unique<Expression>(std::move(first), std::move(second),
                                                     ((OperationToken *) expr->tokens[expr->op_index].get())->op));
}

void CSTConverter::visit(VariableToken *token) {
    values.emplace_back(std::make_unique<VariableIdentifier>(token->value));
}

void CSTConverter::visit(BoolToken *token) {
    values.emplace_back(std::make_unique<BoolValue>(token->value));
}

void CSTConverter::visit(NegativeCST *neg) {
    visit(neg->tokens);
    values.emplace_back(std::make_unique<NegativeValue>(value()));
}

void CSTConverter::visit(NotCST *notCst) {
    visit(notCst->tokens);
    values.emplace_back(std::make_unique<NotValue>(value()));
}