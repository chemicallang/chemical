// Copyright (c) Qinetik 2024.

#include "StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void StructDefinition::code_gen(Codegen &gen) {
    llvm::StructType::create(*gen.ctx, elements_type(gen), name);
}

#endif

StructDefinition::StructDefinition(
        std::string name,
        std::map<std::string, std::unique_ptr<VarInitStatement>> fields,
        std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions,
        std::optional<std::string> overrides
) : name(std::move(name)), variables(std::move(fields)), functions(std::move(functions)),
    overrides(std::move(overrides)) {}

void StructDefinition::accept(Visitor &visitor) {
    visitor.visit(this);
}

void StructDefinition::declare_top_level(ASTLinker &linker) {
    linker.current[name] = this;
}

StructDefinition *StructDefinition::as_struct_def() {
    return this;
}

FunctionDeclaration *StructDefinition::member(const std::string &name) {
    for (const auto &field: functions) {
        auto decl = field.second->as_function();
        if (decl != nullptr && decl->name == name) {
            return decl;
        }
    }
    return nullptr;
}

bool StructDefinition::type_check(InterpretScope &scope) {
    if (overrides.has_value()) {
        auto inter = scope.find_value(overrides.value());
        if (inter == nullptr) {
//                auto interVal = inter->second->as_interface();
//                if (interVal != nullptr) {
//                    if (!interVal->verify(scope, name, fields)) {
//                        return false;
//                    }
//                } else {
//                    scope.error("provided overridden value is not an interface");
//                }
        } else {
            scope.error("couldn't find the overridden interface " + overrides.value());
        }
    }
    return true;
}

void StructDefinition::interpret(InterpretScope &scope) {
    decl_scope = &scope;
}

void StructDefinition::interpret_scope_ends(InterpretScope &scope) {
    decl_scope = nullptr;
}

#ifdef COMPILER_BUILD

std::vector<llvm::Type *> StructDefinition::elements_type(Codegen &gen) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables.size());
    // TODO this doesn't work, a crash happens at variables
    for (const auto &var: variables) {
        vec.push_back(var.second->llvm_type(gen));
    }
    return vec;
}

#endif

std::string StructDefinition::representation() const {
    std::string ret("struct " + name + " ");
    if (overrides.has_value()) {
        ret.append(": " + overrides.value() + " {\n");
    } else {
        ret.append("{\n");
    }
    int i = 0;
    for (const auto &field: variables) {
        ret.append(field.second->representation());
        if (i < variables.size() - 1) {
            ret.append(1, '\n');
        }
        i++;
    }
    ret.append("\n}");
    i = 0;
    for (const auto &field: functions) {
        ret.append(field.second->representation());
        if (i < variables.size() - 1) {
            ret.append(1, '\n');
        }
        i++;
    }
    ret.append("\n}");
    return ret;
}