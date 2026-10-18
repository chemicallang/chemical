#include "MIRLowererInternal.h"
// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"
#include "MIRABI.h"
#include "MIRPrototype.h"

#include <cstdlib>
#include <iostream>
#include <optional>

#include "ast/base/BaseType.h"
#include "ast/types/GenericType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/BitwiseNot.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/DynamicValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/values/ValueNode.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/Scope.h"
#include "ast/structures/BlockScope.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/EnumMember.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/InlineAsmStmt.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/values/IncDecValue.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/OffsetOfValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/ZeroedValue.h"
#include "ast/values/IfValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/ExtractionValue.h"
#include "ast/values/NewTypedValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"
#include "ast/statements/DestructStmt.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/EmbeddedValue.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/values/SwitchValue.h"
#include "ast/values/LoopValue.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/values/InValue.h"
#include "ast/values/BlockValue.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/Return.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/values/AwaitExpression.h"
#include "compiler/async/AsyncCTypes.h"
#include "compiler/async/AwaitNormalizePass.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/ForInLoop.h"
#include "ast/structures/InheritedType.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/StructType.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/symres/CoreNodes.h"
#include "MIREmitter.h"

namespace mir {
MIRExprResult MIRLowerer::lower_captured_variable(CapturedVariable* cap, bool as_address,
                                                  std::string& error) {
    if (lambda_closure_ptr_ == MIR_NULL) {
        error = "captured variable used outside its lambda";
        return MIRExprResult::error();
    }
    const chem::string_view nm = cap->name;
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, nm.data(), static_cast<uint32_t>(nm.size()));
    BaseType* ct = cap->known_type();
    const TypeId ft = types_.map(ct);
    const ValueId ptr = lambda_closure_ptr_;
    if (cap->capture_by_ref) {
        // the field is a pointer to the referent; the captured variable is that
        // reference, so its value is the pointer itself
        return MIRExprResult::value(builder_->field_load_ptr(ptr, fc, ft), ft);
    }
    const MIRTypeRecord& fr = module_.types.get(ft);
    if (fr.kind == MIRTypeKind::Pointer || fr.kind == MIRTypeKind::Reference) {
        // a by-value pointer/reference capture is the pointer itself (e.g. used
        // as a method receiver or indexed), never the field's address
        return MIRExprResult::value(builder_->field_load_ptr(ptr, fc, ft), ft);
    }
    if (as_address || abi::is_aggregate(module_, ft)) {
        return MIRExprResult::address(builder_->field_addr_ptr(ptr, fc, ft), ft);
    }
    return MIRExprResult::value(builder_->field_load_ptr(ptr, fc, ft), ft);
}

MIRExprResult MIRLowerer::lower_extraction_value(ExtractionValue* ev, std::string& error) {
    Value* src = ev ? ev->value : nullptr;
    if (!src || src->val_kind() != ValueKind::LambdaFunc) {
        error = "extraction value source must be a lambda";
        return MIRExprResult::error();
    }
    auto* lamb = src->as_lambda_func_unsafe();
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    if (lamb->captureList.empty()) {
        // a non-capturing lambda has no closure: null data/destructor, size 0
        switch (ev->extractionKind) {
            case ExtractionKind::LambdaFnPtr:
                return materialize_noncapturing_lambda(lamb, error);
            case ExtractionKind::LambdaCapturedPtr:
            case ExtractionKind::LambdaCapturedDestructor:
                return MIRExprResult::value(
                    builder_->const_null(void_ptr, module_.constants.add_null(void_ptr)), void_ptr);
            case ExtractionKind::SizeOfLambdaCaptured:
                return MIRExprResult::value(
                    builder_->const_int(types_.u64_type(),
                                        module_.constants.add_int(types_.u64_type(), 0)),
                    types_.u64_type());
            case ExtractionKind::AlignOfLambdaCaptured:
                return MIRExprResult::value(
                    builder_->const_int(types_.u64_type(),
                                        module_.constants.add_int(types_.u64_type(), 1)),
                    types_.u64_type());
            default:
                error = "unsupported extraction value kind";
                return MIRExprResult::error();
        }
    }
    if (!materialize_capturing_lambda(lamb, error)) return MIRExprResult::error();
    const LambdaMaterialization& m = lambda_materializations_[lamb];
    switch (ev->extractionKind) {
        case ExtractionKind::LambdaFnPtr:
            return MIRExprResult::value(m.fn_ptr, m.fn_ptr_type);
        case ExtractionKind::LambdaCapturedPtr: {
            if (m.cap_temp == MIR_INVALID_ID) return MIRExprResult::value(
                builder_->const_null(void_ptr, module_.constants.add_null(void_ptr)), void_ptr);
            const TypeId ptrt = types_.pointer_type(m.cap_type, true);
            return MIRExprResult::value(builder_->address_of(m.cap_temp, ptrt), ptrt);
        }
        case ExtractionKind::LambdaCapturedDestructor:
            if (m.dtor_ptr == MIR_NULL) {
                return MIRExprResult::value(
                    builder_->const_null(void_ptr, module_.constants.add_null(void_ptr)), void_ptr);
            }
            return MIRExprResult::value(m.dtor_ptr, m.dtor_type);
        case ExtractionKind::SizeOfLambdaCaptured:
            return MIRExprResult::value(builder_->size_of(m.cap_type, types_.u64_type()),
                                        types_.u64_type());
        case ExtractionKind::AlignOfLambdaCaptured:
            return MIRExprResult::value(builder_->align_of(m.cap_type, types_.u64_type()),
                                        types_.u64_type());
        default:
            error = "unsupported extraction value kind";
            return MIRExprResult::error();
    }
}

