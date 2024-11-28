// Copyright (c) Qinetik 2024.

#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/IntNType.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/LinkedType.h"
#include "ast/statements/Assignment.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/Expression.h"
#include "ast/values/BoolValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/ValueNode.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/IntValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/Int128Value.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/ULongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NullValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/NotValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/Import.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/statements/Return.h"
#include "ast/types/GenericType.h"
#include "ast/types/LiteralType.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/WhileLoop.h"
#include "ast/values/StructValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/UCharValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/DoWhileLoop.h"
#include "parser/model/CompilerBinder.h"
#include "ast/statements/Continue.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/statements/DestructStmt.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/Break.h"
#include "ast/statements/Unreachable.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/LoopBlock.h"
#include "cst/base/CSTConverter.h"
#include "ast/values/CastedValue.h"
#include "cst/utils/ValueAndOperatorStack.h"
#include "ast/values/NumberValue.h"
#include "utils/StringHelpers.h"
#include "utils/CSTUtils.h"
#include <functional>
#include <memory>
#include <sstream>
#include "compiler/PrimitiveTypeMap.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/InitBlock.h"
#include "ast/statements/ThrowStatement.h"
#include "preprocess/RepresentationVisitor.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "ast/values/SizeOfValue.h"
#include "ast/types/LinkedValueType.h"
#include "ast/statements/UsingStmt.h"
#include "ast/types/LinkedType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/base/MalformedInput.h"
#include "LocationManager.h"
#include "parser/utils/parse_num.h"

std::optional<Operation> get_operation_safe(CSTToken *token) {
    std::string num;
    unsigned i = 0;
    const auto& value = token->value();
    while(i < value.size()) {
        if(std::isdigit(value[i])) {
            num.append(1, value[i]);
        }
        i++;
    }
    try {
        return (Operation) std::stoi(num);
    } catch(...) {
        return std::nullopt;
    }
}

Operation get_operation(CSTToken *token) {
    std::string num;
    unsigned i = 0;
    const auto& value = token->value();
    while(i < value.size()) {
        if(std::isdigit(value[i])) {
            num.append(1, value[i]);
        }
        i++;
    }
#ifdef DEBUG
    try {
        return (Operation) std::stoi(num);
    } catch(...) {
        throw std::runtime_error("get_operation failed where the token contains value: " + token->value());
    }
#else
    return (Operation) std::stoi(num);
#endif
}

std::vector<ASTNode*> take_body_nodes(CSTConverter *conv, CSTToken *token, ASTNode* parent_node) {
    auto prev_nodes = std::move(conv->nodes);
    auto prev_parent = conv->parent_node;
    conv->parent_node = parent_node;
    token->accept(conv);
    conv->parent_node = prev_parent;
    auto nodes = std::move(conv->nodes);
    conv->nodes = std::move(prev_nodes);
    return nodes;
}

//std::vector<ASTNode*> take_body_nodes(CSTConverter *conv, ASTAllocator& allocator, CSTToken *token, ASTNode* parent_node) {
//    auto local = conv->local_allocator;
//    conv->local_allocator = &allocator;
//    auto nodes = take_body_nodes(conv, token, parent_node);
//    conv->local_allocator = local;
//    return nodes;
//}

std::vector<ASTNode*> take_body_or_single_stmt(CSTConverter *conv, CSTToken *container, unsigned &i, ASTNode* parent_node) {
    auto& token = container->tokens[i];
    const auto type = token->type();
    if(type == LexTokenType::CompBody) {
        return take_body_nodes(conv, container->tokens[i], parent_node);
    } else {
        token->accept(conv);
        std::vector<ASTNode*> nodes(1);
        if(CSTToken::is_value(type)) {
            nodes[0] = new (conv->local<ValueNode>()) ValueNode(conv->pop_last_value(), parent_node, conv->loc(token));
        } else {
            if(type == LexTokenType::CompAccessChainNode) {
                nodes[0] = new (conv->local<ValueNode>()) ValueNode((AccessChain*) conv->pop_last_node(), parent_node, conv->loc(token));
            } else {
                nodes[0] = conv->pop_last_node();
            }
        }
        if(i + 1 < container->tokens.size() && is_char_op(container->tokens[i + 1], ';')) {
            i++;
        }
        return nodes;
    }
}

std::vector<ASTNode*> take_comp_body_nodes(CSTConverter *conv, CSTToken* token, ASTNode* parent_node) {
    auto prev_nodes = std::move(conv->nodes);
    auto prev_parent = conv->parent_node;
    conv->parent_node = parent_node;
    conv->visit(token->tokens, 0);
    conv->parent_node = prev_parent;
    auto nodes = std::move(conv->nodes);
    conv->nodes = std::move(prev_nodes);
    return nodes;
}


Scope take_body(CSTConverter *conv, CSTToken *token, ASTNode* parent_node) {
    return { take_body_nodes(conv, token, parent_node), parent_node, conv->loc(token) };
}

Scope take_body_compound(CSTConverter *conv, CSTToken* token, ASTNode* parent_node) {
    return { take_comp_body_nodes(conv, token, parent_node), parent_node, conv->loc(token) };
}

// TODO support _128bigint, bigfloat
CSTConverter::CSTConverter(
        unsigned int file_id,
        bool is64Bit,
        GlobalInterpretScope& scope,
        CompilerBinder& binder,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        ASTAllocator& file_allocator
) : file_id(file_id), is64Bit(is64Bit), global_scope(scope), binder(binder), loc_man(scope.loc_man),
    global_allocator(global_allocator), mod_allocator(mod_allocator), file_allocator(file_allocator), local_allocator(&mod_allocator) {

}

const std::unordered_map<std::string, MacroHandlerFn> MacroHandlers = {
        { "eval", [](CSTConverter* converter, CSTToken*  container){
            if(container->tokens[2]->is_value()) {
                container->tokens[2]->accept(converter);
                auto take_value = converter->value();
                auto evaluated_value = take_value->evaluated_value(converter->global_scope);
                if(evaluated_value == nullptr) {
                    converter->error("couldn't evaluate value", container);
                    return;
                }
                converter->put_value(evaluated_value, container);
            } else {
                converter->error("expected a value for eval", container);
            }
        }},
        {"sizeof", [](CSTConverter* converter, CSTToken*  container) {
            const auto tok = container->tokens[2];
            if(tok->is_type()) {
                tok->accept(converter);
                auto value = new (converter->local<SizeOfValue>()) SizeOfValue(converter->type(), converter->loc(tok));
                converter->put_value(value, tok);
            } else {
                converter->error("expected a type in sizeof", container);
            }
        }},
        {"tr:debug:chemical", [](CSTConverter* converter, CSTToken*  container) {
            auto body = take_body_compound(converter, container, converter->parent_node);
            std::ostringstream ostring;
            RepresentationVisitor visitor(ostring);
            visitor.translate(body.nodes);
            converter->put_value(new (converter->local<StringValue>()) StringValue(ostring.str(), converter->loc(container)), container);
        }},
        {"tr:debug:chemical:value", [](CSTConverter* converter, CSTToken*  container) {
            if(container->is_value()) {
                container->accept(converter);
                auto value = converter->value();
                std::ostringstream ostring;
                RepresentationVisitor visitor(ostring);
                value->accept(&visitor);
                converter->put_value(new (converter->local<StringValue>()) StringValue(ostring.str(), converter->loc(container)), container);
            } else {
                converter->error("expected a value in tr:debug:chemical:value", container);
            }
        }},
        {"tr:debug:c", [](CSTConverter* converter, CSTToken* container) {
            auto body = take_body_compound(converter, container, converter->parent_node);
            std::ostringstream ostring;
            ToCAstVisitor visitor(converter->global_scope, &ostring, converter->mod_allocator, converter->loc_man);
            visitor.translate(body.nodes);
            converter->put_value(new (converter->local<StringValue>()) StringValue(ostring.str(), converter->loc(container)), container);
        }},
        {"tr:debug:c:value",[](CSTConverter* converter, CSTToken* container) {
            if(container->is_value()) {
                container->accept(converter);
                auto value = converter->value();
                std::ostringstream ostring;
                ToCAstVisitor visitor(converter->global_scope, &ostring, converter->mod_allocator, converter->loc_man);
                value->accept(&visitor);
                converter->put_value(new (converter->local<StringValue>()) StringValue(ostring.str(), converter->loc(container)), container);
            } else {
                converter->error("expected a value in tr:debug:c:value", container);
            }
        }}
};
struct AnnotationHandler {
    AnnotationHandlerFn func;
    AnnotationKind kind;
};

void ignore_annotation_func(CSTConverter* converter, CSTToken* container, AnnotationKind kind) {
    // do nothing
}

void collect_annotation_func(CSTConverter* converter, CSTToken* container, AnnotationKind kind) {
    converter->annotations.emplace_back(kind);
    if(!container->compound()) return;
    auto previous = std::move(converter->values);
    visit(converter, container->tokens, 2);
    auto collected = std::move(converter->values);
    converter->values = std::move(previous);
    converter->annotations.back().values = std::move(collected);
}

