// Copyright (c) Qinetik 2024.

#include "InlayHintAnalyzer.h"
#include "integration/cbi/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"
#include "cst/utils/CSTUtils.h"

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

void InlayHintAnalyzer::visit(VarInitStatement *init) {
    CommonVisitor::visit(init);
    if(init->value && !init->type) {
        const auto token = init->cst_token();
        if(token && token->type() == LexTokenType::CompVarInit) {
            const auto known = init->value->create_type(allocator);
            if(known) {
                const auto name_tok = var_init_name_tok(token);
                const auto& start = name_tok->start();
                hints.emplace_back(lsInlayHint {
                        { (int) start.line, (int) (start.character + name_tok->length()) },
                        " :" + known->representation(),
                        lsInlayHintKind::Type
                });
            }
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