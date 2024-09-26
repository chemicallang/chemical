// Copyright (c) Qinetik 2024.

#include "DocumentLinksAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/cbi/model/LexResult.h"
#include "LibLsp/lsp/lsDocumentUri.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "preprocess/ImportPathHandler.h"

std::string rel_to_lib_system(const std::string& header_path, const std::string& lsp_exe_path);

std::vector<lsDocumentLink> DocumentLinksAnalyzer::analyze(LexResult* result, const std::string& compiler_exe_path, const std::string& lsp_exe_path) {
    std::vector<lsDocumentLink> links;
    ImportPathHandler path_handler(compiler_exe_path);
    for(auto& token : result->unit.tokens) {
        if(token->type() == LexTokenType::CompImport) {
            auto& value = token->tokens[1];
            auto& pos = value->start_token()->position();
            if(value->type() == LexTokenType::String) {
                auto unquoted_str = escaped_str_token(value);
                std::string resolved;
                if(!unquoted_str.empty() && unquoted_str[0] == '@') {
                    if(unquoted_str.starts_with("@system")) {
                        resolved = rel_to_lib_system(unquoted_str.substr(unquoted_str.find('/') + 1), lsp_exe_path);
                    } else {
                        auto replaced = path_handler.replace_at_in_path(unquoted_str);
                        if(replaced.error.empty()) {
                            resolved = replaced.replaced;
                        } else {
                            std::cout << "[DocumentLinksAnalyzer] Error replacing at in path " << unquoted_str << std::endl;
                        }
                    }
                } else {
                    resolved = resolve_rel_parent_path_str(result->abs_path, unquoted_str);
                }
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