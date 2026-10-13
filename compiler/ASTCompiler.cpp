// Copyright (c) Chemical Language Foundation 2025.

#include "Codegen.h"
#include "utils/Benchmark.h"
#include "ASTProcessor.h"
#include "compiler/SymbolResolver.h"
#include "compiler/mir/MIRModule.h"
#include "compiler/mir/MIRTypeBuilder.h"
#include "compiler/mir/MIRLowerer.h"
#include "compiler/mir/MIRArena.h"
#include "compiler/mir/MIRDump.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"
#include "preprocess/2c/TranslationContext.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/If.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/statements/VarInit.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/Scope.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/CastedValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NullValue.h"
#include "ast/structures/VariantMember.h"
#include "ast/base/BaseType.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include "rang.hpp"
#include <iostream>

// ── MIR → LLVM: emit the module's LLVM IR from the collected MIR module ─────
// The MIR module is produced by the *shared* MIR construction used by the C
// backend (`ASTProcessor::implement_module` -> `mir_translate_after_declaration`),
// so the LLVM backend needs no AST traversal of its own. This function only
// turns the fully-populated MIR module into LLVM IR.

namespace {

/**
 * Lower a top-level global initializer AST value into an LLVM constant.
 *
 * This mirrors `MIRDeclEmitter::const_value_expr` (the C backend's declaration
 * emitter): the MIR type supplies the aggregate field order and the AST value
 * supplies the data. MIR symbol records do not carry initializer values, so
 * without this pass every global that is not a plain scalar keeps the zero
 * initializer `mir_declare_globals` gave it — arrays of `'.' as u16`, structs
 * with defaulted/inherited fields, variant-shaped globals, string constants and
 * comptime-constructed values were all silently zero on the LLVM path.
 */
class GlobalConstantLowerer {
public:
    GlobalConstantLowerer(Codegen& gen, const mir::MIRModule& m,
                          GlobalInterpretScope& scope)
        : gen_(gen), m_(m), scope_(scope), ctx_(gen.module->getContext()) {}

    llvm::Constant* build(Value* value, mir::TypeId tid, llvm::Type* want) {
        if (++depth_ > 64) {
            --depth_;
            return llvm::Constant::getNullValue(want);
        }
        llvm::Constant* c = build_impl(value, tid, want);
        --depth_;
        return c;
    }

private:
    Codegen& gen_;
    const mir::MIRModule& m_;
    GlobalInterpretScope& scope_;
    llvm::LLVMContext& ctx_;
    int depth_ = 0;
    unsigned str_counter_ = 0;

    bool valid(mir::TypeId t) const { return t != mir::MIR_INVALID_ID && t < m_.types.size(); }

    llvm::Constant* build_impl(Value* value, mir::TypeId tid, llvm::Type* want) {
        if (!value) return llvm::Constant::getNullValue(want);
        switch (value->val_kind()) {
            case ValueKind::ComptimeValue:
                return build(static_cast<ComptimeValue*>(value)->getValue(), tid, want);
            case ValueKind::CastedValue:
                return build(value->as_casted_value_unsafe()->value, tid, want);
            case ValueKind::NegativeValue: {
                Value* inner = value->as_negative_value_unsafe()->getValue();
                if (!inner) return llvm::Constant::getNullValue(want);
                // A negative float must negate the floating-point constant; a
                // negative integer is a `neg` constant expression.
                if (want->isFloatingPointTy()) {
                    Value* pos = inner->val_kind() == ValueKind::NegativeValue
                                     ? inner->as_negative_value_unsafe()->getValue() : inner;
                    double d = 0.0;
                    if (pos && pos->val_kind() == ValueKind::Float)
                        d = pos->as_float_unsafe()->value;
                    else if (pos && pos->val_kind() == ValueKind::Double)
                        d = pos->as_double_unsafe()->value;
                    else if (pos && pos->val_kind() == ValueKind::IntN)
                        d = static_cast<double>(static_cast<int64_t>(
                            pos->as_int_num_value_unsafe()->get_num_value()));
                    return llvm::ConstantFP::get(want, -d);
                }
                llvm::Constant* c = build(inner, tid, want);
                if (c && c->getType()->isIntegerTy()) return llvm::ConstantExpr::getNeg(c);
                return c;
            }
            case ValueKind::IntN:
                return int_constant(value->as_int_num_value_unsafe(), want);
            case ValueKind::Bool: {
                const bool b = value->as_bool_unsafe()->value;
                if (want->isIntegerTy()) return llvm::ConstantInt::get(want, b ? 1 : 0);
                if (want->isPointerTy()) return int_constant_ptr(b ? 1 : 0, want);
                if (want->isFloatingPointTy()) return llvm::ConstantFP::get(want, b ? 1.0 : 0.0);
                return llvm::Constant::getNullValue(want);
            }
            case ValueKind::Float:
                return fp_constant(value->as_float_unsafe()->value, want);
            case ValueKind::Double:
                return fp_constant(value->as_double_unsafe()->value, want);
            case ValueKind::String: {
                auto* sv = value->as_string_unsafe();
                std::string s(sv->value.data(), sv->value.size());
                if (want->isPointerTy()) {
                    llvm::Constant* arr = llvm::ConstantDataArray::getString(ctx_, s, true);
                    auto* gv = new llvm::GlobalVariable(
                        *gen_.module, arr->getType(), true, llvm::GlobalValue::PrivateLinkage,
                        arr, ".mir.str." + std::to_string(str_counter_++));
                    return gv; // opaque pointer
                }
                // `const x : char[] = "..."` — a string literal initializing a
                // char array (the module-level `char[]` constants that indexing
                // decays to a pointer).
                if (auto* at = llvm::dyn_cast<llvm::ArrayType>(want)) {
                    llvm::Type* et = at->getElementType();
                    // an unsized `char[]` takes its length from the literal (like
                    // C, including the NUL terminator); the global is then re-sized
                    // by the initializer pass.
                    const uint64_t n = at->getNumElements() ? at->getNumElements()
                                                            : s.size() + 1;
                    llvm::ArrayType* rt = llvm::ArrayType::get(et, n);
                    std::vector<llvm::Constant*> elems;
                    for (unsigned char ch : s)
                        elems.push_back(llvm::ConstantInt::get(et, ch));
                    while (elems.size() < n) elems.push_back(llvm::Constant::getNullValue(et));
                    if (elems.size() != n) return llvm::Constant::getNullValue(want);
                    return llvm::ConstantArray::get(rt, elems);
                }
                return llvm::Constant::getNullValue(want);
            }
            case ValueKind::NullValue:
                return llvm::Constant::getNullValue(want);
            case ValueKind::ArrayValue:
                return array_constant(value->as_array_value_unsafe(), tid, want);
            case ValueKind::StructValue:
                return struct_constant(value->as_struct_value_unsafe(), tid, want);
            case ValueKind::FunctionCall:
            case ValueKind::AccessChain:
                return call_constant(value, tid, want);
            case ValueKind::Identifier:
            case ValueKind::GenericInstIdentifier:
                // a bare function reference used as a value (`func`, `ident<int>`)
                return function_ref(value->linked_node(), want);
            default:
                return llvm::Constant::getNullValue(want);
        }
    }

