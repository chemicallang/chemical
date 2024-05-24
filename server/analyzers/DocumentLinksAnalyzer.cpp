// Copyright (c) Qinetik 2024.

#include "DocumentLinksAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/LexResult.h"
#include "lexer/model/tokens/RefToken.h"
#include "LibLsp/lsp/lsDocumentUri.h"
#include "LibLsp/lsp/AbsolutePath.h"

std::string resolve_rel_path_str(const std::string &root_path, const std::string &file_path);

std::vector<lsDocumentLink> DocumentLinksAnalyzer::analyze(LexResult* result) {
    std::vector<lsDocumentLink> links;
    for(auto& token : result->tokens) {
        if(token->type() == LexTokenType::CompImport) {
            auto& value = token->as_compound()->tokens[1];
            auto& pos = value->start_token()->position;
            if(value->type() == LexTokenType::String) {
                auto unquoted_str = escaped_str_token(value.get());
                auto resolved = resolve_rel_path_str(result->abs_path, unquoted_str);
                if(!resolved.empty()) {
                    links.emplace_back(
                            lsRange {
                                    { static_cast<int>(pos.line), static_cast<int>(pos.character) },
                                { static_cast<int>(pos.line), static_cast<int>(pos.character + value->as_lex_token()->length()) },
                            },
                            lsDocumentUri{AbsolutePath(resolved)}
                    );
                } else {
                    std::cout << "[DocumentLinksAnalyzer] Couldn't resolve import path " << unquoted_str << " by " << result->abs_path << std::endl;
                }
            }
        }
    }
    return links;
}