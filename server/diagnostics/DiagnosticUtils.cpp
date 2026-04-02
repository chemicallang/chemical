// Copyright (c) Chemical Language Foundation 2026.

#include "./DiagnosticUtils.h"

void add_diagnostics(std::vector<lsp::Diagnostic> &diagnostics, std::vector<Diag>& diags) {
    for(auto& diag : diags) {
        diagnostics.emplace_back(lsp::Diagnostic(
                lsp::Range(
                        lsp::Position(diag.range.start.line, diag.range.start.character),
                        lsp::Position(diag.range.end.line, diag.range.end.character)
                ),
                diag.message,
                static_cast<lsp::DiagnosticSeverity>(static_cast<int>(diag.severity.value()))
        ));
    }
}