#include "MIRLowererInternal.h"
// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"
#include "MIRABI.h"
#include "MIRPrototype.h"

#include <atomic>
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
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/StructType.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/symres/CoreNodes.h"
#include "MIREmitter.h"

namespace mir {
MIRExprResult MIRLowerer::lower_captured_variable(CapturedVariable* cap, bool as_address,
                                                  std::string& error) {
    if (lambda_closure_ptr_ == MIR_NULL &&
        !(async_ && async_capture_ptr_from_frame_)) {
        error = "captured variable used outside its lambda";
        return MIRExprResult::error();
    }
    const chem::string_view nm = cap->name;
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, nm.data(), static_cast<uint32_t>(nm.size()));
    BaseType* ct = cap->known_type();
    const TypeId ft = types_.map(ct);
    const ValueId ptr = captured_env_ptr();
    if (ptr == MIR_NULL) {
        error = "captured variable has no capture environment";
        return MIRExprResult::error();
    }
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

static std::string capture_flag_name(unsigned index) {
    return "__chx_cap_init_" + std::to_string(index);
}

bool MIRLowerer::capture_has_drop_flag(CapturedVariable* cap) {
    return cap != nullptr && !cap->capture_by_ref &&
           cap->known_type() != nullptr &&
           destructor_symbol(cap->known_type()) != MIR_INVALID_ID;
}

ValueId MIRLowerer::captured_env_ptr() {
    // A polled async closure's frame owns the function instance. The data
    // pointer must be reloaded in the current basic block: caching it in L0
    // leaves resume blocks using an SSA value that does not dominate them.
    if (!async_ || !async_capture_ptr_from_frame_) return lambda_closure_ptr_;
    if (builder_ == nullptr || async_capture_slot_ == MIR_INVALID_ID ||
        async_capture_instance_type_ == nullptr ||
        async_capture_type_ == MIR_INVALID_ID) {
        return MIR_NULL;
    }
    const MIRNumberedName cf = async_slot_field(async_capture_slot_);
    const ConstantId cap_fc = module_.constants.add_string(
        MIR_INVALID_ID, cf.data(), static_cast<uint32_t>(cf.size()));
    const TypeId inst_t = types_.map(async_capture_instance_type_);
    const ValueId inst_ptr = builder_->field_addr_ptr(async_capture_frame_, cap_fc, inst_t);
    const ConstantId data_fc = module_.constants.add_string(
        MIR_INVALID_ID, "fn_data_ptr", 11);
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    const ValueId data = builder_->field_load_ptr(inst_ptr, data_fc, void_ptr);
    const TypeId cap_t = async_capture_type_;
    return builder_->cast(data, types_.pointer_type(cap_t, true));
}

MIRExprResult MIRLowerer::lower_moved_captured_variable(CapturedVariable* cap,
                                                        std::string& error) {
    if (cap == nullptr ||
        (lambda_closure_ptr_ == MIR_NULL &&
         !(async_ && async_capture_ptr_from_frame_))) {
        error = "moved capture used outside its lambda";
        return MIRExprResult::error();
    }
    if (cap->capture_by_ref) {
        return lower_captured_variable(cap, false, error);
    }
    const TypeId ft = types_.map(cap->known_type());
    if (ft == MIR_INVALID_ID) {
        error = "moved capture has no MIR type";
        return MIRExprResult::error();
    }
    const ValueId env = captured_env_ptr();
    if (env == MIR_NULL) {
        error = "moved capture has no capture environment";
        return MIRExprResult::error();
    }
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, cap->name.data(), static_cast<uint32_t>(cap->name.size()));
    if (abi::is_aggregate(module_, ft) && capture_has_drop_flag(cap)) {
        const ValueId src = builder_->field_addr_ptr(env, fc, ft);
        const PlaceId tmp = builder_->alloca(ft, MIRStorageClass::Temporary);
        if (tmp == MIR_INVALID_ID) {
            error = "failed to allocate moved capture temporary";
            return MIRExprResult::error();
        }
        builder_->copy_ptr_to_place(tmp, src);
        // Ownership moved into the temporary; the closure destructor must not
        // destroy the original field.
        const ConstantId flag = module_.constants.add_string(
            MIR_INVALID_ID, capture_flag_name(cap->index).data(),
            static_cast<uint32_t>(capture_flag_name(cap->index).size()));
        const TypeId bool_t = types_.bool_type();
        const ValueId zero = builder_->const_int(
            bool_t, module_.constants.add_int(bool_t, 0));
        builder_->field_store_ptr(env, flag, zero);
        return MIRExprResult::place(tmp, ft);
    }
    return lower_captured_variable(cap, false, error);
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
    auto marker_named = [&](const char* name) -> bool {
        if (cap == nullptr || !cap->capture_env) return false;
        BaseType* env = cap->capture_env->canonical();
        auto* container = env ? env->get_members_container() : nullptr;
        if (container == nullptr) {
            return env != nullptr &&
                   env->representation().find(name) != std::string::npos;
        }
        if (container->kind() == ASTNodeKind::StructDecl) {
            return container->as_struct_def_unsafe()->name_view() ==
                   chem::string_view(name, std::strlen(name));
        }
        if (container->kind() == ASTNodeKind::GenericStructDecl) {
            auto* master = container->as_gen_struct_def_unsafe()->master_impl;
            if (master != nullptr &&
                master->name_view() == chem::string_view(name, std::strlen(name))) {
                return true;
            }
        }
        // A typealias instantiation can leave the marker as an unlinked named
        // type; its spelling is the stable identity in that case.
        return env != nullptr &&
               env->representation().find(name) != std::string::npos;
    };
    const bool fn_once = cap != nullptr &&
        cap->call_kind_value() == CaptureCallKind::FnOnce;
    auto returns_future_handle = [&]() -> bool {
        if (cap == nullptr) return false;
        FunctionType* ft = cap->get_func_type();
        if (ft == nullptr || ft->returnType == nullptr) return false;
        BaseType* rt = const_cast<BaseType*>(ft->returnType.getType());
        if (rt == nullptr) return false;
        rt = rt->canonical();
        if (rt == nullptr || rt->kind() != BaseTypeKind::Generic) return false;
        auto* gen = rt->as_generic_type_unsafe();
        auto* linked = gen->referenced != nullptr ? gen->referenced->linked : nullptr;
        return linked != nullptr && linked->kind() == ASTNodeKind::StructDecl &&
               linked->as_extendable_members_container_unsafe()->name_view() ==
                   chem::string_view("FutureHandle");
    };
    // An explicit `std::FnOnce<(args) => FutureHandle<...>>` carries the
    // `FnOnceCapture` marker rather than the concrete lambda environment. It
    // denotes the async-closure ABI even if a specialized copy lost the flag.
    const bool async_marker = fn_once && returns_future_handle() &&
        marker_named("FnOnceCapture");
    const bool async_closure = cap != nullptr &&
        (cap->async_closure ||
         (cap->get_func_type() != nullptr && cap->get_func_type()->isAsync() &&
          fn_once) ||
         async_marker);
    if (async_closure && !fn_once) {
        error = "an async closure returns a FutureHandle and must be called through FnOnce";
        return MIRExprResult::error();
    }
    ValueId once_source_ptr = MIR_NULL;
    ValueId once_source_size = MIR_NULL;
    if (fn_once) {
        if (recv.kind == MIRExprKind::Address) {
            const TypeId inst_type = types_.map(inst);
            once_source_ptr = static_cast<ValueId>(recv.id);
            const PlaceId tmp = builder_->alloca(inst_type, MIRStorageClass::Temporary);
            if (tmp == MIR_INVALID_ID) {
                error = "failed to allocate moved FnOnce field";
                return MIRExprResult::error();
            }
            builder_->copy_ptr_to_place(tmp, once_source_ptr);
            once_source_size = builder_->size_of(inst_type, types_.u64_type());
            recv = MIRExprResult::place(tmp, inst_type);
        } else if (recv.kind != MIRExprKind::Place) {
            error = "calling a FnOnce callback requires moving the callback into the call";
            return MIRExprResult::error();
        } else {
            const PlaceId pid = static_cast<PlaceId>(recv.id);
            if (pid < builder_->function().places.size() &&
                builder_->function().places[pid].init_state ==
                    static_cast<uint8_t>(MIRInitState::Destroyed)) {
                error = "a FnOnce callback cannot be called after it was moved";
                return MIRExprResult::error();
            }
        }
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
    auto finish_fn_once = [&]() {
        if (!fn_once || recv.kind != MIRExprKind::Place) return;
        if (once_source_ptr != MIR_NULL) {
            // The field's bytes were copied into the temporary. Zero the
            // source so its owner does not drop the capture environment twice
            // and a later call faults through the null function pointer.
            const TypeId u8t = types_.char_type();
            const ValueId zero = builder_->const_int(
                u8t, module_.constants.add_int(u8t, 0));
            builder_->memset_ptr(once_source_ptr, zero, once_source_size);
            return;
        }
        const PlaceId pid = static_cast<PlaceId>(recv.id);
        if (async_closure) {
            // The future frame copied the whole function instance (and owns
            // its capture allocation). Zero the consumed source without
            // running its destructor, which would free that allocation twice.
            const TypeId inst_t = types_.map(inst);
            const ValueId bytes = builder_->size_of(inst_t, types_.u64_type());
            const ValueId zero = builder_->const_int(
                types_.char_type(), module_.constants.add_int(types_.char_type(), 0));
            const TypeId ptr_t = types_.pointer_type(inst_t, true);
            builder_->memset_ptr(builder_->address_of(pid, ptr_t), zero, bytes);
            mark_moved(pid);
            return;
        }
        const SymbolId dtor = destructor_symbol(inst);
        if (dtor != MIR_INVALID_ID) {
            builder_->destroy(pid, dtor);
        }
        // A FnOnce call consumes the callback. The closure body cannot move a
        // capture out yet (SAFE-MOVE-002), so destroying the environment here
        // is the correct drop; mark the place moved so a second call fails.
        mark_moved(pid);
    };
    MIRExprResult fnr = call_method(get_fn);
    if (!fnr.ok()) return fnr;
    MIRExprResult dr = MIRExprResult::void_result();
    if (!async_closure) {
        dr = call_method(get_data);
        if (!dr.ok()) return dr;
    }
    const TypeId vpt = types_.pointer_type(types_.void_type(), true);
    const ValueId fnptr = (fnr.kind == MIRExprKind::Place)
                              ? builder_->load(static_cast<PlaceId>(fnr.id), fnr.type)
                              : static_cast<ValueId>(fnr.id);
    if (fn_once) {
        const TypeId bool_t = types_.bool_type();
        const ValueId null_v = builder_->const_null(
            vpt, module_.constants.add_null(vpt));
        const ConstantId ne = module_.constants.add_int(
            MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ne));
        emit_abort_unless(builder_->compare(fnptr, null_v, ne, bool_t),
                          "__chc_call_moved_fail");
    }
    ValueId dataptr = MIR_NULL;
    if (async_closure) {
        // The ramp takes ownership of the whole function instance, so pass its
        // address rather than the already-erased data pointer.
        const TypeId inst_t = types_.map(inst);
        const TypeId inst_ptr_t = types_.pointer_type(inst_t, true);
        if (recv.kind == MIRExprKind::Address) {
            dataptr = builder_->cast(static_cast<ValueId>(recv.id), vpt);
        } else if (recv.kind == MIRExprKind::Place) {
            const PlaceId pid = static_cast<PlaceId>(recv.id);
            dataptr = builder_->cast(builder_->address_of(pid, inst_ptr_t), vpt);
        } else {
            error = "an async closure call requires a concrete function instance";
            return MIRExprResult::error();
        }
    } else {
        dataptr = (dr.kind == MIRExprKind::Place)
                      ? builder_->load(static_cast<PlaceId>(dr.id), dr.type)
                      : static_cast<ValueId>(dr.id);
    }
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
        finish_fn_once();
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
        finish_fn_once();
        return MIRExprResult::place(tmp, crt);
    }
    const ValueId cv =
        builder_->call_indirect(cfn, crt, cargs.data(), static_cast<uint32_t>(cargs.size()));
    destroy_call_temps_from(ccap, error);
    finish_fn_once();
    return MIRExprResult::value(cv, crt);
}


