// Copyright (c) Chemical Language Foundation 2025.

#include "InlayHintAnalyzer.h"
#include "server/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"
#include "core/source/LocationManager.h"

//std::vector<lsp::InlayHint> inlay_hint_analyze(LocationManager& manager, ASTImportUnitRef& result, const std::string& compiler_exe_path, const std::string& lsp_exe_path) {
//    InlayHintAnalyzer analyzer(manager);
//    analyzer.analyze(result, compiler_exe_path, lsp_exe_path);
//    return std::move(analyzer.hints);
//}

InlayHintAnalyzer::InlayHintAnalyzer(LocationManager& loc_man) : allocator(0), loc_man(loc_man) {

}

void InlayHintAnalyzer::VisitFunctionCall(FunctionCall *call) {
    if(call->values.empty()) {
        return;
    }
    RecursiveVisitor<InlayHintAnalyzer>::VisitFunctionCall(call);
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
    RecursiveVisitor<InlayHintAnalyzer>::VisitVarInitStmt(init);
    if(init->value && !init->type) {
        const auto encoded_loc = init->encoded_location();
        if(encoded_loc.isValid()) {
            const auto location = loc_man.getLocationPos(encoded_loc);
            const auto known = init->value->create_type(allocator);
            if(known) {
                const auto& start = location.end;
                hints.emplace_back(lsp::InlayHint {
                        { start.line, start.character },
                        " :" + known->representation(),
                        lsp::InlayHintKind::Type
                });
            }
        }
    }
}

//std::vector<lsp::InlayHint> InlayHintAnalyzer::analyze(
//    ASTImportUnitRef& result,
//    const std::string& compiler_exe_path,
//    const std::string& lsp_exe_path
//) {
//
//    // visit all the nodes
//    visit(result.ast_result->unit.scope.body);
//
//    // return collected hints
//    return std::move(hints);
//
//}