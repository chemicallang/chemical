// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include <unordered_set>
#include "cst/base/CSTToken.h"
#include "cst/utils/CSTUtils.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Assignment.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/If.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/DoWhileLoop.h"
#include "integration/cbi/model/LexImportUnit.h"
#include "integration/cbi/model/ASTImportUnitRef.h"
#include "integration/cbi/model/ASTResult.h"
#include "integration/cbi/model/LexResult.h"
#include "Documentation.h"

#define DEBUG_COMPLETION true

void CompletionItemAnalyzer::put(const std::string &label, lsCompletionItemKind kind) {
    list.items.emplace_back(label, kind);
}

void CompletionItemAnalyzer::put_with_md_doc(const std::string& label, lsCompletionItemKind kind, const std::string& detail, const std::string& doc) {
    list.items.emplace_back(label, kind, detail, std::pair(std::nullopt, MarkupContent{ "markdown", doc }));
}

void put_with_doc(CompletionItemAnalyzer* analyzer, const std::string& label, lsCompletionItemKind kind, ASTNode* linked_node) {
    std::string doc;
    markdown_documentation(analyzer->loc_man, doc, analyzer->current_file, linked_node);
    std::string detail;
    small_detail_of(detail, linked_node);
    analyzer->put_with_md_doc(label, kind, detail, doc);
}

inline void put_var_init(CompletionItemAnalyzer* analyzer, VarInitStatement* node) {
    put_with_doc(analyzer, node->identifier(), lsCompletionItemKind::Variable, node);
}

inline void put_function(CompletionItemAnalyzer* analyzer, FunctionDeclaration* node) {
    put_with_doc(analyzer, node->name(), lsCompletionItemKind::Function, node);
}

inline void put_func_param(CompletionItemAnalyzer* analyzer, BaseFunctionParam* node) {
    put_with_doc(analyzer, node->name, lsCompletionItemKind::Variable, node);
}

inline void put_enum_decl(CompletionItemAnalyzer* analyzer, EnumDeclaration* decl) {
    put_with_doc(analyzer, decl->name(), lsCompletionItemKind::Enum, decl);
}

inline void put_interface_decl(CompletionItemAnalyzer* analyzer, InterfaceDefinition* def) {
    put_with_doc(analyzer, def->name(), lsCompletionItemKind::Interface, def);
}

inline void put_struct_decl(CompletionItemAnalyzer* analyzer, StructDefinition* def) {
    put_with_doc(analyzer, def->name(), lsCompletionItemKind::Struct, def);
}

inline void put_union_decl(CompletionItemAnalyzer* analyzer, UnionDef* def) {
    put_with_doc(analyzer, def->name(), lsCompletionItemKind::Struct, def);
}

inline void put_namespace_decl(CompletionItemAnalyzer* analyzer, Namespace* ns) {
    put_with_doc(analyzer, ns->name, lsCompletionItemKind::Module, ns);
}

inline void put_variant_decl(CompletionItemAnalyzer* analyzer, VariantDefinition* def) {
    put_with_doc(analyzer, def->name(), lsCompletionItemKind::Struct, def);
}

lsCompletionItemKind toCompletionItemKind(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::StructDecl:
            return lsCompletionItemKind::Struct;
        case ASTNodeKind::UnionDecl:
            return lsCompletionItemKind::Struct;
        case ASTNodeKind::InterfaceDecl:
            return lsCompletionItemKind::Interface;
        case ASTNodeKind::FunctionDecl:
            return lsCompletionItemKind::Function;
        case ASTNodeKind::NamespaceDecl:
            return lsCompletionItemKind::Module;
        case ASTNodeKind::TypealiasStmt:
            return lsCompletionItemKind::Interface;
        case ASTNodeKind::VariantDecl:
            return lsCompletionItemKind::Struct;
        case ASTNodeKind::EnumDecl:
            return lsCompletionItemKind::Enum;
        case ASTNodeKind::VarInitStmt:
            return lsCompletionItemKind::Variable;
        default:
#ifdef DEBUG
            throw std::runtime_error("unknown node completion item kind");
#endif
            return lsCompletionItemKind::Folder;
    }
}

