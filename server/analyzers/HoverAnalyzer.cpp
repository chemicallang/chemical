// Copyright (c) Chemical Language Foundation 2025.

#include "HoverAnalyzer.h"
#include "compiler/cbi/model/LexImportUnit.h"
#include "compiler/cbi/model/LexResult.h"
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


void struct_def_inheritance_doc(std::string& value, StructDefinition* def) {
    if(!def->inherited.empty()) {
        value += " : ";
        bool has_comma = true;
        for(auto& inherit : def->inherited) {
            if(!has_comma) {
                value += ", ";
            }
            switch(inherit.specifier) {
                case AccessSpecifier::Private:
                    value += "private";
                    break;
                case AccessSpecifier::Protected:
                    value += "protected";
                    break;
                default:
                    break;
            }
            value += inherit.type->representation();
            has_comma = false;
        }
    }
}

void small_detail_of(std::string& value, ASTNode* linked) {
    switch (linked->kind()) {
        case ASTNodeKind::FunctionDecl: {
            auto& signature = *linked->as_function_unsafe();
            value += "(";
            unsigned i = 0;
            while (i < signature.params.size()) {
                auto& param = *signature.params[i];
                if (param.type) {
                    value.append(param.name.view());
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
            value.append(struct_def->name_view().view());
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
            if(init->is_const()) {
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

void markdown_documentation(LocationManager& loc_man, std::string& value, const std::string_view& curr_file, ASTNode* linked_node) {
    const auto linked_kind = linked_node->kind();
    const auto parent = linked_node->parent();
    bool parent_handled = false;
    const auto encoded = linked_node->encoded_location();
    if(encoded.isValid()) {
        const auto locData = loc_man.getLocationPos(encoded);
        auto definedPath = loc_man.getPathForFileId(locData.fileId);
        auto relative_path = std::filesystem::relative(
                std::filesystem::path(definedPath),
                std::filesystem::path(curr_file).parent_path()
        );
        value += "**Defined in** : " + relative_path.string() + "\n";
    }
    if(linked_kind == ASTNodeKind::EnumMember && parent) {
        const auto member = linked_node->as_enum_member_unsafe();
        const auto parent_kind = parent->kind();
        if(parent_kind != ASTNodeKind::EnumDecl) {
            const auto enumDecl = parent->as_enum_decl_unsafe();
            value += "```typescript\n";
            value += "enum ";
            value.append(enumDecl->name_view().view());
            value += "\n```\n";
            value += "```typescript\n";
            value.append(enumDecl->name_view().view());
            value += ".";
            value.append(member->name.view());
            value += "\n```";
            parent_handled = true;
        }
    }
    switch (linked_kind) {
        case ASTNodeKind::FunctionDecl: {
            const auto& func_decl = *linked_node->as_function_unsafe();
            value += "```typescript\n";
            value += "func ";
            value.append(func_decl.name_view().view());
            const auto& signature = func_decl;
            value += '(';
            unsigned i = 0;
            while (i < signature.params.size()) {
                const auto param = signature.params[i];
                if (param->type) {
                    value.append(param->name.view());
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
            value += "enum ";
            value.append(enumDecl->name_view().view());
            value += "\n```";
            break;
        }
        case ASTNodeKind::StructDecl: {
            const auto structDecl = linked_node->as_struct_def_unsafe();
            value += "```c\n";
            value += "struct ";
            value.append(structDecl->name_view().view());
            struct_def_inheritance_doc(value, structDecl);
            value += "\n```";
            break;
        }
        case ASTNodeKind::InterfaceDecl: {
            const auto& interface = *linked_node->as_interface_def_unsafe();
            value += "```typescript\n";
            value += "interface ";
            value.append(interface.name_view().view());
            value += "\n```";
            break;
        }
        case ASTNodeKind::VarInitStmt: {
            auto& init = *linked_node->as_var_init_unsafe();
            value += "```typescript\n";
            if(init.is_const()) {
                value += "const";
            } else {
                value += "var";
            }
            value += ' ';
            value.append(init.name_view().view());
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
            value.append(linked.name_view().view());
            value += " : ";
            value += linked.type->representation();
            value += "\n```";
            value += "\nfunction parameter";
            break;
        }
        case ASTNodeKind::TypealiasStmt: {
            auto& linked = *linked_node->as_typealias_unsafe();
            value += "```typescript\n";
            value += "typealias ";
            value.append(linked.name_view().view());
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

std::string HoverAnalyzer::markdown_hover(LexResult* file) {
//    auto token = get_token_at_position(file->tokens, position);
// TODO we must get the token at position
    Token* token = nullptr;
    if (token) {
        // TODO we must get the any ptr for token
        ASTAny* anyPtr = nullptr;
        if (anyPtr) {
            auto ref_linked = anyPtr->get_ref_linked_node();
            if (ref_linked) {
                const auto location = ref_linked->encoded_location();
                if(location.isValid()) {
                    // parent.second.first is the parent token of the linked token
                    markdown_documentation(loc_man, value, file->abs_path, ref_linked);
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