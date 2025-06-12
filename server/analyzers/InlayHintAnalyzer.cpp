// Copyright (c) Chemical Language Foundation 2025.

#include "InlayHintAnalyzer.h"
#include "server/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"
#include "core/source/LocationManager.h"

std::vector<lsp::InlayHint> inlay_hint_analyze(LocationManager& manager, const std::span<ASTNode*>& nodes, const Range& range) {
    InlayHintAnalyzer analyzer(manager, range);
    analyzer.analyze(nodes);
    return std::move(analyzer.hints);
}

InlayHintAnalyzer::InlayHintAnalyzer(LocationManager& loc_man, const Range& range) : allocator(0), loc_man(loc_man), range(range) {

}

bool InlayHintAnalyzer::should_compute(SourceLocation location) {
    const auto locPos = loc_man.getLocationPos(location);
    if(locPos.start.is_behind(range.start) || locPos.end.is_ahead(range.end)) {
        return false;
    } else {
        return true;
    }
}

void InlayHintAnalyzer::VisitFunctionCall(FunctionCall *call) {
    if(!should_compute(call->encoded_location())) {
        return;
    }
    RecursiveVisitor<InlayHintAnalyzer>::VisitFunctionCall(call);
    if(call->values.empty()) {
        return;
    }
    const auto func_type = call->function_type(allocator);
    if(func_type) {
        unsigned i = 0;
        for(auto val : call->values) {
            const auto encodedLoc = val->encoded_location();
            if(encodedLoc.isValid()) {
                const auto location = loc_man.getLocationPos(encodedLoc);
                const auto param = func_type->func_param_for_arg_at(i);
                if(param) {
                    hints.emplace_back(lsp::InlayHint {
                            { location.start.line, location.start.character },
                            param->name.str() + ": ",
                            lsp::InlayHintKind::Parameter
                    });
                }
            }
            i++;
        }
    }
}

void InlayHintAnalyzer::VisitVarInitStmt(VarInitStatement *init) {
    if(!should_compute(init->encoded_location())) {
        return;
    }
    RecursiveVisitor<InlayHintAnalyzer>::VisitVarInitStmt(init);
    if(init->value && !init->type) {
        const auto encoded_loc = init->encoded_location();
        if(encoded_loc.isValid()) {
            const auto location = loc_man.getLocationPos(encoded_loc);
            const auto known = init->value->create_type(allocator);
            if(known) {
                const auto& start = location.end;
                hints.emplace_back(lsp::InlayHint {
                        { start.line, static_cast<unsigned int>(start.character + init->name_view().size() + 1) },
                        " :" + known->representation(),
                        lsp::InlayHintKind::Type
                });
            }
        }
    }
}

void InlayHintAnalyzer::VisitFunctionDecl(FunctionDeclaration *decl) {
    // every function is checked to avoid visiting it at top level
    if(!should_compute(decl->encoded_location())) {
        return;
    }
    RecursiveVisitor<InlayHintAnalyzer>::VisitFunctionDecl(decl);
}

void InlayHintAnalyzer::analyze(const std::span<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        visit(node);
    }
}