// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTUnit.h"
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/llvmfwd.h"
#include "utils/Benchmark.h"
#include "core/diag/Diagnostic.h"

struct ASTFileResult : ASTFileMetaData {

    /**
     * should the processing be continued, this is false, if ast contained errors
     */
    bool continue_processing;

    /**
     * the parsed unit
     */
    ASTUnit unit;

    /**
     * the compile unit
     */
    llvm::DICompileUnit* diCompileUnit = nullptr;

    /**
     * the imported files by this file, these files don't contain duplicates
     * or already imported files
     */
    std::vector<ASTFileMetaData> imports;

    /**
     * if read error occurred this would contain it
     */
    std::string read_error;

    /**
     * diagnotics collected during the lexing process
     */
    std::vector<Diag> lex_diagnostics;

    /**
     * diagnostics collected during the conversion process
     * These diagnostics will be translation diagnostics, if it's a c file
     */
    std::vector<Diag> parse_diagnostics;

    /**
     * the benchmark results are stored here, if user opted for benchmarking
     */
    BenchmarkResults lex_benchmark;

    /**
     * the parsing benchmarks are stored here, if user opted for benchmarking
     * This will be c translation benchmarks, if it's c file
     */
    BenchmarkResults parse_benchmark;

    /**
     * constructor
     */
    ASTFileResult(
            unsigned int file_id,
            std::string abs_path,
            ModuleScope* module
    ) : ASTFileMetaData(file_id, module, std::move(abs_path)), continue_processing(true),
        unit(file_id, chem::string_view(this->abs_path), module)
    {

    }

};