// Copyright (c) Qinetik 2024.

#include "cst/structures/FunctionCST.h"
#include "lexer/model/tokens/AbstractStringToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "ast/structures/FunctionDeclaration.h"
#include "cst/statements/VarInitCST.h"
#include "ast/statements/VarInit.h"
#include "ast/types/IntNType.h"
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
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NullValue.h"
#include "ast/values/NotValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "lexer/model/tokens/StringToken.h"
#include "lexer/model/tokens/CharToken.h"
#include "ast/values/VariableIdentifier.h"
#include "lexer/model/tokens/OperationToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "lexer/model/tokens/BoolToken.h"
#include "ast/statements/Import.h"
#include "cst/statements/ImportCST.h"
#include "ast/structures/TryCatch.h"
#include "cst/structures/TryCatchCST.h"
#include "ast/statements/Return.h"
#include "cst/statements/ReturnCST.h"
#include "ast/types/GenericType.h"
#include "ast/structures/ForLoop.h"
#include "cst/structures/ForLoopCST.h"
#include "ast/structures/InterfaceDefinition.h"
#include "cst/structures/InterfaceCST.h"
#include "ast/structures/WhileLoop.h"
#include "ast/values/StructValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/structures/ImplDefinition.h"
#include "cst/structures/WhileCST.h"
#include "cst/structures/ImplCST.h"
#include "ast/structures/DoWhileLoop.h"
#include "cst/structures/DoWhileCST.h"
#include "ast/statements/Continue.h"
#include "cst/statements/ContinueCST.h"
#include "ast/statements/SwitchStatement.h"
#include "cst/statements/SwitchCST.h"
#include "ast/statements/Break.h"
#include "cst/statements/BreakCST.h"
#include "ast/statements/Typealias.h"
#include "cst/statements/TypealiasCST.h"
#include "ast/values/ArrayValue.h"
#include "cst/values/ArrayValueCST.h"
#include "cst/values/AddrOfCST.h"
#include "cst/values/DereferenceCST.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "cst/statements/IfCST.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "cst/statements/IncDecCST.h"
#include "cst/structures/StructDefCST.h"
#include "cst/values/StructValueCST.h"
#include "cst/base/CSTConverter.h"
#include "cst/values/LambdaCST.h"
#include "ast/values/CastedValue.h"
#include "cst/values/CastCST.h"
#include "cst/utils/ValueAndOperatorStack.h"

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

inline bool is_variable(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::Variable && str_token(token) == x;
}

inline bool is_char_op(CSTToken *token, char x) {
    return token->type() == LexTokenType::CharOperator && char_op(token) == x;
}