const std::unordered_map<std::string, const AnnotationHandler> AnnotationHandlers = {
        { "cbi:create", AnnotationHandler { ignore_annotation_func } },
        { "cbi:import", AnnotationHandler { ignore_annotation_func } },
        { "inline:", { collect_annotation_func, AnnotationKind::Inline } },
        { "inline:always", { collect_annotation_func, AnnotationKind::AlwaysInline } },
        { "noinline", { collect_annotation_func, AnnotationKind::NoInline } },
        { "inline:no", { collect_annotation_func, AnnotationKind::NoInline } },
        { "inline:hint", { collect_annotation_func, AnnotationKind::InlineHint } },
        { "compiler.inline", { collect_annotation_func, AnnotationKind::CompilerInline } },
        { "size:opt", { collect_annotation_func, AnnotationKind::OptSize } },
        { "size:min", { collect_annotation_func, AnnotationKind::MinSize } },
        { "comptime", { collect_annotation_func, AnnotationKind::CompTime } },
        { "compiler.interface", { collect_annotation_func, AnnotationKind::CompilerInterface } },
        { "constructor", { collect_annotation_func, AnnotationKind::Constructor } },
        { "make", { collect_annotation_func, AnnotationKind::Constructor } },
        { "delete", { collect_annotation_func, AnnotationKind::Delete } },
        { "override", { collect_annotation_func, AnnotationKind::Override } },
        { "unsafe", { collect_annotation_func, AnnotationKind::Unsafe } },
        { "no_init", { collect_annotation_func, AnnotationKind::NoInit }},
        { "extern", { collect_annotation_func, AnnotationKind::Extern }},
        { "implicit", { collect_annotation_func, AnnotationKind::Implicit }},
        { "propagate", { collect_annotation_func, AnnotationKind::Propagate }},
        { "direct_init", { collect_annotation_func, AnnotationKind::DirectInit }},
        { "no_return", { collect_annotation_func, AnnotationKind::NoReturn }},
        { "cpp", { collect_annotation_func, AnnotationKind::Cpp }},
        { "clear", { collect_annotation_func, AnnotationKind::Clear }},
        { "copy", { collect_annotation_func, AnnotationKind::Copy }},
        { "deprecated", { collect_annotation_func, AnnotationKind::Deprecated }},
        { "move", { collect_annotation_func, AnnotationKind::Move }},
};

inline void collect_annotations_in(CSTConverter* converter, AnnotableNode* node) {
    node->annotations = std::move(converter->annotations);
}

void CSTConverter::visit(std::vector<CSTToken*> &tokens, unsigned int start, unsigned int end) {
    while (start < end) {
        tokens[start]->accept(this);
        start++;
    }
}

void CSTConverter::error(const std::string_view& message, CSTToken* inside) {
    const auto filePath = loc_man.getPathForFileId(file_id);
    CSTDiagnoser::error(message, filePath, inside);
}

AccessSpecifier CSTConverter::def_specifier(std::optional<AccessSpecifier> opt) {
    if(opt.has_value()) return opt.value();
    if(parent_node) {
        return parent_node->specifier();
    }
    return AccessSpecifier::Internal;
}

Value* CSTConverter::value() {
    auto value = values.back();
    values.pop_back();
    return value;
}

ASTUnit CSTConverter::take_unit() {
    ASTUnit unit;
    unit.scope.nodes = std::move(nodes);
    return unit;
}

BaseType* CSTConverter::type() {
    auto type = types.back();
    types.pop_back();
    return type;
}

uint64_t CSTConverter::loc(CSTToken* token) {
    const auto& start = token->start();
    auto end = token->end_token();
    return loc_man.addLocation(file_id, start.line, start.character, end->lineNumber(), end->lineCharNumber() + end->length());
}

void CSTConverter::visitFunctionParam(CSTToken* param) {
    auto &paramTokens = param->tokens;
    if (is_char_op(paramTokens[0], '&')) { // implicit parameter
        const auto is_mutable = is_keyword(paramTokens[1], "mut");
        const auto name_token = paramTokens[is_mutable ? 2 : 1];
        const auto ref_to_linked  = new (local<ReferenceType>()) ReferenceType(new (local<LinkedType>()) LinkedType(name_token->value(), nullptr, loc(name_token)), loc(name_token), is_mutable);;
        const auto paramDecl = new (local<FunctionParam>()) FunctionParam(name_token->value(), ref_to_linked, param_index, nullptr, true, current_func_type, loc(param));
        put_node(paramDecl, param);
        return;
    }
    auto identifier = str_token(paramTokens[0]);
    visit(paramTokens, 2);
    BaseType *baseType = nullptr;
    if(2 < paramTokens.size() && paramTokens[2]->is_type()) {
        baseType = type();
    }
    Value* def_value = nullptr;
    if(paramTokens.back()->is_value()) {
        paramTokens.back()->accept(this);
        def_value = value();
    }
    put_node(new (local<FunctionParam>()) FunctionParam(identifier, baseType, param_index, def_value, false, current_func_type, loc(param)), param);
}

struct FunctionParamsResult {
    bool isVariadic;
    std::vector<FunctionParam*> params;
    unsigned int index;
};

// will probably leave the index at ')'
FunctionParamsResult function_params(CSTConverter* converter, FunctionType* func_type, cst_tokens_ref_type tokens, unsigned start) {
    auto prev_func_type = converter->current_func_type;
    converter->current_func_type = func_type;
    auto prev_param_index = converter->param_index;
    converter->param_index = 0;
    auto isVariadic = false;
    std::vector<FunctionParam*> params;
    unsigned i = start;
    while (i < tokens.size()) {
//        else if(optional_param_types && tokens[i]->type() == LexTokenType::Variable) {
//            auto strId = str_token(tokens[1]);
//            params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam(strId, std::make_unique<VoidType>(), 0, std::nullopt));
//        }
        if (tokens[i]->compound()) {
            tokens[i]->accept(converter);
            converter->param_index++;
            auto param = (FunctionParam *) converter->nodes.back();
            params.emplace_back(param);
            converter->nodes.pop_back();
            auto& param_tokens = tokens[i]->tokens;
            auto last_token = param_tokens[param_tokens.size() - 1];
            if (is_str_op(last_token, "...")) {
                isVariadic = true;
                i++;
                break;
            }

        } else if (is_char_op(tokens[i], ',')) {
            // do nothing
        } else {
            break;
        }
        i++;
    }
    converter->param_index = prev_param_index;
    converter->current_func_type = prev_func_type;
    return {isVariadic, std::move(params), i};
}

void convert_generic_list(
    CSTConverter *converter,
    ASTAllocator& allocator,
    CSTToken*  compound,
    std::vector<GenericTypeParameter*>& generic_list,
    ASTNode* parent_node
) {
    unsigned i = 1;
    GenericTypeParameter* parameter;
    while(i < compound->tokens.size() && compound->tokens[i]->is_identifier()) {
        BaseType* at_least_type = nullptr;
        BaseType* def_type = nullptr;
        auto name_tok = compound->tokens[i];
        if(is_char_op(compound->tokens[i + 1], ':')) {
            i++;
            compound->tokens[i + 1]->accept(converter);
            at_least_type = converter->type();
            i++;
        }
        if(is_char_op(compound->tokens[i + 1], '=')) {
            compound->tokens[i + 2]->accept(converter);
            def_type = converter->type();
            // 4 -> +1 -> '=' , +2 -> 'type' , +3 -> ',' , +4 -> next identifier
            i += 4;
        } else {
            i += 2;
        }
        parameter = new (allocator.allocate<GenericTypeParameter>()) GenericTypeParameter(str_token(name_tok), at_least_type, def_type, parent_node, generic_list.size(), converter->loc(name_tok));
        generic_list.emplace_back(parameter);
    }
}

void CSTConverter::visitFunction(CSTToken* function) {

    // private func <T, K> (ext : Extension*) name(param1 : int) : long

    const auto tokens_size = function->tokens.size();

    unsigned i = 1;

    auto spec = specifier_token(function->tokens[0]);
    if(spec.has_value()) {
        // set at name, extension or generic params list
        i += 1;
    }

    auto specifier = def_specifier(spec);

    const auto& gen_token = function->tokens[i];
    const auto is_generic = gen_token->type() == LexTokenType::CompGenericParamsList;

    if(is_generic) {
        i += 1;
    }

    const auto extension_start = i;
    const auto is_extension = is_char_op(function->tokens[i], '(');

    if(is_extension) {
        i += 3;
    }

    auto& name_token = function->tokens[i];

    auto& alloc = allocator(specifier);

    auto prev_local = local_allocator;
    local_allocator = &alloc;

    FunctionDeclaration* funcDeclStored;

    if(is_extension) {
        auto& receiver_tok = function->tokens[extension_start + 1];
        receiver_tok->accept(this);
        auto param = (FunctionParam*) nodes.back();
        nodes.pop_back();
        funcDeclStored = new (alloc.allocate<ExtensionFunction>()) ExtensionFunction(
                loc_id(name_token),
                ExtensionFuncReceiver(std::move(param->name), param->type, nullptr, loc(receiver_tok)),
                {},
                nullptr,
                false,
                parent_node,
                loc(function),
                std::nullopt,
                specifier
        );
        ((ExtensionFunction*) funcDeclStored)->receiver.parent_node = funcDeclStored;
    } else {
        funcDeclStored = new (alloc.allocate<FunctionDeclaration>()) FunctionDeclaration(
                loc_id(name_token),
                {},
                nullptr,
                false,
                parent_node,
                loc(function),
                std::nullopt,
                specifier
        );
    }

    const auto funcDecl = funcDeclStored;

    auto params = function_params(this, funcDecl, function->tokens, i + 2);

    i = params.index;

    BaseType* returnType = nullptr;

    const auto type_colon = i + 1;
    if (type_colon < tokens_size && is_char_op(function->tokens[type_colon], ':')) {
        const auto type_ind = type_colon + 1;
        if(type_ind < tokens_size) {
            const auto type_token = function->tokens[type_ind];
            if (type_token->is_type()) {
                type_token->accept(this);
                returnType = type();
                i += 3; // position at body
            } else if (type_token->type() == LexTokenType::CompBody) {
                i = type_ind; // position at body
            }
        }
    } else {
        i++;
    }

    if (!returnType) {
        returnType = new (alloc.allocate<VoidType>()) VoidType(ZERO_LOC);
    }

    funcDecl->params = std::move(params.params);
    funcDecl->returnType = returnType;
    funcDecl->setIsVariadic(params.isVariadic);

    if(is_generic) {
        convert_generic_list(this, alloc, gen_token, funcDecl->generic_params, funcDecl);
    }

    if (i < function->tokens.size()) {
        funcDecl->body.emplace(LoopScope{ funcDecl, loc(function->tokens[i]) });
    }

    collect_annotations_in(this, funcDecl);

    funcDecl->assign_params();

    put_node(funcDecl, function);

    if (i < tokens_size) {
        auto prev_decl = current_func_type;
        current_func_type = funcDecl;
        auto prev_parent = parent_node;
        parent_node = funcDecl;
        funcDecl->body->nodes = take_body_nodes(this, function->tokens[i], funcDecl);
        current_func_type = prev_decl;
        parent_node = prev_parent;
    }

    local_allocator = prev_local;


}

