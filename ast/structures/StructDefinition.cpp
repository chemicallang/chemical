// Copyright (c) Qinetik 2024.

#include "StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void StructDefinition::code_gen(Codegen &gen) {
    llvm::StructType::create(*gen.ctx, elements_type(gen), name);
}

llvm::Type* StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

bool StructDefinition::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string& child_name) {
    auto index = child_index(child_name);
    if(index == -1) {
        return false;
    }
    if(indexes.empty()) {
        indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    return true;
}

bool StructDefinition::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, unsigned int index) {
    if(index >= variables.size()) return false;
    indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    return true;
}

bool StructMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &childName) {
    auto linked = type->linked_node();
    if(!linked) return false;
    linked->add_child_index(gen, indexes, childName);
    return true;
}

llvm::Type *StructDefinition::llvm_type(Codegen &gen) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen));
}

#endif


StructMember::StructMember(
        std::string name,
        std::unique_ptr<BaseType> type,
        std::optional<std::unique_ptr<Value>> defValue
) : name(std::move(name)), type(std::move(type)), defValue(std::move(defValue)) {

}

void StructMember::accept(Visitor &visitor) {
    visitor.visit(this);
}

void StructMember::declare_and_link(ASTLinker &linker) {
    type->link(linker);
    if(defValue.has_value()) {
        defValue.value()->link(linker);
    }
}

ASTNode *StructMember::child(const std::string &childName) {
    auto linked = type->linked_node();
    if(!linked) return nullptr;
    linked->child(childName);
}

std::string StructMember::representation() const {
    std::string rep(name + " : " + type->representation());
    if (defValue.has_value()) {
        rep.append(defValue.value()->representation());
    }
    return rep;
}

StructDefinition::StructDefinition(
        std::string name,
        std::map<std::string, std::unique_ptr<StructMember>> fields,
        std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions,
        std::optional<std::string> overrides
) : name(std::move(name)), variables(std::move(fields)), functions(std::move(functions)),
    overrides(std::move(overrides)) {}

void StructDefinition::accept(Visitor &visitor) {
    visitor.visit(this);
}

ASTNode *StructDefinition::child(const std::string &varName) {
    auto found = variables.find(varName);
    if(found != variables.end()) {
        return found->second.get();
    } else {
        return nullptr;
    }
}

int StructDefinition::child_index(const std::string &varName) {
    auto i = 0;
    for(const auto& var : variables) {
        if(var.first == varName) {
            return i;
        }
        i++;
    }
    return -1;
}

void StructDefinition::declare_top_level(ASTLinker &linker) {
    linker.current[name] = this;
}

void StructDefinition::declare_and_link(ASTLinker &linker) {
    for(const auto& var : variables){
        var.second->declare_and_link(linker);
    }
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