bool MIRLowerer::materialize_async_capturing_lambda(
        LambdaFunction* lamb, TypeId cap_type, BaseType* instance_type,
        SymbolId dtor_sym, SymbolId& ramp_sym, TypeId& ramp_type,
        std::string& error) {
    static std::atomic<uint64_t> s_async_counter{0};
    const std::string name = "__chemda_async_cap_" +
        std::to_string(s_async_counter.fetch_add(1, std::memory_order_relaxed));

    // A synthetic declaration gives the existing coroutine planner/lowering a
    // normal function-shaped body. The lambda body nodes are shared (linked in
    // the enclosing scope); the normalization pass only rewrites the synthetic
    // body vector. The capture environment is supplied as a frame slot and the
    // hidden data pointer is the capturing-function ABI's first argument.
    // The MIR function/region caches are keyed by the declaration address, so
    // this node must outlive the lowering call and have a stable, unique
    // address (a stack temporary gets reused and aliases the previous closure).
    async_synthetic_names_.push_back(name);
    const std::string& stable_name = async_synthetic_names_.back();
    auto decl_owner = std::make_unique<FunctionDeclaration>(
        chem::string_view(stable_name.data(), static_cast<unsigned>(stable_name.size())),
        lamb->returnType, lamb->isVariadic(), lamb->get_parent(),
        lamb->encoded_location());
    FunctionDeclaration& decl = *decl_owner;
    decl.body = lamb->scope.shallow_copy();
    decl.params = lamb->params;
    decl.set_async(true);
    decl.data.is_async = true;
    decl.data.signature_resolved = true;
    async_synthetic_decls_.push_back(std::move(decl_owner));

    // Save the enclosing function's general lowering state.
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    const bool saved_comptime_fn = current_fn_is_comptime_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_scope_places = std::move(scope_places_);
    auto saved_call_temps = std::move(call_temps_);
    auto saved_stmt_temps = std::move(stmt_temps_);
    const bool saved_binding_init = binding_init_;
    const bool saved_temp_extension = temp_extension_operand_;
    const PlaceId saved_extension_anchor = extension_anchor_;
    auto saved_pattern_bases = std::move(pattern_bases_);
    auto saved_pattern_results = std::move(pattern_results_);
    const bool saved_sret = sret_;
    const ValueId saved_sret_ptr = sret_ptr_;
    const TypeId saved_sret_ret = sret_ret_type_;
    const TypeId saved_sret_ptr_t = sret_ptr_type_;
    BaseType* saved_return_type = return_type_;
    auto saved_break = std::move(break_targets_);
    auto saved_continue = std::move(continue_targets_);
    auto saved_inline = std::move(inline_frames_);
    auto saved_comptime = std::move(comptime_bindings_);
    const bool saved_journal = scope_journal_;
    FunctionParam* saved_self_param = current_self_param_;
    ExtendableMembersContainerNode* saved_dtor_def = destructor_def_;

    // Save the enclosing async lowering state (a nested async ramp has its own
    // frame/poll/drop/site tables and must not clobber an outer async body).
    const bool saved_async = async_;
    BaseType* saved_inner = async_inner_;
    BaseType* saved_handle = async_handle_;
    BaseType* saved_table = async_table_;
    BaseType* saved_poll_base = async_poll_base_;
    BaseType* saved_context = async_context_;
    const TypeId saved_frame_type = async_frame_type_;
    const TypeId saved_frame_ptr_type = async_frame_ptr_type_;
    auto saved_plan = std::move(async_plan_);
    MIRFunction saved_poll = std::move(async_poll_);
    MIRFunction saved_drop = std::move(async_drop_);
    std::string saved_pre = std::move(async_pre_decls_);
    std::string saved_post = std::move(async_post_decls_);
    const PlaceId saved_poll_ret = async_poll_ret_;
    const PlaceId saved_poll_frame = async_poll_frame_;
    const PlaceId saved_poll_cx = async_poll_cx_;
    const ValueId saved_poll_af = async_poll_af_val_;
    const ValueId saved_ramp_af = async_ramp_af_val_;
    auto saved_resident = std::move(async_resident_);
    auto saved_drop_flags = std::move(async_drop_flag_slots_);
    auto saved_site_poll = std::move(async_site_poll_);
    auto saved_resume_blocks = std::move(async_resume_blocks_);
    const PlaceId saved_ramp_af_place = async_ramp_af_;
    const PlaceId saved_ramp_result = async_ramp_result_;
    const BlockId saved_ramp_done = async_ramp_done_;
    const bool saved_in_ramp = async_in_ramp_;
    std::string saved_mangled = std::move(async_mangled_);
    auto saved_site_index = std::move(async_site_index_);
    auto saved_child_places = std::move(async_child_places_);
    auto saved_result_places = std::move(async_result_places_);
    auto saved_spills = std::move(async_spills_);
    auto saved_cont_blocks = std::move(async_cont_blocks_);
    const TypeId saved_capture_type = async_capture_type_;
    BaseType* saved_capture_instance_type = async_capture_instance_type_;
    const bool saved_capture_hidden_data = async_capture_hidden_data_;
    const bool saved_is_closure = async_is_closure_;
    const bool saved_capture_ptr_from_frame = async_capture_ptr_from_frame_;
    const ValueId saved_capture_frame = async_capture_frame_;
    const SymbolId saved_capture_dtor = async_capture_dtor_;
    const unsigned saved_capture_slot = async_capture_slot_;
    const ValueId saved_lambda_ptr = lambda_closure_ptr_;
    const TypeId saved_lambda_type = lambda_closure_type_;
    const bool saved_async_main_emitted = async_main_emitted_;

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    scope_places_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    binding_init_ = false;
    temp_extension_operand_ = false;
    extension_anchor_ = MIR_INVALID_ID;
    pattern_bases_.clear();
    pattern_results_.clear();
    break_targets_.clear();
    continue_targets_.clear();
    inline_frames_.clear();
    comptime_bindings_.clear();
    scope_journal_ = false;
    current_func_ = lamb;
    current_fn_is_comptime_ = false;
    current_self_param_ = lamb->get_self_param();
    destructor_def_ = nullptr;
    if (set_current_func_) set_current_func_(lamb);

    async_capture_type_ = cap_type;
    async_capture_instance_type_ = instance_type;
    async_capture_hidden_data_ = instance_type != nullptr ||
        (lamb->isCapturing() && lamb->capture_instance_type != nullptr);
    async_is_closure_ = true;
    async_capture_ptr_from_frame_ = false;
    async_capture_frame_ = MIR_NULL;
    async_capture_dtor_ = dtor_sym;
    async_capture_slot_ = MIR_INVALID_ID;

    MIRFunction ramp;
    bool ok = lower_async_function(&decl, *arena_, ramp, error);

    std::string poll_c;
    std::string ramp_c;
    if (ok) {
        std::string emit_error;
        ok = emit_function_c(async_poll_, module_, poll_c, emit_error);
        if (ok) ok = emit_function_c(ramp, module_, ramp_c, emit_error);
        if (!ok) error = emit_error;
    }
    if (ok) {
        lambda_defs_.push_back(std::move(async_pre_decls_));
        lambda_defs_.push_back(std::move(poll_c));
        lambda_defs_.push_back(std::move(async_post_decls_));
        lambda_defs_.push_back(std::move(ramp_c));
        lambda_functions_.push_back(async_poll_);
        lambda_functions_.push_back(async_drop_);
        lambda_functions_.push_back(ramp);
        ramp_sym = ramp.symbol;
        ramp_type = ramp.function_type;
    }

    // Restore the enclosing function's state.
    builder_ = saved_builder;
    arena_ = saved_arena;
    current_func_ = saved_cur;
    current_fn_is_comptime_ = saved_comptime_fn;
    if (set_current_func_) set_current_func_(saved_cur);
    var_places_ = std::move(saved_var);
    name_places_ = std::move(saved_name);
    destructibles_ = std::move(saved_destructibles);
    scope_places_ = std::move(saved_scope_places);
    call_temps_ = std::move(saved_call_temps);
    stmt_temps_ = std::move(saved_stmt_temps);
    binding_init_ = saved_binding_init;
    temp_extension_operand_ = saved_temp_extension;
    extension_anchor_ = saved_extension_anchor;
    pattern_bases_ = std::move(saved_pattern_bases);
    pattern_results_ = std::move(saved_pattern_results);
    sret_ = saved_sret;
    sret_ptr_ = saved_sret_ptr;
    sret_ret_type_ = saved_sret_ret;
    sret_ptr_type_ = saved_sret_ptr_t;
    return_type_ = saved_return_type;
    break_targets_ = std::move(saved_break);
    continue_targets_ = std::move(saved_continue);
    inline_frames_ = std::move(saved_inline);
    comptime_bindings_ = std::move(saved_comptime);
    scope_journal_ = saved_journal;
    current_self_param_ = saved_self_param;
    destructor_def_ = saved_dtor_def;

    async_ = saved_async;
    async_inner_ = saved_inner;
    async_handle_ = saved_handle;
    async_table_ = saved_table;
    async_poll_base_ = saved_poll_base;
    async_context_ = saved_context;
    async_frame_type_ = saved_frame_type;
    async_frame_ptr_type_ = saved_frame_ptr_type;
    async_plan_ = std::move(saved_plan);
    async_poll_ = std::move(saved_poll);
    async_drop_ = std::move(saved_drop);
    async_pre_decls_ = std::move(saved_pre);
    async_post_decls_ = std::move(saved_post);
    async_poll_ret_ = saved_poll_ret;
    async_poll_frame_ = saved_poll_frame;
    async_poll_cx_ = saved_poll_cx;
    async_poll_af_val_ = saved_poll_af;
    async_ramp_af_val_ = saved_ramp_af;
    async_resident_ = std::move(saved_resident);
    async_drop_flag_slots_ = std::move(saved_drop_flags);
    async_site_poll_ = std::move(saved_site_poll);
    async_resume_blocks_ = std::move(saved_resume_blocks);
    async_ramp_af_ = saved_ramp_af_place;
    async_ramp_result_ = saved_ramp_result;
    async_ramp_done_ = saved_ramp_done;
    async_in_ramp_ = saved_in_ramp;
    async_mangled_ = std::move(saved_mangled);
    async_site_index_ = std::move(saved_site_index);
    async_child_places_ = std::move(saved_child_places);
    async_result_places_ = std::move(saved_result_places);
    async_spills_ = std::move(saved_spills);
    async_cont_blocks_ = std::move(saved_cont_blocks);
    async_capture_type_ = saved_capture_type;
    async_capture_instance_type_ = saved_capture_instance_type;
    async_capture_hidden_data_ = saved_capture_hidden_data;
    async_is_closure_ = saved_is_closure;
    async_capture_ptr_from_frame_ = saved_capture_ptr_from_frame;
    async_capture_frame_ = saved_capture_frame;
    async_capture_dtor_ = saved_capture_dtor;
    async_capture_slot_ = saved_capture_slot;
    lambda_closure_ptr_ = saved_lambda_ptr;
    lambda_closure_type_ = saved_lambda_type;
    async_main_emitted_ = saved_async_main_emitted;
    return ok;
}

