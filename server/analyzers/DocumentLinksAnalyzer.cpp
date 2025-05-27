// Copyright (c) Chemical Language Foundation 2025.

#include "DocumentLinksAnalyzer.h"
#include "compiler/cbi/model/LexResult.h"
#include "preprocess/ImportPathHandler.h"

std::vector<lsp::DocumentLink> DocumentLinksAnalyzer::analyze(LexResult* result, const std::string& compiler_exe_path, const std::string& lsp_exe_path) {
    std::vector<lsp::DocumentLink> links;
    ImportPathHandler path_handler(compiler_exe_path);
    for(auto& token : result->tokens) {
//        if(token.type == TokenType::CompImport) {
//            auto& value = token->tokens[1];
//            auto& pos = value->start_token()->position();
//            if(value->type() == LexTokenType::String) {
//                auto unquoted_str = escaped_str_token(value);
//                auto replaceResult = path_handler.resolve_import_path(result->abs_path, unquoted_str);
//                if(replaceResult.error.empty() && !replaceResult.replaced.empty()) {
//                    links.emplace_back(
//                            lsRange {
//                                    { static_cast<int>(pos.line), static_cast<int>(pos.character) },
//                                    { static_cast<int>(pos.line), static_cast<int>(pos.character + value->as_lex_token()->length()) },
//                            },
//                            lsDocumentUri{AbsolutePath(replaceResult.replaced)}
//                    );
//                } else {
//                    std::cout << "[DocumentLinksAnalyzer] Couldn't resolve import path " << unquoted_str << " by " << result->abs_path << " because " << replaceResult.error << std::endl;
//                }
//            }
//        }
    }
    return links;
}