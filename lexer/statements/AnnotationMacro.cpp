// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/MacroToken.h"
#include "lexer/model/tokens/RawToken.h"

bool Lexer::lexAnnotationMacro() {
    if(provider.peek() == '@' || provider.peek() == '#') {
        auto isAnnotation = provider.peek() == '@';
        provider.readCharacter();
        auto macro = lexIdentifier();
#ifdef LSP_BUILD
        SemanticTokenModifier modifier;
        if(macro == "deprecated") {
            modifier = SemanticTokenModifier::ls_deprecated;
        } else if(macro == "readonly") {
            modifier = SemanticTokenModifier::ls_readonly;
        } else {
            modifier = SemanticTokenModifier::LastModifier;
        }
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), std::move(macro), isAnnotation, modifier));
#else
        tokens.emplace_back(std::make_unique<MacroToken>(backPosition(macro.size() + 1), macro, isAnnotation));
        if(!isAnnotation) {
            std::string ending = "#end" + macro;
            auto current = position();
            std::string content = provider.readUntil(ending, false);
            tokens.emplace_back(std::make_unique<RawToken>(current, std::move(content)));
            auto before_ending = position();
            if(provider.increment(ending)) {
                tokens.emplace_back(std::make_unique<MacroToken>(before_ending, "end" + macro, false, true));
            } else {
                error("expected ending macro with " + ending);
            }
        }
#endif
        return true;
    } else {
        return false;
    }
}