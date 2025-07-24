// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include <unordered_set>
#include "compiler/lab/LabModule.h"
#include "compiler/processor/ASTFileResult.h"
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
#include "server/model/ASTResult.h"
#include "server/model/LexResult.h"
#include "server/model/ModuleData.h"
#include "core/source/LocationManager.h"
#include "Documentation.h"

#define DEBUG_COMPLETION true

void CompletionItemAnalyzer::put(const chem::string_view &label, lsp::CompletionItemKind kind) {
    list.items.emplace_back(lsp::CompletionItem{label.str(), std::nullopt, kind});
}

void CompletionItemAnalyzer::put_with_md_doc(const chem::string_view& label, lsp::CompletionItemKind kind, const std::string& detail, const std::string& doc) {
    list.items.emplace_back(lsp::CompletionItem{label.str(), std::nullopt, kind, {}, detail, lsp::MarkupContent{lsp::MarkupKind::Markdown, doc}});
}

void put_with_doc(CompletionItemAnalyzer* analyzer, const chem::string_view& label, lsp::CompletionItemKind kind, ASTNode* linked_node) {
    std::string doc;
    markdown_documentation(analyzer->loc_man, doc, analyzer->current_file, linked_node);
    std::string detail;
    small_detail_of(detail, linked_node);
    analyzer->put_with_md_doc(label, kind, detail, doc);
}

inline void put_var_init(CompletionItemAnalyzer* analyzer, VarInitStatement* node) {
    put_with_doc(analyzer, node->name_view(), lsp::CompletionItemKind::Variable, node);
}

inline void put_function(CompletionItemAnalyzer* analyzer, FunctionDeclaration* node) {
    put_with_doc(analyzer, node->name_view(), lsp::CompletionItemKind::Function, node);
}

inline void put_func_param(CompletionItemAnalyzer* analyzer, FunctionParam* node) {
    put_with_doc(analyzer, node->name, lsp::CompletionItemKind::Variable, node);
}

inline void put_enum_decl(CompletionItemAnalyzer* analyzer, EnumDeclaration* decl) {
    put_with_doc(analyzer, decl->name_view(), lsp::CompletionItemKind::Enum, decl);
}

inline void put_interface_decl(CompletionItemAnalyzer* analyzer, InterfaceDefinition* def) {
    put_with_doc(analyzer, def->name_view(), lsp::CompletionItemKind::Interface, def);
}

inline void put_struct_decl(CompletionItemAnalyzer* analyzer, StructDefinition* def) {
    put_with_doc(analyzer, def->name_view(), lsp::CompletionItemKind::Struct, def);
}

inline void put_union_decl(CompletionItemAnalyzer* analyzer, UnionDef* def) {
    put_with_doc(analyzer, def->name_view(), lsp::CompletionItemKind::Struct, def);
}

inline void put_namespace_decl(CompletionItemAnalyzer* analyzer, Namespace* ns) {
    put_with_doc(analyzer, ns->name_view(), lsp::CompletionItemKind::Module, ns);
}

inline void put_variant_decl(CompletionItemAnalyzer* analyzer, VariantDefinition* def) {
    put_with_doc(analyzer, def->name_view(), lsp::CompletionItemKind::Struct, def);
}

lsp::CompletionItemKind toCompletionItemKind(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::StructDecl:
            return lsp::CompletionItemKind::Struct;
        case ASTNodeKind::UnionDecl:
            return lsp::CompletionItemKind::Struct;
        case ASTNodeKind::InterfaceDecl:
            return lsp::CompletionItemKind::Interface;
        case ASTNodeKind::FunctionDecl:
            return lsp::CompletionItemKind::Function;
        case ASTNodeKind::NamespaceDecl:
            return lsp::CompletionItemKind::Module;
        case ASTNodeKind::TypealiasStmt:
            return lsp::CompletionItemKind::Interface;
        case ASTNodeKind::VariantDecl:
            return lsp::CompletionItemKind::Struct;
        case ASTNodeKind::EnumDecl:
            return lsp::CompletionItemKind::Enum;
        case ASTNodeKind::VarInitStmt:
            return lsp::CompletionItemKind::Variable;
        default:
#ifdef DEBUG
            throw std::runtime_error("unknown node completion item kind");
#endif
            return lsp::CompletionItemKind::Folder;
    }
}