    std::string mangle_node(ASTNode* n) {
        BufferedWriter w(256);
        gen_.mangler.mangle(w, n);
        return std::string(w.data(), w.getPosition());
    }

    /**
     * A function used as a *value* resolves to the address of the concrete
     * function (`ident<int>`, `ns::ident<int>`, `module::func`). The leaf is a
     * `GenericInstIdentifier` — a `VariableIdentifier` subclass — whose
     * `linked_node()` is the instantiated `FunctionDecl` after symbol
     * resolution; this mirrors `MIRDeclEmitter::const_value_expr`'s
     * `AccessChain`-with-`FunctionDecl`-leaf case. `mir_emit_llvm_module`
     * pre-declares every MIR function before this pass runs, so the lookup by
     * mangled name finds the definition this module emits afterwards.
     */
    llvm::Constant* function_ref(ASTNode* linked, llvm::Type* want) {
        if (!linked || linked->kind() != ASTNodeKind::FunctionDecl)
            return llvm::Constant::getNullValue(want);
        const std::string name = mangle_node(linked);
        llvm::Function* lf = gen_.module->getFunction(name);
        if (!lf) return llvm::Constant::getNullValue(want);
        return lf; // opaque pointer
    }

    llvm::Constant* int_constant_ptr(uint64_t num, llvm::Type* want) {
        return llvm::ConstantExpr::getIntToPtr(
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx_), num), want);
    }

    llvm::Constant* int_constant(IntNumValue* v, llvm::Type* want) {
        const uint64_t num = v->get_num_value();
        if (want->isPointerTy()) return int_constant_ptr(num, want);
        if (want->isIntegerTy(1)) return llvm::ConstantInt::get(want, num ? 1 : 0);
        if (want->isIntegerTy()) return llvm::ConstantInt::get(want, num, /*signed=*/true);
        if (want->isFloatTy()) return llvm::ConstantFP::get(want, static_cast<float>(static_cast<int64_t>(num)));
        if (want->isDoubleTy()) return llvm::ConstantFP::get(want, static_cast<double>(static_cast<int64_t>(num)));
        return llvm::Constant::getNullValue(want);
    }

    llvm::Constant* fp_constant(double d, llvm::Type* want) {
        if (want->isFloatTy()) return llvm::ConstantFP::get(want, static_cast<float>(d));
        if (want->isDoubleTy()) return llvm::ConstantFP::get(want, d);
        if (want->isIntegerTy()) return llvm::ConstantInt::get(want, static_cast<uint64_t>(static_cast<int64_t>(d)));
        return llvm::Constant::getNullValue(want);
    }

    llvm::Constant* array_constant(ArrayValue* av, mir::TypeId tid, llvm::Type* want) {
        auto* at = llvm::dyn_cast<llvm::ArrayType>(want);
        if (!at) return llvm::Constant::getNullValue(want);
        const mir::TypeId elem_tid = valid(tid) ? m_.types.get(tid).element : mir::MIR_INVALID_ID;
        llvm::Type* et = at->getElementType();
        std::vector<llvm::Constant*> elems;
        elems.reserve(av->values.size());
        for (Value* e : av->values) elems.push_back(build(e, elem_tid, et));
        while (elems.size() < at->getNumElements())
            elems.push_back(llvm::Constant::getNullValue(et));
        if (elems.size() != at->getNumElements())
            return llvm::Constant::getNullValue(want);
        return llvm::ConstantArray::get(at, elems);
    }

    Value* field_value(StructValue* sv, const std::string& name) {
        if (!sv) return nullptr;
        auto it = sv->values.find(chem::string_view(name.data(), name.size()));
        return it == sv->values.end() ? nullptr : it->second.value;
    }

    /** Build a struct's *default* constant (no explicit value), reading field
     *  defaults from the source `StructDefinition`. */
    llvm::Constant* default_struct(mir::TypeId tid, llvm::StructType* st,
                                   StructDefinition* sd) {
        if (!valid(tid)) return llvm::Constant::getNullValue(st);
        const mir::MIRTypeRecord& r = m_.types.get(tid);
        std::vector<llvm::Constant*> elems;
        unsigned inherit_index = 0;
        for (uint32_t i = 0; i < r.data_count; ++i) {
            const mir::MIRFieldInfo* f = m_.types.field_of(tid, i);
            llvm::Type* ft = (i < st->getNumElements()) ? st->getElementType(i) : nullptr;
            if (!ft) { elems.push_back(llvm::Constant::getNullValue(llvm::Type::getInt8Ty(ctx_))); continue; }
            if (!f) { elems.push_back(llvm::Constant::getNullValue(ft)); continue; }
            const std::string fname = m_.types.field_name_of(*f);
            if (f->kind == static_cast<uint8_t>(mir::MIRFieldKind::Base)) {
                StructDefinition* base = nullptr;
                if (sd && inherit_index < sd->inherited.size())
                    base = sd->inherited[inherit_index].type
                               ? sd->inherited[inherit_index].type->get_direct_linked_struct() : nullptr;
                ++inherit_index;
                elems.push_back(default_struct(f->type, llvm::cast<llvm::StructType>(ft), base));
                continue;
            }
            Value* dv = nullptr;
            if (sd) {
                BaseDefMember* bm = sd->direct_variable(chem::string_view(fname.data(), fname.size()));
                if (bm) dv = bm->default_value();
            }
            elems.push_back(build(dv, f->type, ft));
        }
        if (elems.size() != st->getNumElements()) return llvm::Constant::getNullValue(st);
        return llvm::ConstantStruct::get(st, elems);
    }

    llvm::Constant* struct_fields(StructValue* sv, mir::TypeId tid, llvm::StructType* st,
                                  StructDefinition* sd) {
        if (!valid(tid)) return llvm::Constant::getNullValue(st);
        const mir::MIRTypeRecord& r = m_.types.get(tid);
        std::vector<llvm::Constant*> elems;
        unsigned inherit_index = 0;
        for (uint32_t i = 0; i < r.data_count; ++i) {
            const mir::MIRFieldInfo* f = m_.types.field_of(tid, i);
            llvm::Type* ft = (i < st->getNumElements()) ? st->getElementType(i) : nullptr;
            if (!ft) { elems.push_back(llvm::Constant::getNullValue(llvm::Type::getInt8Ty(ctx_))); continue; }
            if (!f) { elems.push_back(llvm::Constant::getNullValue(ft)); continue; }
            const std::string fname = m_.types.field_name_of(*f);
            if (f->kind == static_cast<uint8_t>(mir::MIRFieldKind::Base)) {
                StructDefinition* base = nullptr;
                if (sd && inherit_index < sd->inherited.size())
                    base = sd->inherited[inherit_index].type
                               ? sd->inherited[inherit_index].type->get_direct_linked_struct() : nullptr;
                ++inherit_index;
                // `Base { ... }` literal names the base type; the interpreter also
                // flattens inherited members into the value, so a bare recursion
                // over `sv` resolves those.
                Value* explicit_base = field_value(sv, fname);
                if (explicit_base && explicit_base->val_kind() == ValueKind::StructValue)
                    elems.push_back(build(explicit_base, f->type, ft));
                else
                    elems.push_back(struct_fields(sv, f->type, llvm::cast<llvm::StructType>(ft), base));
                continue;
            }
            Value* fv = field_value(sv, fname);
            if (!fv && sd) {
                BaseDefMember* bm = sd->direct_variable(chem::string_view(fname.data(), fname.size()));
                if (bm) fv = bm->default_value();
            }
            elems.push_back(build(fv, f->type, ft));
        }
        if (elems.size() != st->getNumElements()) return llvm::Constant::getNullValue(st);
        return llvm::ConstantStruct::get(st, elems);
    }

    llvm::Constant* struct_constant(StructValue* sv, mir::TypeId tid, llvm::Type* want) {
        auto* st = llvm::dyn_cast<llvm::StructType>(want);
        if (!st) return llvm::Constant::getNullValue(want);
        // Resolve defaults structurally, exactly like the C emitter's
        // `const_value_expr` — do NOT run the interpreter here: evaluating a
        // struct initializer can dereference uninitialized memory ("invalid
        // memory access") and abort the whole compile for std/cstd globals.
        StructDefinition* sd = sv->linked_struct();
        if (!sd) {
            BaseType* bt = sv->getRefType();
            if (bt) sd = bt->get_direct_linked_struct();
        }
        return struct_fields(sv, tid, st, sd);
    }

    llvm::Constant* call_constant(Value* value, mir::TypeId tid, llvm::Type* want) {
        Value* call = value;
        if (value->val_kind() == ValueKind::AccessChain) {
            auto* ch = value->as_access_chain_unsafe();
            if (ch->values.empty()) return llvm::Constant::getNullValue(want);
            call = ch->values.back();
            // `ns::ident<int>` / `module::func` used as a value is a chain whose
            // leaf links to the concrete function, not a call.
            if (call && call->val_kind() != ValueKind::FunctionCall)
                return function_ref(call->linked_node(), want);
        }
        if (call && call->val_kind() == ValueKind::FunctionCall) {
            auto* fc = call->as_func_call_unsafe();
            ASTNode* vleaf = nullptr;
            if (fc->parent_val) {
                if (fc->parent_val->val_kind() == ValueKind::AccessChain) {
                    auto* ch = fc->parent_val->as_access_chain_unsafe();
                    if (!ch->values.empty()) vleaf = ch->values.back()->linked_node();
                } else {
                    vleaf = fc->parent_val->linked_node();
                }
            }
            if (vleaf && vleaf->kind() == ASTNodeKind::VariantMember && want->isStructTy())
                return variant_constant(vleaf->as_variant_member_unsafe(), fc, tid,
                                        llvm::cast<llvm::StructType>(want));
        }
        // Fall back to folding a comptime call through the interpreter.
        if (call && call->val_kind() == ValueKind::FunctionCall && scope_.global) {
            Value* folded = call->evaluated_value(scope_);
            if (folded && folded != call) return build(folded, tid, want);
        }
        return llvm::Constant::getNullValue(want);
    }

    llvm::Constant* variant_constant(VariantMember* vm, FunctionCall* call, mir::TypeId tid,
                                     llvm::StructType* st) {
        if (!valid(tid)) return llvm::Constant::getNullValue(st);
        VariantDefinition* vd = vm->parent();
        const long idx = vd ? vd->direct_child_index(vm->name) : 0;
        const mir::MIRTypeRecord& r = m_.types.get(tid);
        const std::string member_name(vm->name.data(), vm->name.size());
        std::vector<llvm::Constant*> elems;
        for (uint32_t i = 0; i < r.data_count; ++i) {
            const mir::MIRFieldInfo* f = m_.types.field_of(tid, i);
            llvm::Type* ft = (i < st->getNumElements()) ? st->getElementType(i) : nullptr;
            if (!ft) { elems.push_back(llvm::Constant::getNullValue(llvm::Type::getInt8Ty(ctx_))); continue; }
            if (!f) { elems.push_back(llvm::Constant::getNullValue(ft)); continue; }
            switch (static_cast<mir::MIRFieldKind>(f->kind)) {
                case mir::MIRFieldKind::Base: {
                    StructDefinition* base = nullptr;
                    size_t bi = 0;
                    if (vd) {
                        for (; bi < vd->inherited.size(); ++bi) {
                            StructDefinition* b = vd->inherited[bi].type
                                                      ? vd->inherited[bi].type->get_direct_linked_struct() : nullptr;
                            if (!b) continue;
                            const chem::string_view nv = b->name_view();
                            if (m_.types.field_name_of(*f) == std::string(nv.data(), nv.size())) {
                                base = b; break;
                            }
                        }
                    }
                    elems.push_back(default_struct(f->type, llvm::cast<llvm::StructType>(ft), base));
                    break;
                }
                case mir::MIRFieldKind::VariantTag:
                    elems.push_back(llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(ctx_), static_cast<uint64_t>(idx < 0 ? 0 : idx)));
                    break;
                case mir::MIRFieldKind::VariantCase: {
                    const std::string cname = m_.types.field_name_of(*f);
                    if (cname != member_name) { elems.push_back(llvm::Constant::getNullValue(ft)); break; }
                    auto* cst = llvm::dyn_cast<llvm::StructType>(ft);
                    if (!cst) { elems.push_back(llvm::Constant::getNullValue(ft)); break; }
                    std::vector<llvm::Constant*> case_elems;
                    size_t ai = 0;
                    for (auto& entry : vm->values) {
                        VariantMemberParam* param = entry.second;
                        Value* arg = nullptr;
                        if (ai < call->values.size()) arg = call->values[ai];
                        else if (param) arg = param->def_value;
                        ++ai;
                        const mir::MIRFieldInfo* cf = m_.types.field_of(f->type, static_cast<uint32_t>(case_elems.size()));
                        llvm::Type* cft = (case_elems.size() < cst->getNumElements())
                                              ? cst->getElementType(case_elems.size()) : nullptr;
                        if (!cft) break;
                        case_elems.push_back(build(arg, cf ? cf->type : mir::MIR_INVALID_ID, cft));
                    }
                    if (case_elems.size() != cst->getNumElements())
                        elems.push_back(llvm::Constant::getNullValue(ft));
                    else
                        elems.push_back(llvm::ConstantStruct::get(cst, case_elems));
                    break;
                }
                default:
                    elems.push_back(llvm::Constant::getNullValue(ft));
                    break;
            }
        }
        if (elems.size() != st->getNumElements()) return llvm::Constant::getNullValue(st);
        return llvm::ConstantStruct::get(st, elems);
    }
};

} // namespace

