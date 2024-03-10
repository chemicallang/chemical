// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "FileTracker.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"

std::vector<SemanticToken> to_semantic_tokens(FileTracker& tracker, const std::string& path);