void CSTConverter::visitEnumDecl(CSTToken* decl) {

    unsigned i = 1;

    const auto spec = specifier_token(decl->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }

    auto specifier = def_specifier(spec);

    auto& alloc = allocator(specifier);

    auto enum_decl = new (alloc.allocate<EnumDeclaration>()) EnumDeclaration(
            loc_id(decl->tokens[i]),
            std::unordered_map<std::string, EnumMember*> {},
            parent_node,
            loc(decl),
            specifier
    );
    i += 2;
    unsigned position = 0;
    while(!is_char_op(decl->tokens[i], '}')) {
        auto tok = decl->tokens[i];
        if(tok->is_identifier()) {
            auto name = str_token(tok);
            Value* init_value = nullptr;
            if(is_char_op(decl->tokens[i + 1], '=')) {
                i++;
                auto value_tok = decl->tokens[i + 1];
                value_tok->accept(this);
                init_value = value();
                i++;
            }
            enum_decl->members[name] = new (alloc.allocate<EnumMember>()) EnumMember(name, position++, init_value, enum_decl, loc(tok));
            if (is_char_op(decl->tokens[i + 1], ',')) {
                i++;
            }
        }
        i++;
    }
    put_node(enum_decl, decl);
}

void CSTConverter::visitMalformedInput(CSTToken *token) {

    auto prev_nodes = std::move(nodes);
    auto prev_values = std::move(values);
    auto prev_types = std::move(types);

    const auto malformed = new (local<MalformedInput>()) MalformedInput(parent_node, loc(token));

    if(!token->tokens.empty()) {
        visit(token->tokens, 0);
        for (auto thing: nodes) {
            malformed->any_things.emplace_back(thing);
        }
        for (auto thing: values) {
            malformed->any_things.emplace_back(thing);
        }
        for (auto thing: types) {
            malformed->any_things.emplace_back(thing);
        }
    }

    nodes = std::move(prev_nodes);
    values = std::move(prev_values);
    types = std::move(prev_types);

    const auto type = token->type();
    if(type == LexTokenType::CompMalformedNode) {
        nodes.emplace_back(malformed);
    } else if(type == LexTokenType::CompMalformedType) {
        types.emplace_back(malformed);
    } else if(type == LexTokenType::CompMalformedValue) {
        values.emplace_back(malformed);
    } else {
#ifdef DEBUG
        throw std::runtime_error("malformed input not handled");
#endif
    }


}

// TODO use parse_num functions written below
Value* convertNumber_unsafe(CSTConverter* converter, ASTAllocator& alloc, NumberToken* token, ValueType value_type, bool is64Bit) {
    switch(value_type) {
        case ValueType::Int: {
            const auto real_value = std::stoi(token->value());
            return new(alloc.allocate<IntValue>()) IntValue(real_value, converter->loc(token));
        }
        case ValueType::UInt: {
            const auto real_value = std::stoi(token->value());
            return new(alloc.allocate<UIntValue>()) UIntValue(real_value, converter->loc(token));
        }
        case ValueType::Short: {
            const auto real_value = std::stoi(token->value());
            return new(alloc.allocate<ShortValue>()) ShortValue(real_value, converter->loc(token));
        }
        case ValueType::UShort: {
            const auto real_value = std::stoi(token->value());
            return new(alloc.allocate<UShortValue>()) UShortValue(real_value, converter->loc(token));
        }
        case ValueType::Long: {
            const auto real_value = std::stol(token->value());
            return new(alloc.allocate<LongValue>()) LongValue(real_value, is64Bit, converter->loc(token));
        }
        case ValueType::ULong: {
            const auto real_value = std::stoul(token->value());
            return new(alloc.allocate<ULongValue>()) ULongValue(real_value, is64Bit, converter->loc(token));
        }
        case ValueType::BigInt: {
            const auto real_value = std::stoll(token->value());
            return new(alloc.allocate<BigIntValue>()) BigIntValue(real_value, converter->loc(token));
        }
        case ValueType::UBigInt: {
            const auto real_value = std::stoull(token->value());
            return new(alloc.allocate<UBigIntValue>()) UBigIntValue(real_value, converter->loc(token));
        }
        case ValueType::Float: {
            const auto real_value = std::stof(token->value());
            return new(alloc.allocate<FloatValue>()) FloatValue(real_value, converter->loc(token));
        }
        case ValueType::Double: {
            const auto real_value = std::stod(token->value());
            return new(alloc.allocate<DoubleValue>()) DoubleValue(real_value, converter->loc(token));
        }
        default:
            return nullptr;
    }
}

inline Value* convertNumber(CSTConverter* converter, ASTAllocator& alloc, NumberToken* token, ValueType value_type, bool is64Bit) {
#ifdef DEBUG
    try {
        return convertNumber_unsafe(converter, alloc, token, value_type, is64Bit);
    } catch(...) {
        throw std::runtime_error("convert number failed ! where number token contains value : " + token->value());
    }
#else
    return convertNumber_unsafe(alloc, token, value_type, is64Bit);
#endif
}

void CSTConverter::visitVarInit(CSTToken* varInit) {

    // private var i : int = 0
    auto is_struct_member = varInit->type() == LexTokenType::CompStructMember;
    unsigned i = 0;
    auto spec = specifier_token(varInit->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }
    bool is_const = varInit->tokens[i]->value() == "const";
    i += 1;
    auto specifier = is_struct_member ? (spec.has_value() ? spec.value() : AccessSpecifier::Public) : def_specifier(spec);
    auto& alloc = is_struct_member ? global_allocator : (parent_node ? *local_allocator : allocator(specifier));
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    AnnotableNode* init;
    if(is_struct_member) {
        init = new (alloc.allocate<StructMember>()) StructMember(
                varInit->tokens[i]->value(),
                nullptr,
                nullptr,
                parent_node,
                loc(varInit),
                is_const,
                specifier
        );
    } else {
        init = new (alloc.allocate<VarInitStatement>()) VarInitStatement(
                is_const,
                loc_id(varInit->tokens[i]),
                nullptr,
                nullptr,
                parent_node,
                loc(varInit),
                specifier
        );
    }
    auto& type_ref = is_struct_member ? ((StructMember*) init)->type : ((VarInitStatement*) init)->type;
    auto& value_ref = is_struct_member ? ((StructMember*) init)->defValue : ((VarInitStatement*) init)->value;
    auto prev_parent = parent_node;
    parent_node = init;
    i++;
    if(is_char_op(varInit->tokens[i], ':')) {
        varInit->tokens[++i]->accept(this);
        type_ref = type();
        i++;
    }
    if(i < varInit->tokens.size() && is_char_op(varInit->tokens[i], '=')) {
        auto token = varInit->tokens[++i];
        if(type_ref && type_ref->kind() == BaseTypeKind::IntN && token->type() == LexTokenType::Number) {
            // This statement leads to a warning "memory leak", we set the pointer to optVal which is a unique_ptr
            auto conv = convertNumber(this, alloc, (NumberToken*) token, type_ref->value_type(), is64Bit);
            if(conv) {
                value_ref = conv;
            } else {
                error("invalid number for the expected type", token);
            }
        } else {
            token->accept(this);
            value_ref = value();
        }
    }

    collect_annotations_in(this, init);

    put_node(init, varInit);

    parent_node = prev_parent;
    local_allocator = prev_alloc;

}

void CSTConverter::visitAssignment(CSTToken* assignment) {
    visit(assignment->tokens, 0);
    auto val = value();
    auto chain = value();
    put_node(new (local<AssignStatement>()) AssignStatement(
            chain,
            val,
            (assignment->tokens[1]->type() == LexTokenType::Operation)
            ? get_operation(assignment->tokens[1]) : Operation::Assignment,
            parent_node,
            loc(assignment)
    ), assignment);
}

void CSTConverter::visitImport(CSTToken* cst) {
    std::vector<std::string> ids;
    cst->tokens[1]->accept(this);
    auto str_val = (Value*) cst->tokens[1]->straight.data_ptr;
    put_node(new (global<ImportStatement>()) ImportStatement(str_val->get_the_string(), ids, parent_node, loc(cst)), cst);
}

