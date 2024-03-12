// Copyright (c) Qinetik 2024.

#include <memory>
#include <utility>
#include <vector>
#include "ast/base/ASTNode.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

class Codegen {
public:

    std::vector<std::unique_ptr<ASTNode>> nodes;

    explicit Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path) : nodes(std::move(nodes)), path(std::move(path)) {
        module_init();
    }

    void module_init() {
        // context and module
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("TodoName", *ctx);

        // creating a new builder for the module
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    void save_to_file(const std::string &out_path) {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(out_path, errorCode);
        module->print(outLL, nullptr);
    }

private:

    /**
     * path to the file
     */
    std::string path;

    /**
     * LLVM context that holds modules
     */
    std::unique_ptr<llvm::LLVMContext> ctx;

    /**
     * module holds functions, global vars and stuff
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * the builder that builds ir
     */
    std::unique_ptr<llvm::IRBuilder<>> builder;

};