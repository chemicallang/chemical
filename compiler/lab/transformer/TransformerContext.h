// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <vector>
#include <unordered_map>
#include "lexer/Token.h"

struct LabModule;
struct LabJob;
class LabBuildCompiler;
class ASTProcessor;

class TransformerContext {
public:

    LabJob* job;
    LabBuildCompiler* compiler;
    ASTProcessor* processor;

    /**
     * these are flattened modules in the job
     */
    std::vector<LabModule*> flattened_mods;

    /**
     * for each file, the tokens that were lexed
     * ONLY present if keep_comments was true during parsing
     */
    std::unordered_map<unsigned int, std::vector<Token>> file_tokens;

    /**
     * constructor
     */
    TransformerContext(
            LabJob* job,
            LabBuildCompiler* compiler,
            ASTProcessor* processor,
            std::vector<LabModule*> flattened_mods
    ) : job(job), compiler(compiler), processor(processor), flattened_mods(std::move(flattened_mods)) {
        
    }

};