MIRExprResult MIRLowerer::lower_capturing_function_call(MIRExprResult recv, BaseType* recv_type,
                                                        FunctionCall* call, std::string& error) {
    if (!recv.ok()) return recv;
    BaseType* cc = recv_type ? recv_type->canonical() : nullptr;
    CapturingFunctionType* cap = cc ? cc->get_cap_func_type() : nullptr;
    BaseType* inst = cap ? const_cast<BaseType*>(cap->instance_type.getType()) : nullptr;
    if (!inst && cc) {
        // `std::function<T>` may resolve to the instance struct directly
        ASTNode* cln = cc->get_direct_linked_canonical_node();
        if (cln && cln->kind() == ASTNodeKind::StructDecl) {
            inst = cln->as_struct_def_unsafe()->known_type();
        }
    }
    if (!inst) {
        error = "capturing function instance could not be resolved";
        return MIRExprResult::error();
    }
    FunctionDeclaration* get_fn = resolve_method_fallback(inst, "get_fn_ptr");
    FunctionDeclaration* get_data = resolve_method_fallback(inst, "get_data_ptr");
    if (!get_fn || !get_data) {
        error = "capturing function instance methods not found";
        return MIRExprResult::error();
    }
    auto call_method = [&](FunctionDeclaration* m) -> MIRExprResult {
        VariableIdentifier id(chem::string_view(), nullptr, ZERO_LOC);
        id.linked = m;
        FunctionCall sc(&id, m->returnType, ZERO_LOC);
        return lower_method_call(recv, cc, &sc, error);
    };
    MIRExprResult fnr = call_method(get_fn);
    if (!fnr.ok()) return fnr;
    MIRExprResult dr = call_method(get_data);
    if (!dr.ok()) return dr;
    const TypeId vpt = types_.pointer_type(types_.void_type(), true);
    const ValueId fnptr = (fnr.kind == MIRExprKind::Place)
                              ? builder_->load(static_cast<PlaceId>(fnr.id), fnr.type)
                              : static_cast<ValueId>(fnr.id);
    const ValueId dataptr = (dr.kind == MIRExprKind::Place)
                                ? builder_->load(static_cast<PlaceId>(dr.id), dr.type)
                                : static_cast<ValueId>(dr.id);
    MIRArgScope _ag_scope(*this);
    std::vector<MIROperand>& cargs = _ag_scope.v;
    cargs.push_back(MIROperand::value(dataptr, vpt));
    // The closure's AST function type carries the parameter types, so pass them
    // down: `lower_call_args_for` uses them to tell a by-value aggregate
    // argument (the callee materialises and destroys its own copy) from one
    // passed by reference (the caller still owns the temporary). Without them
    // every destructible temporary argument was registered as caller-owned, so
    // both sides destroyed it — a double free (`http::server` handing a
    // `req_opt.take()` temporary to a `std::function` route handler).
    FunctionType* cap_ft = cap ? cap->get_func_type() : nullptr;
    const size_t ccap = call_temps_.size();
    if (!lower_call_args_for(nullptr, false, call->values, cargs, error, cap_ft)) {
        return MIRExprResult::error();
    }
    const TypeId crt = types_.map(call->getType());
    MIRScratch<TypeId> _cptypes(*this);
    std::vector<TypeId>& cptypes = _cptypes.v;
    for (auto& op : cargs) cptypes.push_back(op.type());
    const TypeId cftype = types_.function_signature(crt, cptypes);
    const ValueId cfn = builder_->cast(fnptr, cftype);
    if (is_void_type(module_, crt)) {
        builder_->call_indirect(cfn, crt, cargs.data(), static_cast<uint32_t>(cargs.size()));
        destroy_call_temps_from(ccap, error);
        return MIRExprResult::void_result();
    }
    if (abi::needs_sret(module_, crt)) {
        const PlaceId tmp = builder_->alloca(crt, MIRStorageClass::Temporary);
        builder_->zero_init(tmp);
        // the legacy capturing-function ABI is `(this, sret, params)`, so the
        // sret pointer is a regular leading argument after the data pointer
        const TypeId sptr_t = types_.pointer_type(crt, true);
        MIRScratch<TypeId> _stypes(*this);
        std::vector<TypeId>& stypes = _stypes.v;
        stypes.push_back(vpt);
        stypes.push_back(sptr_t);
        for (size_t i = 1; i < cargs.size(); ++i) stypes.push_back(cargs[i].type());
        const TypeId sftype = types_.function_signature(types_.void_type(), stypes);
        const ValueId scfn = builder_->cast(fnptr, sftype);
        MIRArgScope _sargs_scope(*this);
        std::vector<MIROperand>& sargs = _sargs_scope.v;
        sargs.push_back(cargs[0]);
        sargs.push_back(MIROperand::value(builder_->address_of(tmp, sptr_t), sptr_t));
        for (size_t i = 1; i < cargs.size(); ++i) sargs.push_back(cargs[i]);
        builder_->call_indirect(scfn, types_.void_type(), sargs.data(),
                                static_cast<uint32_t>(sargs.size()));
        destroy_call_temps_from(ccap, error);
        return MIRExprResult::place(tmp, crt);
    }
    const ValueId cv =
        builder_->call_indirect(cfn, crt, cargs.data(), static_cast<uint32_t>(cargs.size()));
    destroy_call_temps_from(ccap, error);
    return MIRExprResult::value(cv, crt);
}