static void mir_emit_llvm_module(Codegen& gen, LabModule* module,
                                 mir::MIRTypeBuilder& types, mir::MIRModule& m) {
    auto mangle = [&gen](ASTNode* n) -> std::string {
        BufferedWriter w(256);
        gen.mangler.mangle(w, n);
        return std::string(w.data(), w.getPosition());
    };

    std::vector<ASTNode*> nodes;
    for (auto& f : module->direct_files) {
        for (ASTNode* n : f.result->unit.scope.body.nodes) nodes.push_back(n);
    }
    if (getenv("MIR_LLVM_TRACE"))
        std::cerr << "[MIR-LLVM] emitting module " << module->format()
                  << " functions=" << m.functions.size() << "\n";

    gen.mir_reset_caches();
    // Debug info: one DICompileUnit per module (the MIR path used to emit none).
    if (gen.di.isEnabled) {
        const std::string cu_path =
            module->direct_files.empty() ? std::string() : module->direct_files.front().abs_path;
        llvm::DICompileUnit* cu = gen.di.createDiCompileUnit(chem::string_view(cu_path));
        gen.di.start_di_compile_unit(cu);
        gen.mir_di_file_ = cu ? cu->getFile() : nullptr;
    }
    gen.mir_declare_globals(m);
    // pre-declare all functions so vtable initializers can reference them
    for (const mir::MIRFunction& fn : m.functions) gen.mir_function(m, fn.symbol);

    // ── interface vtables ───────────────────────────────────────────────────
    auto vtable_type_name = [&](InterfaceDefinition* iface) -> std::string {
        return "__chx_" + mangle(iface) + "_vt_t";
    };
    auto is_agg = [&](mir::TypeId t) -> bool {
        if (t >= m.types.size()) return false;
        const mir::MIRTypeRecord& r = m.types.get(t);
        if (r.flags & mir::TF_TYPEDEF) return false;
        return r.kind == mir::MIRTypeKind::Struct || r.kind == mir::MIRTypeKind::Union ||
               r.kind == mir::MIRTypeKind::Variant;
    };
    std::function<std::vector<FunctionDeclaration*>(InterfaceDefinition*)> slots_of =
        [&](InterfaceDefinition* iface) -> std::vector<FunctionDeclaration*> {
        std::vector<FunctionDeclaration*> out;
        for (auto& inh : iface->inherited) {
            InterfaceDefinition* inherited =
                inh.type ? inh.type->get_direct_linked_interface() : nullptr;
            if (inherited) {
                auto s = slots_of(inherited);
                out.insert(out.end(), s.begin(), s.end());
            }
        }
        for (FunctionDeclaration* f : iface->instantiated_functions()) out.push_back(f);
        return out;
    };
    auto slot_ftype = [&](FunctionDeclaration* func) -> llvm::FunctionType* {
        FunctionParam* self = func->get_self_param();
        mir::TypeId ret = func->returnType ? types.map(func->returnType) : types.void_type();
        const bool sret = is_agg(ret);
        std::vector<mir::TypeId> ptypes;
        if (self) ptypes.push_back(types.pointer_type(types.void_type(), true));
        if (sret) ptypes.push_back(types.pointer_type(ret, true));
        bool skipped_self = false;
        for (FunctionParam* p : func->params) {
            if (p == self && !skipped_self) { skipped_self = true; continue; }
            mir::TypeId pt = p && p->type ? types.map(p->type) : types.opaque_type();
            if (pt < m.types.size()) {
                if (m.types.get(pt).kind == mir::MIRTypeKind::Array) {
                    pt = types.pointer_type(m.types.get(pt).element, true);
                } else if (is_agg(pt)) {
                    pt = types.pointer_type(pt, true);
                }
            }
            ptypes.push_back(pt);
        }
        mir::TypeId stype = types.function_signature(sret ? types.void_type() : ret, ptypes,
                                                     func->isVariadic());
        return gen.mir_fn_type(m, stype);
    };
    for (ASTNode* n : nodes) {
        if (n->kind() != ASTNodeKind::InterfaceDecl) continue;
        auto* iface = n->as_interface_def_unsafe();
        const std::string tn = vtable_type_name(iface);
        auto slots = slots_of(iface);
        std::vector<llvm::FunctionType*> ftys;
        std::vector<std::string> names;
        for (FunctionDeclaration* f : slots) {
            ftys.push_back(slot_ftype(f));
            names.push_back(std::string(f->name_view().data(), f->name_view().size()));
        }
        gen.mir_vtable_struct(tn, ftys);
        gen.mir_vtable_methods_[tn] = names;
    }
    for (ASTNode* n : nodes) {
        if (n->kind() != ASTNodeKind::ImplDecl) continue;
        auto* impl = n->as_impl_def_unsafe();
        InterfaceDefinition* iface =
            impl->interface_type ? impl->interface_type->get_direct_linked_interface() : nullptr;
        if (!iface) continue;
        if (iface->is_static()) continue; // static interfaces have no vtable
        MembersContainer* container_mc =
            impl->struct_type ? impl->struct_type->get_members_container() : nullptr;
        ExtendableMembersContainerNode* container =
            container_mc ? container_mc->as_extendable_members_container_unsafe() : nullptr;
        if (!container) {
            // Primitive impl (`impl Interface for int`): there is no members
            // container, so the vtable is built from the impl's own methods —
            // mirroring `MIRDeclEmitter::create_vtable_primitive`. Without this
            // the `dyn<I>` fat pointer's `second` slot had no global to point at
            // and the store was dropped (a garbage vtable → SIGSEGV on dispatch).
            if (!iface->generates_vtable()) continue;
            const std::string tn = vtable_type_name(iface);
            std::string gname;
            {
                BufferedWriter w(256);
                gen.mangler.mangle_vtable_name(w, iface, impl->struct_type);
                gname.assign(w.data(), w.getPosition());
            }
            auto slots = slots_of(iface);
            std::vector<llvm::FunctionType*> ftys;
            bool ok = true;
            for (FunctionDeclaration* f : slots) {
                llvm::FunctionType* sft = slot_ftype(f);
                if (!sft) { ok = false; break; }
                ftys.push_back(sft);
            }
            if (!ok || ftys.empty()) continue;
            llvm::StructType* vts = gen.mir_vtable_struct(tn, ftys);
            if (vts->getNumElements() != slots.size()) continue;
            std::vector<llvm::Constant*> init;
            for (size_t si = 0; si < slots.size(); ++si) {
                FunctionDeclaration* f = slots[si];
                llvm::Function* lf = nullptr;
                if (f->has_self_param()) {
                    FunctionDeclaration* overrider = impl->direct_child_function(f->name_view());
                    if (overrider) lf = gen.module->getFunction(mangle(overrider));
                }
                init.push_back(lf ? static_cast<llvm::Constant*>(lf)
                                  : llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
                                        vts->getElementType(si))));
            }
            if (gen.mir_vtable_methods_.find(tn) == gen.mir_vtable_methods_.end()) {
                std::vector<std::string> slot_names;
                for (FunctionDeclaration* f : slots)
                    slot_names.emplace_back(f->name_view().data(), f->name_view().size());
                gen.mir_vtable_methods_[tn] = std::move(slot_names);
            }
            // External linkage: the vtable name is program-unique (it embeds
            // both the interface's and the impl type's mangled names), and a
            // `dyn<I>` built in *another* module references it — an internal
            // definition cannot satisfy that reference across per-module LLVM
            // objects (the C backend emits one translation unit, so `static`
            // was enough there).
            auto* gv = new llvm::GlobalVariable(
                *gen.module, vts, true, llvm::GlobalValue::ExternalLinkage,
                llvm::ConstantStruct::get(vts, init), gname);
            gen.mir_register_named_global(gname, gv);
            continue;
        }
        const std::string tn = vtable_type_name(iface);
        std::string gname;
        {
            BufferedWriter w(256);
            gen.mangler.mangle_vtable_name(w, iface, container);
            gname.assign(w.data(), w.getPosition());
        }
        auto slots = slots_of(iface);
        std::vector<llvm::Constant*> init;
        bool ok = true;
        for (FunctionDeclaration* f : slots) {
            FunctionDeclaration* overrider = container->direct_child_function(f->name_view());
            std::string fname;
            if (overrider) {
                fname = mangle(overrider);
            } else {
                ExtendableMembersContainerNode* prev = iface->active_user;
                iface->active_user = container;
                fname = mangle(f);
                iface->active_user = prev;
            }
            llvm::Function* lf = gen.module->getFunction(fname);
            if (!lf) { ok = false; break; }
            init.push_back(lf);
        }
        if (!ok || init.empty()) continue;
        // Build (or fetch) the vtable struct with its real slot types. The
        // interface declaration may live in a *different* module than the impl,
        // so the type must be registered here from the impl's interface.
        std::vector<llvm::FunctionType*> ftys;
        for (FunctionDeclaration* f : slots) {
            llvm::FunctionType* sft = slot_ftype(f);
            if (!sft) { ok = false; break; }
            ftys.push_back(sft);
        }
        if (!ok) continue;
        llvm::StructType* vts = gen.mir_vtable_struct(tn, ftys);
        if (vts->getNumElements() != init.size()) continue;
        // `DynamicCall` resolves the method name to a slot index via this map;
        // the interface may be declared in another module, so make sure the
        // slot-name list is registered for the impl's interface too.
        if (gen.mir_vtable_methods_.find(tn) == gen.mir_vtable_methods_.end()) {
            std::vector<std::string> slot_names;
            for (FunctionDeclaration* f : slots) {
                slot_names.emplace_back(f->name_view().data(), f->name_view().size());
            }
            gen.mir_vtable_methods_[tn] = std::move(slot_names);
        }
        auto* gv = new llvm::GlobalVariable(
            *gen.module, vts, true, llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantStruct::get(vts, init), gname);
        gen.mir_register_named_global(gname, gv);
    }

    // Apply global initializers from the AST (MIR symbol records do not carry
    // initializer values yet). `mir_declare_globals` gave every internal global a
    // null initializer; overwrite it with the value `const_value_expr` would
    // spell in C. Never touch an `@extern` global: it is declared (not defined)
    // in this module, and an initializer would make LLVM emit a definition that
    // interposes the real symbol (e.g. libc's `stdout`).
    {
        std::unordered_map<std::string, mir::TypeId> global_type;
        std::unordered_map<std::string, uint32_t> global_sym;
        for (uint32_t i = 0; i < m.symbols.size(); ++i) {
            const mir::MIRSymbolRecord& rec = m.symbols.get(i);
            if (rec.kind != mir::MIRSymbolKind::Global && rec.kind != mir::MIRSymbolKind::External)
                continue;
            if (rec.type == mir::MIR_INVALID_ID) continue;
            const std::string gname = m.symbols.mangled_name(rec);
            global_type[gname] = rec.type;
            global_sym[gname] = i;
        }
        // Descend namespaces / taken comptime branches / resolved CBI nodes,
        // mirroring `MIRDeclEmitter::declare`: a global may be declared inside a
        // namespace (`namespace wrapped_variable { const thing = 3456 }`).
        std::vector<VarInitStatement*> globals;
        std::function<void(const std::vector<ASTNode*>&)> collect =
            [&](const std::vector<ASTNode*>& ns) {
            for (ASTNode* n : ns) {
                if (!n) continue;
                switch (n->kind()) {
                    case ASTNodeKind::VarInitStmt:
                        globals.push_back(n->as_var_init_unsafe());
                        break;
                    case ASTNodeKind::NamespaceDecl: {
                        auto* nsd = n->as_namespace_unsafe();
                        if (!nsd->is_comptime()) collect(nsd->nodes);
                        break;
                    }
                    case ASTNodeKind::Scope:
                        collect(n->as_scope_unsafe()->nodes);
                        break;
                    case ASTNodeKind::IfStmt: {
                        auto* st = n->as_if_stmt_unsafe();
                        if (st->is_top_level() && st->computed_scope.has_value()) {
                            Scope* taken = st->computed_scope.value();
                            if (taken) collect(taken->nodes);
                        }
                        break;
                    }
                    case ASTNodeKind::EmbeddedNode: {
                        ASTNode* r = n->as_embedded_node_unsafe()->replacement;
                        if (r) collect({r});
                        break;
                    }
                    default: break;
                }
            }
        };
        collect(nodes);
        GlobalConstantLowerer lowerer(gen, m, gen.comptime_scope);
        for (VarInitStatement* vi : globals) {
            if (!vi->value || vi->is_comptime() || vi->is_extern()) continue;
            const std::string name = mangle(vi);
            llvm::GlobalVariable* gv = gen.module->getNamedGlobal(name);
            if (!gv) continue;
            if (gv->hasInitializer() && gv->getInitializer() &&
                !llvm::isa<llvm::ConstantAggregateZero>(gv->getInitializer()) &&
                !gv->getInitializer()->isNullValue()) {
                continue; // already defined by another pass
            }
            auto it = global_type.find(name);
            const mir::TypeId gt = (it != global_type.end()) ? it->second : mir::MIR_INVALID_ID;
            llvm::Constant* c = lowerer.build(vi->value, gt, gv->getValueType());
            if (getenv("MIR_LLVM_TRACE"))
                std::cerr << "[MIR-LLVM] global init " << name << " vk="
                          << static_cast<int>(vi->value->val_kind()) << "\n";
            if (!c) continue;
            if (c->getType() == gv->getValueType()) {
                gv->setInitializer(c);
                continue;
            }
            // An unsized `[0 x T]` global whose initializer carries the real
            // extent (`const x : char[] = "..."`): rebuild it with the inferred
            // type so indexing/GEP sees the data, as C's `char x[] = "..."` does.
            auto* old_at = llvm::dyn_cast<llvm::ArrayType>(gv->getValueType());
            auto* new_at = llvm::dyn_cast<llvm::ArrayType>(c->getType());
            if (old_at && new_at && old_at->getNumElements() == 0 &&
                new_at->getNumElements() > 0) {
                auto* ng = new llvm::GlobalVariable(*gen.module, c->getType(),
                                                    gv->isConstant(), gv->getLinkage(), c,
                                                    name);
                ng->setAlignment(gv->getAlign());
                auto sit = global_sym.find(name);
                if (sit != global_sym.end()) gen.mir_global_cache_[sit->second] = ng;
                gv->eraseFromParent();
            }
        }
    }
    for (const mir::MIRFunction& fn : m.functions) {
        if (!gen.mir_emit_function(m, fn)) {
            std::cerr << "[MIR-LLVM] LLVM emission failed\n";
            return;
        }
    }
    if (gen.di.isEnabled) {
        gen.di.end_di_compile_unit();
        if (gen.diBuilder) gen.diBuilder->finalize();
    }
    if (getenv("MIR_LLVM_DUMP_IR")) {
        std::string path = std::string(getenv("MIR_LLVM_DUMP_IR"));
        std::error_code ec;
        llvm::raw_fd_ostream os(path, ec);
        gen.module->print(os, nullptr);
    }
}

