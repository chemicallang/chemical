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
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/statements/Continue.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/DestructStmt.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/Break.h"
#include "ast/statements/Typealias.h"
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
    return (Operation) std::stoi(num);
}

std::vector<std::unique_ptr<ASTNode>> take_body_nodes(CSTConverter *conv, CSTToken *token, ASTNode* parent_node) {
    auto prev_nodes = std::move(conv->nodes);
    auto prev_parent = conv->parent_node;
    conv->parent_node = parent_node;
    token->accept(conv);
    conv->parent_node = prev_parent;
    auto nodes = std::move(conv->nodes);
    conv->nodes = std::move(prev_nodes);
    return nodes;
}

std::vector<std::unique_ptr<ASTNode>> take_body_or_single_stmt(CSTConverter *conv, CSTToken *container, unsigned &i, ASTNode* parent_node) {
    auto& token = container->tokens[i];
    const auto type = token->type();
    if(type == LexTokenType::CompBody) {
        return take_body_nodes(conv, container->tokens[i], parent_node);
    } else {
        token->accept(conv);
        std::vector<std::unique_ptr<ASTNode>> nodes(1);
        if(CSTToken::is_value(type)) {
            nodes[0] = std::make_unique<ValueNode>(std::unique_ptr<Value>(conv->pop_last_value()), parent_node, token);
        } else {
            nodes[0] = std::unique_ptr<ASTNode>(conv->pop_last_node());
        }
        if(i + 1 < container->tokens.size() && is_char_op(container->tokens[i + 1], ';')) {
            i++;
        }
        return nodes;
    }
}

std::vector<std::unique_ptr<ASTNode>> take_comp_body_nodes(CSTConverter *conv, CSTToken* token, ASTNode* parent_node) {
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
    return { take_body_nodes(conv, token, parent_node), parent_node, token };
}

Scope take_body_compound(CSTConverter *conv, CSTToken* token, ASTNode* parent_node) {
    return { take_comp_body_nodes(conv, token, parent_node), parent_node, token };
}

// TODO support _128bigint, bigfloat
CSTConverter::CSTConverter(std::string path, bool is64Bit, std::string target) : path(std::move(path)), is64Bit(is64Bit), target(std::move(target)), global_scope() {

}

const std::unordered_map<std::string, MacroHandlerFn> MacroHandlers = {
        { "eval", [](CSTConverter* converter, CSTToken*  container){
            if(container->tokens[2]->is_value()) {
                container->tokens[2]->accept(converter);
                auto take_value = converter->value();
                InterpretScope child_scope{&converter->global_scope, &converter->global_scope};
                auto evaluated_value = take_value->evaluated_value(child_scope);
                if(evaluated_value.get() == nullptr) {
                    converter->error("couldn't evaluate value", container);
                    return;
                }
                converter->put_value(evaluated_value.release(), container);
                if(take_value.get() == evaluated_value.get()) {
                    take_value.release();
                }
            } else {
                converter->error("expected a value for eval", container);
            }
        }},
        { "target", [](CSTConverter* converter, CSTToken* container) {
            converter->put_value(new StringValue(converter->target, container), container);
        }},
        { "file:path", [](CSTConverter* converter, CSTToken* container) {
            converter->put_value(new StringValue(converter->path, container), container);
        }},
        {"target:is64bit", [](CSTConverter* converter, CSTToken* container) {
            converter->put_value(new BoolValue(converter->is64Bit, container), container);
        }},
        {"sizeof", [](CSTConverter* converter, CSTToken*  container) {
            auto& tok = container->tokens[2];
            if(tok->is_type()) {
                tok->accept(converter);
                auto value = new SizeOfValue(converter->type().release(), tok);
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
            converter->put_value(new StringValue(ostring.str(), container), container);
        }},
        {"tr:debug:chemical:value", [](CSTConverter* converter, CSTToken*  container) {
            if(container->is_value()) {
                container->accept(converter);
                auto value = converter->value();
                std::ostringstream ostring;
                RepresentationVisitor visitor(ostring);
                value->accept(&visitor);
                converter->put_value(new StringValue(ostring.str(), container), container);
            } else {
                converter->error("expected a value in tr:debug:chemical:value", container);
            }
        }},
        {"tr:debug:c", [](CSTConverter* converter, CSTToken* container) {
            auto body = take_body_compound(converter, container, converter->parent_node);
            std::ostringstream ostring;
            GlobalInterpretScope scope;
            ToCAstVisitor visitor(scope, &ostring);
            visitor.translate(body.nodes);
            converter->put_value(new StringValue(ostring.str(), container), container);
        }},
        {"tr:debug:c:value",[](CSTConverter* converter, CSTToken* container) {
            if(container->is_value()) {
                container->accept(converter);
                auto value = converter->value();
                std::ostringstream ostring;
                GlobalInterpretScope scope;
                ToCAstVisitor visitor(scope, &ostring);
                value->accept(&visitor);
                converter->put_value(new StringValue(ostring.str(), container), container);
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
        { "cbi:global", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            if(!converter->isCBIEnabled) return;
            converter->dispose_node = true;
        }}},
        { "cbi:to", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            if(!converter->isCBIEnabled) return;
            converter->dispose_node = true;
        }}},
        { "cbi:begin", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            if(!converter->isCBIEnabled) return;
            converter->dispose_node = true;
            converter->keep_disposing = true;
        }}},
        { "cbi:end", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            if(!converter->isCBIEnabled) return;
            converter->dispose_node = false;
            converter->keep_disposing = false;
        }}},
        { "cbi:compile", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            if(!converter->isCBIEnabled) return;
            converter->dispose_node = false;
            converter->keep_disposing = false;
        }}},
        { "dispose", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            auto result = annotation_bool_arg(0, container);
            if(result.has_value()) {
                converter->dispose_node = result.value();
            } else {
                converter->error("unknown value given to dispose annotation", container);
            }
        }}},
        { "dispose:end", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            converter->dispose_node = false;
            converter->keep_disposing = false;
        }}},
        { "dispose:begin:", { [](CSTConverter* converter, CSTToken* container, AnnotationKind kind){
            converter->dispose_node = true;
            converter->keep_disposing = true;
        }}},
        { "inline:", { collect_annotation_func, AnnotationKind::Inline } },
        { "inline:always", { collect_annotation_func, AnnotationKind::AlwaysInline } },
        { "noinline", { collect_annotation_func, AnnotationKind::NoInline } },
        { "inline:no", { collect_annotation_func, AnnotationKind::NoInline } },
        { "inline:hint", { collect_annotation_func, AnnotationKind::InlineHint } },
        { "size:opt", { collect_annotation_func, AnnotationKind::OptSize } },
        { "size:min", { collect_annotation_func, AnnotationKind::MinSize } },
        { "comptime", { collect_annotation_func, AnnotationKind::CompTime } },
        { "constructor", { collect_annotation_func, AnnotationKind::Constructor } },
        { "delete", { collect_annotation_func, AnnotationKind::Delete } },
        { "override", { collect_annotation_func, AnnotationKind::Override } },
        { "no_init", { collect_annotation_func, AnnotationKind::NoInit }},
        { "extern", { collect_annotation_func, AnnotationKind::Extern }},
        { "implicit", { collect_annotation_func, AnnotationKind::Implicit }},
        { "propagate", { collect_annotation_func, AnnotationKind::Propagate }},
        { "direct_init", { collect_annotation_func, AnnotationKind::DirectInit }},
        { "cpp", { collect_annotation_func, AnnotationKind::Cpp }},
        { "clear", { collect_annotation_func, AnnotationKind::Clear }},
        { "copy", { collect_annotation_func, AnnotationKind::Copy }},
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