lsCompletionItemKind toCompletionItemKind(BaseType* type) {
    const auto linked = type->linked_node();
    if(linked) {
        return toCompletionItemKind(linked);
    } else {
        return lsCompletionItemKind::Variable;
    }
}

bool put_node(CompletionItemAnalyzer* analyzer, ASTNode* node) {
    const auto kind = node->kind();
    switch(kind) {
        case ASTNodeKind::StructDecl:
            put_struct_decl(analyzer, node->as_struct_def_unsafe());
            return true;
        case ASTNodeKind::VarInitStmt:
            put_var_init(analyzer, node->as_var_init_unsafe());
            return true;
        case ASTNodeKind::FunctionParam:
        case ASTNodeKind::ExtensionFuncReceiver:
            put_func_param(analyzer, node->as_base_func_param_unsafe());
            return true;
        case ASTNodeKind::UnionDecl:
            put_union_decl(analyzer, node->as_union_def_unsafe());
            return true;
        case ASTNodeKind::InterfaceDecl:
            put_interface_decl(analyzer, node->as_interface_def_unsafe());
            return true;
        case ASTNodeKind::FunctionDecl:
            put_function(analyzer, node->as_function_unsafe());
            return true;
        case ASTNodeKind::NamespaceDecl:
            put_namespace_decl(analyzer, node->as_namespace_unsafe());
            return true;
        case ASTNodeKind::TypealiasStmt:
            put_with_doc(analyzer, node->as_typealias_unsafe()->name_view(), lsCompletionItemKind::Interface, node);
            return true;
        case ASTNodeKind::VariantDecl:
            put_variant_decl(analyzer, node->as_variant_def_unsafe());
            return true;
        case ASTNodeKind::EnumDecl:
            put_enum_decl(analyzer, node->as_enum_decl_unsafe());
            return true;
        case ASTNodeKind::UsingStmt:
            // TODO we must provide completions for all under using statement
            return true;
        default:
            return false;
    }
}

void CompletionItemAnalyzer::visit(VarInitStatement *init) {
    put_var_init(this, init);
    if(init->type && is_caret_inside(init->type->encoded_location())) {
        init->type->accept(this);
    } else if(init->value && is_caret_inside(init->value->encoded_location())) {
        init->value->accept(this);
    }
}

void CompletionItemAnalyzer::visit(AssignStatement *assign) {
    if(is_caret_inside(assign->lhs->encoded_location())) {
        assign->lhs->accept(this);
    }
    if(is_caret_inside(assign->value->encoded_location())) {
        assign->value->accept(this);
    }
}

void CompletionItemAnalyzer::visit(FunctionDeclaration *decl) {
    put_function(this, decl);
    if(decl->body.has_value() && is_caret_inside(decl->body->encoded_location())) {
        for(auto& param : decl->params) {
            put_func_param(this, param);
        }
        decl->body.value().accept(this);
    }
}

void CompletionItemAnalyzer::visit(EnumDeclaration *decl) {
    put_enum_decl(this, decl);
    // caret is inside the enum, what should we have
}

void visit_inside(CompletionItemAnalyzer* analyzer, MembersContainer* container) {
    for(auto& var : container->variables) {
        if(analyzer->is_caret_inside(var.second->encoded_location())) {
            var.second->accept(analyzer);
            return;
        }
    }
    for(auto func : container->functions()) {
        if(analyzer->is_caret_inside(func->encoded_location())) {
            func->accept(analyzer);
            return;
        }
    }
}

void CompletionItemAnalyzer::visit(StructDefinition *def) {
    put_struct_decl(this, def);
    visit_inside(this, def);
}

void CompletionItemAnalyzer::visit(InterfaceDefinition *def) {
    put_interface_decl(this, def);
    visit_inside(this, def);
}

void CompletionItemAnalyzer::visit(ImplDefinition *def) {
    visit_inside(this, def);
}