int ASTProcessor::mir_compile_module(Codegen& gen, LabModule* module) {
    // Drive the *same* MIR construction the C backend uses (declarations +
    // implementations, including impls, generic instantiations, CBI embedded
    // nodes and async), but collect the lowered functions instead of emitting C
    // bodies; the collected MIR is then emitted as LLVM IR by the finish hook.
    CTranslationContext c_visitor(gen.binder, gen.comptime_scope, gen.mangler, gen.allocator,
                                  loc_man, gen.coreNodes, gen.implsIndex,
                                  options->debug_info, /*minify=*/false);
    // CBI macro replacements are stored on their `EmbeddedNode` and re-read at
    // the end of the module (in `mir_emit_llvm_module`), after `file_level_reset()`
    // has cleared the file allocator. Allocate them in the module allocator, which
    // lives until the module has been fully emitted.
    c_visitor.embedded_replacement_allocator = &mod_allocator;
    c_visitor.prepare_translate();

    mir_llvm_collect = true;
    mir_llvm_finish = [&](mir::MIRTypeBuilder& types, mir::MIRModule& m) {
        mir_emit_llvm_module(gen, module, types, m);
    };

    int result = declare_module(c_visitor, module);
    if (result == 0) result = implement_module(c_visitor, module);

    mir_llvm_collect = false;
    mir_llvm_finish = nullptr;
    return result;
}