AccessSpecifier CSTConverter::def_specifier(std::optional<AccessSpecifier> opt) {
    if(opt.has_value()) return opt.value();
    if(parent_node) {
        return parent_node->specifier();
    }
    return AccessSpecifier::Internal;
}

std::unique_ptr<Value> CSTConverter::value() {
#ifdef DEBUG
    if(values.empty()) {
        throw std::runtime_error("CSTConverter::value called when values vector is empty");
    }
#endif
    auto value = std::move(values.back());
    values.pop_back();
    return value;
}

void CSTConverter::put_type(BaseType* type, CSTToken* token) {
    types.emplace_back(type);
#ifdef LSP_BUILD
    token->any = type;
#endif
}

void CSTConverter::put_value(Value* value, CSTToken* token) {
    values.emplace_back(value);
#ifdef LSP_BUILD
    token->any = value;
#endif
}

void CSTConverter::put_node(ASTNode* node, CSTToken* token) {
    nodes.emplace_back(node);
#ifdef LSP_BUILD
    token->any = node;
#endif
}

ASTNode* CSTConverter::pop_last_node() {
    const auto last = nodes.back().release();
    nodes.pop_back();
    return last;
}

Value* CSTConverter::pop_last_value() {
    const auto last = values.back().release();
    values.pop_back();
    return last;
}

ASTUnit CSTConverter::take_unit() {
    ASTUnit unit;
    unit.scope.nodes = std::move(nodes);
    unit.types = std::move(types);
    unit.values = std::move(values);
    unit.nested_nodes = std::move(nested_nodes);
    return unit;
}

std::unique_ptr<BaseType> CSTConverter::type() {
#ifdef DEBUG
    if(types.empty()) {
        throw std::runtime_error("CSTConverter::type called when vector array empty");
    }
#endif
    auto type = std::move(types.back());
    types.pop_back();
    return type;
}

PointerType* current_self_pointer(CSTConverter* converter, CSTToken* token) {
#ifdef DEBUG
    if(!converter->current_members_container) {
        throw std::runtime_error("members container is nullptr");
    }
#endif
    std::string type;
    auto impl_container = converter->current_members_container->as_impl_def();
    if(impl_container) {
        type = impl_container->struct_type->linked_name();
    } else {
        type = converter->current_members_container->ns_node_identifier();
    }
    return new PointerType(std::make_unique<LinkedType>(type, token, converter->current_members_container), token);
}

void CSTConverter::visitFunctionParam(CSTToken* param) {
    auto identifier = str_token(param->tokens[0]);
    visit(param->tokens, 2);
    BaseType *baseType = nullptr;
    if (optional_param_types) {
        std::unique_ptr<BaseType> t = nullptr;
        if(2 < param->tokens.size() && param->tokens[2]->is_type()) {
            t = type();
        }
        if (t) {
            baseType = t.release();
        }
    } else {
        baseType = type().release();
    }
    std::unique_ptr<Value> def_value = nullptr;
    if(param->tokens.back()->is_value()) {
        param->tokens.back()->accept(this);
        def_value = value();
    }
    put_node(new FunctionParam(identifier, std::unique_ptr<BaseType>(baseType), param_index,
                                            std::move(def_value), nullptr, param), param);
}

struct FunctionParamsResult {
    bool isVariadic;
    std::vector<std::unique_ptr<FunctionParam>> params;
    unsigned int index;
};

