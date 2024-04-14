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
#include "ast/values/StructValue.h"
#include "cst/structures/WhileCST.h"
#include "ast/structures/DoWhileLoop.h"
#include "cst/structures/DoWhileCST.h"
#include "ast/statements/Continue.h"
#include "cst/statements/ContinueCST.h"
#include "ast/statements/SwitchStatement.h"
#include "cst/statements/SwitchCST.h"
#include "ast/statements/Break.h"
#include "cst/statements/BreakCST.h"
#include "ast/values/ArrayValue.h"
#include "cst/values/ArrayValueCST.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "cst/statements/IfCST.h"
#include "ast/structures/StructDefinition.h"
#include "cst/statements/IncDecCST.h"
#include "cst/structures/StructDefCST.h"
#include "cst/values/StructValueCST.h"
#include "cst/base/CSTConverter.h"
#include "cst/values/LambdaCST.h"

using tokens_vec_type = std::vector<std::unique_ptr<CSTToken>> &;

inline std::string str_token(CSTToken *token) {
    return static_cast<AbstractStringToken *>(token)->value;
}

inline std::string str_token(tokens_vec_type tokens, unsigned int index) {
    return str_token(tokens[index].get());
}

inline char char_op(CSTToken *token) {
    return static_cast<CharOperatorToken *>(token)->op;
}

inline bool is_keyword(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::Keyword && str_token(token) == x;
}