void ASTProcessor::code_gen_declare(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Declare", &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Declare");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_compile(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.compile_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Compile", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement_declare(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_implement_declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "ExtDeclare", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "ExtDeclare");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_implement_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "ExtCompile", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "ExtCompile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::declare_and_compile(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    // create a di compile unit
    if(gen.di.isEnabled) {
        const auto compileUnit = gen.di.createDiCompileUnit(chem::string_view(abs_path.data(), abs_path.size()));
        gen.di.start_di_compile_unit(compileUnit);
        gen.declare_and_compile(nodes_vec);
        gen.di.end_di_compile_unit();
    } else {
        gen.declare_and_compile(nodes_vec);
    }
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Compile", &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::external_declare_nodes(
        Codegen& gen,
        Scope& import_res,
        const std::string& abs_path
) {
    auto& nodes_vec = import_res.nodes;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Declare", &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Declare");
        gen.diagnostics.clear();
    }
}

int ASTProcessor::compile_module(
    Codegen& gen,
    LabModule* module
) {

    // MIR-driven LLVM backend (migration): build MIR and emit LLVM IR from it.
    // The AST-based lowering below is retained only until the MIR path covers
    // every construct, then it is deleted.
    return mir_compile_module(gen, module);

    // let's create a flat vector of direct dependencies, that we want to process
    std::vector<LabModule*> dependencies;
    shallow_dedupe_sorted(dependencies, module->dependencies);

    // creating a compile unit
    const auto compileUnit = gen.di.createDiCompileUnit(module->direct_files.empty() ? chem::string_view("") : chem::string_view(module->direct_files.front().abs_path));
    gen.di.start_di_compile_unit(compileUnit);

    // we will declare the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& metaFile : dep->direct_files) {

            auto& file = *metaFile.result;
            auto& body = file.unit.scope.body;
            auto& abs_path = file.abs_path;

            if (gen.di.isEnabled) {

                // start the file scope
                gen.di.start_file_scope(metaFile.result);

                // declare external nodes
                external_declare_nodes(gen, body, file.abs_path);

                // end the file scope
                gen.di.end_file_scope();

            } else {

                // declare external nodes
                external_declare_nodes(gen, body, file.abs_path);

            }

        }
    }

    // The second loop deals with files that are within current module
    // We just declare the files in current module in this loop that's it, we create prototypes of structs and functions
    // in the next loop we will finally create bodies
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

        auto& abs_path = file.abs_path;

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(&result);

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // these are the generically monomorphized instantiations
    // that were created by this module
    auto& instantiations = container.get_current_module_instantiations();

    // we track for which files we created new di units
    // we must set them to null (so they don't get reused)
    std::vector<ASTFileResult*> newly_created_di_units;

    // declaring the generic instantiations created in this module
    for (const auto node : instantiations) {
        const auto file_scope = node->get_file_scope();
        if (file_scope == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file scope from a generically monomorphized declaration");
#endif
            continue;
        }
        const auto result = file_scope->meta.result;
        if (result == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file result from a generically monomorphized declaration");
#endif
            continue;
        }

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(result);

            // just declare
            node->code_gen_declare(gen);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // just declare
            node->code_gen_declare(gen);

        }

    }

    // implementing the generic instantiations created in this module
    for (const auto node : instantiations) {
        const auto file_scope = node->get_file_scope();
        if (file_scope == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file scope from a generically monomorphized declaration");
#endif
            continue;
        }
        const auto result = file_scope->meta.result;
        if (result == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file result from a generically monomorphized declaration");
#endif
            continue;
        }

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(result);

            // implement the node
            node->code_gen(gen);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // implement the node
            node->code_gen(gen);

        }

    }

    // The fourth loop also only compiles the files that present inside this module
    // This loop will compile the bodies of the functions inside the current module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(&result);

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // end the di compile unit
    // this causes struct types to die
    gen.di.end_di_compile_unit();
    // this is really important
    // it removes cached types, cached scopes
    // if this is not called, freed memory can make into next module's compilation
    gen.di.finalize();
    // clear the generic instantiations for this module in the container
    // so these don't get generated when generating code for the next module
    container.clear_current_module_instantiations();
    // here we store struct types, function callee values to reuse them inside module
    // clearing this after module has compiled prevents reusing function callee's created
    // for this module, forces us to redeclare functions and structs in external module
    gen.mod_ptr_cache.clear();
    // here we store pointers (function callee values) for functions in different implementations
    // llvm pointers for functions inside impl blocks (that implement interfaces for structs) are stored here
    // clearing this has the same advantage of preventing reuse
    gen.trait_impl_func_map.clear();

    return 0;

}