// will probably leave the index at ')'
FunctionParamsResult function_params(CSTConverter* converter, cst_tokens_ref_type tokens, unsigned start) {
    auto prev_param_index = converter->param_index;
    converter->param_index = 0;
    auto isVariadic = false;
    std::vector<std::unique_ptr<FunctionParam>> params;
    unsigned i = start;
    while (i < tokens.size()) {
        if (converter->param_index == 0 && is_char_op(tokens[i]->start_token(), '&')) {
            auto &paramTokens = ((CSTToken* ) tokens[i])->tokens;
            auto strId = str_token(paramTokens[1]);
            if (strId != "this" && strId != "self") {
                converter->error("expected self parameter to be named 'self' or 'this'", tokens[i]);
            }
            params.emplace_back(new FunctionParam(strId, std::unique_ptr<PointerType>(current_self_pointer(converter, paramTokens[1])), 0, nullptr, nullptr, tokens[i]));
            converter->param_index = 1;
        }
//        else if(optional_param_types && tokens[i]->type() == LexTokenType::Variable) {
//            auto strId = str_token(tokens[1]);
//            params.emplace_back(new FunctionParam(strId, std::make_unique<VoidType>(), 0, std::nullopt));
//        }
        else if (tokens[i]->compound()) {
            tokens[i]->accept(converter);
            converter->param_index++;
            auto param = (FunctionParam *) converter->nodes.back().release();
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
    return {isVariadic, std::move(params), i};
}

void convert_generic_list(
    CSTConverter *converter,
    CSTToken*  compound,
    std::vector<std::unique_ptr<GenericTypeParameter>>& generic_list,
    ASTNode* parent_node
) {
    unsigned i = 1;
    unsigned inc = 0;
    GenericTypeParameter* parameter;
    while(i < compound->tokens.size() && compound->tokens[i]->is_identifier()) {
        std::unique_ptr<BaseType> def_type = nullptr;
        if(is_char_op(compound->tokens[i + 1], '=')) {
            compound->tokens[i + 2]->accept(converter);
            def_type = converter->type();
            inc = 4;
        } else {
            inc = 2;
        }
        parameter = new GenericTypeParameter(str_token(compound->tokens[i]), std::move(def_type), parent_node, generic_list.size(), compound->tokens[i]);
        generic_list.emplace_back(parameter);
        i += inc; // 4 -> +1 -> '=' , +2 -> 'type' , +3 -> ',' , +4 -> next identifier
    }
}

void CSTConverter::visitFunction(CSTToken* function) {

    // private func <T, K> (ext : Extension*) name(param1 : int) : long

    if(is_dispose()) {
        return;
    }

    unsigned i = 1;

    auto specifier = specifier_token(function->tokens[0]);
    if(specifier.has_value()) {
        // set at name, extension or generic params list
        i += 1;
    }

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

    auto params = function_params(this, function->tokens, i + 2);

    i = params.index;

    std::unique_ptr<BaseType> returnType = nullptr;

    if (i + 1 < function->tokens.size() && is_char_op(function->tokens[i + 1], ':')) {
        function->tokens[i + 2]->accept(this);
        returnType = type();
        i += 3; // position at body
    } else {
        i++;
    }

    if (!returnType) {
        returnType = std::make_unique<VoidType>(nullptr);
    }

    FunctionDeclaration* funcDecl;

    if(is_extension) {
        auto& receiver_tok = function->tokens[extension_start + 1];
        receiver_tok->accept(this);
        auto param = (FunctionParam*) nodes.back().release();
        nodes.pop_back();
        funcDecl = new ExtensionFunction(
                name_token->value(),
                ExtensionFuncReceiver(std::move(param->name), std::move(param->type), nullptr, receiver_tok),
                std::move(params.params),
                std::move(returnType), params.isVariadic,
                parent_node,
                function,
                std::nullopt,
                def_specifier(specifier)
        );
        ((ExtensionFunction*) funcDecl)->receiver.parent_node = funcDecl;
        delete param;
    } else {
        funcDecl = new FunctionDeclaration(
                name_token->value(),
                std::move(params.params),
                std::move(returnType), params.isVariadic,
                parent_node,
                function,
                std::nullopt,
                def_specifier(specifier)
        );
        if(!parent_node && !specifier.has_value() && name_token->value() == "main") {
            funcDecl->specifier = AccessSpecifier::Public;
            if(funcDecl->returnType->kind() != BaseTypeKind::IntN) {
                funcDecl->returnType = std::make_unique<IntType>(funcDecl->returnType->cst_token());
            }
        }
    }

    if(is_generic) {
        convert_generic_list(this, gen_token, funcDecl->generic_params, funcDecl);
    }

    if (i < function->tokens.size()) {
        funcDecl->body.emplace(LoopScope{ funcDecl, function->tokens[i] });
    }

    collect_annotations_in(this, funcDecl);

    funcDecl->assign_params();

    put_node(funcDecl, function);

    if (i >= function->tokens.size()) {
        return;
    }

    auto prev_decl = current_func_type;
    current_func_type = funcDecl;
    funcDecl->body->nodes = take_body_nodes(this, function->tokens[i], funcDecl);
    current_func_type = prev_decl;


}

void CSTConverter::visitEnumDecl(CSTToken* decl) {
    if(is_dispose()) {
        return;
    }
    unsigned i = 1;

    const auto spec = specifier_token(decl->tokens[0]);
    if(spec.has_value()) {
        i += 1;
    }

    auto enum_decl = new EnumDeclaration(
            str_token(decl->tokens[i]),
            std::unordered_map<std::string, std::unique_ptr<EnumMember>> {},
            parent_node,
            decl,
            def_specifier(spec)
    );
    i += 2;
    unsigned position = 0;
    while(!is_char_op(decl->tokens[i], '}')) {
        auto& tok = decl->tokens[i];
        if(tok->is_identifier()) {
            auto name = str_token(tok);
            enum_decl->members[name] = std::make_unique<EnumMember>(name, position++, enum_decl, tok);
            if (is_char_op(decl->tokens[i + 1], ',')) {
                i++;
            }
        }
        i++;
    }
    put_node(enum_decl, decl);
}

Value* convertNumber(NumberToken* token, ValueType value_type, bool is64Bit) {
    switch(value_type) {
        case ValueType::Int:
            return new IntValue(std::stoi(token->value()), token);
        case ValueType::UInt:
            return new UIntValue(std::stoi(token->value()), token);
        case ValueType::Short:
            return new ShortValue(std::stoi(token->value()), token);
        case ValueType::UShort:
            return new UShortValue(std::stoi(token->value()), token);
        case ValueType::Long:
            return new LongValue(std::stol(token->value()), is64Bit, token);
        case ValueType::ULong:
            return new ULongValue(std::stoul(token->value()), is64Bit, token);
        case ValueType::BigInt:
            return new BigIntValue(std::stoll(token->value()), token);
        case ValueType::UBigInt:
            return new UBigIntValue(std::stoull(token->value()), token);
        case ValueType::Float:
            return new FloatValue(std::stof(token->value()), token);
        case ValueType::Double:
            return new DoubleValue(std::stod(token->value()), token);
        default:
            return nullptr;
    }
}

void CSTConverter::visitVarInit(CSTToken* varInit) {
    if(is_dispose()) return;
    // private var i : int = 0
    auto is_struct_member = varInit->type() == LexTokenType::CompStructMember;
    unsigned i = 0;
    auto specifier = specifier_token(varInit->tokens[0]);
    if(specifier.has_value()) {
        i += 1;
    }
    bool is_const = varInit->tokens[i]->value() == "const";
    i += 1;
    AnnotableNode* init;
    if(is_struct_member) {
        init = new StructMember(
                varInit->tokens[i]->value(),
                nullptr,
                nullptr,
                parent_node,
                varInit,
                is_const,
                specifier.has_value() ? specifier.value() : AccessSpecifier::Public
        );
    } else {
        init = new VarInitStatement(
                is_const,
                varInit->tokens[i]->value(),
                nullptr,
                nullptr,
                parent_node,
                varInit,
                def_specifier(specifier)
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
            auto conv = convertNumber((NumberToken*) token, type_ref->value_type(), is64Bit);
            if(conv) {
                value_ref.reset(conv);
            } else {
                error("invalid number for the expected type", token);
            }
        } else {
            token->accept(this);
            value_ref = value();
        }
    }

    collect_annotations_in(this, init);

    parent_node = prev_parent;
    put_node(init, varInit);
}

void CSTConverter::visitAssignment(CSTToken* assignment) {
    visit(assignment->tokens, 0);
    auto val = value();
    auto chain = value().release();
    put_node( new AssignStatement(
            std::unique_ptr<Value>(chain),
            std::move(val),
            (assignment->tokens[1]->type() == LexTokenType::Operation)
            ? get_operation(assignment->tokens[1]) : Operation::Assignment,
            parent_node,
            assignment
    ), assignment);
}

void CSTConverter::visitImport(CSTToken* cst) {
    std::vector<std::string> ids;
    put_node(new ImportStatement(escaped_str_token(cst->tokens[1]), ids, parent_node, cst), cst);
}

void CSTConverter::visitReturn(CSTToken* cst) {
    std::unique_ptr<Value> return_value = nullptr;
    if(1 < cst->tokens.size() && cst->tokens[1]->is_value()) {
        cst->tokens[1]->accept(this);
        return_value = value();
    }
    put_node(new ReturnStatement(std::move(return_value), current_func_type, parent_node, cst), cst);
}

void CSTConverter::visitDestruct(CSTToken* delStmt) {
    auto is_array = is_char_op(delStmt->tokens[1], '[');
    std::unique_ptr<Value> array_value = nullptr;
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
    put_node(new DestructStmt(std::move(array_value), std::move(val_ptr), is_array, parent_node, delStmt), delStmt);
}

void CSTConverter::visitUsing(CSTToken* usingStmt) {
    std::vector<std::unique_ptr<ChainValue>> curr_values;
    unsigned i = 1;
    while(i < usingStmt->tokens.size()) {
        auto& tok = usingStmt->tokens[i];
        if(tok->is_identifier()) {
            curr_values.emplace_back(new VariableIdentifier(str_token(usingStmt->tokens[i]), tok, true));
        }
        i++;
    }
    const auto stmt = new UsingStmt(std::move(curr_values), parent_node, is_keyword(usingStmt->tokens[1], "namespace"), usingStmt);
    collect_annotations_in(this, stmt);
    put_node(stmt, usingStmt);
}

void CSTConverter::visitTypealias(CSTToken* alias) {
    if(is_dispose()) return;
    auto spec = specifier_token(alias->tokens[0]);
    unsigned i = spec.has_value() ? 2 : 1;
    const auto& name_token = alias->tokens[i];
    alias->tokens[i + 2]->accept(this);
    auto stmt = new TypealiasStatement(name_token->value(), type(), parent_node, alias, spec.has_value() ? spec.value() : AccessSpecifier::Internal);
    collect_annotations_in(this, stmt);
    put_node(stmt, alias);
}

void CSTConverter::visitTypeToken(CSTToken* token) {
    auto primitive = TypeMakers::PrimitiveMap.find(token->value());
    if (primitive == TypeMakers::PrimitiveMap.end()) {
        put_type(new LinkedType(token->value(), token), token);
    } else {
        put_type(primitive->second(is64Bit, token), token);
    }
}

void CSTConverter::visitLinkedValueType(CSTToken* ref_value) {
    ref_value->tokens[0]->accept(this);
    auto ref = new LinkedValueType(value(), ref_value);
    put_type(ref, ref_value);
}

void CSTConverter::visitContinue(CSTToken* continueCst) {
    put_node(new ContinueStatement(current_loop_node, parent_node, continueCst), continueCst);
}

void CSTConverter::visitBreak(CSTToken* breakCST) {
    auto stmt = new BreakStatement(current_loop_node, parent_node, breakCST);
    if(1 < breakCST->tokens.size() && breakCST->tokens[1]->is_value()) {
        breakCST->tokens[1]->accept(this);
        stmt->value = value();
    }
    put_node(stmt, breakCST);
}

void CSTConverter::visitIncDec(CSTToken* incDec) {
    incDec->tokens[0]->accept(this);
    auto acOp = (get_operation(incDec->tokens[1]) == Operation::PostfixIncrement ? Operation::Addition : Operation::Subtraction);
    put_node(new AssignStatement(value(), std::make_unique<IntValue>(1, incDec), acOp, parent_node, incDec), incDec);
}

void CSTConverter::visitLambda(CSTToken* cst) {

    std::vector<std::unique_ptr<CapturedVariable>> captureList;

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
                captureList.emplace_back(new CapturedVariable(((CSTToken* ) (cst->tokens[i]))->value(), capInd++, capture_by_ref, cst->tokens[i]));
            }
            i++;
        }
        i += 2;
    }

    auto prev = optional_param_types;
    optional_param_types = true;
    auto result = function_params(this, cst->tokens, i);
    optional_param_types = prev;

    auto lambda = new LambdaFunction(std::move(captureList), std::move(result.params), result.isVariadic, Scope {parent_node, nullptr}, cst);

    auto bodyIndex = result.index + 2;
    if (cst->tokens[bodyIndex]->type() == LexTokenType::CompBody) {
        auto prev_decl = current_func_type;
        current_func_type = lambda;
        lambda->scope.nodes = take_body_nodes(this, cst->tokens[bodyIndex], &lambda->scope);
        current_func_type = prev_decl;
    } else {
        visit(cst->tokens, bodyIndex);
        auto returnStmt = new ReturnStatement(value(), lambda, &lambda->scope, nullptr);
        lambda->scope.nodes.emplace_back(returnStmt);
    }
    lambda->scope.token = cst->tokens[bodyIndex];

    lambda->assign_params();

    for(auto& c : lambda->captureList) {
        c->lambda = lambda;
    }

    put_value(lambda, cst);
}

