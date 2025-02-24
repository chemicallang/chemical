// Copyright (c) Chemical Language Foundation 2025.

#include "InlayHintAnalyzer.h"
#include "integration/cbi/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"
#include "cst/utils/CSTUtils.h"

InlayHintAnalyzer::InlayHintAnalyzer(LocationManager& loc_man) : allocator(nullptr, 0, 0), loc_man(loc_man) {

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
            const auto encodedLoc = val->encoded_location();
            if(encodedLoc.isValid()) {
                const auto location = loc_man.getLocationPos(encodedLoc);
                const auto param = func_type->func_param_for_arg_at(i);
                if(param) {
                    hints.emplace_back(lsInlayHint {
                            { (int) location.start.line, (int) location.start.character },
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
        const auto encoded_loc = init->located_id.location;
        if(encoded_loc.isValid()) {
            const auto location = loc_man.getLocationPos(encoded_loc);
            const auto known = init->value->create_type(allocator);
            if(known) {
                const auto& start = location.end;
                hints.emplace_back(lsInlayHint {
                        { (int) start.line, (int) start.character },
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