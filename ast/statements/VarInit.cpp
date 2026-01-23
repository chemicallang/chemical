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

llvm::GlobalValue::LinkageTypes global_var_linkage(VarInitStatement* stmt) {
    switch(stmt->specifier()) {
        case AccessSpecifier::Private:
        default:
            return llvm::GlobalValue::LinkageTypes::PrivateLinkage;
        case AccessSpecifier::Internal:
            return llvm::GlobalValue::LinkageTypes::InternalLinkage;
        case AccessSpecifier::Public:
        case AccessSpecifier::Protected:
            return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    }
}

void global_var_set_defaults(VarInitStatement* stmt, llvm::GlobalVariable* global) {
    global->setDSOLocal(true);
    if(stmt->is_thread_local()) {
        global->setThreadLocal(true);
        global->setThreadLocalMode(llvm::GlobalValue::LocalExecTLSModel);
    }
}

void VarInitStatement::code_gen_global_var(Codegen &gen, bool initialize, bool initialize_strings) {
    llvm::Constant* initializer;
    if(value) {
        if(value->kind() == ValueKind::String) {
            if(initialize_strings) {
                const auto string_val = value->as_string_unsafe();
                ScratchString<128> temp_name;
                gen.mangler.mangle(temp_name, this);
                const auto global = gen.builder->CreateGlobalString(llvm::StringRef(string_val->value.data(), string_val->value.size()), (std::string_view) temp_name, 0, gen.module.get());
                global->setLinkage(global_var_linkage(this));
                global->setConstant(is_const());
                global_var_set_defaults(this, global);
                llvm_ptr = global;
                return;
            } else {
                initializer = nullptr;
            }
        } else {
            if(initialize) {
                initializer = (llvm::Constant*) initializer_value(gen);
            } else {
                initializer = nullptr;
            }
        }
    } else {
        initializer = nullptr;
    }
    ScratchString<128> temp_name;
    gen.mangler.mangle(temp_name, this);
    const auto global_type = llvm_type(gen);
    const auto global = new llvm::GlobalVariable(*gen.module, global_type, is_const(), global_var_linkage(this), initializer, (std::string_view) temp_name);
    global_var_set_defaults(this, global);
    llvm_ptr = global;
}

void VarInitStatement::code_gen_declare(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    code_gen_global_var(gen, false, true);
    gen.di.declare(this, llvm_ptr);
}

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if(is_comptime()) {
            return;
        }
        if(value == nullptr) {
            return;
        }
        // strings are initialized
        if(value->kind() == ValueKind::String) {
            return;
        }
        const auto global = ((llvm::GlobalVariable*) llvm_ptr);
        // probably done in code_gen_declare
        // why are we doing this here ?
        // well value can contain something that hasn't been declared yet
        // so we have to wait for all calls to code_gen_declare to complete
        global->setInitializer((llvm::Constant*) initializer_value(gen));
        return;
    } else {
        if (value) {

            if(is_const() && !value->as_struct_value() && !value->as_array_value()) {
                llvm_ptr = initializer_value(gen);
                put_destructible(gen);
                gen.di.declare(this, llvm_ptr);
                return;
            }

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
        if(value->kind() == ValueKind::String) {
            // llvm_ptr is not null, belongs to previous module
            if(llvm_ptr != nullptr) {
                // lets declare this string (aleady present in the executable
                code_gen_global_var(gen, false, false);
            }
            // if llvm_ptr is null, since this is a comptime constant string, we'll declare once anyone loads it
            return;
        }
        llvm_ptr = initializer_value(gen);
        return;
    }
    code_gen_global_var(gen, false, false);
}

llvm::Value *VarInitStatement::llvm_load(Codegen& gen, SourceLocation location) {
    if(is_comptime()) {
        if(!value) {
            gen.error("comptime global variable/constant must be initialized", this);
            return gen.builder->getInt32(0);
        }
        if(is_const()) {
            if(value->kind() == ValueKind::String) {
                if(llvm_ptr == nullptr) {
                    // string doesn't exist in the executable, lets put it there
                    code_gen_global_var(gen, true, true);
                }
                // string exists in the executable
                return llvm_ptr;
            }
            // quickly create a value that we already hold
            return initializer_value(gen);
        } else {
            const auto stored = gen.comptime_scope.find_value(name_view());
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
    if(v == nullptr) {
        gen.error("nullptr received from llvm_pointer in var init", location);
        return gen.builder->getInt32(0);
    }
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

llvm::Value* VarInitStatement::loadable_llvm_pointer(Codegen& gen, SourceLocation location) {
    if(is_const()) {
        // you write this code:
        // const val = 3; take_ref(val); // func take_ref(r : &int) {}
        // we pass the val as a loadable pointer to take_ref
        // this makes const val = 3 basically a var val = 3, maybe this should be explicit
        return ASTNode::turnPtrValueToLoadablePtr(gen, llvm_ptr, location);
    }
    return llvm_ptr;
}

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return type ? type->llvm_type(gen) : value->llvm_type(gen);
}

llvm::Type *VarInitStatement::llvm_chain_type(Codegen &gen, std::vector<Value*> &values, unsigned int index) {
    check_has_type(gen);
    return type ? type->llvm_chain_type(gen, values, index) : value->llvm_chain_type(gen, values, index);
}

#endif

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