void CSTConverter::visitReturn(CSTToken* cst) {
    Value* return_value = nullptr;
    if(1 < cst->tokens.size() && cst->tokens[1]->is_value()) {
        cst->tokens[1]->accept(this);
        return_value = value();
    }
    put_node(new (local<ReturnStatement>()) ReturnStatement(return_value, current_func_type, parent_node, loc(cst)), cst);
}

void CSTConverter::visitDestruct(CSTToken* delStmt) {
    auto is_array = is_char_op(delStmt->tokens[1], '[');
    Value* array_value = nullptr;
    unsigned index;
    if(is_array) {
        if(delStmt->tokens[2]->is_value()) {
            delStmt->tokens[2]->accept(this);
            array_value = value();
            index = 4;
        } else {
            index = 3;
        }
    } else {
        index = 1;
    }
    delStmt->tokens[index]->accept(this);
    auto val_ptr = value();
    put_node(new (local<DestructStmt>()) DestructStmt(array_value, val_ptr, is_array, parent_node, loc(delStmt)), delStmt);
}

void CSTConverter::visitUsing(CSTToken* usingStmt) {
    std::vector<ChainValue*> curr_values;
    unsigned i = 1;
    while(i < usingStmt->tokens.size()) {
        auto& tok = usingStmt->tokens[i];
        if(tok->is_identifier()) {
            curr_values.emplace_back(new (local<VariableIdentifier>()) VariableIdentifier(str_token(usingStmt->tokens[i]), loc(tok), true));
        }
        i++;
    }
    const auto stmt = new (local<UsingStmt>()) UsingStmt(std::move(curr_values), parent_node, is_keyword(usingStmt->tokens[1], "namespace"), loc(usingStmt));
    collect_annotations_in(this, stmt);
    put_node(stmt, usingStmt);
}

void CSTConverter::visitProvide(CSTToken *provideStmt) {
    provideStmt->tokens[1]->accept(this);
    const auto val = value();
    if(val->val_kind() != ValueKind::CastedValue) {
        error("expected value to be in format 'value as identifier' in provide", provideStmt);
        return;
    }
    const auto castedVal = (CastedValue*) val;
    if(castedVal->type->kind() != BaseTypeKind::Linked) {
        error("expected identifier after the 'as' keyword in provide statement", provideStmt);
        return;
    }
    const auto stmt = new (local<ProvideStmt>()) ProvideStmt(
            castedVal->value,
            ((LinkedType*) castedVal->type)->type,
            { parent_node, loc(provideStmt->tokens[2]) },
            parent_node,
            loc(provideStmt)
    );
    stmt->body.nodes = take_body_nodes(this, provideStmt->tokens[2], stmt);
    put_node(stmt, provideStmt);
}

void CSTConverter::visitComptimeBlock(CSTToken *block) {
    const auto body_tok = block->tokens[1];
    const auto stmt = new (local<ComptimeBlock>()) ComptimeBlock(
            { parent_node, loc(body_tok) },
            parent_node,
            loc(block)
    );
    stmt->body.nodes = take_body_nodes(this, body_tok, stmt);
    put_node(stmt, block);
}

void CSTConverter::visitTypealias(CSTToken* alias) {
    auto spec = specifier_token(alias->tokens[0]);
    unsigned i = spec.has_value() ? 2 : 1;
    const auto& name_token = alias->tokens[i];
    auto& type_token = alias->tokens[i + 2];
    auto specifier = spec.has_value() ? spec.value() : AccessSpecifier::Internal;
    auto& alloc = parent_node ? *local_allocator : allocator(specifier);
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    type_token->accept(this);
    auto stmt = new (alloc.allocate<TypealiasStatement>()) TypealiasStatement(loc_id(name_token), type(), parent_node, loc(alias), specifier);
    collect_annotations_in(this, stmt);
    put_node(stmt, alias);
    local_allocator = prev_alloc;
}

void CSTConverter::visitTypeToken(CSTToken* token) {
    auto primitive = TypeMakers::PrimitiveMap.find(token->value());
    if (primitive == TypeMakers::PrimitiveMap.end()) {
        put_type(new (local<LinkedType>()) LinkedType(token->value(), loc(token)), token);
    } else {
        put_type(primitive->second(*local_allocator, is64Bit, loc(token)), token);
    }
}

void CSTConverter::visitLinkedValueType(CSTToken* ref_value) {
    ref_value->tokens[0]->accept(this);
    auto ref = new (local<LinkedValueType>()) LinkedValueType(value(), loc(ref_value));
    put_type(ref, ref_value);
}

void CSTConverter::visitContinue(CSTToken* continueCst) {
    put_node(new (local<ContinueStatement>()) ContinueStatement(current_loop_node, parent_node, loc(continueCst)), continueCst);
}

void CSTConverter::visitStraightValue(CSTToken* token) {
    put_value((Value*) token->straight.data_ptr, token);
}

void CSTConverter::visitStraightType(CSTToken* token) {
    put_type((BaseType*) token->straight.data_ptr, token);
}

void CSTConverter::visitStraightNode(CSTToken* token) {
    put_node((ASTNode*) token->straight.data_ptr, token);
}

void CSTConverter::visitBreak(CSTToken* breakCST) {
    auto stmt = new (local<BreakStatement>()) BreakStatement(current_loop_node, parent_node, loc(breakCST));
    if(1 < breakCST->tokens.size() && breakCST->tokens[1]->is_value()) {
        breakCST->tokens[1]->accept(this);
        stmt->value = value();
    }
    put_node(stmt, breakCST);
}

void CSTConverter::visitUnreachable(CSTToken *cst) {
    put_node(new (local<UnreachableStmt>()) UnreachableStmt(parent_node, loc(cst)), cst);
}

void CSTConverter::visitIncDec(CSTToken* incDec) {
    incDec->tokens[0]->accept(this);
    auto acOp = (get_operation(incDec->tokens[1]) == Operation::PostfixIncrement ? Operation::Addition : Operation::Subtraction);
    put_node(new (local<AssignStatement>()) AssignStatement(value(), new (local<NumberValue>()) NumberValue(1, loc(incDec)), acOp, parent_node, loc(incDec)), incDec);
}

void CSTConverter::visitLambda(CSTToken* cst) {

    std::vector<CapturedVariable*> captureList;

    auto no_capture_list = is_char_op(cst->tokens[0], '(');

    unsigned i = 1;
    unsigned capInd = 0;
    if (!no_capture_list) {
        while (!is_char_op(cst->tokens[i], ']')) {
            bool capture_by_ref = false;
            if(is_char_op(cst->tokens[i], '&')) {
                capture_by_ref = true;
                i++;
            }
            if (cst->tokens[i]->type() == LexTokenType::Variable) {
                captureList.emplace_back(new (local<CapturedVariable>()) CapturedVariable(((CSTToken* ) (cst->tokens[i]))->value(), capInd++, capture_by_ref, loc(cst->tokens[i])));
            }
            i++;
        }
        i += 2;
    }

    auto lambda = new (local<LambdaFunction>()) LambdaFunction(std::move(captureList), {}, false, Scope { parent_node, ZERO_LOC }, parent_node, loc(cst));

    auto result = function_params(this, lambda, cst->tokens, i);

    lambda->params = std::move(result.params);
    lambda->setIsVariadic(result.isVariadic);

    auto bodyIndex = result.index + 2;
    if (cst->tokens[bodyIndex]->type() == LexTokenType::CompBody) {
        auto prev_decl = current_func_type;
        current_func_type = lambda;
        lambda->scope.nodes = take_body_nodes(this, cst->tokens[bodyIndex], &lambda->scope);
        current_func_type = prev_decl;
    } else {
        visit(cst->tokens, bodyIndex);
        auto returnStmt = new (local<ReturnStatement>()) ReturnStatement(value(), lambda, &lambda->scope, ZERO_LOC);
        lambda->scope.nodes.emplace_back(returnStmt);
    }
    lambda->scope.location = loc(cst->tokens[bodyIndex]);

    lambda->assign_params();

    for(auto& c : lambda->captureList) {
        c->lambda = lambda;
    }

    put_value(lambda, cst);
}

void CSTConverter::visitBody(CSTToken* bodyCst) {
    visit(bodyCst->tokens, 0);
}

void CSTConverter::visitInitBlock(CSTToken *initBlock) {
    auto& block_token = initBlock->tokens[1];
    auto init_block = new (local<InitBlock>()) InitBlock(Scope(nullptr, loc(block_token)), parent_node, loc(initBlock));
    init_block->scope.parent_node = init_block;
    init_block->scope.nodes = take_body_nodes(this, block_token, init_block);
    nodes.emplace_back(init_block);
}

void CSTConverter::visitUnsafeBlock(CSTToken *block) {
    auto& block_token = block->tokens[1];
    auto unsafe_block = new (local<UnsafeBlock>()) UnsafeBlock(Scope(nullptr, loc(block_token)), loc(block));
    unsafe_block->scope.nodes = take_body_nodes(this, block_token, unsafe_block);
    nodes.emplace_back(unsafe_block);
}

void CSTConverter::visitMacro(CSTToken* macroCst) {
    auto name = str_token(macroCst->tokens[0]);
    auto annon_name = name.substr(1);
    auto macro = MacroHandlers.find(annon_name);
    if (macro != MacroHandlers.end()) {
        macro->second(this, macroCst);
    } else {
        auto func = binder.provide_parse_macro_func(annon_name);
        if(func) {
            func(this, macroCst);
        } else {
            error("couldn't find annotation or macro handler for " + name, macroCst);
        }
    }
}