void CSTConverter::visitBody(CSTToken* bodyCst) {
    visit(bodyCst->tokens, 0);
}

void CSTConverter::visitMacro(CSTToken*  macroCst) {
    auto name = str_token(macroCst->tokens[0]);
    auto annon_name = name.substr(1);
    auto macro = MacroHandlers.find(annon_name);
    if (macro != MacroHandlers.end()) {
        macro->second(this, macroCst);
    } else {
        error("couldn't find annotation or macro handler for " + name, macroCst);
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
    put_node(new ValueNode(value(), parent_node, cst), cst);
}

void CSTConverter::visitIf(CSTToken* ifCst) {

    auto is_value  = ifCst->type() == LexTokenType::CompIfValue;

    // if condition
    ifCst->tokens[2]->accept(this);
    auto cond = value();

    auto if_statement = new IfStatement(
            std::move(cond),
            Scope {nullptr, nullptr},
            std::vector<std::pair<std::unique_ptr<Value>, Scope>>{},
            std::nullopt,
            parent_node,
            is_value,
            ifCst
    );

    auto prev_parent = parent_node;
    parent_node = if_statement;

    // first if body
    if_statement->ifBody.parent_node = if_statement;
    unsigned i = 4;
    if_statement->ifBody.nodes = take_body_or_single_stmt(this, ifCst, i, if_statement);
    i++; // position after body
    while ((i + 1) < ifCst->tokens.size() && is_keyword(ifCst->tokens[i + 1], "if")) {

        i += 3;
        ifCst->tokens[i]->accept(this);
        auto elseIfCond = value();
        i += 2;

        // else if body
        if_statement->elseIfs.emplace_back( std::move(elseIfCond), Scope { if_statement, ifCst->tokens[i] });
        auto& elseif_pair = if_statement->elseIfs.back();
        elseif_pair.second.nodes = take_body_or_single_stmt(this, ifCst, i, if_statement);

        // position after the body
        i++;

    }

    // last else
    if (i < ifCst->tokens.size() && str_token(ifCst->tokens[i]) == "else") {
        i++;
        if_statement->elseBody.emplace(if_statement, ifCst->tokens[i]);
        if_statement->elseBody->nodes = take_body_or_single_stmt(this, ifCst, i, if_statement);
    }

    parent_node = prev_parent;

    if(is_value) {
        put_value(if_statement, ifCst);
    } else {
        put_node(if_statement, ifCst);
    }

}

void CSTConverter::visitSwitch(CSTToken* switchCst) {
    bool is_value = switchCst->type() == LexTokenType::CompSwitchValue;
    auto switch_statement = new SwitchStatement(
            nullptr,
            std::vector<std::pair<std::unique_ptr<Value>, Scope>> {},
            std::nullopt,
            parent_node,
            is_value,
            switchCst
    );
    auto prev_parent = parent_node;
    parent_node = switch_statement;
    switchCst->tokens[2]->accept(this);
    switch_statement->expression = value();
    unsigned i = 5; // positioned at first 'case' or 'default'
    auto has_default = false;
    while (true) {
        if (is_keyword(switchCst->tokens[i], "default")) {
            i += 2; // body
            switch_statement->defScope.emplace(Scope { switch_statement, switchCst->tokens[i] });
            auto& defScope = switch_statement->defScope.value();
            defScope.nodes = take_body_or_single_stmt(this, switchCst, i, switch_statement);
            i++;
            if (has_default) {
                error("multiple defaults in switch statement detected", switchCst->tokens[i - 3]);
            }
            has_default = true;
        } else {
            if(is_keyword(switchCst->tokens[i], "case")) {
                i++;
            } else if(!switchCst->tokens[i]->is_value()) {
                break;
            }
            switchCst->tokens[i]->accept(this);
            auto caseVal = value();
            i += 2; // body
            switch_statement->scopes.emplace_back(std::move(caseVal), Scope { switch_statement, switchCst->tokens[i] });
            auto& switch_case = switch_statement->scopes.back();
            switch_case.second.nodes = take_body_or_single_stmt(this, switchCst, i, switch_statement);
            i++;
        }
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
    put_node(new ThrowStatement(value(), parent_node, throwStmt), throwStmt);
}

void CSTConverter::visitNamespace(CSTToken* ns) {
    auto spec = specifier_token(ns->tokens[0]);
    auto pNamespace = new Namespace(str_token(ns->tokens[spec.has_value() ? 2 : 1]), parent_node, ns, def_specifier(spec));
    pNamespace->nodes = take_comp_body_nodes(this, ns, pNamespace);
    put_node(pNamespace, ns);
}

void CSTConverter::visitForLoop(CSTToken* forLoop) {

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
                            std::unique_ptr<ASTNode>(assignment), LoopScope { nullptr, nullptr }, parent_node, forLoop);

    auto prevLoop = current_loop_node;
    current_loop_node = loop;
    auto& body = forLoop->tokens[8];
    loop->body.nodes = take_body_nodes(this, body, loop);
    loop->body.token = body;
    current_loop_node = prevLoop;

    put_node(loop, forLoop);

}

void CSTConverter::visitWhile(CSTToken* whileCst) {
    // visiting the condition expression
    whileCst->tokens[2]->accept(this);
    // get it
    auto cond = value();
    // construct a loop
    auto loop = new WhileLoop(std::move(cond), LoopScope{ nullptr, whileCst->tokens[4] }, parent_node, whileCst);
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
    auto loop = new DoWhileLoop(std::move(cond), LoopScope{nullptr, doWhileCst->tokens[1]});
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
                variables[node->name] = std::unique_ptr<StructMember>(node);
                break;
            }
            case LexTokenType::CompFunction:{
                const auto node = (FunctionDeclaration *) conv->pop_last_node();
                if(!mem_container->insert_multi_func(std::unique_ptr<FunctionDeclaration>(node))) {
                    conv->error("conflict inserting function with name " + node->name, token);
                }
                break;
            }
            case LexTokenType::CompStructDef:{
                const auto node = (UnnamedStruct*) conv->pop_last_node();
                variables[node->name] = std::unique_ptr<UnnamedStruct>(node);
                break;
            }
            case LexTokenType::CompUnionDef:{
                const auto node = (UnnamedUnion*) conv->pop_last_node();
                variables[node->name] = std::unique_ptr<UnnamedUnion>(node);
                break;
            }
            case LexTokenType::CompVariantMember:{
                const auto node = (VariantMember*) conv->pop_last_node();
                variables[node->name] = std::unique_ptr<BaseDefMember>(node);
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
    if(is_dispose()) {
        return;
    }
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
    AnnotableNode* def;
    if(named) {
        def = new StructDefinition(name_token->value(), parent_node, structDef, def_specifier(spec));
        if (has_override) {
            i++; // set on access specifier or the inherited struct / interface name
            get_inherit_list(this, structDef, i, ((StructDefinition*) def)->inherited);
        }
    } else {
        def = new UnnamedStruct(structDef->tokens[structDef->tokens.size() - 1]->value(), parent_node, structDef, spec.has_value() ? spec.value() : AccessSpecifier::Public);
    }
    i += 1;// positioned at first node or '}'
    if(is_generic) {
        convert_generic_list(this, gen_token, ((StructDefinition*) def)->generic_params, def);
    }
    auto prev_struct_decl = current_members_container;
    current_members_container = (StructDefinition*) def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, structDef, def->as_variables_container(), (MembersContainer*) def, i);
    parent_node = prev_parent;
    current_members_container = prev_struct_decl;
    collect_annotations_in(this, def);
    put_node(def, structDef);
}

