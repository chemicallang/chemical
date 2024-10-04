// Copyright (c) Qinetik 2024.


#include "ProvideStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/Value.h"
#include "compiler/Codegen.h"

#ifdef COMPILER_BUILD

void ProvideStmt::code_gen(Codegen &gen) {
    const auto val = value->llvm_value(gen, nullptr);
    put_in(gen.implicit_args, val, &gen, [](ProvideStmt* stmt, void* data) {
        stmt->body.code_gen(*((Codegen*) data));
    });
}

#endif

template<typename T>
void ProvideStmt::put_in(std::unordered_map<std::string, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*)) {
    auto& implicit_args = value_map;
    auto found = implicit_args.find(identifier);
    if(found != implicit_args.end()) {
        // record the previous value
        const auto previous = found->second;
        // set the new value
        found->second = new_value;
        // link the body
        do_body(this, data);
        // set previous value
        found->second = previous;
    } else {
        // set the value
        implicit_args[identifier] = new_value;
        // link the body
        do_body(this, data);
        // remove the value
        implicit_args.erase(identifier);
    }
}

ProvideStmt::ProvideStmt(
    Value* value,
    std::string identifier,
    Scope scope,
    ASTNode* parent,
    CSTToken* token
) : value(value), identifier(std::move(identifier)), body(std::move(scope)), parent_node(parent), token(token) {

}

void ProvideStmt::declare_and_link(SymbolResolver &linker) {
    if(value->link(linker, value, nullptr)) {
        put_in(linker.implicit_args, value, &linker, [](ProvideStmt* stmt, void* data) {
            stmt->body.link_sequentially((*(SymbolResolver*) data));
        });
    }
}
