// Copyright (c) Qinetik 2024.

#ifdef COMPILER_BUILD
#include "ast/base/ASTNode.h"
#include "Codegen.h"

#include <utility>
#include "llvmimpl.h"

Codegen::Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path, std::string target_triple, std::string curr_exe_path): nodes(std::move(nodes)), path(std::move(path)), target_triple(std::move(target_triple)), curr_exe_path(std::move(curr_exe_path)) {
    module_init();
}

void Codegen::module_init() {
    // context and module
    ctx = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("TodoName", *ctx);

    // creating a new builder for the module
    builder = new llvm::IRBuilder<>(*ctx);
}

void Codegen::compile() {
    for(const auto& node : nodes) {
        node->code_gen(*this);
        position++;
    }
}

void Codegen::createFunctionBlock(llvm::Function* fn) {
    auto entry = createBB("entry", fn);
    SetInsertPoint(entry);
}

llvm::Function* Codegen::create_function(const std::string& name, llvm::FunctionType* type) {
    if(!has_current_block_ended) {
        builder->CreateRetVoid();
        has_current_block_ended = true;
    }
    current_function = module->getFunction(name);
    if(current_function == nullptr) {
        current_function = create_function_proto(name, type);
    }
    createFunctionBlock(current_function);
    return current_function;
}

llvm::FunctionCallee Codegen::declare_function(const std::string& name, llvm::FunctionType* type) {
    return module->getOrInsertFunction(name, type);
}

llvm::Function* Codegen::create_function_proto(const std::string& name, llvm::FunctionType* type) {
    auto fn = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, *module);
    llvm::verifyFunction(*fn);
    return fn;
}

llvm::BasicBlock* Codegen::createBB(const std::string& name, llvm::Function* fn) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
}

void Codegen::print_to_console() {
    module->print(llvm::outs(), nullptr);
}

void Codegen::save_to_file(const std::string &out_path) {
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(out_path, errorCode);
    module->print(outLL, nullptr);
    outLL.close();
}

void Codegen::loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock) {
    // set current loop exit, so it can be broken
    current_loop_continue = condBlock;
    current_loop_exit = endBlock;
}

void Codegen::error(const std::string& err){
    std::string errStr = "[Codegen] ERROR\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- node representation : " + nodes[position]->representation() + '\n';
    errStr += "---- node position : " + std::to_string(position);
#ifdef DEBUG
    std::cerr << errStr << std::endl;
#endif
    errors.push_back(errStr);
}

Codegen::~Codegen(){
    delete builder;
}

#endif