bool MIRLowerer::materialize_capturing_lambda(LambdaFunction* lamb, std::string& error) {
    if (lambda_materializations_.find(lamb) != lambda_materializations_.end()) return true;
    if (!lamb || lamb->captureList.empty()) {
        error = "materialize_capturing_lambda on a non-capturing lambda";
        return false;
    }

    static std::atomic<uint64_t> s_counter{0};
    const std::string lname = "__chemda_cap_" +
        std::to_string(s_counter.fetch_add(1, std::memory_order_relaxed));
    const std::string cap_name = lname + "_cap";

    // 1. the closure struct definition + type
    const TypeId cap_type = types_.named_struct(cap_name);
    std::string sdef = "struct " + cap_name + " {\n";
    for (CapturedVariable* cap : lamb->captureList) {
        const TypeId ft = types_.map(cap->known_type());
        sdef += "    " + c_type_of(module_, ft) + " " +
                std::string(cap->name.data(), cap->name.size()) + ";\n";
    }
    for (unsigned i = 0; i < lamb->captureList.size(); ++i) {
        if (capture_has_drop_flag(lamb->captureList[i])) {
            sdef += "    bool " + capture_flag_name(i) + ";\n";
        }
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
        for (unsigned i = 0; i < lamb->captureList.size(); ++i) {
            if (!capture_has_drop_flag(lamb->captureList[i])) continue;
            const std::string flag_name = capture_flag_name(i);
            MIRFieldInfo fi;
            fi.type = types_.bool_type();
            fi.kind = static_cast<uint8_t>(MIRFieldKind::Direct);
            fi.index = static_cast<uint32_t>(cap_fields.size());
            fi.name_offset = module_.types.append_field_name(
                flag_name.data(), static_cast<uint32_t>(flag_name.size()));
            fi.name_length = static_cast<uint32_t>(flag_name.size());
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
    drec.has_body = true;
    drec.type = types_.function_signature(types_.void_type(), {void_ptr});
    const SymbolId dtor_sym = module_.symbols.add(
        drec, dname.data(), static_cast<uint32_t>(dname.size()), dname.data(),
        static_cast<uint32_t>(dname.size()));
    std::string ddef = "static void " + dname + "(struct " + cap_name + "* self) {\n";
    for (unsigned i = 0; i < lamb->captureList.size(); ++i) {
        CapturedVariable* cap = lamb->captureList[i];
        const SymbolId d = destructor_symbol(cap->known_type());
        if (d == MIR_INVALID_ID) continue;
        const MIRSymbolRecord& dr = module_.symbols.get(d);
        const std::string dn(module_.symbols.mangled_data(dr), dr.mangled_length);
        const std::string fname(cap->name.data(), cap->name.size());
        if (capture_has_drop_flag(cap)) {
            const std::string flag = capture_flag_name(i);
            ddef += "    if(self->" + flag + ") {\n";
            ddef += "        " + dn + "(&self->" + fname + ");\n";
            ddef += "        self->" + flag + " = false;\n";
            ddef += "    }\n";
        } else {
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
        const TypeId bool_t = types_.bool_type();
        for (unsigned i = 0; i < lamb->captureList.size(); ++i) {
            CapturedVariable* cap = lamb->captureList[i];
            const SymbolId d = destructor_symbol(cap->known_type());
            if (d == MIR_INVALID_ID) continue;
            const TypeId ft = types_.map(cap->known_type());
            const ConstantId fc = module_.constants.add_string(
                MIR_INVALID_ID, cap->name.data(),
                static_cast<uint32_t>(cap->name.size()));
            const ValueId fa = db.field_addr_ptr(cap_ptr, fc, ft);
            MIROperand a = MIROperand::value(fa, ft);
            if (!capture_has_drop_flag(cap)) {
                db.call_scalar(d, types_.void_type(), &a, 1);
                continue;
            }
            const std::string flag_name = capture_flag_name(i);
            const ConstantId flag = module_.constants.add_string(
                MIR_INVALID_ID, flag_name.data(),
                static_cast<uint32_t>(flag_name.size()));
            const ValueId flag_v = db.field_load_ptr(cap_ptr, flag, bool_t);
            const BlockId do_b = db.create_block();
            const BlockId end_b = db.create_block();
            db.cond_br(flag_v, do_b, end_b);
            db.set_block(do_b);
            db.call_scalar(d, types_.void_type(), &a, 1);
            const ValueId zero = db.const_int(
                bool_t, module_.constants.add_int(bool_t, 0));
            db.field_store_ptr(cap_ptr, flag, zero);
            db.br(end_b);
            db.set_block(end_b);
        }
        db.ret_void();
        builder_ = saved_b;
        lambda_functions_.push_back(dfn);
    }

    TypeId async_ramp_type = MIR_INVALID_ID;
    SymbolId async_ramp_sym = MIR_INVALID_ID;
    if (lamb->isAsync()) {
        // The symres callable inference stores the runtime instance type on
        // the lambda; the lambda node itself is a FunctionType, not its
        // capturing wrapper, so getType() cannot reveal the instance layout.
        BaseType* instance_type = lamb->capture_instance_type;
        if (instance_type == nullptr) {
            error = "async closure callable instance type could not be resolved";
            return false;
        }
        const SymbolId instance_dtor = destructor_symbol(instance_type);
        if (instance_dtor == MIR_INVALID_ID) {
            error = "async closure function instance has no destructor";
            return false;
        }
        if (!materialize_async_capturing_lambda(
                lamb, cap_type, instance_type, instance_dtor,
                async_ramp_sym, async_ramp_type, error)) {
            return false;
        }
    }

    TypeId lambda_ftype = MIR_INVALID_ID;
    SymbolId lambda_sym = MIR_INVALID_ID;
    if (!lamb->isAsync()) {
    // 3. the lambda function: `ret <lname>(void* this, <sret?>, <params>)`
    //    (matches the legacy capturing-function ABI: the hidden data pointer
    //    comes before the sret pointer)
    const TypeId ret = lamb->returnType ? types_.map(lamb->returnType) : types_.void_type();
    const bool l_sret = abi::needs_sret(module_, ret);
    MIRScratch<TypeId> _ptypes(*this);
    std::vector<TypeId>& ptypes = _ptypes.v;
    ptypes.push_back(void_ptr);
    if (l_sret) ptypes.push_back(types_.pointer_type(ret, true));
    const size_t lambda_param_count = lamb->isVariadic() && !lamb->params.empty()
        ? lamb->params.size() - 1 : lamb->params.size();
    for (size_t pi = 0; pi < lambda_param_count; ++pi) {
        FunctionParam* p = lamb->params[pi];
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    const TypeId ftype = types_.function_signature(
        l_sret ? types_.void_type() : ret, ptypes, lamb->isVariadic());
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    rec.has_body = true;
    rec.type = ftype;
    const SymbolId sym = module_.symbols.add(
        rec, lname.data(), static_cast<uint32_t>(lname.size()), lname.data(),
        static_cast<uint32_t>(lname.size()));

    MIRFunction fn;
    if (lamb != nullptr && source_loc_provider_ != nullptr) {
        const uint64_t enc = lamb->encoded_location().encoded;
        if (enc != 0 && source_loc_provider_(enc, fn.decl_loc)) {
            fn.has_decl_loc = true;
        }
    }
    // `debug_quick` has no source provider, but a lambda diagnostic should
    // still point at the enclosing function rather than nowhere.
    if (!fn.has_decl_loc && builder_ != nullptr) {
        const MIRFunction& outer = builder_->function();
        if (outer.has_decl_loc) {
            fn.decl_loc = outer.decl_loc;
            fn.has_decl_loc = true;
        }
    }
    fn.symbol = sym;
    fn.function_type = ftype;
    if (builder_ != nullptr) {
        fn.parent_symbol = builder_->function().symbol;
    }

    // save the enclosing function's lowering state
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    const bool saved_comptime_fn = current_fn_is_comptime_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_scope_places = std::move(scope_places_);
    auto saved_call_temps = std::move(call_temps_);
    auto saved_stmt_temps = std::move(stmt_temps_);
    const bool saved_binding_init = binding_init_;
    const bool saved_temp_extension = temp_extension_operand_;
    const PlaceId saved_extension_anchor = extension_anchor_;
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
    scope_places_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    binding_init_ = false;
    temp_extension_operand_ = false;
    extension_anchor_ = MIR_INVALID_ID;
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
        fn.sret_pointer = sret_ptr_;
    }
    const TypeId cap_ptr_t = types_.pointer_type(cap_type, true);
    lambda_closure_ptr_ = b.cast(this_v, cap_ptr_t);
    lambda_closure_type_ = cap_type;

    for (size_t pi = 0; pi < lambda_param_count; ++pi) {
        FunctionParam* p = lamb->params[pi];
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
    scope_places_ = std::move(saved_scope_places);
    call_temps_ = std::move(saved_call_temps);
    stmt_temps_ = std::move(saved_stmt_temps);
    binding_init_ = saved_binding_init;
    temp_extension_operand_ = saved_temp_extension;
    extension_anchor_ = saved_extension_anchor;
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

    lambda_ftype = ftype;
    lambda_sym = sym;
    } // !lamb->isAsync()

    // 4. materialize the closure temp in the enclosing function
    const PlaceId cap_temp = builder_->alloca(cap_type, MIRStorageClass::Temporary);
    if (cap_temp == MIR_INVALID_ID) {
        error = "failed to allocate lambda closure";
        return false;
    }
    builder_->zero_init(cap_temp);
    {
        const TypeId bool_t = types_.bool_type();
        const ValueId one = builder_->const_int(
            bool_t, module_.constants.add_int(bool_t, 1));
        for (unsigned i = 0; i < lamb->captureList.size(); ++i) {
            if (!capture_has_drop_flag(lamb->captureList[i])) continue;
            const std::string flag_name = capture_flag_name(i);
            const ConstantId flag = module_.constants.add_string(
                MIR_INVALID_ID, flag_name.data(),
                static_cast<uint32_t>(flag_name.size()));
            builder_->field_store(cap_temp, flag, one);
        }
    }
    for (CapturedVariable* cap : lamb->captureList) {
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, cap->name.data(), static_cast<uint32_t>(cap->name.size()));
        PlaceId lp = place_for_linked(cap->linked);
        if (lp == MIR_INVALID_ID) lp = place_for_name(cap->name);
        if (lp == MIR_INVALID_ID && cap->linked != nullptr &&
            cap->linked->kind() == ASTNodeKind::CapturedVariable) {
            // A nested closure capturing a variable that the enclosing closure
            // already captured. The outer capture lives in the enclosing
            // closure environment, not in a local place; resolve it through
            // that environment instead. A borrowed nested capture stores the
            // address of the original variable; a by-value capture loads its
            // value (a by-reference outer capture is dereferenced first).
            auto* outer = cap->linked->as_captured_var_unsafe();
            const TypeId oft = types_.map(outer->known_type());
            const TypeId original =
                outer->capture_by_ref && outer->refType.type != nullptr
                    ? types_.map(outer->refType.type)
                    : oft;
            if (cap->capture_by_ref) {
                if (outer->capture_by_ref) {
                    // The enclosing field already holds the referent pointer.
                    MIRExprResult v = lower_captured_variable(outer, false, error);
                    if (!v.ok()) return false;
                    builder_->field_store(cap_temp, fc, static_cast<ValueId>(v.id));
                } else {
                    // The enclosing field owns the variable; store its address.
                    MIRExprResult a = lower_captured_variable(outer, true, error);
                    if (!a.ok()) return false;
                    const TypeId ptrt = types_.pointer_type(oft, true);
                    ValueId ptr = a.kind == MIRExprKind::Place
                                      ? builder_->address_of(static_cast<PlaceId>(a.id), ptrt)
                                      : builder_->cast(static_cast<ValueId>(a.id), ptrt);
                    builder_->field_store(cap_temp, fc, ptr);
                }
            } else {
                // A by-value nested capture of an aggregate copies the value.
                // A non-Copy aggregate would have to move the field out of the
                // enclosing closure environment; that needs callable/move
                // semantics (FnOnce) that do not exist yet.
                if (abi::is_aggregate(module_, original)) {
                    if (!mir_type_is_copy(module_.types, original)) {
                        error = "nested closure cannot move a non-Copy aggregate capture out of the enclosing closure";
                        return false;
                    }
                    MIRExprResult a = lower_captured_variable(outer, true, error);
                    if (!a.ok()) return false;
                    const TypeId ptrt = types_.pointer_type(original, true);
                    ValueId ptr = a.kind == MIRExprKind::Place
                                      ? builder_->address_of(static_cast<PlaceId>(a.id), ptrt)
                                      : builder_->cast(static_cast<ValueId>(a.id), ptrt);
                    const PlaceId tmp = builder_->alloca(original, MIRStorageClass::Temporary);
                    if (tmp == MIR_INVALID_ID) {
                        error = "failed to allocate nested closure copy";
                        return false;
                    }
                    builder_->copy_ptr_to_place(tmp, ptr);
                    builder_->mark_initialized(tmp);
                    builder_->field_store_place(cap_temp, fc, tmp);
                    continue;
                }
                MIRExprResult v = lower_captured_variable(outer, false, error);
                if (!v.ok()) return false;
                ValueId value = static_cast<ValueId>(v.id);
                if (outer->capture_by_ref) {
                    value = builder_->load_indirect(value, original);
                }
                builder_->field_store(cap_temp, fc, value);
            }
            continue;
        }
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
    m.fn_ptr_type = lamb->isAsync() ? async_ramp_type : lambda_ftype;
    m.fn_ptr = builder_->function_addr(
        lamb->isAsync() ? async_ramp_sym : lambda_sym, m.fn_ptr_type);
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
    if (lamb->isAsync()) {
        SymbolId ramp_sym = MIR_INVALID_ID;
        TypeId ramp_type = MIR_INVALID_ID;
        if (!materialize_async_capturing_lambda(
                lamb, MIR_INVALID_ID, nullptr, MIR_INVALID_ID,
                ramp_sym, ramp_type, error)) {
            return MIRExprResult::error();
        }
        TypeId lam_type = types_.map(lamb->getType());
        if (lam_type == MIR_INVALID_ID ||
            (module_.types.get(lam_type).kind != MIRTypeKind::Function &&
             module_.types.get(lam_type).kind != MIRTypeKind::Pointer)) {
            lam_type = types_.pointer_type(ramp_type, true);
        }
        return MIRExprResult::value(builder_->function_addr(ramp_sym, lam_type), lam_type);
    }
    static std::atomic<uint64_t> s_lambda_counter{0};
    const std::string lname = "__chemda_" +
        std::to_string(s_lambda_counter.fetch_add(1, std::memory_order_relaxed));

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
    const size_t lambda_param_count = lamb->isVariadic() && !lamb->params.empty()
        ? lamb->params.size() - 1 : lamb->params.size();
    for (size_t pi = 0; pi < lambda_param_count; ++pi) {
        FunctionParam* p = lamb->params[pi];
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    const TypeId ftype = types_.function_signature(
        l_sret ? types_.void_type() : ret, ptypes, lamb->isVariadic());

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    rec.has_body = true;
    rec.type = ftype;
    const SymbolId sym = module_.symbols.add(
        rec, lname.data(), static_cast<uint32_t>(lname.size()),
        lname.data(), static_cast<uint32_t>(lname.size()));

    MIRFunction fn;
    if (lamb != nullptr && source_loc_provider_ != nullptr) {
        const uint64_t enc = lamb->encoded_location().encoded;
        if (enc != 0 && source_loc_provider_(enc, fn.decl_loc)) {
            fn.has_decl_loc = true;
        }
    }
    // `debug_quick` has no source provider, but a lambda diagnostic should
    // still point at the enclosing function rather than nowhere.
    if (!fn.has_decl_loc && builder_ != nullptr) {
        const MIRFunction& outer = builder_->function();
        if (outer.has_decl_loc) {
            fn.decl_loc = outer.decl_loc;
            fn.has_decl_loc = true;
        }
    }
    fn.symbol = sym;
    fn.function_type = ftype;
    if (builder_ != nullptr) {
        fn.parent_symbol = builder_->function().symbol;
    }

    // save the enclosing function's lowering state
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    const bool saved_comptime_fn = current_fn_is_comptime_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_scope_places = std::move(scope_places_);
    auto saved_call_temps = std::move(call_temps_);
    auto saved_stmt_temps = std::move(stmt_temps_);
    const bool saved_binding_init = binding_init_;
    const bool saved_temp_extension = temp_extension_operand_;
    const PlaceId saved_extension_anchor = extension_anchor_;
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
    scope_places_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    binding_init_ = false;
    temp_extension_operand_ = false;
    extension_anchor_ = MIR_INVALID_ID;
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
        fn.sret_pointer = sret_ptr_;
    }

    bool ok = true;
    for (size_t pi = 0; pi < lambda_param_count; ++pi) {
        FunctionParam* p = lamb->params[pi];
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
    scope_places_ = std::move(saved_scope_places);
    call_temps_ = std::move(saved_call_temps);
    stmt_temps_ = std::move(saved_stmt_temps);
    binding_init_ = saved_binding_init;
    temp_extension_operand_ = saved_temp_extension;
    extension_anchor_ = saved_extension_anchor;
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
