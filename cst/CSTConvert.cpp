// Copyright (c) Qinetik 2024.

#include "cst/structures/FunctionCST.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/IntNType.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/ReferencedType.h"
#include "ast/statements/Assignment.h"
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
#include "lexer/model/tokens/CharToken.h"
#include "ast/values/VariableIdentifier.h"
#include "lexer/model/tokens/OperationToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "ast/statements/Import.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/statements/Return.h"
#include "ast/types/GenericType.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/WhileLoop.h"
#include "ast/values/StructValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/UnionDef.h"
#include "cst/structures/WhileCST.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/statements/Continue.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/Break.h"
#include "ast/statements/Typealias.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "cst/base/CSTConverter.h"
#include "ast/values/CastedValue.h"
#include "cst/utils/ValueAndOperatorStack.h"
#include "ast/values/NumberValue.h"
#include "utils/StringHelpers.h"
#include "utils/CSTUtils.h"
#include <functional>
#include <sstream>
#include "compiler/PrimitiveTypeMap.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/statements/ThrowStatement.h"
#include "preprocess/RepresentationVisitor.h"
#include "preprocess/2cASTVisitor.h"
#include "ast/values/SizeOfValue.h"

Operation get_operation(CSTToken *token) {
    auto op = (OperationToken*) token;
    std::string num;
    unsigned i = 0;
    while(i < op->value.size()) {
        if(std::isdigit(op->value[i])) {
            num.append(1, op->value[i]);
        }
        i++;
    }
    return (Operation) std::stoi(num);
}

Scope take_body(CSTConverter *conv, CSTToken *token) {
    auto prev_nodes = std::move(conv->nodes);
    token->accept(conv);
    Scope scope(std::move(conv->nodes));
    conv->nodes = std::move(prev_nodes);
    return scope;
}

Scope take_body(CSTConverter *conv, CompoundCSTToken *token) {
    auto prev_nodes = std::move(conv->nodes);
    conv->visit(token->tokens, 0);
    Scope scope(std::move(conv->nodes));
    conv->nodes = std::move(prev_nodes);
    return scope;
}

// TODO support _128bigint, bigfloat
CSTConverter::CSTConverter(bool is64Bit, std::string target) : is64Bit(is64Bit), target(std::move(target)), global_scope(nullptr, nullptr, "") {
    ExpressionEvaluator::prepareFunctions(global_scope);
    init_macro_handlers();
    init_annotation_handlers();
}

void CSTConverter::init_macro_handlers() {
    macro_handlers["eval"] = [](CSTConverter* converter, CompoundCSTToken* container){
        if(container->tokens[2]->is_value()) {
            container->tokens[2]->accept(converter);
            auto take_value = converter->value();
            InterpretScope child_scope{&converter->global_scope, &converter->global_scope, nullptr, nullptr};
            auto evaluated_value = take_value->evaluated_value(child_scope);
            if(evaluated_value == nullptr) {
                converter->error("couldn't evaluate value", container);
                return;
            }
            converter->values.emplace_back(evaluated_value);
            if(!take_value->computed()) {
                take_value.release(); // release in case evaluated_value and take_value are equal
            }
        } else {
            converter->error("expected a value for eval", container);
        }
    };
    macro_handlers["target"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        converter->values.emplace_back(new StringValue(converter->target));
    };
    macro_handlers["target:is64bit"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        converter->values.emplace_back(new BoolValue(converter->is64Bit));
    };
    macro_handlers["sizeof"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        if(container->tokens[2]->is_type()) {
            container->tokens[2]->accept(converter);
            auto value = new SizeOfValue(converter->type().release());
            converter->values.emplace_back(value);
        } else {
            converter->error("expected a type in sizeof", container);
        }
    };
    macro_handlers["tr:debug:chemical"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        auto body = take_body(converter, container);
        std::ostringstream ostring;
        RepresentationVisitor visitor(ostring);
        visitor.translate(body.nodes);
        converter->values.emplace_back(new StringValue(ostring.str()));
    };
    macro_handlers["tr:debug:chemical:value"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        if(container->is_value()) {
            container->accept(converter);
            auto value = converter->value();
            std::ostringstream ostring;
            RepresentationVisitor visitor(ostring);
            value->accept(&visitor);
            converter->values.emplace_back(new StringValue(ostring.str()));
        } else {
            converter->error("expected a value in tr:debug:chemical:value", container);
        }
    };
    macro_handlers["tr:debug:c"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        auto body = take_body(converter, container);
        std::ostringstream ostring;
        ToCAstVisitor visitor(&ostring, "");
        visitor.translate(body.nodes);
        converter->values.emplace_back(new StringValue(ostring.str()));
    };
    macro_handlers["tr:debug:c:value"] = [](CSTConverter* converter, CompoundCSTToken* container) {
        if(container->is_value()) {
            container->accept(converter);
            auto value = converter->value();
            std::ostringstream ostring;
            ToCAstVisitor visitor(&ostring, "");
            value->accept(&visitor);
            converter->values.emplace_back(new StringValue(ostring.str()));
        } else {
            converter->error("expected a value in tr:debug:c:value", container);
        }
    };
}

