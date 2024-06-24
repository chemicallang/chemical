// Copyright (c) Qinetik 2024.

#include "Value.h"
#include "ast/values/StructValue.h"
#include "ast/statements/Assignment.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/PointerType.h"
#include "ast/values/UIntValue.h"
#include "ast/values/AccessChain.h"
#include "ast/types/ArrayType.h"
#include <ranges>
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::AllocaInst* Value::llvm_allocate_with(Codegen& gen, llvm::Value* value, llvm::Type* type) {
    auto x = gen.builder->CreateAlloca(type, nullptr);
    gen.builder->CreateStore(value, x);
    return x;
}

llvm::AllocaInst *Value::llvm_allocate(Codegen &gen, const std::string &identifier) {
    return llvm_allocate_with(gen, llvm_value(gen), llvm_type(gen));
}

llvm::GlobalVariable* Value::llvm_global_variable(Codegen& gen, bool is_const, const std::string& name) {
    return new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, llvm::GlobalValue::LinkageTypes::PrivateLinkage, (llvm::Constant*) llvm_value(gen), name);
}

llvm::AllocaInst* Value::access_chain_allocate(Codegen& gen, std::vector<std::unique_ptr<Value>>& values, unsigned int until) {
    return llvm_allocate_with(gen, values[until]->access_chain_value(gen, values, until), values[until]->llvm_type(gen));
}

llvm::Value* Value::get_element_pointer(
        Codegen& gen,
        llvm::Type* in_type,
        llvm::Value* ptr,
        std::vector<llvm::Value *>& idxList,
        unsigned int index
) {
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    return gen.builder->CreateGEP(in_type, ptr, idxList, "", gen.inbounds);
}

unsigned int Value::store_in_struct(
        Codegen& gen,
        StructValue* parent,
        llvm::Value* allocated,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    auto elementPtr = Value::get_element_pointer(gen, parent->llvm_type(gen), allocated, idxList, index);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

unsigned int Value::store_in_array(
        Codegen& gen,
        ArrayValue* parent,
        llvm::AllocaInst* ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    auto elementPtr = Value::get_element_pointer(gen, parent->llvm_type(gen), ptr, idxList, index);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

llvm::Value* Value::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>>& values, unsigned int until) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    auto loaded = gen.builder->CreateLoad(values[until]->llvm_type(gen), access_chain_pointer(gen, values, destructibles, until), "acc");
    for(auto& val : std::ranges::reverse_view(destructibles)) {
        val.first->llvm_destruct(gen, val.second);
    }
    return loaded;
}

llvm::Value* create_gep(Codegen &gen, Value* parent, llvm::Value* pointer, std::vector<llvm::Value*>& idxList) {
    auto type_kind = parent->type_kind();
    if(type_kind == BaseTypeKind::Array && parent->linked_node() && parent->linked_node()->as_func_param()) {
        auto type = parent->create_type();
        auto arr_type = (ArrayType*) type.get();
        return gen.builder->CreateGEP(arr_type->elem_type->llvm_type(gen), pointer, idxList, "", gen.inbounds);
    } else if(type_kind == BaseTypeKind::Pointer) {
        auto ty = parent->create_type();
        return gen.builder->CreateGEP(((PointerType*) (ty.get()))->type->llvm_type(gen), pointer, idxList, "", gen.inbounds);
    } else {
        return gen.builder->CreateGEP(parent->llvm_type(gen), pointer, idxList, "", gen.inbounds);
    }
}

// stored pointer into a variable, that must be loaded, before using
bool is_stored_pointer(Value* value) {
    auto linked = value->linked_node();
    if(!linked) return false;
    if(linked->as_struct_member()) {
        return linked->as_struct_member()->type->is_pointer();
    } else if(linked->as_var_init()) {
        auto kind = linked->as_var_init()->type_kind();
        return kind == BaseTypeKind::Pointer || kind == BaseTypeKind::String;
    }
    return false;
}

llvm::Value* Value::access_chain_pointer(
        Codegen &gen,
        std::vector<std::unique_ptr<Value>>& values,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        unsigned int until
) {

    if(until == 0) {
        return values[0]->llvm_pointer(gen);
    }

    Value* parent = values[0].get();
    llvm::Value* pointer = parent->llvm_pointer(gen);

    unsigned i = 1;
    unsigned j = 1;
    while(j <= until) {
        if(values[j]->as_func_call()) {
            pointer = values[j]->access_chain_allocate(gen, values, j);
            parent = values[j].get();
            if(j + 1 <= until) {
                destructibles.emplace_back(parent, pointer);
            }
            i = j + 1;
        }
        j++;
    }

    if(is_stored_pointer(parent) && i <= until) {
        pointer = gen.builder->CreateLoad(parent->llvm_type(gen), pointer);
    }

    std::vector<llvm::Value*> idxList;

    while (i <= until) {
        if(i + 1 <= until && values[i]->is_pointer()) {
            llvm::Value* gep;
            if(idxList.empty()) {
                gep = pointer;
            } else {
                gep = create_gep(gen, parent, pointer, idxList);
            }
            pointer = gen.builder->CreateLoad(values[i]->llvm_type(gen), gep);
            parent = values[i].get();
            idxList.clear();
        } else {
            if (!values[i]->add_member_index(gen, values[i - 1].get(), idxList)) {
                gen.error("couldn't add member index for fragment '" + values[i]->representation() +
                          "' in access chain '" + representation() + "'");
            }
        }
        i++;
    }
    return create_gep(gen, parent, pointer, idxList);
}

#endif

unsigned Value::as_uint() {
    return ((UIntValue*) this)->value;
}

std::string Value::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

void Value::link(SymbolResolver& linker, VarInitStatement* stmnt) {
    link(linker, stmnt->value.value());
}

void Value::link(SymbolResolver& linker, AssignStatement* stmnt) {
    link(linker, stmnt->value);
}

void Value::link(SymbolResolver& linker, StructValue* value, const std::string& name) {
    link(linker, value->values[name]);
}

void Value::link(SymbolResolver& linker, FunctionCall* call, unsigned int index) {
    link(linker, call->values[index]);
}

void Value::link(SymbolResolver& linker, ReturnStatement* returnStmt) {
    link(linker, returnStmt->value.value());
}