Scope take_body(CSTConverter *conv, CSTToken *token) {
    auto prev_nodes = std::move(conv->nodes);
    token->accept(conv);
    Scope scope(std::move(conv->nodes));
    conv->nodes = std::move(prev_nodes);
    return scope;
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

// TODO support _128bigint, bigfloat
CSTConverter::CSTConverter(bool is64Bit) {
    primitive_type_map["any"] = []() -> BaseType * {
        return new AnyType();
    };
    primitive_type_map["bool"] = []() -> BaseType * {
        return new BoolType();
    };
    primitive_type_map["char"] = []() -> BaseType * {
        return new CharType();
    };
    primitive_type_map["double"] = []() -> BaseType * {
        return new DoubleType();
    };
    primitive_type_map["float"] = []() -> BaseType * {
        return new FloatType();
    };
    primitive_type_map["int"] = []() -> BaseType * {
        return new IntNType(32);
    };
    primitive_type_map["uint"] = []() -> BaseType * {
        return new IntNType(32, true);
    };
    primitive_type_map["short"] = []() -> BaseType * {
        return new IntNType(16);
    };
    primitive_type_map["ushort"] = []() -> BaseType * {
        return new IntNType(16, true);
    };
    primitive_type_map["long"] = [is64Bit]() -> BaseType * {
        return new IntNType(is64Bit ? 64 : 32);
    };
    primitive_type_map["ulong"] = [is64Bit]() -> BaseType * {
        return new IntNType(is64Bit ? 64 : 32, true);
    };
    primitive_type_map["bigint"] = []() -> BaseType * {
        return new IntNType(64);
    };
    primitive_type_map["ubigint"] = []() -> BaseType * {
        return new IntNType(64, true);
    };
    primitive_type_map["string"] = []() -> BaseType * {
        return new StringType();
    };
    primitive_type_map["void"] = []() -> BaseType * {
        return new VoidType();
    };
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
    BaseType *baseType;
    if (optional_param_types) {
        auto t = opt_type();
        if (t.has_value()) {
            baseType = t.value().release();
        } else {
            baseType = new VoidType();
        }
    } else {
        baseType = type().release();
    }
    nodes.emplace_back(
            std::make_unique<FunctionParam>(identifier, std::unique_ptr<BaseType>(baseType), param_index, isVariadic,
                                            opt_value()));
}

// will probably leave the index at ')'
FunctionParamsResult CSTConverter::function_params(cst_tokens_ref_type tokens, unsigned start) {
    auto prev_param_index = param_index;
    param_index = 0;
    auto isVariadic = false;
    func_params params;
    unsigned i = start;
    while (i < tokens.size()) {
        if (param_index == 0 && is_char_op(tokens[i]->start_token(), '&')) {
            auto &paramTokens = ((FunctionParamCST *) tokens[i].get())->tokens;
            auto strId = str_token(paramTokens[1].get());
            if (strId != "this" && strId != "self") {
                error("expected self parameter to be named 'self' or 'this'", tokens[i].get());
            }
            auto type = current_struct_decl ? current_struct_decl->name : (current_interface_decl
                                                                           ? current_interface_decl->name
                                                                           : current_impl_decl->struct_name.value());
            params.emplace_back(new FunctionParam(strId, std::make_unique<PointerType>(
                    std::make_unique<ReferencedType>(type)), 0, false, std::nullopt));
        } else if (tokens[i]->compound()) {
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
        } else if (is_char_op(tokens[i].get(), ',')) {
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
    if (i < function->tokens.size()) {
        fnBody.emplace(LoopScope{});
    }

    auto funcDecl = new FunctionDeclaration(function->func_name(), std::move(params.params),
                                            std::move(returnType.value()), params.isVariadic,
                                            std::move(fnBody));

    nodes.emplace_back(std::unique_ptr<FunctionDeclaration>(funcDecl));

    if (i >= function->tokens.size()) {
        return;
    }

    auto prev_decl = current_func_decl;
    auto prev_nodes = std::move(nodes);
    current_func_decl = funcDecl;
    function->tokens[i]->accept(this);
    funcDecl->body->nodes = std::move(nodes);
    nodes = std::move(prev_nodes);
    current_func_decl = prev_decl;


}

void CSTConverter::visit(VarInitCST *varInit) {
    visit(varInit->tokens, 3);
    std::optional<std::unique_ptr<BaseType>> optType = std::nullopt;
    if(is_char_op(varInit->tokens[2].get(), ':')) {
        optType.emplace(type());
        expected_type = optType.value().get();
    }
    std::optional<std::unique_ptr<Value>> optVal = std::nullopt;
    auto token = varInit->tokens[varInit->tokens.size() - 1].get();
    if(token->is_value()) {
        optVal.emplace(value());
    }
    nodes.emplace_back(std::make_unique<VarInitStatement>(
            varInit->is_const(),
            varInit->identifier(),
            std::move(optType),
            std::move(optVal)
    ));
    expected_type = nullptr;
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

void CSTConverter::visit(TypealiasCST *alias) {
    auto identifier = str_token(alias->tokens[1].get());
    alias->tokens[3]->accept(this);
    nodes.emplace_back(std::make_unique<TypealiasStatement>(identifier, type()));
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

    auto no_capture_list = is_char_op(cst->tokens[0].get(), '(');

    unsigned i = 1;
    if (!no_capture_list) {
        while (!is_char_op(cst->tokens[i].get(), ']')) {
            if (cst->tokens[i]->type() == LexTokenType::Variable) {
                captureList.push_back(((VariableToken *) (cst->tokens[i].get()))->value);
            }
            i++;
        }
        i += 2;
    }

    auto prev = optional_param_types;
    optional_param_types = true;
    auto result = function_params(cst->tokens, i);
    optional_param_types = prev;

    Scope scope;
    auto bodyIndex = result.index + 2;
    if (cst->tokens[bodyIndex]->type() == LexTokenType::CompBody) {
        auto prev_nodes = std::move(nodes);
        auto prev_decl = current_func_decl;
        current_func_decl = nullptr;
        cst->tokens[bodyIndex]->accept(this);
        current_func_decl = prev_decl;
        scope.nodes = std::move(nodes);
        nodes = std::move(prev_nodes);
    } else {
        visit(cst->tokens, bodyIndex);
        scope.nodes.emplace_back(new ReturnStatement(value(), nullptr));
    }

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

unsigned int collect_struct_members(
        CSTConverter *conv,
        std::vector<std::unique_ptr<CSTToken>> &tokens,
        std::map<std::string, std::unique_ptr<StructMember>> &variables,
        std::map<std::string, std::unique_ptr<FunctionDeclaration>> &decls,
        unsigned i
) {
    while (!is_char_op(tokens[i].get(), '}')) {

        tokens[i]->accept(conv);
        auto is_var_init = tokens[i]->is_var_init();
        auto is_func_decl = tokens[i]->is_func_decl();
        if (!is_var_init && !is_func_decl) {
            i++;
            continue;
        }
        auto node = conv->nodes.back().release();
        conv->nodes.pop_back();
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

        if (is_char_op(tokens[i + 1].get(), ';')) {
            i++;
        }

        i++;

    }
    return i;
}

void CSTConverter::visit(StructDefCST *structDef) {
    std::optional<std::string> overrides = std::nullopt;
    auto has_override = is_char_op(structDef->tokens[2].get(), ':');
    if (has_override) {
        overrides.emplace(str_token(structDef->tokens[3].get()));
    }
    unsigned i = has_override ? 5 : 3; // positioned at first node or '}'
    auto def = new StructDefinition(str_token(structDef->tokens[1].get()), overrides);
    current_struct_decl = def;
    collect_struct_members(this, structDef->tokens, def->variables, def->functions, i);
    current_struct_decl = nullptr;
    nodes.emplace_back(def);
}

void CSTConverter::visit(InterfaceCST *interface) {
    auto def = new InterfaceDefinition(str_token(interface->tokens[1].get()));
    unsigned i = 3; // positioned at first node or '}'
    current_interface_decl = def;
    collect_struct_members(this, interface->tokens, def->variables, def->functions, i);
    current_interface_decl = nullptr;
    nodes.emplace_back(def);
}

void CSTConverter::visit(ImplCST *impl) {
    bool has_for = is_keyword(impl->tokens[2].get(), "for");
    std::optional<std::string> struct_name;
    if (has_for) {
        struct_name.emplace(str_token(impl->tokens[3].get()));
    } else {
        struct_name = std::nullopt;
    }
    auto def = new ImplDefinition(str_token(impl->tokens[1].get()), struct_name);
    unsigned i = has_for ? 5 : 3; // positioned at first node or '}'
    current_impl_decl = def;
    collect_struct_members(this, impl->tokens, def->variables, def->functions, i);
    current_impl_decl = nullptr;
    nodes.emplace_back(def);
}

void CSTConverter::visit(TryCatchCST *tryCatch) {
    auto chain = ((AccessChainCST *) tryCatch->tokens[1].get());
    if (chain->tokens.size() != 1 || chain->tokens[0]->type() != LexTokenType::CompFunctionCall) {
        error("expected a function call after try keyword", chain);
        return;
    }
    chain->accept(this);
    auto call = value().release()->as_access_chain();
    std::optional<Scope> catchScope = std::nullopt;
    auto last = tryCatch->tokens[tryCatch->tokens.size() - 1].get();
    if (is_keyword(tryCatch->tokens[2].get(), "catch") && last->type() == LexTokenType::CompBody) {
        catchScope.emplace(take_body(this, last));
    }
    // TODO catch variable not supported yet
    nodes.emplace_back(
            new TryCatch(std::unique_ptr<FunctionCall>((FunctionCall *) call->values[0].release()), std::nullopt,
                         std::move(catchScope)));
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
    bool is_capturing = is_char_op(funcType->tokens[0].get(), '[');
    auto params = function_params(funcType->tokens, is_capturing ? 3 : 1);
    visit(funcType->tokens, params.index + 2);
    types.emplace_back(
            std::make_unique<FunctionType>(std::move(params.params), type(), params.isVariadic, is_capturing));
}


void CSTConverter::visit(StringToken *token) {
    values.emplace_back(std::make_unique<StringValue>(token->value));
}

void CSTConverter::visit(CharToken *token) {
    values.emplace_back(std::make_unique<CharValue>(token->value));
}

void CSTConverter::visit(NumberToken *token) {
    try {
        if(expected_type != nullptr) {
            auto value_type = expected_type->value_type();
            switch(value_type) {
                case ValueType::Int:
                    values.emplace_back(new IntValue(std::stoi(token->value)));
                    break;
                case ValueType::UInt:
                    values.emplace_back(new UIntValue(std::stoi(token->value)));
                    break;
                case ValueType::Short:
                    values.emplace_back(new ShortValue(std::stoi(token->value)));
                    break;
                case ValueType::UShort:
                    values.emplace_back(new UShortValue(std::stoi(token->value)));
                    break;
                case ValueType::Long:
                    values.emplace_back(new LongValue(std::stol(token->value)));
                    break;
                case ValueType::ULong:
                    values.emplace_back(new ULongValue(std::stoul(token->value)));
                    break;
                case ValueType::BigInt:
                    values.emplace_back(new BigIntValue(std::stoll(token->value)));
                    break;
                case ValueType::UBigInt:
                    values.emplace_back(new UBigIntValue(std::stoull(token->value)));
                    break;
                case ValueType::Float:
                    values.emplace_back(new FloatValue(std::stof(token->value)));
                    break;
                case ValueType::Double:
                    values.emplace_back(new DoubleValue(std::stod(token->value)));
                    break;
                default:
                    error("expected value type" + to_string(value_type) + " cannot take a number", token);
            }
            return;
        }
        if (token->has_dot()) {
            if (token->is_float()) {
                std::string substring = token->value.substr(0, token->value.size() - 1);
                values.emplace_back(new FloatValue(std::stof(substring)));
            } else {
                values.emplace_back(new DoubleValue(std::stod(token->value)));
            }
        } else {
            if(token->is_long()) {
                if(token->is_unsigned()) {
                    values.emplace_back(new ULongValue(std::stoul(token->value)));
                } else {
                    values.emplace_back(new LongValue(std::stol(token->value)));
                }
            } else {
                values.emplace_back(new IntValue(std::stoi(token->value)));
            }
        }
    } catch (...) {
        error("invalid value provided", token);
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

std::vector<std::unique_ptr<Value>> take_values(CSTConverter *converter, const std::function<void()> &visit) {
    auto prev_values = std::move(converter->values);
    visit();
    auto new_values = std::move(converter->values);
    converter->values = std::move(prev_values);
    return new_values;
}

void CSTConverter::visit(FunctionCallCST *call) {
    auto prev_values = std::move(values);
    visit(call->tokens, 1);
    auto func_call = new FunctionCall(std::move(values));
    values = std::move(prev_values);
    values.emplace_back(func_call);
}

void CSTConverter::visit(IndexOpCST *op) {
    auto indexes = take_values(this, [&op, this]() {
        visit(op->tokens, 1);
    });
    values.emplace_back(std::make_unique<IndexOperator>(std::move(indexes)));
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

// shunting yard on right parenthesis
void sy_onRParen(ValueAndOperatorStack &op_stack, ValueAndOperatorStack &output) {
    //              while the operator at the top of the operator stack is not a left parenthesis:
    while (!(op_stack.has_character_top() && op_stack.peakChar() == '(')) {
//                  {assert the operator stack is not empty}
//                  /* If the stack runs out without finding a left parenthesis, then there are mismatched parentheses. */
//                  pop the operator from the operator stack into the output queue
        output.putOperator(op_stack.popOperator());
    }
//              {assert there is a left parenthesis at the top of the operator stack}
//              pop the left parenthesis from the operator stack and discard it
    op_stack.popChar();
//              if there is a function token at the top of the operator stack, then:
    if (op_stack.has_value_top()) {
//                  pop the function from the operator stack into the output queue
        output.putValue(op_stack.popValue());
    }
}

/**
 * This is a shunting yard implementation, however this is a little complex because it is implemented for a CST
 * but the concept is same, only catch is recursion is being used to traverse to the deeply nested first expression to put
 * it values, operators or parens in order to simulate a flattened CST
 * https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 */
void visitNestedExpr(CSTConverter *converter, CSTToken *expr, ValueAndOperatorStack &op_stack,
                     ValueAndOperatorStack &output) {
    if (expr->is_value()) {
        if (expr->type() != LexTokenType::CompExpression) {
            if (expr->type() == LexTokenType::CompFunctionCall) {
                // - a function:
                // push it onto the operator stack
                expr->accept(converter);
                op_stack.putValue(converter->value().release());
            } else {
                // - a number:
                // put it into the output queue
                expr->accept(converter);
                output.putValue(converter->value().release());
            }
        } else {
            auto nested = ((ExpressionCST *) expr);
            auto is_braced = is_char_op(nested->tokens[0].get(), '(');
            auto first_val_index = is_braced ? 1 : 0;
            auto op_index = is_braced ? 3 : 1;
            auto second_val_index = op_index + 1;
            if (is_braced) { //    - a left parenthesis (i.e. "("):
                // push it onto the operator stack
                op_stack.putCharacter('(');
            }
            // visiting the first value
            visitNestedExpr(converter, nested->tokens[first_val_index].get(), op_stack, output);
            if (is_braced) { // a right parenthesis (i.e. ")"):
                sy_onRParen(op_stack, output);
            }
            auto o1 = ((OperationToken *) nested->tokens[op_index].get())->op;
//            while (
//                   there is an operator o2 at the top of the operator stack which is not a left parenthesis,
//                   and (o2 has greater precedence than o1 or (o1 and o2 have the same precedence and o1 is left-associative))
//            ):
            auto o1precedence = to_precedence(o1);
            while (op_stack.has_operation_top() &&
                   (to_precedence(op_stack.peakOperator()) > o1precedence ||
                    (to_precedence(op_stack.peakOperator()) == o1precedence && is_assoc_left_to_right(o1)))) {
//              pop o2 from the operator stack into the output queue
                output.putOperator(op_stack.popOperator());
            }
            // push o1 onto the operator stack
            op_stack.putOperator(o1);
            // visiting the second value
            if (is_char_op(nested->tokens[second_val_index].get(), '(')) {
                op_stack.putCharacter('(');
                visitNestedExpr(converter, nested->tokens[second_val_index + 1].get(), op_stack, output);
                sy_onRParen(op_stack, output);
            } else {
                visitNestedExpr(converter, nested->tokens[second_val_index].get(), op_stack, output);
            }
        }
    } else {
        throw std::runtime_error("unknown type of value provided to visitNestedExpr");
    }
}

void CSTConverter::visit(ExpressionCST *expr) {
    auto is_braced = is_char_op(expr->tokens[0].get(), '(');
    auto first_val_index = is_braced ? 1 : 0;
    auto op_index = is_braced ? 3 : 1;
    auto second_val_index = op_index + 1;
    if (expr->tokens[first_val_index]->is_primitive_var() && expr->tokens[second_val_index]->is_primitive_var()) {
        // no need to create stacks for values that are primitive variables, a single expression like 1 + 2 or x + 1
        visit(expr->tokens);
        auto second = value();
        auto first = value();
        values.emplace_back(std::make_unique<Expression>(std::move(first), std::move(second),
                                                         ((OperationToken *) expr->tokens[op_index].get())->op));
    } else {
        ValueAndOperatorStack op_stack, output;
        visitNestedExpr(this, expr, op_stack, output);
        //  while there are tokens on the operator stack:
        while (!op_stack.empty()) {
            //    /* If the operator token on the top of the stack is a parenthesis, then there are mismatched parentheses. */
            //    {assert the operator on top of the stack is not a (left) parenthesis}
            //    pop the operator from the operator stack onto the output queue
            output.putOperator(op_stack.popOperator());
        }
        values.emplace_back(output.toExpression());
    }
}

void CSTConverter::visit(CastCST *castCst) {
    visit(castCst->tokens);
    values.emplace_back(std::make_unique<CastedValue>(value(), type()));
}

void CSTConverter::visit(AddrOfCST *addrOf) {
    addrOf->tokens[1]->accept(this);
    values.emplace_back(std::make_unique<AddrOfValue>(value()));
}

void CSTConverter::visit(DereferenceCST *deref) {
    deref->tokens[1]->accept(this);
    values.emplace_back(std::make_unique<DereferenceValue>(value()));
}

void CSTConverter::visit(VariableToken *token) {
    values.emplace_back(std::make_unique<VariableIdentifier>(token->value));
}

void CSTConverter::visit(BoolToken *token) {
    values.emplace_back(std::make_unique<BoolValue>(token->value));
}

void CSTConverter::visit(NullToken *token) {
    values.emplace_back(std::make_unique<NullValue>());
}

void CSTConverter::visit(NegativeCST *neg) {
    visit(neg->tokens);
    values.emplace_back(std::make_unique<NegativeValue>(value()));
}

void CSTConverter::visit(NotCST *notCst) {
    visit(notCst->tokens);
    values.emplace_back(std::make_unique<NotValue>(value()));
}