void collect_annotation_func(CSTConverter* converter, CSTToken* container, AnnotationKind kind) {
    converter->annotations.emplace_back(kind);
    if(!container->compound()) return;
    auto compound = container->as_compound();
    auto previous = std::move(converter->values);
    visit(converter, compound->tokens, 2);
    auto collected = std::move(converter->values);
    converter->values = std::move(previous);
    converter->annotations.back().values = std::move(collected);
}

void ignore_annotation_func(CSTConverter* converter, CSTToken* container) {
    // do nothing
}

void CSTConverter::init_annotation_handlers() {
    annotation_handlers["cbi:create"] = ignore_annotation_func;
    annotation_handlers["cbi:import"] = ignore_annotation_func;
    annotation_handlers["cbi:global"] = [](CSTConverter* converter, CSTToken* container){
        if(!converter->isCBIEnabled) return;
        converter->dispose_node = true;
    };
    annotation_handlers["cbi:to"] = [](CSTConverter* converter, CSTToken* container){
        if(!converter->isCBIEnabled) return;
        converter->dispose_node = true;
    };
    annotation_handlers["cbi:begin"] = [](CSTConverter* converter, CSTToken* container){
        if(!converter->isCBIEnabled) return;
        converter->dispose_node = true;
        converter->keep_disposing = true;
    };
    annotation_handlers["cbi:end"] = [](CSTConverter* converter, CSTToken* container){
        if(!converter->isCBIEnabled) return;
        converter->dispose_node = false;
        converter->keep_disposing = false;
    };
    annotation_handlers["cbi:compile"] = [](CSTConverter* converter, CSTToken* container){
        if(!converter->isCBIEnabled) return;
        converter->dispose_node = false;
        converter->keep_disposing = false;
    };
    annotation_handlers["dispose"] = [](CSTConverter* converter, CSTToken* container){
        auto result = annotation_bool_arg(0, container);
        if(result.has_value()) {
            converter->dispose_node = result.value();
        } else {
            converter->error("unknown value given to dispose annotation", container);
        }
    };
    annotation_handlers["dispose:begin"] = [](CSTConverter* converter, CSTToken* container){
        converter->dispose_node = true;
        converter->keep_disposing = true;
    };
    annotation_handlers["dispose:end"] = [](CSTConverter* converter, CSTToken* container){
        converter->dispose_node = false;
        converter->keep_disposing = false;
    };
    annotation_handlers["inline"] = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::Inline);
    };
    annotation_handlers["inline:always"] = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::AlwaysInline);
    };
    auto no_inline = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::NoInline);
    };
    annotation_handlers["noinline"] = no_inline;
    annotation_handlers["inline:no"] = no_inline;
    annotation_handlers["inline:hint"] = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::InlineHint);
    };
    annotation_handlers["size:opt"] = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::OptSize);
    };
    annotation_handlers["size:min"] = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::MinSize);
    };
    annotation_handlers["destructor"] = [](CSTConverter* converter, CSTToken* container){
        collect_annotation_func(converter, container, AnnotationKind::Destructor);
    };
}