void CSTConverter::visitInterface(CSTToken* interface) {
    if(is_dispose()) {
        return;
    }
    // private interface Point <T, K> {
    unsigned i = 1;
    auto specifier = specifier_token(interface->tokens[0]);
    if(specifier.has_value()) {
        i += 1;
    }
    auto def = new InterfaceDefinition(str_token(interface->tokens[i]), parent_node, interface, def_specifier(specifier));
    i += 1;
    const auto& gen_token = interface->tokens[i];
    if(gen_token->type() == LexTokenType::CompGenericParamsList) {
        convert_generic_list(this, gen_token, def->generic_params, def);
        i += 2; // positioned at first node or '}'
    } else {
        i += 1;
    }
    auto prev_container = current_members_container;
    current_members_container = def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, interface, def, def, i);
    parent_node = prev_parent;
    current_members_container = prev_container;
    put_node(def, interface);
}

void CSTConverter::visitUnionDef(CSTToken* unionDef) {
    if(is_dispose()) {
        return;
    }
    // private union Point <T, K> {
    unsigned i = 1;
    auto specifier = specifier_token(unionDef->tokens[0]);
    if(specifier.has_value()) {
        i += 1;
    }
    const auto& name_token = unionDef->tokens[i];
    bool named = name_token->is_identifier();
    if(named) {
        i += 2; // positioned at first node or '}'
    } else {
        i += 1; // positioned at first node or '}'
    }
    AnnotableNode* def;
    if(named) {
        def = new UnionDef(
            name_token->value(),
            parent_node,
            unionDef,
            def_specifier(specifier)
        );
    } else {
        def = new UnnamedUnion(
                str_token(unionDef->tokens[unionDef->tokens.size() - 1]),
                parent_node,
                unionDef,
                specifier.has_value() ? specifier.value() : AccessSpecifier::Public
        );
    }
    auto prev_container = current_members_container;
    current_members_container = (MembersContainer*) def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, unionDef, def->as_variables_container(), (MembersContainer*) def, i);
    parent_node = prev_parent;
    current_members_container = prev_container;
    put_node(def, unionDef);
}