void CSTConverter::visitAnnotation(CSTToken* annotation) {
    auto name = str_token(annotation->tokens[0]);
    auto annon_name = name.substr(1);
    auto macro = AnnotationHandlers.find(annon_name);
    if (macro != AnnotationHandlers.end()) {
        macro->second.func(this, annotation, macro->second.kind);
    } else {
        error("couldn't find annotation handler for " + annon_name, annotation);
    }
}

void CSTConverter::visitAnnotationToken(CSTToken* token) {
    auto annon_name = token->value().substr(1);
    auto macro = AnnotationHandlers.find(annon_name);
    if (macro != AnnotationHandlers.end()) {
        macro->second.func(this, token, macro->second.kind);
    } else {
        error("couldn't find annotation handler for " + annon_name, token);
    }
}

void CSTConverter::visitValueNode(CSTToken *cst) {
    cst->tokens[0]->accept(this);
    put_node(new (local<ValueNode>()) ValueNode(value(), parent_node, loc(cst)), cst);
}

void CSTConverter::visitIf(CSTToken* ifCst) {

    auto is_value  = ifCst->type() == LexTokenType::CompIfValue;

    auto& alloc = parent_node ? *local_allocator : global_allocator;
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;

    // if condition
    ifCst->tokens[2]->accept(this);
    auto cond = value();

    auto if_statement = new (alloc.allocate<IfStatement>()) IfStatement(
            cond,
            Scope {nullptr, ZERO_LOC},
            std::vector<std::pair<Value*, Scope>>{},
            std::nullopt,
            parent_node,
            is_value,
            loc(ifCst)
    );

    auto prev_parent = parent_node;
    parent_node = if_statement;

    // first if body
    if_statement->ifBody.parent_node = if_statement;
    unsigned i = 4;
    if_statement->ifBody.location = loc(ifCst->tokens[i]);
    if_statement->ifBody.nodes = take_body_or_single_stmt(this, ifCst, i, if_statement);
    i++; // position after body
    while ((i + 1) < ifCst->tokens.size() && is_keyword(ifCst->tokens[i + 1], "if")) {

        i += 3;
        ifCst->tokens[i]->accept(this);
        auto elseIfCond = value();
        i += 2;

        // else if body
        if_statement->elseIfs.emplace_back( elseIfCond, Scope { if_statement, loc(ifCst->tokens[i]) });
        auto& elseif_pair = if_statement->elseIfs.back();
        elseif_pair.second.nodes = take_body_or_single_stmt(this, ifCst, i, if_statement);

        // position after the body
        i++;

    }

    // last else
    if (i < ifCst->tokens.size() && str_token(ifCst->tokens[i]) == "else") {
        i++;
        if_statement->elseBody.emplace(if_statement, loc(ifCst->tokens[i]));
        if_statement->elseBody->nodes = take_body_or_single_stmt(this, ifCst, i, if_statement);
    }

    parent_node = prev_parent;

    if(is_value) {
        put_value(if_statement, ifCst);
    } else {
        put_node(if_statement, ifCst);
    }

    local_allocator = prev_alloc;

}

void CSTConverter::visitSwitch(CSTToken* switchCst) {
    bool is_value = switchCst->type() == LexTokenType::CompSwitchValue;
    auto switch_statement = new (local<SwitchStatement>()) SwitchStatement(
            nullptr,
            parent_node,
            is_value,
            loc(switchCst)
    );
    auto prev_parent = parent_node;
    parent_node = switch_statement;
    switchCst->tokens[2]->accept(this);
    switch_statement->expression = value();
    unsigned i = 5; // positioned at first 'case' or 'default'
    while (true) {
        const auto scope_ind = switch_statement->scopes.size();
        const auto first_tok = switchCst->tokens[i];
        if(first_tok->is_value() || is_keyword(first_tok, "default")) {
            while(true) {
                const auto tok = switchCst->tokens[i];
                if(tok->is_value()) {
                    tok->accept(this);
                    auto caseVal = value();
                    switch_statement->cases.emplace_back(caseVal, scope_ind);
                    i++;
                } else if(is_keyword(tok, "default")) {
                    if(switch_statement->defScopeInd == -1) {
                        switch_statement->defScopeInd = (int) scope_ind;
                    } else {
                        error("multiple defaults in switch statement detected", tok);
                    }
                    i++;
                } else if(is_char_op(tok, '|')) {
                    i++;
                } else {
                    break;
                }
            }
        } else {
            break;
        }
        i += 1; // body
        // create a scope
        switch_statement->scopes.emplace_back(switch_statement, loc(switchCst->tokens[i]));
        auto& case_scope = switch_statement->scopes.back();
        case_scope.nodes = take_body_or_single_stmt(this, switchCst, i, switch_statement);
        i++;
    }
    parent_node = prev_parent;
    if(is_value) {
        put_value(switch_statement, switchCst);
    } else {
        put_node(switch_statement, switchCst);
    }
}

void CSTConverter::visitThrow(CSTToken* throwStmt) {
    throwStmt->tokens[1]->accept(this);
    put_node(new (local<ThrowStatement>()) ThrowStatement(value(), parent_node, loc(throwStmt)), throwStmt);
}

void CSTConverter::visitNamespace(CSTToken* ns) {
    const auto spec = specifier_token(ns->tokens[0]);
    const auto specifier = def_specifier(spec);
    auto& alloc = allocator(specifier);
    auto pNamespace = new (alloc.allocate<Namespace>()) Namespace(str_token(ns->tokens[spec.has_value() ? 2 : 1]), parent_node, loc(ns), specifier);
    const auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    pNamespace->nodes = take_comp_body_nodes(this, ns, pNamespace);
    local_allocator = prev_alloc;
    put_node(pNamespace, ns);
}

void CSTConverter::visitForLoop(CSTToken* forLoop) {

    forLoop->tokens[2]->accept(this);
    auto varInit = nodes.back()->as_var_init();
    nodes.pop_back();
    forLoop->tokens[4]->accept(this);
    auto cond = value();
    forLoop->tokens[6]->accept(this);
    auto assignment = (AssignStatement *) (nodes.back());
    nodes.pop_back();

    auto loop = new (local<ForLoop>()) ForLoop(varInit,
                            cond,
                            assignment, LoopScope { nullptr, ZERO_LOC }, parent_node, loc(forLoop));

    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    const auto body = forLoop->tokens[8];
    loop->body.nodes = take_body_nodes(this, body, loop);
    loop->body.location = loc(body);
    current_loop_node = prevLoop;

    put_node(loop, forLoop);

}

void CSTConverter::visitWhile(CSTToken* whileCst) {
    // visiting the condition expression
    whileCst->tokens[2]->accept(this);
    // get it
    auto cond = value();
    // construct a loop
    auto loop = new (local<WhileLoop>()) WhileLoop(cond, LoopScope{ nullptr, loc(whileCst->tokens[4]) }, parent_node, loc(whileCst));
    loop->body.parent_node = loop;
    // save current nodes
    auto previous = std::move(nodes);
    // visit the body
    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    loop->body.nodes = take_body_nodes(this, whileCst->tokens[4], loop);
    current_loop_node = prevLoop;
    // restore nodes
    nodes = std::move(previous);
    put_node(loop, whileCst);
}

void CSTConverter::visitDoWhile(CSTToken* doWhileCst) {
    // visit the 2nd last token which is the expression for condition
    auto cond_index = doWhileCst->tokens.size() - 2;
    doWhileCst->tokens[cond_index]->accept(this);
    // get it
    auto cond = value();
    // construct a loop
    auto loop = new (local<DoWhileLoop>()) DoWhileLoop(cond, LoopScope{nullptr, loc(doWhileCst->tokens[1])}, parent_node, loc(doWhileCst));
    loop->body.parent_node = loop;
    // save current nodes
    auto previous = std::move(nodes);
    // visit the body
    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    loop->body.nodes = take_body_nodes(this, doWhileCst->tokens[1], loop);
    current_loop_node = prevLoop;
    // restore nodes
    nodes = std::move(previous);
    put_node(loop, doWhileCst);
}

unsigned int collect_struct_members(
        CSTConverter *conv,
        CSTToken*  comp_token,
        VariablesContainer *container,
        MembersContainer* mem_container,
        unsigned i
) {
    auto& tokens = comp_token->tokens;
    auto& variables = container->variables;
    auto prev_anns = std::move(conv->annotations);
    while (!is_char_op(tokens[i], '}')) {
        auto& token = tokens[i];
        token->accept(conv);
        switch(token->type()) {
            case LexTokenType::CompStructMember:{
                const auto node = (StructMember *) conv->pop_last_node();
                variables[node->name] = node;
                break;
            }
            case LexTokenType::CompFunction:{
                const auto node = (FunctionDeclaration *) conv->pop_last_node();
                if(!mem_container->insert_multi_func(node)) {
                    conv->error("conflict inserting function with name " + node->name(), token);
                }
                break;
            }
            case LexTokenType::CompStructDef:{
                const auto node = (UnnamedStruct*) conv->pop_last_node();
                variables[node->name] = node;
                break;
            }
            case LexTokenType::CompUnionDef:{
                const auto node = (UnnamedUnion*) conv->pop_last_node();
                variables[node->name] = node;
                break;
            }
            case LexTokenType::CompVariantMember:{
                const auto node = (VariantMember*) conv->pop_last_node();
                variables[node->name] = node;
                break;
            }
            default: {
                i++;
                continue;
            }
        }

        if (is_char_op(tokens[i + 1], ';')) {
            i++;
        }

        i++;

    }
    conv->annotations = std::move(prev_anns);
    return i;
}

