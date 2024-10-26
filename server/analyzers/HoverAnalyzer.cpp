// Copyright (c) Qinetik 2024.

#include "HoverAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/cbi/model/LexImportUnit.h"
#include "integration/cbi/model/LexResult.h"
#include "ast/base/ASTNode.h"
#include <filesystem>
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "Documentation.h"
#include "cst/LocationManager.h"

HoverAnalyzer::HoverAnalyzer(LocationManager& locMan, Position position) : loc_man(locMan), position(position) {

}

struct FuncParam {
    std::string name;
    CSTToken *type;
};

struct FuncSignature {
    // function params, name's and type tokens
    std::vector<FuncParam> params;
    // if function signature has a given return type (user wrote it)
    CSTToken *returnType;
};

FuncSignature get_signature(CSTToken* func) {
    std::vector<FuncParam> params;
    unsigned i = 2;
    CSTToken *token;
    while (i < func->tokens.size()) {
        token = func->tokens[i];
        if (token->type() == LexTokenType::CompFunctionParam) {
            params.emplace_back(
                    param_name(token->as_compound()),
                    2 < token->as_compound()->tokens.size() ? token->as_compound()->tokens[2] : nullptr
            );
        } else if (is_char_op(token, ')')) {
            break;
        }
        i++;
    }
    CSTToken *ret = nullptr;
    if (is_char_op(func->tokens[i + 1], ':')) {
        ret = func->tokens[i + 2];
    }
    return {std::move(params), ret};
}


void struct_def_inheritance_doc(std::string& value, StructDefinition* def) {
    if(!def->inherited.empty()) {
        value += " : ";
        bool has_comma = true;
        for(auto& inherit : def->inherited) {
            if(!has_comma) {
                value += ", ";
            }
            switch(inherit->specifier) {
                case AccessSpecifier::Private:
                    value += "private";
                    break;
                case AccessSpecifier::Protected:
                    value += "protected";
                    break;
                default:
                    break;
            }
            value += inherit->type->representation();
            has_comma = false;
        }
    }
}

void small_detail_of(std::string& value, ASTNode* linked) {
    switch (linked->kind()) {
        case ASTNodeKind::FunctionDecl:
        case ASTNodeKind::ExtensionFunctionDecl: {
            auto& signature = *linked->as_function_unsafe();
            value += "(";
            unsigned i = 0;
            while (i < signature.params.size()) {
                auto& param = *signature.params[i];
                if (param.type) {
                    value += param.name;
                    value += " : ";
                    value += param.type->representation();
                } else if (param.name == "self") {
                    value += "&self";
                }
                if (i != signature.params.size() - 1) {
                    value += ", ";
                }
                i++;
            }
            value += ')';
            if (signature.returnType) {
                value += " : ";
                value += signature.returnType->representation();
            }
            break;
        }
        case ASTNodeKind::EnumDecl:
            value += "enum";
            break;
        case ASTNodeKind::StructDecl: {
            value += "struct";
            value += ' ';
            const auto struct_def = linked->as_struct_def_unsafe();
            value += struct_def->name;
            struct_def_inheritance_doc(value, struct_def);
            break;
        }
        case ASTNodeKind::UnionDecl:
            value += "union";
            break;
        case ASTNodeKind::UnnamedStruct:
            value += "struct";
            break;
        case ASTNodeKind::UnnamedUnion:
            value += "union";
            break;
        case ASTNodeKind::InterfaceDecl:
            value += "interface";
            break;
        case ASTNodeKind::VarInitStmt:{
            const auto init = linked->as_var_init_unsafe();
            if(init->is_const) {
                value += "const";
            } else {
                value += "var";
            }
            value += ' ';
            if (init->type) {
                value += init->type->representation();
            }
            break;
        }
        case ASTNodeKind::TypealiasStmt: {
            const auto alias = linked->as_typealias_unsafe();
            if(alias->actual_type) {
                value += alias->actual_type->representation();
            }
            break;
        }
        default:
            break;
    }
}

void small_detail_of_old(std::string& value, CSTToken* linked) {
    switch (linked->type()) {
        case LexTokenType::CompFunction: {
            auto signature = get_signature(linked->as_compound());
            value += "(";
            unsigned i = 0;
            FuncParam *param;
            while (i < signature.params.size()) {
                param = &signature.params[i];
                if (param->type) {
                    value += param->name;
                    value += " : ";
                    param->type->append_representation(value);
                } else if (param->name == "self") {
                    value += "&self";
                }
                if (i != signature.params.size() - 1) {
                    value += ", ";
                }
                i++;
            }
            value += ')';
            if (signature.returnType) {
                value += " : ";
                signature.returnType->append_representation(value);
            }
            break;
        }
        case LexTokenType::CompEnumDecl:
            value += "enum ";
            break;
        case LexTokenType::CompStructDef:
            value += "struct ";
            break;
        case LexTokenType::CompInterface:
            value += "interface ";
            break;
        case LexTokenType::CompVarInit:
            if (is_char_op(linked->as_compound()->tokens[2], ':')) {
                linked->as_compound()->tokens[3]->append_representation(value);
            } else {
                // TODO get type by value
            }
            break;
        case LexTokenType::CompTypealias:
            value += "type = ";
            linked->as_compound()->tokens[3]->append_representation(value);
            break;
        default:
            break;
    }
}