inline bool is_char_op(CSTToken *token, char x) {
    return token->type() == LexTokenType::CharOperator && char_op(token) == x;
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

void CSTConverter::visit(FunctionParamCST *param) {
    auto identifier = str_token(param->tokens, 0);
    visit(param->tokens, 2);
    auto lastToken = param->tokens[param->tokens.size() - 1].get();
    auto isVariadic = !lastToken->compound() && lastToken->start_token()->is_abs_string() &&
                      ((AbstractStringToken *) lastToken)->value == "...";
    BaseType* baseType;
    if (optional_param_types) {
        auto t = opt_type();
        if(t.has_value()) {
            baseType = t.value().release();
        } else {
            baseType = new VoidType();
        }
    } else {
        baseType = type().release();
    }
    nodes.emplace_back(
            std::make_unique<FunctionParam>(identifier, std::unique_ptr<BaseType>(baseType), param_index, isVariadic, opt_value()));
}

// will probably leave the index at ')'
FunctionParamsResult CSTConverter::function_params(cst_tokens_ref_type tokens, unsigned start) {
    auto prev_param_index = param_index;
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
                i++;
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
    param_index = prev_param_index;
    return {isVariadic, std::move(params), i};
}

void CSTConverter::visit(FunctionCST *function) {

    auto params = function_params(function->tokens, 3);

    auto i = params.index;

    if (char_op(function->tokens[i + 1].get()) == ':') {
        function->tokens[i + 2]->accept(this);
        i += 3; // position at body
    } else {
        i++;
    }

    auto returnType = opt_type();
    if (!returnType.has_value()) {
        returnType.emplace(std::make_unique<VoidType>());
    }

    std::optional<LoopScope> fnBody = std::nullopt;
    if(i < function->tokens.size()) {
        fnBody.emplace(LoopScope{});
    }

    auto funcDecl = new FunctionDeclaration(function->func_name(), std::move(params.params),
                                            std::move(returnType.value()), params.isVariadic,
                                            std::move(fnBody));

    nodes.emplace_back(std::unique_ptr<FunctionDeclaration>(funcDecl));

    if(i >= function->tokens.size()) {
        return;
    }

    auto prev_nodes = std::move(nodes);
    function->tokens[i]->accept(this);
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

void CSTConverter::visit(IncDecCST *incDec) {
    incDec->tokens[0]->accept(this);
    auto acOp = ((OperationToken *) incDec->tokens[1].get())->op == Operation::PostfixIncrement ? Operation::Addition
                                                                                                : Operation::Subtraction;
    nodes.emplace_back(std::make_unique<AssignStatement>(value(), std::make_unique<IntValue>(1), acOp));
}

void CSTConverter::visit(LambdaCST *cst) {

    std::vector<std::string> captureList;

    unsigned i = 1;
    while (!is_char_op(cst->tokens[i].get(), ']')) {
        if (cst->tokens[i]->type() == LexTokenType::Variable) {
            captureList.push_back(((VariableToken *) (cst->tokens[i].get()))->value);
        }
        i++;
    }
    i += 2;

    auto prev = optional_param_types;
    optional_param_types = true;
    auto result = function_params(cst->tokens, i);
    optional_param_types = prev;

    auto prev_nodes = std::move(nodes);
    auto prev_decl = current_func_decl;
    current_func_decl = nullptr;
    cst->tokens[result.index + 2]->accept(this);
    current_func_decl = prev_decl;
    auto scope = Scope(std::move(nodes));
    nodes = std::move(prev_nodes);

    values.emplace_back(
            std::make_unique<LambdaFunction>(std::move(captureList), std::move(result.params), result.isVariadic,
                                             std::move(scope)));
}

void CSTConverter::visit(BodyCST *bodyCst) {
    visit(bodyCst->tokens, 0);
}

void CSTConverter::visit(IfCST *ifCst) {

    // if condition
    ifCst->tokens[2]->accept(this);
    auto cond = value();

    // first if body
    auto prev_nodes = std::move(nodes);
    ifCst->tokens[4]->accept(this);
    auto body_nodes = std::move(nodes);
    nodes = std::move(prev_nodes);

    std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs;

    auto i = 5; // position after body
    while ((i + 1) < ifCst->tokens.size() && is_keyword(ifCst->tokens[i + 1].get(), "if")) {

        i += 3;
        ifCst->tokens[i]->accept(this);
        auto elseIfCond = value();
        i += 2;

        // else if body
        prev_nodes = std::move(nodes);
        ifCst->tokens[i]->accept(this);
        elseIfs.emplace_back(std::move(elseIfCond), Scope(std::move(nodes)));
        nodes = std::move(prev_nodes);

        // position after the body
        i++;

    }

    std::optional<Scope> elseBody = std::nullopt;

    // last else
    if (i < ifCst->tokens.size() && str_token(ifCst->tokens[i].get()) == "else") {
        prev_nodes = std::move(nodes);
        ifCst->tokens[i + 1]->accept(this);
        elseBody.emplace(Scope(std::move(nodes)));
        nodes = std::move(prev_nodes);
    }

    nodes.emplace_back(std::make_unique<IfStatement>(std::move(cond), Scope(std::move(body_nodes)), std::move(elseIfs),
                                                     std::move(elseBody)));

}

void CSTConverter::visit(SwitchCST *switchCst) {
    switchCst->tokens[2]->accept(this);
    auto expr = value();
    auto i = 5; // positioned at first 'case' or 'default'
    std::vector<std::pair<std::unique_ptr<Value>, Scope>> cases;
    std::optional<Scope> defScope = std::nullopt;
    auto has_default = false;
    while (true) {
        if (is_keyword(switchCst->tokens[i].get(), "case")) {
            i++;
            switchCst->tokens[i]->accept(this);
            auto caseVal = value();
            i += 2; // body
            auto prev_nodes = std::move(nodes);
            switchCst->tokens[i]->accept(this);
            auto scope = Scope(std::move(nodes));
            nodes = std::move(prev_nodes);
            cases.emplace_back(std::move(caseVal), std::move(scope));
            i++;
        } else if (is_keyword(switchCst->tokens[i].get(), "default")) {
            i += 2; // body
            auto prev_nodes = std::move(nodes);
            switchCst->tokens[i]->accept(this);
            defScope.emplace(Scope(std::move(nodes)));
            nodes = std::move(prev_nodes);
            i++;
            if (has_default) {
                error("multiple defaults in switch statement detected", switchCst->tokens[i - 3].get());
            }
            has_default = true;
        } else {
            break;
        }
    }
    nodes.emplace_back(std::make_unique<SwitchStatement>(std::move(expr), std::move(cases), std::move(defScope)));
}

void CSTConverter::visit(ForLoopCST *forLoop) {

    forLoop->tokens[2]->accept(this);
    auto varInit = nodes.back().release()->as_var_init();
    nodes.pop_back();
    forLoop->tokens[4]->accept(this);
    auto cond = value();
    forLoop->tokens[6]->accept(this);
    auto assignment = (AssignStatement *) (nodes.back().release());
    nodes.pop_back();

    auto loop = new ForLoop(std::unique_ptr<VarInitStatement>(varInit),
                            std::move(cond),
                            std::unique_ptr<ASTNode>(assignment));

    auto prevLoop = current_loop_node;
    auto prev_nodes = std::move(nodes);
    current_loop_node = loop;
    forLoop->tokens[8]->accept(this);
    loop->body.nodes = std::move(nodes);
    nodes = std::move(prev_nodes);
    current_loop_node = prevLoop;

    nodes.emplace_back(std::unique_ptr<ForLoop>(loop));

}

void CSTConverter::visit(WhileCST *whileCst) {
    // visiting the condition expression
    whileCst->tokens[2]->accept(this);
    // get it
    auto cond = value();
    // construct a loop
    auto loop = new WhileLoop(std::move(cond), LoopScope{});
    // save current nodes
    auto previous = std::move(nodes);
    // visit the body
    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    whileCst->tokens[4]->accept(this);
    current_loop_node = prevLoop;
    loop->body.nodes = std::move(nodes);
    // restore nodes
    nodes = std::move(previous);
    nodes.emplace_back(std::unique_ptr<ASTNode>(loop));
}

void CSTConverter::visit(DoWhileCST *doWhileCst) {
    // visit the 2nd last token which is the expression for condition
    auto cond_index = doWhileCst->tokens.size() - 2;
    doWhileCst->tokens[cond_index]->accept(this);
    // get it
    auto cond = value();
    // construct a loop
    auto loop = new DoWhileLoop(std::move(cond), LoopScope{});
    // save current nodes
    auto previous = std::move(nodes);
    // visit the body
    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    doWhileCst->tokens[1]->accept(this);
    current_loop_node = prevLoop;
    loop->body.nodes = std::move(nodes);
    // restore nodes
    nodes = std::move(previous);
    nodes.emplace_back(std::unique_ptr<ASTNode>(loop));
}

void CSTConverter::visit(StructDefCST *structDef) {
    std::optional<std::string> overrides = std::nullopt;
    auto has_override = is_char_op(structDef->tokens[3].get(), ':');
    if (has_override) {
        overrides.emplace(str_token(structDef->tokens[4].get()));
    }
    unsigned i = has_override ? 5 : 3; // positioned at first node or '}'
    std::map<std::string, std::unique_ptr<StructMember>> variables;
    std::map<std::string, std::unique_ptr<FunctionDeclaration>> decls;
    while (!is_char_op(structDef->tokens[i].get(), '}')) {

        structDef->tokens[i]->accept(this);
        auto is_var_init = structDef->tokens[i]->is_var_init();
        auto node = nodes.back().release();
        nodes.pop_back();
        if (is_var_init) {
            auto init = ((VarInitStatement *) node);
            variables[init->identifier] = std::make_unique<StructMember>(init->identifier,
                                                                         std::move(init->type.value()),
                                                                         std::move(init->value));
            delete init;
        } else {
            decls[((FunctionDeclaration *) node)->name] = std::unique_ptr<FunctionDeclaration>(
                    (FunctionDeclaration *) node);
        }

        if (is_char_op(structDef->tokens[i + 1].get(), ';')) {
            i++;
        }

        i++;

    }

    nodes.emplace_back(std::make_unique<StructDefinition>(str_token(structDef->tokens[1].get()), std::move(variables),
                                                          std::move(decls), std::move(overrides)));

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

void CSTConverter::visit(StructValueCST *cst) {
    auto name = str_token(cst->tokens[0].get());
    auto i = 2; // first identifier or '}'
    std::unordered_map<std::string, std::unique_ptr<Value>> vals;
    while (!is_char_op(cst->tokens[i].get(), '}')) {
        auto id = str_token(cst->tokens[i].get());
        i += 2;
        cst->tokens[i]->accept(this);
        vals[id] = value();
        i++;
        if (is_char_op(cst->tokens[i].get(), ',')) {
            i++;
        }
    }
    values.emplace_back(std::make_unique<StructValue>(name, std::move(vals)));
}

void CSTConverter::visit(ArrayValueCST *arrayValue) {
    unsigned i = 1;
    std::vector<std::unique_ptr<Value>> arrValues;
    while (char_op(arrayValue->tokens[i].get()) != '}') {
        if (char_op(arrayValue->tokens[i].get()) != ',') {
            arrayValue->tokens[i]->accept(this);
            // consume the value
            arrValues.emplace_back(value());
        }
        i++;
    }
    i++;
    std::optional<std::unique_ptr<BaseType>> arrType = std::nullopt;
    std::vector<unsigned int> sizes;
    if (i < arrayValue->tokens.size()) {
        arrayValue->tokens[i++]->accept(this);
        arrType = opt_type();
        if (i < arrayValue->tokens.size() && char_op(arrayValue->tokens[i++].get()) == '(') {
            while (i < arrayValue->tokens.size() && char_op(arrayValue->tokens[i].get()) != ')') {
                if (char_op(arrayValue->tokens[i].get()) != ',') {
                    arrayValue->tokens[i]->accept(this);
                    // consume the value
                    sizes.emplace_back(value()->as_int());
                }
                i++;
            }
        }
    }
    values.emplace_back(std::make_unique<ArrayValue>(std::move(arrValues), std::move(arrType), std::move(sizes)));
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