void get_inherit_list(CSTConverter* converter, CSTToken*  container, unsigned& i, std::vector<std::unique_ptr<InheritedType>>& inherited) {
    while(true) {
        auto specifier = specifier_token(container->tokens[i]);
        if(specifier.has_value()) {
            i++;
        } else {
            specifier.emplace(AccessSpecifier::Public);
        }
        container->tokens[i]->accept(converter);
        inherited.emplace_back(new InheritedType(converter->type(), specifier.value()));
        i++;
        if(is_char_op(container->tokens[i], ',')) {
            i++;
        } else {
            break;
        }
    };
}

void CSTConverter::visitStructDef(CSTToken* structDef) {
    // private struct Point<T, K> {
    unsigned i = 1;
    auto spec = specifier_token(structDef->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }
    auto& name_token = structDef->tokens[i];
    bool named = name_token->is_identifier();
    if(named) {
        i += 1;
    }
    auto& gen_token = structDef->tokens[i];
    const bool is_generic = gen_token->type() == LexTokenType::CompGenericParamsList;
    if(is_generic) {
        i += 1; // expected index of the ':'
    }
    auto has_override = is_char_op(structDef->tokens[i], ':');
    auto specifier = named ? def_specifier(spec) : (spec.has_value() ? spec.value() : AccessSpecifier::Public);
    auto& alloc = named ? allocator(specifier) : *local_allocator;
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    AnnotableNode* def;
    if(named) {
        def = new (alloc.allocate<StructDefinition>()) StructDefinition(loc_id(name_token), parent_node, loc(structDef), specifier);
        if (has_override) {
            i++; // set on access specifier or the inherited struct / interface name
            get_inherit_list(this, structDef, i, ((StructDefinition*) def)->inherited);
        }
    } else {
        def = new (local<StructDefinition>()) UnnamedStruct(structDef->tokens[structDef->tokens.size() - 1]->value(), parent_node, loc(structDef), specifier);
    }
    i += 1;// positioned at first node or '}'
    if(is_generic) {
        convert_generic_list(this, alloc, gen_token, ((StructDefinition*) def)->generic_params, def);
    }
    auto prev_struct_decl = current_members_container;
    current_members_container = (StructDefinition*) def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, structDef, def->as_variables_container(), (MembersContainer*) def, i);
    collect_annotations_in(this, def);
    put_node(def, structDef);
    parent_node = prev_parent;
    current_members_container = prev_struct_decl;
    local_allocator = prev_alloc;
}

void CSTConverter::visitInterface(CSTToken* interface) {
    // private interface Point <T, K> {
    unsigned i = 1;
    auto spec = specifier_token(interface->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }
    auto specifier = def_specifier(spec);
    auto& alloc = allocator(specifier);
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    auto def = new (alloc.allocate<InterfaceDefinition>()) InterfaceDefinition(loc_id(interface->tokens[i]), parent_node, loc(interface), specifier);
    i += 1;
    const auto& gen_token = interface->tokens[i];
    if(gen_token->type() == LexTokenType::CompGenericParamsList) {
        convert_generic_list(this, alloc, gen_token, def->generic_params, def);
        i += 2; // positioned at first node or '}'
    } else {
        i += 1;
    }
    auto prev_container = current_members_container;
    current_members_container = def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, interface, def, def, i);
    put_node(def, interface);
    parent_node = prev_parent;
    current_members_container = prev_container;
    local_allocator = prev_alloc;
}

void CSTConverter::visitUnionDef(CSTToken* unionDef) {
    // private union Point <T, K> {
    unsigned i = 1;
    auto spec = specifier_token(unionDef->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }
    const auto& name_token = unionDef->tokens[i];
    bool named = name_token->is_identifier();
    if(named) {
        i += 2; // positioned at first node or '}'
    } else {
        i += 1; // positioned at first node or '}'
    }
    auto specifier = named ? def_specifier(spec) : (spec.has_value() ? spec.value() : AccessSpecifier::Public);
    auto& alloc = allocator(specifier);
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    AnnotableNode* def;
    if(named) {
        def = new (alloc.allocate<UnionDef>()) UnionDef(
            loc_id(name_token),
            parent_node,
            loc(unionDef),
            specifier
        );
    } else {
        def = new (alloc.allocate<UnnamedUnion>()) UnnamedUnion(
                str_token(unionDef->tokens[unionDef->tokens.size() - 1]),
                parent_node,
                loc(unionDef),
                specifier
        );
    }
    auto prev_container = current_members_container;
    current_members_container = (MembersContainer*) def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, unionDef, def->as_variables_container(), (MembersContainer*) def, i);
    put_node(def, unionDef);
    current_members_container = prev_container;
    parent_node = prev_parent;
    local_allocator = prev_alloc;
}

void CSTConverter::visitImpl(CSTToken* impl) {
    unsigned i = 1;
    auto& alloc = global_allocator;
    auto def = new (alloc.allocate<ImplDefinition>()) ImplDefinition(parent_node, loc(impl));
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    if(impl->tokens[i]->type() == LexTokenType::CompGenericParamsList) {
        convert_generic_list(this, alloc, impl->tokens[i], def->generic_params, def);
        i++;
    }
    impl->tokens[i]->accept(this);
    def->interface_type = type();
    bool has_for = is_keyword(impl->tokens[++i], "for");
    if (has_for) {
        impl->tokens[++i]->accept(this);
        def->struct_type = type();
    } else {
        def->struct_type = nullptr;
    }
    i = has_for ? 5 : 3; // positioned at first node or '}'
    auto prev_container = current_members_container;
    current_members_container = def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, impl, def, def, i);
    parent_node = prev_parent;
    current_members_container = prev_container;
    put_node(def, impl);
    local_allocator = prev_alloc;
}

void CSTConverter::visitVariantMember(CSTToken* variant_member) {
    const auto member = new (local<VariantMember>()) VariantMember(str_token(variant_member->tokens[0]), (VariantDefinition*) parent_node, loc(variant_member));
    auto result = function_params(this, nullptr, variant_member->tokens, 2);
    for(auto& value : result.params) {
        // TODO allow user to make variant member param const
        member->values[value->name] =  new (local<VariantMemberParam>()) VariantMemberParam(value->name, value->index, false, value->type, value->defValue ? value->defValue : nullptr, member, value->location);
    }
    put_node(member, variant_member);
}

void CSTConverter::visitVariant(CSTToken* variantDef) {
    unsigned i = 1;
    // private variant Optional <T, K> : Other {
    // don't even know if inheritance in variants is something valid
    // so just leaving the code here, if some day I decide to do it
    auto spec = specifier_token(variantDef->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }
    auto specifier = def_specifier(spec);
    auto& alloc = allocator(specifier);
    auto prev_alloc = local_allocator;
    local_allocator = &alloc;
    const auto& name_token = variantDef->tokens[i];
    bool named = name_token->is_identifier();
    if(named) {
        i += 1;
    }
    const auto& gen_token = variantDef->tokens[i];
    const bool is_generic = gen_token->type() == LexTokenType::CompGenericParamsList;
    if(is_generic) {
        i += 1; // expected index of the ':'
    }
    auto has_override = is_char_op(variantDef->tokens[i], ':');
    auto def = new (alloc.allocate<VariantDefinition>()) VariantDefinition(
            loc_id(named ? name_token : variantDef->tokens[variantDef->tokens.size() - 1]),
            parent_node,
            loc(variantDef),
            specifier
    );
    if (has_override) {
        i++; // set on access specifier or the inherited struct / interface name
        get_inherit_list(this, variantDef, i, def->inherited);
    }
    i += 1;// positioned at first node or '}'
    if(is_generic) {
        convert_generic_list(this, alloc, gen_token, def->generic_params, def);
    }
    auto prev_struct_decl = current_members_container;
    current_members_container = def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, variantDef, def, def, i);
    collect_annotations_in(this, def);
    put_node(def, variantDef);
    parent_node = prev_parent;
    current_members_container = prev_struct_decl;
    local_allocator = prev_alloc;
}

void CSTConverter::visitTryCatch(CSTToken* tryCatch) {
    auto chain = (CSTToken* ) tryCatch->tokens[1];
    if (chain->tokens.size() != 1 || chain->tokens[0]->type() != LexTokenType::CompFunctionCall) {
        error("expected a function call after try keyword", chain);
        return;
    }
    chain->accept(this);
    auto call = value()->as_access_chain();
    auto try_catch = new (local<TryCatch>()) TryCatch(std::unique_ptr<FunctionCall>((FunctionCall *) call->values[0]), std::nullopt,
                                  std::nullopt, parent_node, loc(tryCatch));
    auto last = tryCatch->tokens[tryCatch->tokens.size() - 1];
    if (is_keyword(tryCatch->tokens[2], "catch") && last->type() == LexTokenType::CompBody) {
        try_catch->catchScope.emplace(take_body(this, last, try_catch));
    }
    // TODO catch variable not supported yet
    put_node(try_catch, tryCatch);
}

// (mut x) <-- first token is a keyword 'mut'
// (dyn (mut x)) <--- second token is a qualified type that has 'mut'
bool get_is_qual_mutable(CSTToken* tok) {
    if(tok->type() == LexTokenType::CompQualifiedType) {
        if(is_keyword(tok->tokens[0], "mut")) {
            return true;
        } else {
            return get_is_qual_mutable(tok->tokens[1]);
        }
    } else {
        return false;
    }
}

