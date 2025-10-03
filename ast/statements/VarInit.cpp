// Copyright (c) Chemical Language Foundation 2025.

#include "VarInit.h"
#include "ast/types/ArrayType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/values/StringValue.h"
#include "ast/values/VariableIdentifier.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/types/GenericType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/VariantDefinition.h"
#include "preprocess/2c/BufferedWriter.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value* VarInitStatement::initializer_value(Codegen &gen) {
    const auto ty = type_ptr_fast();
    const auto llvmValue = value->llvm_value(gen, ty);
    return ty ? gen.implicit_cast(llvmValue, ty, ty->llvm_type(gen)) : llvmValue;
}

void VarInitStatement::code_gen_global_var(Codegen &gen, bool initialize) {
    llvm::Constant* initializer;
    llvm::GlobalValue::LinkageTypes linkage;
    switch(specifier()) {
        case AccessSpecifier::Private:
            linkage = llvm::GlobalValue::LinkageTypes::PrivateLinkage;
            break;
        case AccessSpecifier::Internal:
            linkage = llvm::GlobalValue::LinkageTypes::InternalLinkage;
            break;
        case AccessSpecifier::Public:
        case AccessSpecifier::Protected:
            linkage = llvm::GlobalValue::LinkageTypes::ExternalLinkage;
            break;
    }
    if(value && initialize) {
        const auto string_val = value->as_string_value();
        if(string_val) {
            ScratchString<128> temp_name;
            gen.mangler.mangle(temp_name, this);
            const auto global = gen.builder->CreateGlobalString(llvm::StringRef(string_val->value.data(), string_val->value.size()), (std::string_view) temp_name, 0, gen.module.get());
            global->setLinkage(linkage);
            global->setConstant(is_const());
            llvm_ptr = global;
            return;
        }
        initializer = (llvm::Constant*) initializer_value(gen);
    } else {
        initializer = nullptr;
    }
    ScratchString<128> temp_name;
    gen.mangler.mangle(temp_name, this);
    const auto global = new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const(), linkage, initializer, (std::string_view) temp_name);
    global->setDSOLocal(true);
    if(is_thread_local()) {
        global->setThreadLocal(true);
        global->setThreadLocalMode(llvm::GlobalValue::LocalExecTLSModel);
    }
    llvm_ptr = global;
}

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if(is_comptime()) {
            return;
        }
        code_gen_global_var(gen, true);
        gen.di.declare(this, llvm_ptr);
        return;
    } else {
        if (value) {

            if(is_const() && !value->as_struct_value() && !value->as_array_value()) {
                llvm_ptr = initializer_value(gen);
                put_destructible(gen);
                gen.di.declare(this, llvm_ptr);
                return;
            }

//            bool moved = false;
//            if(value->is_ref_moved()) {
//                auto known_t = value->pure_type_ptr();
//                auto node = known_t->get_direct_linked_node();
//                if(node && node->isStoredStructType(node->kind())) {
//                    const auto& name_v = name_view();
//                    const auto allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, llvm::StringRef(name_v.data(), name_v.size()));
//                    gen.di.instr(allocaInst, encoded_location());
//                    llvm_ptr = allocaInst;
//                    gen.move_by_memcpy(node, value, llvm_ptr, value->llvm_value(gen));
//                    moved = true;
//                }
//            }

                // copy or move the struct, if required
                const auto exp_type = known_type_or_err();
                if(value->requires_memcpy_ref_struct(exp_type)) {
                    const auto& name_v = name_view();
                    const auto allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, llvm::StringRef(name_v.data(), name_v.size()));
                    gen.di.instr(allocaInst, encoded_location());
                    llvm_ptr = allocaInst;
                    if(!gen.copy_or_move_struct(exp_type, value, allocaInst)) {
                        gen.warn("couldn't copy or move the struct to location", this);
                    }
                    put_destructible(gen);
                    gen.di.declare(this, llvm_ptr);
                    return;
                }

//            if(!moved) {

                if(type) {
                    const auto canType = type->canonical();
                    const auto mutated = gen.mutate_capturing_function(canType, value);
                    if(mutated) {
                        llvm_ptr = mutated;
                        put_destructible(gen);
                        gen.di.declare(this, llvm_ptr);
                        return;
                    }
                }

                llvm_ptr = value->llvm_allocate(gen, name_str(),type_ptr_fast());
                gen.di.declare(this, llvm_ptr);

//            }

        } else {
            const auto t = llvm_type(gen);
            const auto& v = name_view();
            const auto allocaInst = gen.builder->CreateAlloca(t, nullptr, llvm::StringRef(v.data(), v.size()));
            gen.di.instr(allocaInst, encoded_location());
            llvm_ptr = allocaInst;
            const auto var = type->get_direct_linked_variant();
            if(var) {
                auto gep = gen.builder->CreateGEP(t, llvm_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
                const auto storeInst = gen.builder->CreateStore(gen.builder->getInt32(var->variables().size()), gep);
                gen.di.instr(storeInst, encoded_location());
            }
        }
        put_destructible(gen);
        gen.di.declare(this, llvm_ptr);
    }
}