void markdown_documentation(LocationManager& loc_man, std::string& value, LexResult* current, ASTNode* linked_node) {
    const auto linked_kind = linked_node->kind();
    const auto parent = linked_node->parent();
    bool parent_handled = false;
    const auto encoded = linked_node->encoded_location();
    if(encoded.isValid()) {
        const auto locData = loc_man.getLocationPos(encoded);
        auto definedPath = loc_man.getPathForFileId(locData.fileId);
        auto relative_path = std::filesystem::relative(
                std::filesystem::path(definedPath),
                std::filesystem::path(current->abs_path).parent_path()
        );
        value += "**Defined in** : " + relative_path.string() + "\n";
    }
    if(linked_kind == ASTNodeKind::EnumMember && parent) {
        const auto member = linked_node->as_enum_member_unsafe();
        const auto parent_kind = parent->kind();
        if(parent_kind != ASTNodeKind::EnumDecl) {
            const auto enumDecl = parent->as_enum_decl_unsafe();
            value += "```typescript\n";
            value += "enum " + enumDecl->name;
            value += "\n```\n";
            value += "```typescript\n";
            value += enumDecl->name;
            value += ".";
            value += member->name;
            value += "\n```";
            parent_handled = true;
        }
    }
    switch (linked_kind) {
        case ASTNodeKind::FunctionDecl:
        case ASTNodeKind::ExtensionFunctionDecl: {
            const auto& func_decl = *linked_node->as_function_unsafe();
            value += "```typescript\n";
            value += "func " + func_decl.name;
            const auto& signature = func_decl;
            value += '(';
            unsigned i = 0;
            while (i < signature.params.size()) {
                const auto param = signature.params[i];
                if (param->type) {
                    value += param->name;
                    value += " : ";
                    value += param->type->representation();
                } else if (param->name == "self") {
                    value += "&self";
                }
                if (i != signature.params.size() - 1) {
                    value += ", ";
                }
                i++;
            }
            value += ')';
            if (signature.returnType) {
                value += " : ";
                value += signature.returnType->representation();
            }
            value += "\n```";
            break;
        }
        case ASTNodeKind::EnumDecl: {
            const auto enumDecl = linked_node->as_enum_decl_unsafe();
            value += "```typescript\n";
            value += "enum " + enumDecl->name;
            value += "\n```";
            break;
        }
        case ASTNodeKind::StructDecl: {
            const auto structDecl = linked_node->as_struct_def_unsafe();
            value += "```c\n";
            value += "struct " + structDecl->name;
            struct_def_inheritance_doc(value, structDecl);
            value += "\n```";
            break;
        }
        case ASTNodeKind::InterfaceDecl: {
            const auto& interface = *linked_node->as_interface_def_unsafe();
            value += "```typescript\n";
            value += "interface " + interface.name;
            value += "\n```";
            break;
        }
        case ASTNodeKind::VarInitStmt: {
            auto& init = *linked_node->as_var_init_unsafe();
            value += "```typescript\n";
            if(init.is_const) {
                value += "const";
            } else {
                value += "var";
            }
            value += ' ';
            value += init.identifier;
            const auto type = init.known_type();
            if (type) {
                value += " : ";
                value += type->representation();
            } else {
                // TODO create type using allocator
            }
            value += "\n```";
            break;
        }
        case ASTNodeKind::FunctionParam: {
            auto& linked = *linked_node->as_func_param_unsafe();
            value += "```typescript\n";
            value += "const";
            value += ' ';
            value += linked.name;
            value += " : ";
            value += linked.type->representation();
            value += "\n```";
            value += "\nfunction parameter";
            break;
        }
        case ASTNodeKind::TypealiasStmt: {
            auto& linked = *linked_node->as_typealias_unsafe();
            value += "```typescript\n";
            value += "typealias " + linked.identifier;
            value += " = ";
            value += linked.actual_type->representation();
            value += "\n```";
            break;
        }
        default:
            if (!parent_handled) {
                value += "don't know what it is";
            }
            break;
    }
}

std::string HoverAnalyzer::markdown_hover(LexImportUnit *unit) {
    auto file = unit->files[unit->files.size() - 1];
    auto token = get_token_at_position(file->unit.tokens, position);
    if (token) {
        if (token->is_ref() && token->any) {
            auto ref_linked = token->any->get_ref_linked_node();
            if (ref_linked) {
                const auto location = ref_linked->encoded_location();
                if(location.isValid()) {
                    // parent.second.first is the parent token of the linked token
                    markdown_documentation(loc_man, value, file.get(), ref_linked);
                } else {
                    value += "couldn't get the linked token";
                }
            } else {
                value += "has no linked declaration node";
            }
        }
        else {
            value += "hovering an unknown reference token";
        }
    } else {
        value += "couldn't find the token at position";
    }
    return std::move(value);
}