/**
 * *mut Phone <-- direct linked type made mutable, ptr before type finds the linked type, makes itself mutable \n
 * *mut dyn Phone <-- mut finds dynamic type, get's the linked type, makes it mutable, ptr before type, finds the dynamic type, makes it self mutable based on it's child linked type \n
 * *dyn mut Phone <-- mut finds linked type, makes it mutable, ptr before type finds dynamic type, makes it self mutable based on it's child linked type type \n
 * **mut Phone <-- pointer finds pointer, makes itself mutable based on it \n
 * mut Phone* <-- mut finds pointer type, makes its child linked type mutable \n
 * mut dyn Phone* <-- mut finds dyn type, makes its child linked type mutable \n
 * dyn mut Phone* <--- mut finds pointer type, makes it's child linked type mutable \n
 * mut Phone** <-- mut finds pointer type, makes it's child pointer type mutable and grand child linked type mutable \n
 */
void CSTConverter::visitPointerType(CSTToken* cst) {
    const auto tok_zero = cst->tokens[0];
    const auto is_pointer_before = is_char_op(tok_zero, '*');
    if(is_pointer_before) {
        // user wrote the '*' first, user likes rust
        cst->tokens[1]->accept(this);
    } else {
        // user likes C++, he wrote type first
        tok_zero->accept(this);
    }
    const auto elem_type = type();
    const auto ptr_type = new (local<PointerType>()) PointerType(elem_type, loc(cst));
    if(is_pointer_before){
        const auto child_tok = cst->tokens[1];
        ptr_type->is_mutable = get_is_qual_mutable(child_tok);
    }
    put_type(ptr_type, cst);
}

void CSTConverter::visitReferenceType(CSTToken* cst) {
    const auto tok_zero = cst->tokens[0];
    const auto is_pointer_before = is_char_op(tok_zero, '&');
    if(is_pointer_before) {
        // user wrote the '&' first, user likes rust
        cst->tokens[1]->accept(this);
    } else {
        // user likes C++, he wrote type first
        tok_zero->accept(this);
    }
    const auto elem_type = type();
    const auto ref_type = new (local<ReferenceType>()) ReferenceType(elem_type, loc(cst));
    if(is_pointer_before) {
        const auto child_tok = cst->tokens[1];
        ref_type->is_mutable = get_is_qual_mutable(child_tok);
    }
    put_type(ref_type, cst);
}

void CSTConverter::visitGenericType(CSTToken* cst) {
    const auto ref_tok = cst->tokens[0]; // type or straight type
    ref_tok->accept(this);
    auto base_type = (LinkedType*) type();
    if(base_type->type == "literal" && cst->tokens.size() == 4) {
        const auto id_tok = cst->tokens[2]; // type or straight type
        id_tok->accept(this);
        auto child_type = type();
        if(((LinkedType*) child_type)->type == "string") {
            child_type = new (local<StringType>()) StringType(loc(id_tok));
        }
        put_type(new (local<LiteralType>()) LiteralType(child_type, loc(cst)), cst);
        return;
    }
    auto generic_type = new (local<GenericType>()) GenericType(base_type);
    unsigned i = 1;
    while(i < cst->tokens.size()) {
        if(cst->tokens[i]->is_type()) {
            cst->tokens[i]->accept(this);
            generic_type->types.emplace_back(type());
        }
        i++;
    }
    put_type(generic_type, cst);
}

void CSTConverter::visitQualifiedType(CSTToken *qualType) {
    const auto qual_tok = qualType->tokens[0];
    const auto& val = qual_tok->value();
    const auto child_tok = qualType->tokens[1];
    // visiting the child type
    child_tok->accept(this);
    const auto t = type();
    if(val == "dyn") {
        const auto kind = t->kind();
        switch(kind) {
            case BaseTypeKind::Array:{
                // since dyn Phone[] means (dyn Phone)[] and not dyn (Phone[])
                const auto elem_type = ((ArrayType*) t)->elem_type;
                ((ArrayType*) t)->elem_type = new (local<DynamicType>()) DynamicType(elem_type, loc(qualType));
                re_put_type(t);
                break;
            }
            case BaseTypeKind::Pointer: {
                // since dyn Phone* or dyn *Phone means (dyn Phone)* or *(dyn Phone) and not dyn (*Phone) or dyn (Phone*)
                const auto elem_type = ((PointerType*) t)->type;
                ((PointerType*) t)->type = new(local<DynamicType>()) DynamicType(elem_type, loc(qualType));
                re_put_type(t);
                break;
            }
            case BaseTypeKind::Reference: {
                // since dyn &Phone or dyn &Phone means (dyn Phone)& or &(dyn Phone) and not dyn (&Phone) or dyn (Phone&)
                const auto elem_type = ((ReferenceType*) t)->type;
                ((ReferenceType*) t)->type = new(local<DynamicType>()) DynamicType(elem_type, loc(qualType));
                re_put_type(t);
                break;
            }
            case BaseTypeKind::Linked:
            case BaseTypeKind::Generic:
                put_type(new (local<DynamicType>()) DynamicType(t, loc(qualType)), qualType);
                break;
            default:
                put_type(new (local<DynamicType>()) DynamicType(t, loc(qualType)), qualType);
                error("child type cannot be used with dynamic type", child_tok);
                break;
        }
    } else if(val == "mut") {
        if(t->make_mutable(t->kind())) {
            re_put_type(t);
        } else {
            error("couldn't make type mutable", child_tok);
        };
    } else {
        error("unknown qualified type given", qual_tok);
    }
}

void CSTConverter::visitArrayType(CSTToken* arrayType) {
    arrayType->tokens[0]->accept(this);
    BaseType* elem_type = type();
    Value* val = nullptr;
    if(arrayType->tokens[2]->is_value()) {
        arrayType->tokens[2]->accept(this);
        val = value();
    }
    put_type(new (local<ArrayType>()) ArrayType(elem_type, val, loc(arrayType)), arrayType);
}

void CSTConverter::visitFunctionType(CSTToken* funcType) {
    bool is_capturing = is_char_op(funcType->tokens[0], '[');
    const auto func_type = new (local<FunctionType>()) FunctionType({}, nullptr, false, is_capturing, parent_node, loc(funcType));
    auto params = function_params(this, func_type, funcType->tokens, is_capturing ? 3 : 1);

    visit(funcType->tokens, params.index + 2);

    func_type->params = std::move(params.params);
    func_type->setIsVariadic(params.isVariadic);
    func_type->returnType = type();

    put_type(func_type, funcType);
}


void CSTConverter::visitStringToken(CSTToken* token) {
    auto escaped = escape_all(token->value(), 1, token->value().size() - 1, [this, token](const std::string& value, unsigned int index) {
        error("invalid escape sequence found, character '" + std::string(1, value[index]) + "'", token);
    });
    put_value(new (local<StringValue>()) StringValue(escaped, loc(token)), token);
}

void CSTConverter::visitCharToken(CSTToken* token) {
    char value;
    if(token->value()[1] == '\\') {
        auto result = escape_single(token->value(), 2);
        value = result.first;
        if(!result.second) {
            error("unknown / invalid escape sequence present", token);
        }
    } else {
        value = token->value()[1];
    }
    put_value(new (local<CharValue>()) CharValue(value, loc(token)), token);
}

void CSTConverter::visitNumberToken(NumberToken *token) {
    // we take the mutable value of the token
    // every change we make to the token's value, we revert that change back
    // to keep the token value legitimate
    auto& mut_value = token->unsafe_mutable_value();
    const auto result = convert_number_to_value(*local_allocator, const_cast<char*>(mut_value.c_str()), mut_value.size(), is64Bit, loc(token));
    if(!result.error.empty()) {
        error(result.error, token);
    }
    put_value(result.result, token);
}

BaseType* convert_ref_value_to_type(CSTConverter* converter, Value* value, CSTToken* generic_token, ASTAllocator& allocator) {
    LinkedType* linked_type;
    const auto has_generic_list = generic_token->type() == LexTokenType::CompGenericList;
    const auto id = value->as_identifier();
    if(id) {
        linked_type = new (allocator.allocate<LinkedType>()) LinkedType(id->value, id->location);
    } else {
        const auto chain = value->as_access_chain();
        if(chain) {
            linked_type = new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain, chain->location);
        } else {
#ifdef DEBUG
            throw std::runtime_error("unknown ref value type provided to convert_ref_value_to_type");
#else
            linked_type = nullptr;
#endif
        }
    }
    if(has_generic_list) {
        const auto gen_type = new (allocator.allocate<GenericType>()) GenericType(linked_type);
        converter->to_generic_arg_list(gen_type->types, generic_token);
        return gen_type;
    } else {
        return linked_type;
    }
}

void CSTConverter::visitStructValue(CSTToken* cst) {
    cst->tokens[0]->accept(this);
    const auto generic_token = cst->tokens[1];
    const auto has_generic_list = cst->tokens[1]->type() == LexTokenType::CompGenericList;
    auto& alloc = *local_allocator;
    auto struct_value = new (alloc.allocate<StructValue>()) StructValue(
            convert_ref_value_to_type(this, value(), generic_token, alloc),
            {}, nullptr, loc(cst), parent_node
        );
    auto i = has_generic_list ? 3 : 2; // first identifier or '}'
    while (!is_char_op(cst->tokens[i], '}')) {
        auto id = str_token(cst->tokens[i]);
        i += 2;
        cst->tokens[i]->accept(this);
        auto& member_ptr = struct_value->values[id]; // <--- automatic construction
        member_ptr = new (local<StructMemberInitializer>()) StructMemberInitializer(
            id,
            value(),
            struct_value
        );
        i++;
        if (is_char_op(cst->tokens[i], ',')) {
            i++;
        }
    }
    put_value(struct_value, cst);
}