void VarInitStatement::put_destructible(Codegen& gen) {
    gen.enqueue_destructible(known_type_or_err(), this, llvm_ptr);
}

void VarInitStatement::code_gen_external_declare(Codegen &gen) {
    if(is_comptime() && is_const()) {
        llvm_ptr = initializer_value(gen);
        return;
    }
    code_gen_global_var(gen, false);
}

llvm::Value *VarInitStatement::llvm_load(Codegen& gen, SourceLocation location) {
    if(is_comptime()) {
        if(!value) {
            gen.error("comptime global variable/constant must be initialized", this);
            return gen.builder->getInt32(0);
        }
        if(is_const()) {
            // quickly create a value that we already hold
            return initializer_value(gen);
        } else {
            const auto stored = gen.comptime_scope.find(name_view());
            if(stored) {
                return stored->llvm_value(gen);
            } else {
                gen.error("couldn't get value for comptime variable", this);
                return gen.builder->getInt32(0);
            };
        }
    }
    if(is_const()) {
        if(is_top_level()) {
        } else if(value) {
            return llvm_pointer(gen);
        }
        if(value && value->val_kind() == ValueKind::String) {
            return llvm_pointer(gen);
        }
    } else {
        const auto k_type = known_type_or_err();
        if(k_type->isStructLikeType()) {
            return llvm_pointer(gen);
        }
    }
    auto v = llvm_pointer(gen);
    const auto& name = name_view();
    const auto loadInst = gen.builder->CreateLoad(llvm_type(gen), v, llvm::StringRef(name.data(), name.size()));
    gen.di.instr(loadInst, location);
    return loadInst;
}

bool VarInitStatement::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    if (type) {
        return type->linked_node()->add_child_index(gen, indexes, name);
    } else if (value) {
        return value->add_child_index(gen, indexes, name);
    }
    return false;
}

inline void VarInitStatement::check_has_type(Codegen &gen) {
    if (!type && !value) {
        gen.error("neither variable type nor variable value were given", this);
        return;
    }
}

llvm::Value *VarInitStatement::llvm_pointer(Codegen &gen) {
    return llvm_ptr;
}

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return type ? type->llvm_type(gen) : value->llvm_type(gen);
}

llvm::Type *VarInitStatement::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    check_has_type(gen);
    return type ? type->llvm_chain_type(gen, values, index) : value->llvm_chain_type(gen, values, index);
}

#endif

BaseType* VarInitStatement::get_or_create_type(ASTAllocator& allocator) {
    if(type) {
        return type;
    } else {
        return value->getType();
    }
}

BaseType* VarInitStatement::known_type() {
    if(type) {
        return type;
    }
    return value->getType();
}

/**
 * called by assignment to assign a new value in the scope that this variable was declared
 */
void VarInitStatement::declare(Value *new_value) {
    decl_scope->declare(name_view(), new_value);
}