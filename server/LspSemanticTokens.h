// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "WorkspaceManager.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "LibLsp/JsonRpc/RemoteEndPoint.h"

std::vector<SemanticToken> to_semantic_tokens(WorkspaceManager& tracker, const lsDocumentUri &uri, RemoteEndPoint &sp);