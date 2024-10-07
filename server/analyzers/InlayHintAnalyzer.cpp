// Copyright (c) Qinetik 2024.

#include "InlayHintAnalyzer.h"
#include "integration/cbi/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"

InlayHintAnalyzer::InlayHintAnalyzer() : allocator(nullptr, 0, 0) {

}

void InlayHintAnalyzer::visit(FunctionCall *call) {
    if(call->values.empty()) {
        return;
    }
    CommonVisitor::visit(call);
    const auto func_type = call->function_type(allocator);
    if(func_type) {
        unsigned i = 0;
        for(auto val : call->values) {
            const auto token = val->cst_token();
            if(token) {
                const auto param = func_type->func_param_for_arg_at(i);
                if(param) {
                    const auto& pos = token->start();
                    hints.emplace_back(lsInlayHint {
                            { (int) pos.line, (int) pos.character },
                            param->name + ": ",
                            lsInlayHintKind::Parameter
                    });
                }
            }
            i++;
        }
    }
}

std::vector<lsInlayHint> InlayHintAnalyzer::analyze(
    ASTImportUnitRef& result,
    const std::string& compiler_exe_path,
    const std::string& lsp_exe_path
) {
    if(result.files.empty()) {
        return {};
    }

    auto last = result.files.back();

    // visit all the nodes
    last->unit.scope.accept(this);

    // return collected hints
    return std::move(hints);

}