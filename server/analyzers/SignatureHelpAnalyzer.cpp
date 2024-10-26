// Copyright (c) Qinetik 2024.

#include "InlayHintAnalyzer.h"
#include "integration/cbi/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"
#include "cst/utils/CSTUtils.h"
#include "SignatureHelpAnalyzer.h"

SignatureHelpAnalyzer::SignatureHelpAnalyzer(LocationManager& loc_man, Position position) : CaretPositionAnalyzer(loc_man, position), allocator(nullptr, 0, 0) {

}

void SignatureHelpAnalyzer::visit(Scope *scope) {
    // we will only visit a scope, if the cursor position is inside it
    if(is_caret_inside(scope->encoded_location())) {
        CommonVisitor::visit(scope);
    }
}

void SignatureHelpAnalyzer::visit(FunctionCall *call) {
    CommonVisitor::visit(call);
    const auto func_type = call->function_type(allocator);
    if(func_type) {
        unsigned i = 0;
        for(auto val : call->values) {
            const auto encodedLocation = val->encoded_location();
            if(encodedLocation.isValid()) {
                const auto param = func_type->func_param_for_arg_at(i);
                if(param) {
                    const auto loc = loc_man.getLocationPos(encodedLocation);
                    // TODO
//                    hints.emplace_back(lsInlayHint {
//                            { (int) pos.line, (int) pos.character },
//                            param->name + ": ",
//                            lsInlayHintKind::Parameter
//                    });
                }
            }
            i++;
        }
    }
}

void SignatureHelpAnalyzer::analyze(
    ASTImportUnitRef& result
) {
    // no signature information available
    if(result.files.empty()) {
        return;
    }

    // getting the last file and visiting every node in it
    auto last = result.files.back();

    // visit all the nodes
    last->unit.scope.accept(this);

}