lsp::CompletionItemKind toCompletionItemKind(BaseType* type) {
    const auto linked = type->linked_node();
    if(linked) {
        return toCompletionItemKind(linked);
    } else {
        return lsp::CompletionItemKind::Variable;
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
            put_func_param(analyzer, node->as_func_param_unsafe());
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
            put_with_doc(analyzer, node->as_typealias_unsafe()->name_view(), lsp::CompletionItemKind::Interface, node);
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

void CompletionItemAnalyzer::VisitVarInitStmt(VarInitStatement *init) {
    //TODO: put_var_init(this, init);
    // TODO is_caret_inside should get location of the type, however we're not storing TypeLoc yet
    if(init->type && is_caret_inside(SourceLocation(0))) {
        visit(init->type);
    } else if(init->value && is_caret_inside(init->value->encoded_location())) {
        visit(init->value);
    }
}

void CompletionItemAnalyzer::VisitAssignmentStmt(AssignStatement *assign) {
    if(is_caret_inside(assign->lhs->encoded_location())) {
        visit(assign->lhs);
    }
    if(is_caret_inside(assign->value->encoded_location())) {
        visit(assign->value);
    }
}

void CompletionItemAnalyzer::VisitFunctionDecl(FunctionDeclaration *decl) {
    //TODO: put_function(this, decl);
    if(decl->body.has_value() && is_caret_inside(decl->body->encoded_location())) {
        for(auto& param : decl->params) {
            //TODO: put_func_param(this, param);
        }
        visit(decl->body.value());
    }
}

void CompletionItemAnalyzer::VisitEnumDecl(EnumDeclaration *decl) {
    //TODO: put_enum_decl(this, decl);
    // caret is inside the enum, what should we have
}

void visit_inside(CompletionItemAnalyzer* analyzer, MembersContainer* container) {
    for(auto& var : container->variables()) {
        if(analyzer->is_caret_inside(var->encoded_location())) {
            analyzer->visit(var);
            return;
        }
    }
    for(auto func : container->master_functions()) {
        if(analyzer->is_caret_inside(func->encoded_location())) {
            analyzer->visit(func);
            return;
        }
    }
}

void CompletionItemAnalyzer::VisitStructDecl(StructDefinition *def) {
    //TODO: put_struct_decl(this, def);
    visit_inside(this, def);
}

void CompletionItemAnalyzer::VisitInterfaceDecl(InterfaceDefinition *def) {
    //TODO: put_interface_decl(this, def);
    visit_inside(this, def);
}

void CompletionItemAnalyzer::VisitImplDecl(ImplDefinition *def) {
    visit_inside(this, def);
}

void CompletionItemAnalyzer::VisitIfStmt(IfStatement *stmt) {
    if(is_caret_inside(stmt->ifBody.encoded_location())) {
        visit(stmt->ifBody);
    } else {
        bool found = false;
        for(auto& elseIf : stmt->elseIfs) {
            if(is_caret_inside(elseIf.second.encoded_location())) {
                visit(elseIf.second);
                found = true;
                break;
            }
        }
        if(!found && stmt->elseBody.has_value() && is_caret_inside(stmt->elseBody.value().encoded_location())) {
            visit(stmt->elseBody.value());
        }
    }
}

void CompletionItemAnalyzer::VisitWhileLoopStmt(WhileLoop *loop) {
    if(is_caret_inside(loop->body.encoded_location())) {
        visit(loop->body);
    }
}

void CompletionItemAnalyzer::VisitDoWhileLoopStmt(DoWhileLoop *loop) {
    if(is_caret_inside(loop->body.encoded_location())) {
        visit(loop->body);
    }
}

void CompletionItemAnalyzer::VisitForLoopStmt(ForLoop *loop) {
    if(is_caret_inside(loop->body.encoded_location())) {
        visit(loop->body);
    }
}

void CompletionItemAnalyzer::VisitSwitchStmt(SwitchStatement *stmt) {
    for(auto& caseBody : stmt->scopes) {
        if(is_caret_inside(caseBody.encoded_location())) {
            visit(caseBody);
        }
    }
}

void CompletionItemAnalyzer::VisitLambdaFunction(LambdaFunction *func) {
    if(is_caret_inside(func->scope.encoded_location())) {
        for(auto& param : func->params) {
            //TODO: put_func_param(this, param);
        }
        visit(func->scope);
    }
}

void CompletionItemAnalyzer::VisitStructValue(StructValue *structValue) {
    for(auto& val : structValue->values) {
        auto value = val.second.value;
        if(is_caret_inside(value->encoded_location())) {
            visit(value);
        }
    }
}

void CompletionItemAnalyzer::VisitArrayValue(ArrayValue *arrayValue) {
    for(auto value : arrayValue->values) {
        if(is_caret_inside(value->encoded_location())) {
            visit(value);
        }
    }
}

void CompletionItemAnalyzer::VisitScope(Scope *scope) {
    for(const auto node : scope->nodes) {
        const auto sourceLoc = node->encoded_location();
        const auto position = loc_man.getLocationPos(sourceLoc);
        if(is_caret_ahead(position.start)) {
            //TODO: put_node(this, node);
        } else if(is_caret_inside(position.start, position.end)) {
            visit(node);
        }
    }
}

void put_variables_of(CompletionItemAnalyzer* analyzer, VariablesContainer* node) {
    for(auto& var : node->variables()) {
        //TODO: put_with_doc(analyzer, var->name_view(), lsCompletionItemKind::Field, var);
    }
}

void put_functions_of(CompletionItemAnalyzer* analyzer, ExtendableMembersContainerNode* node) {
    for(const auto func : node->master_functions()) {
        //TODO: put_with_doc(analyzer, func->name_view(), lsCompletionItemKind::Function, func);
    }
}

void put_non_self_param_functions_of(CompletionItemAnalyzer* analyzer, ExtendableMembersContainerNode* node) {
    for(const auto func : node->master_functions()) {
        if(!func->has_self_param()) {
            //TODO: put_with_doc(analyzer, func->name_view(), lsCompletionItemKind::Function, func);
        }
    }
}

bool put_children_of(CompletionItemAnalyzer* analyzer, BaseType* type, bool has_self);

bool put_children_of(CompletionItemAnalyzer* analyzer, ASTNode* linked_node, bool has_self) {
    const auto linked_kind = linked_node->kind();
    switch(linked_kind) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::UnionDecl: {
            const auto node = linked_node->as_extendable_members_container_unsafe();
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
            const auto node = linked_node->as_extendable_members_container_unsafe();
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
        case ASTNodeKind::FunctionParam: {
            auto& param = *linked_node->as_func_param_unsafe();
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
                //TODO: analyzer->put(mem.first, lsCompletionItemKind::EnumMember);
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

void analyze_module(CompletionItemAnalyzer* analyzer, ModuleData* modData) {
    for(const auto file : modData->fileUnits) {
        auto& fileNodes = file->unit.scope.body.nodes;
        for(const auto node : fileNodes) {
            put_node(analyzer, node);
        }
    }
}

void CompletionItemAnalyzer::analyze(LabModule* module, ModuleData* modData, LexResult* lexResult, ASTUnit* unit) {


    // check is caret position before a chain
    current_file = unit->scope.file_path.view();
    auto chain = chain_before_caret(lexResult->tokens);
    if(chain) {
        // TODO handle access chain before caret, we must provide completions
//        if(handle_chain_before_caret(chain)) {
//            return;
//        } else {
//            std::cout << "[Unknown] member access into access chain : " + chain->type_string() << std::endl;
//        }
    }

    // add completions for the last file by analyzing it in reverse fashion
    // only nodes in which caret is inside are visited using this visitor
    auto& currentFileNodes = unit->scope.body.nodes;
    int i = ((int) currentFileNodes.size()) - 1;
    while(i >= 0) {
        const auto node = currentFileNodes[i];
        const auto sourceLoc = node->encoded_location();
        const auto location = loc_man.getLocationPos(sourceLoc);
        if(is_caret_ahead(location.start)) {
            put_node(this, node);
        } else if(is_caret_inside(location.start, location.end)) {
            visit(node);
        }
        i--;
    }

    if(modData) {

        // add completions for other files (no analyzing)
        analyze_module(this, modData);

        // add completions for direct dependencies
        for(const auto dep : modData->dependencies) {
            analyze_module(this, dep);
        }

    }

}