inline void collect_annotations_in(CSTConverter* converter, AnnotableNode* node) {
    node->annotations = std::move(converter->annotations);
}

void CSTConverter::visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start, unsigned int end) {
    while (start < end) {
        tokens[start]->accept(this);
        start++;
    }
}

bool CSTConverter::is_dispose() {
    if(dispose_node) {
        if(!keep_disposing) {
            dispose_node = false;
        }
        return true;
    } else {
        if(keep_disposing) {
            dispose_node = true;
        }
        return false;
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

PointerType* current_self_pointer(CSTConverter* converter) {
    auto type = converter->current_struct_decl ? converter->current_struct_decl->name : (converter->current_interface_decl
                                                                   ? converter->current_interface_decl->name
                                                                   : (converter->current_union_decl ? converter->current_union_decl->name : converter->current_impl_decl->struct_name.value()));
    return new PointerType(std::make_unique<ReferencedType>(type));
}

void CSTConverter::visitFunctionParam(CompoundCSTToken *param) {
    auto identifier = str_token(param->tokens[0].get());
    visit(param->tokens, 2);
    BaseType *baseType = nullptr;
    if (optional_param_types) {
        std::optional<std::unique_ptr<BaseType>> t = std::nullopt;
        if(2 < param->tokens.size() && param->tokens[2]->is_type()) {
            t.emplace(type());
        }
        if (t.has_value()) {
            baseType = t.value().release();
        }
    } else {
        baseType = type().release();
    }
    std::optional<std::unique_ptr<Value>> def_value = std::nullopt;
    if(param->tokens.back()->is_value()) {
        param->tokens.back()->accept(this);
        def_value.emplace(value());
    }
    nodes.emplace_back(std::make_unique<FunctionParam>(identifier, std::unique_ptr<BaseType>(baseType), param_index,
                                            std::move(def_value)));
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
            params.emplace_back(new FunctionParam(strId, std::unique_ptr<PointerType>(current_self_pointer(this)), 0, std::nullopt));
        }
//        else if(optional_param_types && tokens[i]->type() == LexTokenType::Variable) {
//            auto strId = str_token(tokens[1].get());
//            params.emplace_back(new FunctionParam(strId, std::make_unique<VoidType>(), 0, std::nullopt));
//        }
        else if (tokens[i]->compound()) {
            tokens[i]->accept(this);
            param_index++;
            auto param = (FunctionParam *) nodes.back().release();
            params.emplace_back(param);
            nodes.pop_back();
            auto& param_tokens = tokens[i]->as_compound()->tokens;
            auto last_token = param_tokens[param_tokens.size() - 1].get();
            if (is_str_op(last_token, "...")) {
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

void CSTConverter::visitFunction(CompoundCSTToken *function) {

    auto is_extension = is_char_op(function->tokens[1].get(), '(');

    if(is_dispose()) {
        return;
    }

    auto params = function_params(function->tokens, is_extension ? 6 : 3);

    auto i = params.index;

    std::optional<std::unique_ptr<BaseType>> returnType = std::nullopt;

    if (i + 1 < function->tokens.size() && is_char_op(function->tokens[i + 1].get(), ':')) {
        function->tokens[i + 2]->accept(this);
        returnType.emplace(type());
        i += 3; // position at body
    } else {
        i++;
    }

    if (!returnType.has_value()) {
        returnType.emplace(std::make_unique<VoidType>());
    }

    std::optional<LoopScope> fnBody = std::nullopt;
    if (i < function->tokens.size()) {
        fnBody.emplace(LoopScope{});
    }

    FunctionDeclaration* funcDecl;

    if(is_extension) {
        function->tokens[2]->accept(this);
        auto param = (FunctionParam*) nodes.back().release();
        nodes.pop_back();
        funcDecl = new ExtensionFunction(
                func_name(function),
                ExtensionFuncReceiver(std::move(param->name), std::move(param->type)),
                std::move(params.params),
                std::move(returnType.value()), params.isVariadic,
                std::move(fnBody)
        );
        delete param;
    } else {
        funcDecl = new FunctionDeclaration(func_name(function), std::move(params.params),
                                                std::move(returnType.value()), params.isVariadic,
                                                std::move(fnBody));
    }

    collect_annotations_in(this, funcDecl);

    funcDecl->assign_params();

    nodes.emplace_back(std::unique_ptr<FunctionDeclaration>(funcDecl));

    if (i >= function->tokens.size()) {
        return;
    }

    auto prev_decl = current_func_type;
    auto prev_nodes = std::move(nodes);
    current_func_type = funcDecl;
    function->tokens[i]->accept(this);
    funcDecl->body->nodes = std::move(nodes);
    nodes = std::move(prev_nodes);
    current_func_type = prev_decl;


}

void CSTConverter::visitEnumDecl(CompoundCSTToken *decl) {
    if(is_dispose()) {
        return;
    }
    std::unordered_map<std::string, std::unique_ptr<EnumMember>> members;
    auto i = 3; // first enum member or '}'
    unsigned position = 0;
    while(!is_char_op(decl->tokens[i].get(), '}')) {
        if(decl->tokens[i]->is_identifier()) {
            auto name = str_token(decl->tokens[i].get());
            members[name] = std::make_unique<EnumMember>(name, position++);
            if (is_char_op(decl->tokens[i + 1].get(), ',')) {
                i++;
            }
        }
        i++;
    }
    auto enum_decl = new EnumDeclaration(str_token(decl->tokens[1].get()), std::move(members));
    nodes.emplace_back(enum_decl);
}

Value* convertNumber(NumberToken* token, ValueType value_type, bool is64Bit) {
    switch(value_type) {
        case ValueType::Int:
            return new IntValue(std::stoi(token->value));
        case ValueType::UInt:
            return new UIntValue(std::stoi(token->value));
        case ValueType::Short:
            return new ShortValue(std::stoi(token->value));
        case ValueType::UShort:
            return new UShortValue(std::stoi(token->value));
        case ValueType::Long:
            return new LongValue(std::stol(token->value), is64Bit);
        case ValueType::ULong:
            return new ULongValue(std::stoul(token->value), is64Bit);
        case ValueType::BigInt:
            return new BigIntValue(std::stoll(token->value));
        case ValueType::UBigInt:
            return new UBigIntValue(std::stoull(token->value));
        case ValueType::Float:
            return new FloatValue(std::stof(token->value));
        case ValueType::Double:
            return new DoubleValue(std::stod(token->value));
        default:
            return nullptr;
    }
}

void CSTConverter::visitVarInit(CompoundCSTToken *varInit) {
    if(is_dispose()) return;
    std::optional<std::unique_ptr<BaseType>> optType = std::nullopt;
    if(is_char_op(varInit->tokens[2].get(), ':')) {
        varInit->tokens[3]->accept(this);
        optType.emplace(type());
    }
    std::optional<std::unique_ptr<Value>> optVal = std::nullopt;
    auto token = varInit->tokens[varInit->tokens.size() - 1].get();
    if(token->is_value()) {
        if(optType.has_value() && optType.value()->kind() == BaseTypeKind::IntN && token->type() == LexTokenType::Number) {
            // This statement leads to a warning "memory leak", we set the pointer to optVal which is a unique_ptr
            auto conv = convertNumber((NumberToken*) token, optType.value()->value_type(), is64Bit);
            if(conv) {
                optVal.emplace(conv);
            } else {
                error("invalid number for the expected type", token);
            }
        } else {
            token->accept(this);
            optVal.emplace(value());
        }
    }
    nodes.emplace_back(std::make_unique<VarInitStatement>(
            is_var_init_const(varInit),
            var_init_identifier(varInit),
            std::move(optType),
            std::move(optVal)
    ));
}

void CSTConverter::visitAssignment(CompoundCSTToken *assignment) {
    visit(assignment->tokens, 0);
    auto val = value();
    auto chain = value().release();
    nodes.emplace_back(std::make_unique<AssignStatement>(
            std::unique_ptr<Value>(chain),
            std::move(val),
            (assignment->tokens[1]->type() == LexTokenType::Operation)
            ? get_operation(assignment->tokens[1].get()) : Operation::Assignment
    ));
}

void CSTConverter::visitImport(CompoundCSTToken *cst) {
    if(no_imports) return;
    std::vector<std::string> ids;
    nodes.emplace_back(std::make_unique<ImportStatement>(escaped_str_token(cst->tokens[1].get()), ids));
}

void CSTConverter::visitReturn(CompoundCSTToken *cst) {
    std::optional<std::unique_ptr<Value>> return_value = std::nullopt;
    if(1 < cst->tokens.size() && cst->tokens[1]->is_value()) {
        cst->tokens[1]->accept(this);
        return_value.emplace(value());
    }
    nodes.emplace_back(std::make_unique<ReturnStatement>(std::move(return_value), current_func_type));
}

void CSTConverter::visitTypealias(CompoundCSTToken *alias) {
    if(is_dispose()) return;
    auto identifier = str_token(alias->tokens[1].get());
    alias->tokens[3]->accept(this);
    nodes.emplace_back(std::make_unique<TypealiasStatement>(identifier, type()));
}

void CSTConverter::visitTypeToken(LexToken *token) {
    auto primitive = TypeMakers::PrimitiveMap.find(token->value);
    if (primitive == TypeMakers::PrimitiveMap.end()) {
        types.emplace_back(std::make_unique<ReferencedType>(token->value));
    } else {
        types.emplace_back(std::unique_ptr<BaseType>(primitive->second(is64Bit)));
    }
}

void CSTConverter::visitContinue(CompoundCSTToken *continueCst) {
    nodes.emplace_back(std::make_unique<ContinueStatement>(current_loop_node));
}

void CSTConverter::visitBreak(CompoundCSTToken *breakCST) {
    nodes.emplace_back(std::make_unique<BreakStatement>(current_loop_node));
}

void CSTConverter::visitIncDec(CompoundCSTToken *incDec) {
    incDec->tokens[0]->accept(this);
    auto acOp = (get_operation(incDec->tokens[1].get()) == Operation::PostfixIncrement ? Operation::Addition
                                                                                                : Operation::Subtraction);
    nodes.emplace_back(std::make_unique<AssignStatement>(value(), std::make_unique<IntValue>(1), acOp));
}

void CSTConverter::visitLambda(CompoundCSTToken *cst) {

    std::vector<std::unique_ptr<CapturedVariable>> captureList;

    auto no_capture_list = is_char_op(cst->tokens[0].get(), '(');

    unsigned i = 1;
    unsigned capInd = 0;
    if (!no_capture_list) {
        while (!is_char_op(cst->tokens[i].get(), ']')) {
            bool capture_by_ref = false;
            if(is_char_op(cst->tokens[i].get(), '&')) {
                capture_by_ref = true;
                i++;
            }
            if (cst->tokens[i]->type() == LexTokenType::Variable) {
                captureList.emplace_back(new CapturedVariable(((VariableToken *) (cst->tokens[i].get()))->value, capInd++, capture_by_ref));
            }
            i++;
        }
        i += 2;
    }

    auto prev = optional_param_types;
    optional_param_types = true;
    auto result = function_params(cst->tokens, i);
    optional_param_types = prev;

    auto lambda = new LambdaFunction(std::move(captureList), std::move(result.params), result.isVariadic, Scope{});

    auto bodyIndex = result.index + 2;
    if (cst->tokens[bodyIndex]->type() == LexTokenType::CompBody) {
        auto prev_nodes = std::move(nodes);
        auto prev_decl = current_func_type;
        current_func_type = lambda;
        cst->tokens[bodyIndex]->accept(this);
        current_func_type = prev_decl;
        lambda->scope.nodes = std::move(nodes);
        nodes = std::move(prev_nodes);
    } else {
        visit(cst->tokens, bodyIndex);
        auto returnStmt = new ReturnStatement(value(), lambda);
        lambda->scope.nodes.emplace_back(returnStmt);
    }

    lambda->assign_params();

    for(auto& c : lambda->captureList) {
        c->lambda = lambda;
    }

    values.emplace_back(lambda);
}

void CSTConverter::visitBody(CompoundCSTToken *bodyCst) {
    visit(bodyCst->tokens, 0);
}

void CSTConverter::visitMacro(CompoundCSTToken* macroCst) {
    auto name = str_token(macroCst->tokens[0].get());
    auto annon_name = name.substr(1);
    auto macro = macro_handlers.find(annon_name);
    if (macro != macro_handlers.end()) {
        macro->second(this, macroCst);
    } else {
        error("couldn't find annotation or macro handler for " + name, macroCst);
    }
}

void CSTConverter::visitAnnotation(CompoundCSTToken *annotation) {
    auto name = str_token(annotation->tokens[0].get());
    auto annon_name = name.substr(1);
    auto macro = annotation_handlers.find(annon_name);
    if (macro != annotation_handlers.end()) {
        macro->second(this, annotation);
    } else {
        error("couldn't find annotation handler for " + annon_name, annotation);
    }
}

void CSTConverter::visitAnnotationToken(LexToken *token) {
    auto annon_name = token->value.substr(1);
    auto macro = annotation_handlers.find(annon_name);
    if (macro != annotation_handlers.end()) {
        macro->second(this, token);
    } else {
        error("couldn't find annotation handler for " + annon_name, token);
    }
}

void CSTConverter::visitIf(CompoundCSTToken *ifCst) {

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

void CSTConverter::visitSwitch(CompoundCSTToken *switchCst) {
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

void CSTConverter::visitThrow(CompoundCSTToken *throwStmt) {
    throwStmt->tokens[1]->accept(this);
    nodes.emplace_back(new ThrowStatement(value()));
}

void CSTConverter::visitNamespace(CompoundCSTToken *ns) {
    auto prev_nodes = std::move(nodes);
    visit(ns->tokens, 0);
    auto pNamespace = new Namespace(str_token(ns->tokens[1].get()));
    pNamespace->nodes = std::move(nodes);
    nodes = std::move(prev_nodes);
    nodes.emplace_back(pNamespace);
}

void CSTConverter::visitForLoop(CompoundCSTToken *forLoop) {

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

void CSTConverter::visitWhile(CompoundCSTToken *whileCst) {
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

void CSTConverter::visitDoWhile(CompoundCSTToken *doWhileCst) {
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
        tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> &variables,
        tsl::ordered_map<std::string, std::unique_ptr<FunctionDeclaration>> &decls,
        unsigned i
) {
    auto prev_anns = std::move(conv->annotations);
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
    conv->annotations = std::move(prev_anns);
    return i;
}

void CSTConverter::visitStructDef(CompoundCSTToken *structDef) {
    if(is_dispose()) {
        return;
    }
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

void CSTConverter::visitInterface(CompoundCSTToken *interface) {
    if(is_dispose()) {
        return;
    }
    auto def = new InterfaceDefinition(str_token(interface->tokens[1].get()));
    unsigned i = 3; // positioned at first node or '}'
    current_interface_decl = def;
    collect_struct_members(this, interface->tokens, def->variables, def->functions, i);
    current_interface_decl = nullptr;
    nodes.emplace_back(def);
}

void CSTConverter::visitUnionDef(CompoundCSTToken *unionDef) {
    if(is_dispose()) {
        return;
    }
    unsigned i = 3; // positioned at first node or '}'
    auto def = new UnionDef(str_token(unionDef->tokens[1].get()));
    current_union_decl = def;
    collect_struct_members(this, unionDef->tokens, def->variables, def->functions, i);
    current_union_decl = nullptr;
    nodes.emplace_back(def);
}

void CSTConverter::visitImpl(CompoundCSTToken *impl) {
    if(is_dispose()) {
        return;
    }
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

void CSTConverter::visitTryCatch(CompoundCSTToken *tryCatch) {
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

void CSTConverter::visitPointerType(CompoundCSTToken *cst) {
    visit(cst->tokens, 0);
    types.emplace_back(std::make_unique<PointerType>(type()));
}

void CSTConverter::visitGenericType(CompoundCSTToken *cst) {
    visit(cst->tokens, 0);
    types.emplace_back(std::make_unique<GenericType>(str_token(cst->tokens[0].get()), type()));
}

void CSTConverter::visitArrayType(CompoundCSTToken *arrayType) {
    convert(arrayType->tokens);
    std::optional<std::unique_ptr<Value>> val = std::nullopt;
    if(arrayType->tokens[2]->is_value()) {
        arrayType->tokens[2]->accept(this);
        val.emplace(value());
    }
    auto arraySize = (val.has_value() && val.value()->value_type() == ValueType::Int) ? val.value()->as_int() : -1;
    types.emplace_back(std::make_unique<ArrayType>(std::move(type()), arraySize));
}

void CSTConverter::visitFunctionType(CompoundCSTToken *funcType) {
    bool is_capturing = is_char_op(funcType->tokens[0].get(), '[');
    auto params = function_params(funcType->tokens, is_capturing ? 3 : 1);
    visit(funcType->tokens, params.index + 2);
    types.emplace_back(
            std::make_unique<FunctionType>(std::move(params.params), type(), params.isVariadic, is_capturing));
}


void CSTConverter::visitStringToken(LexToken *token) {
    auto escaped = escape_all(token->value, 1, token->value.size() - 1, [this, token](const std::string& value, unsigned int index) {
        error("invalid escape sequence found, character '" + std::string(1, value[index]) + "'", token);
    });
    values.emplace_back(std::make_unique<StringValue>(escaped));
}

void CSTConverter::visitCharToken(LexToken *token) {
    char value;
    if(token->value[1] == '\\') {
        auto result = escape_single(token->value, 2);
        value = result.first;
        if(!result.second) {
            error("unknown / invalid escape sequence present", token);
        }
    } else {
        value = token->value[1];
    }
    values.emplace_back(std::make_unique<CharValue>(value));
}

void CSTConverter::visitNumberToken(NumberToken *token) {
    try {
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
                    values.emplace_back(new ULongValue(std::stoul(token->value), is64Bit));
                } else {
                    values.emplace_back(new LongValue(std::stol(token->value), is64Bit));
                }
            } else {
                values.emplace_back(new NumberValue(std::stoll(token->value)));
            }
        }
    } catch (...) {
        error("invalid value provided", token);
    }
}

void CSTConverter::visitStructValue(CompoundCSTToken *cst) {
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

void CSTConverter::visitArrayValue(CompoundCSTToken *arrayValue) {
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
        if(arrayValue->tokens[i]->is_type()) {
            arrayValue->tokens[i]->accept(this);
            arrType.emplace(type());
        }
        i++;
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

void CSTConverter::visitFunctionCall(CompoundCSTToken *call) {
    auto prev_values = std::move(values);
    visit(call->tokens, 1);
    auto func_call = new FunctionCall(std::move(values));
    values = std::move(prev_values);
    values.emplace_back(func_call);
}

void CSTConverter::visitIndexOp(CompoundCSTToken *op) {
    auto indexes = take_values(this, [&op, this]() {
        visit(op->tokens, 1);
    });
    values.emplace_back(std::make_unique<IndexOperator>(std::move(indexes)));
}

void CSTConverter::visitAccessChain(AccessChainCST *chain) {
    auto prev_values = std::move(values);
    visit(chain->tokens);
    auto ret_chain = std::make_unique<AccessChain>(std::move(values));
    values = std::move(prev_values);
    if (chain->is_node) {
        nodes.emplace_back(std::move(ret_chain));
    } else {
        if(ret_chain->values.size() == 1) {
            values.emplace_back(std::move(ret_chain->values[0]));
        } else {
            values.emplace_back(std::move(ret_chain));
        }
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
            if (is_braced) { //    - a left parenthesis '(':
                // push it onto the operator stack
                op_stack.putCharacter('(');
            }
            // visiting the first value
            visitNestedExpr(converter, nested->tokens[first_val_index].get(), op_stack, output);
            if (is_braced) { // a right parenthesis ')':
                sy_onRParen(op_stack, output);
            }
            if(nested->tokens.size() <= op_index) { // no operator present, just expression like '(' 'nested_expr' ')'
                return;
            }
            auto o1 = get_operation(nested->tokens[op_index].get());
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

void CSTConverter::visitExpression(CompoundCSTToken *expr) {
    auto is_braced = is_char_op(expr->tokens[0].get(), '(');
    if(is_braced && is_char_op(expr->tokens[2].get(), ')') && expr->tokens.size() <= 3) {
        // handles braced expression (1 + 1) that's it
        expr->tokens[1]->accept(this);
        return;
    }
    auto first_val_index = is_braced ? 1 : 0;
    auto op_index = is_braced ? 3 : 1;
    auto second_val_index = op_index + 1;
    if (expr->tokens[first_val_index]->is_primitive_var() && expr->tokens[second_val_index]->is_primitive_var()) {
        // no need to create stacks for values that are primitive variables, a single expression like 1 + 2 or x + 1
        visit(expr->tokens);
        auto second = value();
        auto first = value();
        values.emplace_back(std::make_unique<Expression>(std::move(first), std::move(second),
                                                         (get_operation(expr->tokens[op_index].get())), is64Bit));
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
        values.emplace_back(output.toExpression(is64Bit));
    }
}

void CSTConverter::visitCast(CompoundCSTToken *castCst) {
    visit(castCst->tokens);
    values.emplace_back(std::make_unique<CastedValue>(value(), type()));
}

void CSTConverter::visitAddrOf(CompoundCSTToken *addrOf) {
    addrOf->tokens[1]->accept(this);
    values.emplace_back(std::make_unique<AddrOfValue>(value()));
}

void CSTConverter::visitDereference(CompoundCSTToken *deref) {
    deref->tokens[1]->accept(this);
    values.emplace_back(std::make_unique<DereferenceValue>(value()));
}

void CSTConverter::visitVariableToken(LexToken *token) {
    values.emplace_back(std::make_unique<VariableIdentifier>(token->value));
}

void CSTConverter::visitBoolToken(LexToken *token) {
    values.emplace_back(std::make_unique<BoolValue>(token->value[0] == 't'));
}

void CSTConverter::visitNullToken(LexToken *token) {
    values.emplace_back(std::make_unique<NullValue>());
}

void CSTConverter::visitNegative(CompoundCSTToken *neg) {
    visit(neg->tokens);
    values.emplace_back(std::make_unique<NegativeValue>(value()));
}

void CSTConverter::visitNot(CompoundCSTToken *notCst) {
    visit(notCst->tokens);
    values.emplace_back(std::make_unique<NotValue>(value()));
}