void CSTConverter::visitArrayValue(CSTToken* arrayValue) {
    unsigned i = 1;
    std::vector<Value*> arrValues;
    while (arrayValue->tokens[i]->type() != LexTokenType::CharOperator || char_op(arrayValue->tokens[i]) != '}') {
        if (arrayValue->tokens[i]->type() != LexTokenType::CharOperator || char_op(arrayValue->tokens[i]) != ',') {
            arrayValue->tokens[i]->accept(this);
            // consume the value
            arrValues.emplace_back(value());
        }
        i++;
    }
    i++;
    BaseType* arrType = nullptr;
    std::vector<unsigned int> sizes;
    if (i < arrayValue->tokens.size()) {
        if(arrayValue->tokens[i]->is_type()) {
            arrayValue->tokens[i]->accept(this);
            arrType = type();
        }
        i++;
        if (i < arrayValue->tokens.size() && char_op(arrayValue->tokens[i++]) == '(') {
            while (i < arrayValue->tokens.size() && (arrayValue->tokens[i]->type() != LexTokenType::CharOperator || char_op(arrayValue->tokens[i]) != ')')) {
                if ((arrayValue->tokens[i]->type() != LexTokenType::CharOperator || char_op(arrayValue->tokens[i]) != ',')) {
                    arrayValue->tokens[i]->accept(this);
                    // consume the value
                    sizes.emplace_back(value()->get_the_int());
                }
                i++;
            }
        }
    }
    put_value(new (local<ArrayValue>()) ArrayValue(
            std::move(arrValues),
            arrType,
            std::move(sizes),
            loc(arrayValue),
            *local_allocator
        ),
      arrayValue
  );
}

std::vector<Value*> take_values(CSTConverter *converter, const std::function<void()> &visit) {
    auto prev_values = std::move(converter->values);
    visit();
    auto new_values = std::move(converter->values);
    converter->values = std::move(prev_values);
    return new_values;
}

void CSTConverter::to_generic_arg_list(std::vector<BaseType*>& generic_list, CSTToken*  container) {
    auto& generic_tokens = container->tokens;
    unsigned i = 0;
    CSTToken* token;
    while(i < generic_tokens.size()) {
        token = generic_tokens[i];
        if(token->is_type()) {
            token->accept(this);
            generic_list.emplace_back(type());
        }
        i++;
    }
}

void CSTConverter::visitFunctionCall(CSTToken* call) {
    std::vector<BaseType*> generic_list;
    if(call->tokens[0]->type() == LexTokenType::CompGenericList) {
        to_generic_arg_list(generic_list, call->tokens[0]);
    }
    auto prev_values = std::move(values);
    visit(call->tokens, 1);
    auto func_call = new (local<FunctionCall>()) FunctionCall(std::move(values), loc(call));
    func_call->generic_list = std::move(generic_list);
    values = std::move(prev_values);
    put_value(func_call, call);
}

void CSTConverter::visitIndexOp(CSTToken* op) {
    auto indexes = take_values(this, [&op, this]() {
        visit(op->tokens, 1);
    });
    put_value(new (local<IndexOperator>()) IndexOperator(std::move(indexes), loc(op)), op);
}

void CSTConverter::visitLoopBlock(CSTToken *block) {

    auto is_value = block->type() == LexTokenType::CompLoopValue;

    auto loop_block = new (local<LoopBlock>()) LoopBlock(LoopScope { nullptr, loc(block->tokens[1]) }, parent_node, loc(block));
    loop_block->body.nodes = take_body_nodes(this, block->tokens[1], parent_node);

    if(is_value) {
        put_value(loop_block, block);
    } else {
        put_node(loop_block, block);
    }

}

void CSTConverter::visitAccessChain(CSTToken* chain) {
    auto prev_values = std::move(values);
    unsigned int i = 0;
    unsigned int size = chain->tokens.size();
    while(i < size) {
        auto token = chain->tokens[i];
        if(is_str_op(token, "::")) {
            auto prev = pop_last_value();
            auto as_id = prev->as_identifier();
            if(as_id) {
                put_value(new (local<VariableIdentifier>()) VariableIdentifier(as_id->value, loc(token), true), token);
            } else {
                put_value(prev, token);
            }
        } else if(!is_char_op(token, '.')) {
            token->accept(this);
        }
        i++;
    }
    const auto is_node = chain->type() == LexTokenType::CompAccessChainNode;
    auto ret_chain = new (local<AccessChain>()) AccessChain(std::vector<ChainValue*> {}, parent_node, is_node, loc(chain));
    for(auto& value : values) {
        ret_chain->values.emplace_back((ChainValue*) value);
    }
    values = std::move(prev_values);
    if (is_node) {
        put_node(ret_chain, chain);
    } else {
        if(ret_chain->values.size() == 1) {
            put_value(ret_chain->values[0], chain);
        } else {
            put_value(ret_chain, chain);
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
                op_stack.putValue(converter->value());
            } else {
                // - a number:
                // put it into the output queue
                expr->accept(converter);
                output.putValue(converter->value());
            }
        } else {
            auto nested = ((CSTToken* ) expr);
            auto is_braced = is_char_op(nested->tokens[0], '(');
            auto first_val_index = is_braced ? 1 : 0;
            auto op_index = is_braced ? 3 : 1;
            auto second_val_index = op_index + 1;
            if (is_braced) { //    - a left parenthesis '(':
                // push it onto the operator stack
                op_stack.putCharacter('(');
            }
            // visiting the first value
            visitNestedExpr(converter, nested->tokens[first_val_index], op_stack, output);
            if (is_braced) { // a right parenthesis ')':
                sy_onRParen(op_stack, output);
            }
            if(nested->tokens.size() <= op_index) { // no operator present, just expression like '(' 'nested_expr' ')'
                return;
            }
            const auto opt = get_operation_safe(nested->tokens[op_index]);
            if(!opt.has_value()) {
                return;
            }
            const auto o1 = opt.value();
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
            if (is_char_op(nested->tokens[second_val_index], '(')) {
                op_stack.putCharacter('(');
                visitNestedExpr(converter, nested->tokens[second_val_index + 1], op_stack, output);
                sy_onRParen(op_stack, output);
            } else {
                visitNestedExpr(converter, nested->tokens[second_val_index], op_stack, output);
            }
        }
    }
}

void CSTConverter::visitExpression(CSTToken* expr) {
    auto is_braced = is_char_op(expr->tokens[0], '(');
    const auto expr_tokens_size = expr->tokens.size();
    if(is_braced && is_char_op(expr->tokens[2], ')') && expr_tokens_size <= 3) {
        // handles braced expression (1 + 1) that's it
        expr->tokens[1]->accept(this);
        return;
    }
    auto first_val_index = is_braced ? 1 : 0;
    auto op_index = is_braced ? 3 : 1;
    auto second_val_index = op_index + 1;
    if(second_val_index >= expr_tokens_size) {
        return;
    }
    if (expr->tokens[first_val_index]->is_primitive_var() && expr->tokens[second_val_index]->is_primitive_var()) {
        // no need to create stacks for values that are primitive variables, a single expression like 1 + 2 or x + 1
        visit(expr->tokens);
        auto second = value();
        auto first = value();
        put_value(new (local<Expression>()) Expression(first, second,
                                                         (get_operation(expr->tokens[op_index])), is64Bit, loc(expr)), expr);
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
        put_value(output.toExpressionRaw(*local_allocator, is64Bit, loc(expr)), expr);
    }
}

void CSTConverter::visitCast(CSTToken* castCst) {
    visit(castCst->tokens);
    put_value(new (local<CastedValue>()) CastedValue(value(), type(), loc(castCst)), castCst);
}

void CSTConverter::visitIsValue(CSTToken* isCst) {
    visit(isCst->tokens);
    put_value( new (local<IsValue>()) IsValue(value(), type(), isCst->tokens[1]->value()[0] == '!', loc(isCst)), isCst);
}

void CSTConverter::visitAddrOf(CSTToken* addrOf) {
    addrOf->tokens[1]->accept(this);
    put_value(new (local<AddrOfValue>()) AddrOfValue(value(), loc(addrOf)), addrOf);
}

void CSTConverter::visitDereference(CSTToken* deref) {
    deref->tokens[1]->accept(this);
    put_value(new (local<DereferenceValue>()) DereferenceValue(value(), loc(deref)), deref);
}

void CSTConverter::visitVariableToken(CSTToken* token) {
    put_value(new (local<VariableIdentifier>()) VariableIdentifier(token->value(), loc(token)), token);
}

void CSTConverter::visitBoolToken(CSTToken* token) {
    put_value(new (local<BoolValue>()) BoolValue(token->value()[0] == 't', loc(token)), token);
}

void CSTConverter::visitNullToken(CSTToken* token) {
    put_value(new (local<NullValue>()) NullValue(loc(token)), token);
}

void CSTConverter::visitNegative(CSTToken* neg) {
    visit(neg->tokens);
    put_value(new (local<NegativeValue>()) NegativeValue(value(), loc(neg)), neg);
}

void CSTConverter::visitNot(CSTToken* notCst) {
    visit(notCst->tokens);
    put_value(new (local<NotValue>()) NotValue(value(), loc(notCst)), notCst);
}

CSTConverter::~CSTConverter() = default;