// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/MacroToken.h"

bool Lexer::lexHashMacro() {
    if(provider.increment('#')) {
        auto macro = lexAlphaNum();
        LspSemanticTokenModifier modifier = LspSemanticTokenModifier::LastModifier;
        if(macro == "deprecated") {
            modifier = LspSemanticTokenModifier::ls_deprecated;
        } else if(macro == "readonly") {
            modifier = LspSemanticTokenModifier::ls_readonly;
        }
        modifiers |= 1 << (unsigned int) modifier;
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), macro.size() + 1, modifier));
        return true;
    } else {
        return false;
    }
}