void CompletionItemAnalyzer::visit(IfStatement *stmt) {
    if(is_caret_inside(stmt->ifBody.encoded_location())) {
        stmt->ifBody.accept(this);
    } else {
        bool found = false;
        for(auto& elseIf : stmt->elseIfs) {
            if(is_caret_inside(elseIf.second.encoded_location())) {
                elseIf.second.accept(this);
                found = true;
                break;
            }
        }
        if(!found && stmt->elseBody.has_value() && is_caret_inside(stmt->elseBody.value().encoded_location())) {
            stmt->elseBody.value().accept(this);
        }
    }
}

void CompletionItemAnalyzer::visit(WhileLoop *loop) {
    if(is_caret_inside(loop->body.encoded_location())) {
        loop->body.accept(this);
    }
}

void CompletionItemAnalyzer::visit(DoWhileLoop *loop) {
    if(is_caret_inside(loop->body.encoded_location())) {
        loop->body.accept(this);
    }
}

void CompletionItemAnalyzer::visit(ForLoop *loop) {
    if(is_caret_inside(loop->body.encoded_location())) {
        loop->body.accept(this);
    }
}

void CompletionItemAnalyzer::visit(SwitchStatement *stmt) {
    for(auto& caseBody : stmt->scopes) {
        if(is_caret_inside(caseBody.encoded_location())) {
            caseBody.accept(this);
        }
    }
}

void CompletionItemAnalyzer::visit(LambdaFunction *func) {
    if(is_caret_inside(func->scope.encoded_location())) {
        for(auto& param : func->params) {
            put_func_param(this, param);
        }
        func->scope.accept(this);
    }
}

void CompletionItemAnalyzer::visit(StructValue *structValue) {
    for(auto& val : structValue->values) {
        auto value = val.second->value;
        if(is_caret_inside(value->encoded_location())) {
            value->accept(this);
        }
    }
}

void CompletionItemAnalyzer::visit(ArrayValue *arrayValue) {
    for(auto value : arrayValue->values) {
        if(is_caret_inside(value->encoded_location())) {
            value->accept(this);
        }
    }
}

void CompletionItemAnalyzer::visit(Scope *scope) {
    for(const auto node : scope->nodes) {
        const auto sourceLoc = node->encoded_location();
        const auto position = loc_man.getLocationPos(sourceLoc);
        if(is_caret_ahead(position.start)) {
            put_node(this, node);
        } else if(is_caret_inside(position.start, position.end)) {
            node->accept(this);
        }
    }
}

void put_variables_of(CompletionItemAnalyzer* analyzer, VariablesContainer* node) {
    for(auto& var : node->variables) {
        put_with_doc(analyzer, var.first, lsCompletionItemKind::Field, var.second);
    }
}

void put_functions_of(CompletionItemAnalyzer* analyzer, ExtendableMembersContainerNode* node) {
    for(auto& func : node->functions()) {
        put_with_doc(analyzer, func->name(), lsCompletionItemKind::Function, func);
    }
}

void put_non_self_param_functions_of(CompletionItemAnalyzer* analyzer, ExtendableMembersContainerNode* node) {
    for(auto& func : node->functions()) {
        if(!func->has_self_param()) {
            put_with_doc(analyzer, func->name(), lsCompletionItemKind::Function, func);
        }
    }
}

bool put_children_of(CompletionItemAnalyzer* analyzer, BaseType* type, bool has_self);