void CSTConverter::visitImpl(CSTToken* impl) {
    if(is_dispose()) {
        return;
    }
    unsigned i = 1;
    auto def = new ImplDefinition(parent_node);
    if(impl->tokens[i]->type() == LexTokenType::CompGenericParamsList) {
        convert_generic_list(this, impl->tokens[i], def->generic_params, def);
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
}

void CSTConverter::visitVariantMember(CSTToken* variant_member) {
    const auto member = new VariantMember(str_token(variant_member->tokens[0]), (VariantDefinition*) parent_node, variant_member);
    auto result = function_params(this, variant_member->tokens, 2);
    for(auto& value : result.params) {
        member->values[value->name] = std::make_unique<VariantMemberParam>(value->name, value->index, std::move(value->type), value->defValue ? std::move(value->defValue) : nullptr, member, value->token);
    }
    put_node(member, variant_member);
}

void CSTConverter::visitVariant(CSTToken* variantDef) {
    unsigned i = 1;
    // private variant Optional <T, K> : Other {
    // don't even know if inheritance in variants is something valid
    // so just leaving the code here, if some day I decide to do it
    auto specifier = specifier_token(variantDef->tokens[0]);
    if(specifier.has_value()) {
        i += 1;
    }
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
    auto def = new VariantDefinition(
            str_token(named ? name_token : variantDef->tokens[variantDef->tokens.size() - 1]),
            parent_node,
            variantDef,
            def_specifier(specifier)
    );
    if (has_override) {
        i++; // set on access specifier or the inherited struct / interface name
        get_inherit_list(this, variantDef, i, def->inherited);
    }
    i += 1;// positioned at first node or '}'
    if(is_generic) {
        convert_generic_list(this, gen_token, def->generic_params, def);
    }
    auto prev_struct_decl = current_members_container;
    current_members_container = def;
    auto prev_parent = parent_node;
    parent_node = def;
    collect_struct_members(this, variantDef, def, def, i);
    parent_node = prev_parent;
    current_members_container = prev_struct_decl;
    collect_annotations_in(this, def);
    put_node(def, variantDef);
}

void CSTConverter::visitTryCatch(CSTToken* tryCatch) {
    auto chain = (CSTToken* ) tryCatch->tokens[1];
    if (chain->tokens.size() != 1 || chain->tokens[0]->type() != LexTokenType::CompFunctionCall) {
        error("expected a function call after try keyword", chain);
        return;
    }
    chain->accept(this);
    auto call = value().release()->as_access_chain();
    auto try_catch = new TryCatch(std::unique_ptr<FunctionCall>((FunctionCall *) call->values[0].release()), std::nullopt,
                                  std::nullopt, parent_node, tryCatch);
    auto last = tryCatch->tokens[tryCatch->tokens.size() - 1];
    if (is_keyword(tryCatch->tokens[2], "catch") && last->type() == LexTokenType::CompBody) {
        try_catch->catchScope.emplace(take_body(this, last, try_catch));
    }
    // TODO catch variable not supported yet
    put_node(try_catch, tryCatch);
}

void CSTConverter::visitPointerType(CSTToken* cst) {
    visit(cst->tokens, 0);
    put_type(new PointerType(type(), cst), cst);
}

void CSTConverter::visitReferenceType(CSTToken* cst) {
    visit(cst->tokens, 1);
    put_type(new ReferenceType(type(), cst), cst);
}

void CSTConverter::visitGenericType(CSTToken* cst) {
    const auto& base = str_token(cst->tokens[0]);
    if(base == "literal" && cst->tokens.size() == 4) {
        const auto id = cst->tokens[2];
        BaseType* child_type;
        if(id->value() == "string") {
            child_type = new StringType(id);
        } else {
            auto found = TypeMakers::PrimitiveMap.find(id->value());
            if (found != TypeMakers::PrimitiveMap.end()) {
                child_type = found->second(is64Bit, id);
            } else {
                error("couldn't find literal type by name " + id->value(), id);
                return;
            }
        }
        put_type(new LiteralType(std::unique_ptr<BaseType>(child_type), cst), cst);
        return;
    }
    auto generic_type = new GenericType(base, cst);
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

void CSTConverter::visitSpecializedType(CSTToken* specType) {
    // currently only one is supported dyn, which is a dynamic type
    specType->tokens[1]->accept(this);
    put_type(new DynamicType(type(), specType), specType);
}

void CSTConverter::visitArrayType(CSTToken* arrayType) {
    arrayType->tokens[0]->accept(this);
    std::unique_ptr<BaseType> elem_type = type();
    std::unique_ptr<Value> val = nullptr;
    if(arrayType->tokens[2]->is_value()) {
        arrayType->tokens[2]->accept(this);
        val = value();
    }
    auto arraySize = (val && val->value_type() == ValueType::Int) ? val->as_int() : -1;
    put_type(new ArrayType(std::move(elem_type), arraySize, arrayType), arrayType);
}

void CSTConverter::visitFunctionType(CSTToken* funcType) {
    bool is_capturing = is_char_op(funcType->tokens[0], '[');
    auto params = function_params(this, funcType->tokens, is_capturing ? 3 : 1);
    visit(funcType->tokens, params.index + 2);
    put_type(new FunctionType(std::move(params.params), type(), params.isVariadic, is_capturing, funcType), funcType);
}


void CSTConverter::visitStringToken(CSTToken* token) {
    auto escaped = escape_all(token->value(), 1, token->value().size() - 1, [this, token](const std::string& value, unsigned int index) {
        error("invalid escape sequence found, character '" + std::string(1, value[index]) + "'", token);
    });
    put_value(new StringValue(escaped, token), token);
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
    put_value(new CharValue(value, token), token);
}

void CSTConverter::visitNumberToken(NumberToken *token) {
    try {
        if (token->has_dot()) {
            if (token->is_float()) {
                std::string substring = token->value().substr(0, token->value().size() - 1);
                put_value(new FloatValue(std::stof(substring), token), token);
            } else {
                put_value(new DoubleValue(std::stod(token->value()), token), token);
            }
        } else {
            if(token->is_long()) {
                if(token->is_unsigned()) {
                    put_value(new ULongValue(std::stoul(token->value()), is64Bit, token), token);
                } else {
                    put_value(new LongValue(std::stol(token->value()), is64Bit, token), token);
                }
            } else {
                put_value(new NumberValue(std::stoll(token->value()), token), token);
            }
        }
    } catch (...) {
        error("invalid value provided", token);
    }
}

void CSTConverter::visitStructValue(CSTToken* cst) {
    cst->tokens[0]->accept(this);
    const auto has_generic_list = cst->tokens[1]->type() == LexTokenType::CompGenericList;
    auto struct_value = new StructValue(value(), {}, {}, nullptr, cst, parent_node);
    if(has_generic_list) {
        to_generic_arg_list(struct_value->generic_list, cst->tokens[1]);
    }
    auto i = has_generic_list ? 3 : 2; // first identifier or '}'
    while (!is_char_op(cst->tokens[i], '}')) {
        auto id = str_token(cst->tokens[i]);
        i += 2;
        cst->tokens[i]->accept(this);
        auto& member_ptr = struct_value->values[id]; // <--- automatic construction
        member_ptr = std::make_unique<StructMemberInitializer>(
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
    std::vector<std::unique_ptr<Value>> arrValues;
    while (char_op(arrayValue->tokens[i]) != '}') {
        if (char_op(arrayValue->tokens[i]) != ',') {
            arrayValue->tokens[i]->accept(this);
            // consume the value
            arrValues.emplace_back(value());
        }
        i++;
    }
    i++;
    std::unique_ptr<BaseType> arrType = nullptr;
    std::vector<unsigned int> sizes;
    if (i < arrayValue->tokens.size()) {
        if(arrayValue->tokens[i]->is_type()) {
            arrayValue->tokens[i]->accept(this);
            arrType = type();
        }
        i++;
        if (i < arrayValue->tokens.size() && char_op(arrayValue->tokens[i++]) == '(') {
            while (i < arrayValue->tokens.size() && char_op(arrayValue->tokens[i]) != ')') {
                if (char_op(arrayValue->tokens[i]) != ',') {
                    arrayValue->tokens[i]->accept(this);
                    // consume the value
                    sizes.emplace_back(value()->as_int());
                }
                i++;
            }
        }
    }
    put_value(new ArrayValue(std::move(arrValues), std::move(arrType), std::move(sizes), arrayValue), arrayValue);
}

std::vector<std::unique_ptr<Value>> take_values(CSTConverter *converter, const std::function<void()> &visit) {
    auto prev_values = std::move(converter->values);
    visit();
    auto new_values = std::move(converter->values);
    converter->values = std::move(prev_values);
    return new_values;
}

void CSTConverter::to_generic_arg_list(std::vector<std::unique_ptr<BaseType>>& generic_list, CSTToken*  container) {
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
    std::vector<std::unique_ptr<BaseType>> generic_list;
    if(call->tokens[0]->type() == LexTokenType::CompGenericList) {
        to_generic_arg_list(generic_list, call->tokens[0]);
    }
    auto prev_values = std::move(values);
    visit(call->tokens, 1);
    auto func_call = new FunctionCall(std::move(values), call);
    func_call->generic_list = std::move(generic_list);
    values = std::move(prev_values);
    put_value(func_call, call);
}

void CSTConverter::visitIndexOp(CSTToken* op) {
    auto indexes = take_values(this, [&op, this]() {
        visit(op->tokens, 1);
    });
    put_value(new IndexOperator(std::move(indexes), op), op);
}

void CSTConverter::visitLoopBlock(CSTToken *block) {

    auto is_value = block->type() == LexTokenType::CompLoopValue;

    auto loop_block = new LoopBlock(LoopScope { nullptr, block->tokens[1] }, parent_node, block);
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
        auto& token = chain->tokens[i];
        if(is_str_op(token, "::")) {
            auto prev = pop_last_value();
            auto as_id = prev->as_identifier();
            if(as_id) {
                put_value(new VariableIdentifier(as_id->value, token, true), token);
            } else {
                put_value(prev, token);
            }
        } else {
            token->accept(this);
        }
        i++;
    }
    const auto is_node = chain->type() == LexTokenType::CompAccessChainNode;
    auto ret_chain = new AccessChain(std::vector<std::unique_ptr<ChainValue>> {}, parent_node, is_node, chain);
    for(auto& value : values) {
        ret_chain->values.emplace_back((ChainValue*) value.release());
    }
    values = std::move(prev_values);
    if (is_node) {
        put_node(ret_chain, chain);
    } else {
        if(ret_chain->values.size() == 1) {
            put_value(ret_chain->values[0].release(), chain);
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
                op_stack.putValue(converter->value().release());
            } else {
                // - a number:
                // put it into the output queue
                expr->accept(converter);
                output.putValue(converter->value().release());
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
            auto o1 = get_operation(nested->tokens[op_index]);
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
    } else {
        throw std::runtime_error("unknown type of value provided to visitNestedExpr");
    }
}

void CSTConverter::visitExpression(CSTToken* expr) {
    auto is_braced = is_char_op(expr->tokens[0], '(');
    if(is_braced && is_char_op(expr->tokens[2], ')') && expr->tokens.size() <= 3) {
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
        put_value(new Expression(std::move(first), std::move(second),
                                                         (get_operation(expr->tokens[op_index])), is64Bit, expr), expr);
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
        put_value(output.toExpressionRaw(is64Bit, expr), expr);
    }
}

void CSTConverter::visitCast(CSTToken* castCst) {
    visit(castCst->tokens);
    put_value(new CastedValue(value(), type(), castCst), castCst);
}

void CSTConverter::visitIsValue(CSTToken* isCst) {
    visit(isCst->tokens);
    put_value( new IsValue(value(), type(), isCst->tokens[1]->value()[0] == '!', isCst), isCst);
}

void CSTConverter::visitAddrOf(CSTToken* addrOf) {
    addrOf->tokens[1]->accept(this);
    put_value(new AddrOfValue(value(), addrOf), addrOf);
}

void CSTConverter::visitDereference(CSTToken* deref) {
    deref->tokens[1]->accept(this);
    put_value(new DereferenceValue(value(), deref), deref);
}

void CSTConverter::visitVariableToken(CSTToken* token) {
    put_value(new VariableIdentifier(token->value(), token), token);
}

void CSTConverter::visitBoolToken(CSTToken* token) {
    put_value(new BoolValue(token->value()[0] == 't', token), token);
}

void CSTConverter::visitNullToken(CSTToken* token) {
    put_value(new NullValue(token), token);
}

void CSTConverter::visitNegative(CSTToken* neg) {
    visit(neg->tokens);
    put_value(new NegativeValue(value(), neg), neg);
}

void CSTConverter::visitNot(CSTToken* notCst) {
    visit(notCst->tokens);
    put_value(new NotValue(value(), notCst), notCst);
}

CSTConverter::~CSTConverter() = default;