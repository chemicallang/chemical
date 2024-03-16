// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/MacroToken.h"

bool Lexer::lexAnnotationMacro() {
    if(provider.increment('@')) {
        auto macro = lexAlphaNum();
#ifdef LSP_BUILD
        SemanticTokenModifier modifier;
        if(macro == "deprecated") {
            modifier = SemanticTokenModifier::ls_deprecated;
        } else if(macro == "readonly") {
            modifier = SemanticTokenModifier::ls_readonly;
        } else {
            modifier = SemanticTokenModifier::LastModifier;
        }
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), macro.size() + 1, modifier));
#else
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), macro.size() + 1));
#endif
        return true;
    } else {
        return false;
    }
}