bool put_children_of(CompletionItemAnalyzer* analyzer, ASTNode* linked_node, bool has_self) {
    const auto linked_kind = linked_node->kind();
    switch(linked_kind) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::UnionDecl: {
            const auto node = linked_node->as_extendable_members_container_node();
            if(has_self) {
                put_functions_of(analyzer, node);
            } else {
                put_non_self_param_functions_of(analyzer, node);
            }
            return true;
        }
        case ASTNodeKind::UnnamedUnion:
            put_variables_of(analyzer, linked_node->as_unnamed_union_unsafe());
            return true;
        case ASTNodeKind::UnnamedStruct:
            put_variables_of(analyzer, linked_node->as_unnamed_struct_unsafe());
            return true;
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::InterfaceDecl: {
            const auto node = linked_node->as_extendable_members_container_node();
            if(has_self) {
                put_functions_of(analyzer, node);
            } else {
                put_non_self_param_functions_of(analyzer, node);
            }
            return true;
        }
        case ASTNodeKind::VarInitStmt: {
            auto& init = *linked_node->as_var_init_unsafe();
            const auto type = init.known_type();
            if(type) {
                put_children_of(analyzer, type, true);
            } else {
                // TODO create type with allocator
            }
            return true;
        }
        case ASTNodeKind::FunctionParam:
        case ASTNodeKind::ExtensionFuncReceiver: {
            auto& param = *linked_node->as_base_func_param_unsafe();
            const auto type = param.known_type();
            return put_children_of(analyzer, type, true);
        }
        case ASTNodeKind::TypealiasStmt: {
            auto& alias = *linked_node->as_typealias_unsafe();
            return put_children_of(analyzer, alias.actual_type, has_self);
        }
        case ASTNodeKind::EnumDecl:{
            const auto decl = linked_node->as_enum_decl_unsafe();
            for(auto& mem : decl->members) {
                analyzer->put(mem.first, lsCompletionItemKind::EnumMember);
            }
            return true;
        }
        default:
            return false;
    }
}

bool put_children_of(CompletionItemAnalyzer* analyzer, BaseType* type, bool has_self) {
    const auto linked = type->linked_node();
    if(linked) {
        return put_children_of(analyzer, linked, has_self);
    }
    return false;
}

bool put_children_of_ref(CompletionItemAnalyzer* analyzer, CSTToken* chain) {
    auto parent = chain->tokens[chain->tokens.size() - 2];
    const auto ref_any = parent->any;
    if(ref_any) {
        const auto linked_node = ref_any->get_ref_linked_node();
        if(linked_node) {
            return put_children_of(analyzer, linked_node, false);
        }
    }
#ifdef DEBUG
    else {
        std::cout << "chain doesn't have a ref any " << chain->representation() << std::endl;
    }
#endif
    return false;
}

bool CompletionItemAnalyzer::handle_chain_before_caret(CSTToken* chain) {
    if(!chain->tokens.empty() && is_char_op(chain->tokens[chain->tokens.size() - 1], '.')) {
        return put_children_of_ref(this, chain);
    }
    return false;
}

void CompletionItemAnalyzer::analyze(ASTImportUnitRef& unit) {

    auto lex_files = unit.lex_unit.files;
    const auto lex_files_size = lex_files.size();

    if(lex_files_size == 0) {
        return;
    }

    const auto last_lex_file = lex_files[lex_files.size() - 1];

    const auto unit_files_size = (int) unit.files.size();
    auto& last_file = unit.files[unit_files_size - 1];

    // check is caret position before a chain
    current_file = last_lex_file.get();
    auto chain = chain_before_caret(last_lex_file->unit.tokens);
    if(chain) {
        if(handle_chain_before_caret(chain)) {
            return;
        } else {
            std::cout << "[Unknown] member access into access chain : " + chain->type_string() << std::endl;
        }
    }

    // add completions for the last file by analyzing it in reverse fashion
    // only nodes in which caret is inside are visited using this visitor
    current_file = last_lex_file.get();
    auto& last_file_nodes = last_file->unit.scope.nodes;
    int i = ((int) last_file_nodes.size()) - 1;
    while(i >= 0) {
        const auto node = last_file_nodes[i];
        const auto sourceLoc = node->encoded_location();
        const auto location = loc_man.getLocationPos(sourceLoc);
        if(is_caret_ahead(location.start)) {
            put_node(this, node);
        } else if(is_caret_inside(location.start, location.end)) {
            node->accept(this);
        }
        i--;
    }

    // add completions for other files in reverse fashion (no analyzing)
    i = unit_files_size - 2;
    while(i >= 0) {
        auto& file = unit.files[i];
        auto& file_nodes = file->unit.scope.nodes;
        if(i < lex_files_size) {
            current_file = lex_files[i].get();
        } else {
            current_file = nullptr;
        }
        for(const auto node : file_nodes) {
            put_node(this, node);
        }
        i--;
    }

}