bool MIRLowerer::materialize_capturing_lambda(LambdaFunction* lamb, std::string& error) {
    if (lambda_materializations_.find(lamb) != lambda_materializations_.end()) return true;
    if (!lamb || lamb->captureList.empty()) {
        error = "materialize_capturing_lambda on a non-capturing lambda";
        return false;
    }

    static uint64_t s_counter = 0;
    const std::string lname = "__chemda_cap_" + std::to_string(s_counter++);
    const std::string cap_name = lname + "_cap";

    // 1. the closure struct definition + type
    const TypeId cap_type = types_.named_struct(cap_name);
    std::string sdef = "struct " + cap_name + " {\n";
    for (CapturedVariable* cap : lamb->captureList) {
        const TypeId ft = types_.map(cap->known_type());
        sdef += "    " + c_type_of(module_, ft) + " " +
                std::string(cap->name.data(), cap->name.size()) + ";\n";
    }
    sdef += "};\n";
    lambda_defs_.push_back(sdef);

    // 1b. the closure struct's *runtime* fields must also live in the MIR type
    // table: the C emitter spells accesses from the struct text above, but the
    // LLVM emitter lays the struct out and resolves field names from the table.
    // Without this the closure LLVM type is an opaque `{ i8 }` and every
    // `self->capture` GEP is out of bounds.
    {
        std::vector<MIRFieldInfo> cap_fields;
        cap_fields.reserve(lamb->captureList.size());
        for (CapturedVariable* cap : lamb->captureList) {
            const TypeId ft = types_.map(cap->known_type());
            MIRFieldInfo fi;
            fi.type = ft;
            fi.kind = static_cast<uint8_t>(MIRFieldKind::Direct);
            fi.index = static_cast<uint32_t>(cap_fields.size());
            fi.name_offset = module_.types.append_field_name(cap->name.data(), static_cast<uint32_t>(cap->name.size()));
            fi.name_length = static_cast<uint32_t>(cap->name.size());
            cap_fields.push_back(fi);
        }
        const uint32_t cfidx = module_.types.append_fields(cap_fields);
        module_.types.update(cap_type, [&](MIRTypeRecord& cr) {
            cr.data_count = static_cast<uint32_t>(cap_fields.size());
            cr.field_index = cfidx;
        });
        // a closure struct is an aggregate too: give it its layout
        compute_aggregate_layout(module_.types, cap_type);
    }

    // 2. the closure destructor
    const std::string dname = cap_name + "_destr";
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    MIRSymbolRecord drec;
    drec.kind = MIRSymbolKind::Function;
    drec.linkage = MIRLinkage::Internal;
    drec.type = types_.function_signature(types_.void_type(), {void_ptr});
    const SymbolId dtor_sym = module_.symbols.add(
        drec, dname.data(), static_cast<uint32_t>(dname.size()), dname.data(),
        static_cast<uint32_t>(dname.size()));
    std::string ddef = "static void " + dname + "(struct " + cap_name + "* self) {\n";
    if (lamb->has_destructor_for_capture()) {
        for (CapturedVariable* cap : lamb->captureList) {
            const SymbolId d = destructor_symbol(cap->known_type());
            if (d == MIR_INVALID_ID) continue;
            const MIRSymbolRecord& dr = module_.symbols.get(d);
            const std::string dn(module_.symbols.mangled_data(dr), dr.mangled_length);
            const std::string fname(cap->name.data(), cap->name.size());
            ddef += "    " + dn + "(&self->" + fname + ");\n";
        }
    }
    ddef += "}\n";
    lambda_defs_.push_back(ddef);

    // 2b. the same destructor as MIR, so the LLVM backend emits a definition for
    // it (the C backend emits the text above instead; it never consumes the
    // collected MIR functions, which are cleared per function).
    {
        MIRFunction dfn;
        dfn.symbol = dtor_sym;
        dfn.function_type = drec.type;
        MIRBuilder db(*arena_, module_, dfn);
        MIRBuilder* saved_b = builder_;
        builder_ = &db;
        const BlockId de = db.create_block();
        db.set_block(de);
        dfn.entry_block = de;
        const ValueId self_v = db.param(void_ptr);
        const TypeId cap_ptr_t = types_.pointer_type(cap_type, true);
        const ValueId cap_ptr = db.cast(self_v, cap_ptr_t);
        if (lamb->has_destructor_for_capture()) {
            for (CapturedVariable* cap : lamb->captureList) {
                const SymbolId d = destructor_symbol(cap->known_type());
                if (d == MIR_INVALID_ID) continue;
                const TypeId ft = types_.map(cap->known_type());
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, cap->name.data(),
                    static_cast<uint32_t>(cap->name.size()));
                const ValueId fa = db.field_addr_ptr(cap_ptr, fc, ft);
                MIROperand a = MIROperand::value(fa, ft);
                db.call_scalar(d, types_.void_type(), &a, 1);
            }
        }
        db.ret_void();
        builder_ = saved_b;
        lambda_functions_.push_back(dfn);
    }

    // 3. the lambda function: `ret <lname>(void* this, <sret?>, <params>)`
    //    (matches the legacy capturing-function ABI: the hidden data pointer
    //    comes before the sret pointer)
    const TypeId ret = lamb->returnType ? types_.map(lamb->returnType) : types_.void_type();
    const bool l_sret = abi::needs_sret(module_, ret);
    MIRScratch<TypeId> _ptypes(*this);
    std::vector<TypeId>& ptypes = _ptypes.v;
    ptypes.push_back(void_ptr);
    if (l_sret) ptypes.push_back(types_.pointer_type(ret, true));
    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    const TypeId ftype = types_.function_signature(l_sret ? types_.void_type() : ret, ptypes);
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    rec.type = ftype;
    const SymbolId sym = module_.symbols.add(
        rec, lname.data(), static_cast<uint32_t>(lname.size()), lname.data(),
        static_cast<uint32_t>(lname.size()));

    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = ftype;

    // save the enclosing function's lowering state
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    const bool saved_comptime_fn = current_fn_is_comptime_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_call_temps = std::move(call_temps_);
    auto saved_stmt_temps = std::move(stmt_temps_);
    auto saved_pattern_bases = std::move(pattern_bases_);
    auto saved_pattern_results = std::move(pattern_results_);
    const bool saved_sret = sret_;
    const ValueId saved_sret_ptr = sret_ptr_;
    const TypeId saved_sret_ret = sret_ret_type_;
    const TypeId saved_sret_ptr_t = sret_ptr_type_;
    BaseType* saved_return_type = return_type_;
    const bool saved_async = async_;
    auto saved_break = std::move(break_targets_);
    auto saved_continue = std::move(continue_targets_);
    const ValueId saved_closure = lambda_closure_ptr_;
    const TypeId saved_closure_t = lambda_closure_type_;
    // a lambda body is its own function: do not inherit the enclosing splice's
    // inline frames (a `return` would branch to the caller's block/place) or its
    // comptime bindings; and do not journal the lambda's locals into it
    auto saved_inline = std::move(inline_frames_);
    auto saved_comptime = std::move(comptime_bindings_);
    const bool saved_journal = scope_journal_;

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    break_targets_.clear();
    continue_targets_.clear();
    inline_frames_.clear();
    comptime_bindings_.clear();
    scope_journal_ = false;
    // a lambda body is synchronous even when declared inside an async function
    async_ = false;

    MIRBuilder b(*saved_arena, module_, fn);
    builder_ = &b;
    current_func_ = lamb;
    current_fn_is_comptime_ = false;
    if (set_current_func_) set_current_func_(lamb);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    sret_ = l_sret;
    sret_ret_type_ = ret;
    return_type_ = lamb->returnType;

    bool ok = true;
    const ValueId this_v = b.param(void_ptr);
    if (l_sret) {
        sret_ptr_type_ = types_.pointer_type(ret, true);
        sret_ptr_ = b.param(sret_ptr_type_);
    }
    const TypeId cap_ptr_t = types_.pointer_type(cap_type, true);
    lambda_closure_ptr_ = b.cast(this_v, cap_ptr_t);
    lambda_closure_type_ = cap_type;

    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        if (place == MIR_INVALID_ID) {
            ok = false;
            error = "failed to allocate lambda parameter place";
            break;
        }
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        if (p->type) {
            const TypeId ptt = types_.map(p->type);
            const MIRTypeRecord& prr = module_.types.get(ptt);
            if (!(prr.flags & TF_TYPEDEF) && abi::is_aggregate(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (ok) ok = lower_scope(lamb->scope, error);
    if (ok && !b.current_block_terminated()) {
        emit_drops();
        b.ret_void();
    }
    if (ok && !b.ok()) {
        ok = false;
        error = b.error() ? b.error() : "MIR builder failure";
    }

    std::string c;
    if (ok) {
        std::string e;
        if (!emit_function_c(fn, module_, c, e)) {
            ok = false;
            error = e;
        }
    }
    if (ok) lambda_defs_.push_back(std::move(c));
    if (ok) lambda_functions_.push_back(fn);

    // restore the enclosing function's state
    builder_ = saved_builder;
    arena_ = saved_arena;
    current_func_ = saved_cur;
    current_fn_is_comptime_ = saved_comptime_fn;
    if (set_current_func_) set_current_func_(saved_cur);
    var_places_ = std::move(saved_var);
    name_places_ = std::move(saved_name);
    destructibles_ = std::move(saved_destructibles);
    call_temps_ = std::move(saved_call_temps);
    stmt_temps_ = std::move(saved_stmt_temps);
    pattern_bases_ = std::move(saved_pattern_bases);
    pattern_results_ = std::move(saved_pattern_results);
    sret_ = saved_sret;
    sret_ptr_ = saved_sret_ptr;
    sret_ret_type_ = saved_sret_ret;
    sret_ptr_type_ = saved_sret_ptr_t;
    return_type_ = saved_return_type;
    async_ = saved_async;
    break_targets_ = std::move(saved_break);
    continue_targets_ = std::move(saved_continue);
    lambda_closure_ptr_ = saved_closure;
    lambda_closure_type_ = saved_closure_t;
    inline_frames_ = std::move(saved_inline);
    comptime_bindings_ = std::move(saved_comptime);
    scope_journal_ = saved_journal;

    if (!ok) return false;

    // 4. materialize the closure temp in the enclosing function
    const PlaceId cap_temp = builder_->alloca(cap_type, MIRStorageClass::Temporary);
    if (cap_temp == MIR_INVALID_ID) {
        error = "failed to allocate lambda closure";
        return false;
    }
    builder_->zero_init(cap_temp);
    for (CapturedVariable* cap : lamb->captureList) {
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, cap->name.data(), static_cast<uint32_t>(cap->name.size()));
        PlaceId lp = place_for_linked(cap->linked);
        if (lp == MIR_INVALID_ID) lp = place_for_name(cap->name);
        if (lp == MIR_INVALID_ID) {
            error = "captured variable does not resolve to a local place";
            return false;
        }
        const TypeId lt = builder_->function().places[lp].type;
        if (cap->capture_by_ref) {
            const MIRTypeRecord& lr = module_.types.get(lt);
            if (lr.kind == MIRTypeKind::Reference || lr.kind == MIRTypeKind::Pointer) {
                // the captured variable is already a reference: store the pointer
                builder_->field_store(cap_temp, fc, builder_->load(lp, lt));
            } else {
                const TypeId ptrt = types_.pointer_type(lt, true);
                builder_->field_store(cap_temp, fc, builder_->address_of(lp, ptrt));
            }
        } else if (abi::is_aggregate(module_, lt)) {
            builder_->field_store_place(cap_temp, fc, lp);
            mark_moved(lp);
        } else {
            builder_->field_store(cap_temp, fc, builder_->load(lp, lt));
        }
    }

    LambdaMaterialization m;
    m.cap_type = cap_type;
    m.cap_temp = cap_temp;
    m.fn_ptr_type = ftype;
    m.fn_ptr = builder_->function_addr(sym, m.fn_ptr_type);
    m.dtor_type = types_.function_signature(types_.void_type(), {void_ptr});
    m.dtor_ptr = builder_->function_addr(dtor_sym, m.dtor_type);
    lambda_materializations_[lamb] = m;
    return true;
}

MIRExprResult MIRLowerer::lower_lambda(LambdaFunction* lamb, std::string& error) {
    if (!lamb) {
        error = "null lambda";
        return MIRExprResult::error();
    }
    // a lambda body is its own function: an unqualified field there must not
    // resolve against the enclosing method's `self`
    FunctionParam* prev_self = current_self_param_;
    ExtendableMembersContainerNode* prev_dtor_def = destructor_def_;
    current_self_param_ = lamb->get_self_param();
    destructor_def_ = nullptr;
    MIRExprResult result;
    if (!lamb->captureList.empty()) {
        if (!materialize_capturing_lambda(lamb, error)) {
            current_self_param_ = prev_self;
            destructor_def_ = prev_dtor_def;
            return MIRExprResult::error();
        }
        const LambdaMaterialization& m = lambda_materializations_[lamb];
        result = MIRExprResult::value(m.fn_ptr, m.fn_ptr_type);
    } else {
        result = materialize_noncapturing_lambda(lamb, error);
    }
    current_self_param_ = prev_self;
    destructor_def_ = prev_dtor_def;
    return result;
}

MIRExprResult MIRLowerer::materialize_noncapturing_lambda(LambdaFunction* lamb,
                                                          std::string& error) {
    static uint64_t s_lambda_counter = 0;
    const std::string lname = "__chemda_" + std::to_string(s_lambda_counter++);

    const TypeId ret = lamb->returnType ? types_.map(lamb->returnType) : types_.void_type();
    const bool l_sret = abi::needs_sret(module_, ret);
    // a non-capturing lambda converted to a `std::function` still receives the
    // hidden `void*` data pointer as its first argument (the call convention is
    // shared with capturing lambdas), so emit a matching unused parameter
    const bool needs_this = lamb->isCapturing();
    const TypeId void_ptr_t = types_.pointer_type(types_.void_type(), true);

    MIRScratch<TypeId> _ptypes(*this);
    std::vector<TypeId>& ptypes = _ptypes.v;
    if (needs_this) ptypes.push_back(void_ptr_t);
    if (l_sret) ptypes.push_back(types_.pointer_type(ret, true));
    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    const TypeId ftype = types_.function_signature(l_sret ? types_.void_type() : ret, ptypes);

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    rec.type = ftype;
    const SymbolId sym = module_.symbols.add(
        rec, lname.data(), static_cast<uint32_t>(lname.size()),
        lname.data(), static_cast<uint32_t>(lname.size()));

    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = ftype;

    // save the enclosing function's lowering state
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    const bool saved_comptime_fn = current_fn_is_comptime_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_call_temps = std::move(call_temps_);
    auto saved_stmt_temps = std::move(stmt_temps_);
    const bool saved_sret = sret_;
    const ValueId saved_sret_ptr = sret_ptr_;
    const TypeId saved_sret_ret = sret_ret_type_;
    const TypeId saved_sret_ptr_t = sret_ptr_type_;
    BaseType* saved_return_type = return_type_;
    const bool saved_async = async_;
    auto saved_break = std::move(break_targets_);
    auto saved_continue = std::move(continue_targets_);
    // a lambda body is its own function: do not inherit the enclosing splice's
    // inline frames or comptime bindings, and do not journal its locals into it
    auto saved_inline = std::move(inline_frames_);
    auto saved_comptime = std::move(comptime_bindings_);
    const bool saved_journal = scope_journal_;

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    break_targets_.clear();
    continue_targets_.clear();
    inline_frames_.clear();
    comptime_bindings_.clear();
    scope_journal_ = false;
    // a lambda body is synchronous even when declared inside an async function
    async_ = false;

    MIRBuilder b(*saved_arena, module_, fn);
    builder_ = &b;
    current_func_ = lamb;
    current_fn_is_comptime_ = false;
    if (set_current_func_) set_current_func_(lamb);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    sret_ = l_sret;
    sret_ret_type_ = ret;
    return_type_ = lamb->returnType;
    if (needs_this) {
        // consume the hidden data-pointer parameter (unused: no captures)
        b.param(void_ptr_t);
    }
    if (l_sret) {
        sret_ptr_type_ = types_.pointer_type(ret, true);
        sret_ptr_ = b.param(sret_ptr_type_);
    }

    bool ok = true;
    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        if (place == MIR_INVALID_ID) {
            ok = false;
            error = "failed to allocate lambda parameter place";
            break;
        }
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        if (p->type) {
            const TypeId ptt = types_.map(p->type);
            const MIRTypeRecord& prr = module_.types.get(ptt);
            if (!(prr.flags & TF_TYPEDEF) && abi::is_aggregate(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (ok) ok = lower_scope(lamb->scope, error);
    if (ok && !b.current_block_terminated()) {
        emit_drops();
        b.ret_void();
    }
    if (ok && !b.ok()) {
        ok = false;
        error = b.error() ? b.error() : "MIR builder failure";
    }

    std::string c;
    if (ok) {
        std::string e;
        if (!emit_function_c(fn, module_, c, e)) {
            ok = false;
            error = e;
        }
    }
    if (ok) lambda_defs_.push_back(std::move(c));
    if (ok) lambda_functions_.push_back(fn);

    // restore the enclosing function's state
    builder_ = saved_builder;
    arena_ = saved_arena;
    current_func_ = saved_cur;
    current_fn_is_comptime_ = saved_comptime_fn;
    if (set_current_func_) set_current_func_(saved_cur);
    var_places_ = std::move(saved_var);
    name_places_ = std::move(saved_name);
    destructibles_ = std::move(saved_destructibles);
    call_temps_ = std::move(saved_call_temps);
    stmt_temps_ = std::move(saved_stmt_temps);
    sret_ = saved_sret;
    sret_ptr_ = saved_sret_ptr;
    sret_ret_type_ = saved_sret_ret;
    sret_ptr_type_ = saved_sret_ptr_t;
    return_type_ = saved_return_type;
    async_ = saved_async;
    break_targets_ = std::move(saved_break);
    continue_targets_ = std::move(saved_continue);
    inline_frames_ = std::move(saved_inline);
    comptime_bindings_ = std::move(saved_comptime);
    scope_journal_ = saved_journal;

    if (!ok) return MIRExprResult::error();

    TypeId lam_type = types_.map(lamb->getType());
    if (lam_type == MIR_INVALID_ID ||
        (module_.types.get(lam_type).kind != MIRTypeKind::Function &&
         module_.types.get(lam_type).kind != MIRTypeKind::Pointer)) {
        lam_type = types_.pointer_type(ftype, true);
    }
    ValueId v = builder_->function_addr(sym, lam_type);
    return MIRExprResult::value(v, lam_type);
}

} // namespace mir
