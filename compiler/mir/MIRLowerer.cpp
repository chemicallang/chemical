// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"

#include <cstdlib>
#include <iostream>

#include "ast/base/BaseType.h"
#include "ast/types/GenericType.h"
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
#include "ast/values/RuntimeValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/structures/CapturedComptimeVariable.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/values/ValueNode.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
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
#include "ast/values/SwitchValue.h"
#include "ast/values/LoopValue.h"
#include "ast/values/AwaitExpression.h"
#include "compiler/async/AsyncCTypes.h"
#include "compiler/async/AwaitNormalizePass.h"
#include "ast/types/ArrayType.h"
#include "MIREmitter.h"

namespace mir {

namespace {

bool is_void_type(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    return module.types.get(type).kind == MIRTypeKind::Void;
}

/** Resolve a captured-comptime bridge node to its evaluated value, if any. */
Value* captured_comptime_value(ASTNode* linked) {
    if (!linked || linked->kind() != ASTNodeKind::CapturedComptimeVariable) return nullptr;
    auto* ccv = static_cast<CapturedComptimeVariable*>(linked);
    if (!ccv->parent_refs || ccv->index >= ccv->parent_refs->refs.size()) return nullptr;
    return ccv->parent_refs->refs[ccv->index].evaluated;
}

bool needs_aggregate_path(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeRecord& rec = module.types.get(type);
    if (rec.flags & TF_TYPEDEF) return false; // dynamic fat pointer: passed by value
    const MIRTypeKind k = rec.kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union || k == MIRTypeKind::Variant ||
           k == MIRTypeKind::Array;
}

bool map_binary(Operation op, MIROpcode& out, MIRBinaryOp& bop) {
    switch (op) {
        case Operation::Addition: out = MIROpcode::Binary; bop = MIRBinaryOp::Add; return true;
        case Operation::Subtraction: out = MIROpcode::Binary; bop = MIRBinaryOp::Sub; return true;
        case Operation::Multiplication: out = MIROpcode::Binary; bop = MIRBinaryOp::Mul; return true;
        case Operation::Division: out = MIROpcode::Binary; bop = MIRBinaryOp::Div; return true;
        case Operation::Modulus: out = MIROpcode::Binary; bop = MIRBinaryOp::Rem; return true;
        case Operation::LeftShift: out = MIROpcode::Binary; bop = MIRBinaryOp::Shl; return true;
        case Operation::RightShift: out = MIROpcode::Binary; bop = MIRBinaryOp::Shr; return true;
        case Operation::BitwiseAND: out = MIROpcode::Binary; bop = MIRBinaryOp::BitAnd; return true;
        case Operation::BitwiseOR: out = MIROpcode::Binary; bop = MIRBinaryOp::BitOr; return true;
        case Operation::BitwiseXOR: out = MIROpcode::Binary; bop = MIRBinaryOp::BitXor; return true;
        case Operation::GreaterThan: out = MIROpcode::Compare; bop = MIRBinaryOp::Gt; return true;
        case Operation::GreaterThanOrEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Ge; return true;
        case Operation::LessThan: out = MIROpcode::Compare; bop = MIRBinaryOp::Lt; return true;
        case Operation::LessThanOrEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Le; return true;
        case Operation::IsEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Eq; return true;
        case Operation::IsNotEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Ne; return true;
        default: return false;
    }
}

bool compound_binary(Operation op, MIRBinaryOp& out) {
    switch (op) {
        case Operation::AddTo: out = MIRBinaryOp::Add; return true;
        case Operation::SubtractFrom: out = MIRBinaryOp::Sub; return true;
        case Operation::MultiplyBy: out = MIRBinaryOp::Mul; return true;
        case Operation::DivideBy: out = MIRBinaryOp::Div; return true;
        case Operation::ModuloBy: out = MIRBinaryOp::Rem; return true;
        case Operation::ShiftLeftBy: out = MIRBinaryOp::Shl; return true;
        case Operation::ShiftRightBy: out = MIRBinaryOp::Shr; return true;
        case Operation::ANDWith: out = MIRBinaryOp::BitAnd; return true;
        case Operation::ExclusiveORWith: out = MIRBinaryOp::BitXor; return true;
        case Operation::InclusiveORWith: out = MIRBinaryOp::BitOr; return true;
        default: return false;
    }
}

} // namespace

PlaceId MIRLowerer::place_for_linked(ASTNode* linked) const {
    if (!linked) return MIR_INVALID_ID;
    auto it = var_places_.find(linked);
    if (it == var_places_.end()) return MIR_INVALID_ID;
    return it->second;
}

PlaceId MIRLowerer::resolve_place(Value* v, std::string& error) {
    if (!v) {
        error = "null place expression";
        return MIR_INVALID_ID;
    }
    Value* t = v;
    if (t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (t->val_kind() != ValueKind::Identifier) {
        error = "expression is not a simple place (kind " +
                std::to_string(static_cast<int>(t->val_kind())) + ")";
        return MIR_INVALID_ID;
    }
    auto* id = t->as_identifier_unsafe();
    const PlaceId p = place_for_linked(id->linked);
    if (p == MIR_INVALID_ID) {
        error = "identifier '" + std::string(id->value.data(), id->value.size()) +
                "' does not resolve to a local place";
        return MIR_INVALID_ID;
    }
    return p;
}

PlaceId MIRLowerer::resolve_place_or_lower(Value* v, std::string& error) {
    const PlaceId direct = resolve_place(v, error);
    if (direct != MIR_INVALID_ID) return direct;
    // member access on the result of an expression (e.g. a function call that
    // returns an aggregate temporary): lower it and use its place if any.
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t) return MIR_INVALID_ID;
    std::string inner;
    MIRExprResult r = lower_expr(t, inner);
    if (r.ok() && r.kind == MIRExprKind::Place) return static_cast<PlaceId>(r.id);
    error = "expression is not a simple place (kind " +
            std::to_string(static_cast<int>(t->val_kind())) + ")" +
            (inner.empty() ? std::string() : ("; " + inner));
    return MIR_INVALID_ID;
}

FunctionDeclaration* MIRLowerer::resolve_method_fallback(BaseType* recv_type,
                                                         const std::string& name) {
    if (name.empty()) return nullptr;
    if (recv_type) {
        ASTNode* node = recv_type->get_direct_linked_canonical_node();
        if (!node) node = recv_type->get_direct_linked_node();
        if (node) {
            MembersContainer* mc = node->get_members_container();
            if (mc) {
                FunctionDeclaration* f =
                    mc->any_child_function(chem::string_view(name.data(), name.size()));
                if (f) return f;
            }
        }
    }
    // last resort: a top-level function with this name (extension functions on
    // non-static interfaces are not registered on the receiver's container)
    if (symbol_lookup_) return symbol_lookup_(name);
    return nullptr;
}

MIRExprResult MIRLowerer::lower_address_of(Value* inner, std::string& error) {
    Value* t = inner;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() >= 2) {
            // `&raw obj.field` / `&raw ptr->field`: return the last field address
            MIRExprResult first = lower_expr(c->values[0], error);
            if (!first.ok()) return first;
            PlaceId cur_place = MIR_INVALID_ID;
            ValueId cur_ptr = MIR_NULL;
            bool have_ptr = false;
            if (first.kind == MIRExprKind::Place) {
                cur_place = static_cast<PlaceId>(first.id);
            } else if (first.kind == MIRExprKind::Value ||
                       first.kind == MIRExprKind::Address) {
                cur_ptr = static_cast<ValueId>(first.id);
                have_ptr = true;
            } else {
                error = "address-of member base is not a place or pointer";
                return MIRExprResult::error();
            }
            for (size_t i = 1; i < c->values.size(); ++i) {
                std::string fname;
                if (!member_name(c->values[i], fname, error)) return MIRExprResult::error();
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                const TypeId ft = types_.map(c->values[i]->getType());
                const ValueId faddr = have_ptr ? builder_->field_addr_ptr(cur_ptr, fc, ft)
                                               : builder_->field_addr(cur_place, fc, ft);
                const MIRTypeRecord& ftr = module_.types.get(ft);
                if (ftr.kind == MIRTypeKind::Pointer || ftr.kind == MIRTypeKind::Reference) {
                    cur_ptr = builder_->load_indirect(faddr, ft);
                } else {
                    cur_ptr = faddr;
                }
                have_ptr = true;
            }
            const TypeId ptrt = types_.pointer_type(types_.map(t->getType()), true);
            return MIRExprResult::value(cur_ptr, ptrt);
        }
    }
    if (t && t->val_kind() == ValueKind::IndexOperator) {
        // `&raw arr[i]` / `&mut ptr[i]` -> the element address, not the element value
        auto* io = t->as_index_op_unsafe();
        MIRExprResult base = lower_expr(io->parent_val, error);
        if (!base.ok()) return base;
        MIRExprResult idx = lower_expr(io->idx, error);
        if (!idx.ok()) return idx;
        const TypeId et = types_.map(io->getType());
        const TypeId ptrt = types_.pointer_type(et, true);
        if (base.kind == MIRExprKind::Place) {
            return MIRExprResult::value(
                builder_->index_addr(static_cast<PlaceId>(base.id),
                                     static_cast<ValueId>(idx.id), et),
                ptrt);
        }
        if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
            return MIRExprResult::value(
                builder_->index_addr_ptr(static_cast<ValueId>(base.id),
                                         static_cast<ValueId>(idx.id), et),
                ptrt);
        }
        error = "address-of index base is not a place or pointer";
        return MIRExprResult::error();
    }
    if (t && t->val_kind() == ValueKind::Identifier) {
        auto* id = t->as_identifier_unsafe();
        if (Value* cv = captured_comptime_value(id->linked)) {
            return lower_expr(cv, error);
        }
        const PlaceId p = place_for_linked(id->linked);
        if (p != MIR_INVALID_ID) {
            const TypeId pt = builder_->function().places[p].type;
            const MIRTypeRecord& pr = module_.types.get(pt);
            const TypeId ast_t = types_.map(id->getType());
            if (pr.kind == MIRTypeKind::Reference ||
                (needs_aggregate_path(module_, ast_t) && pr.kind == MIRTypeKind::Pointer)) {
                // a reference denotes its referent, so `&x` is the reference value;
                // an aggregate parameter passed by pointer likewise
                return MIRExprResult::value(builder_->load(p, pt), pt);
            }
            const TypeId ptrt = types_.pointer_type(pt, true);
            return MIRExprResult::value(builder_->address_of(p, ptrt), ptrt);
        }
        const PlaceId pn = place_for_name(id->value);
        if (pn != MIR_INVALID_ID) {
            const TypeId pt = builder_->function().places[pn].type;
            const MIRTypeRecord& pr = module_.types.get(pt);
            const TypeId ast_t = types_.map(id->getType());
            if (pr.kind == MIRTypeKind::Reference ||
                (needs_aggregate_path(module_, ast_t) && pr.kind == MIRTypeKind::Pointer)) {
                return MIRExprResult::value(builder_->load(pn, pt), pt);
            }
            const TypeId ptrt = types_.pointer_type(pt, true);
            return MIRExprResult::value(builder_->address_of(pn, ptrt), ptrt);
        }
        if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
            auto* vi = id->linked->as_var_init();
            const SymbolId gs = intern_global(vi);
            const TypeId vt = types_.map(id->getType());
            const TypeId ptrt = types_.pointer_type(vt, false);
            return MIRExprResult::address(builder_->global_addr(gs, ptrt), ptrt);
        }
        const std::string gname(id->value.data(), id->value.size());
        if (!gname.empty()) {
            const SymbolId gs = intern_named_global(gname);
            const TypeId vt = types_.map(id->getType());
            const TypeId ptrt = types_.pointer_type(vt, false);
            return MIRExprResult::address(builder_->global_addr(gs, ptrt), ptrt);
        }
    }
    MIRExprResult in = lower_expr(inner, error);
    return in;
}

bool MIRLowerer::member_name(Value* v, std::string& out, std::string& error) {
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t || t->val_kind() != ValueKind::Identifier) {
        error = "member is not an identifier";
        return false;
    }
    const chem::string_view name = t->as_identifier_unsafe()->value;
    out.assign(name.data(), name.size());
    return true;
}

bool MIRLowerer::lower_call_args(const std::vector<Value*>& values,
                                 std::vector<MIROperand>& args, std::string& error) {
    return lower_call_args_for(nullptr, false, values, args, error);
}

bool MIRLowerer::lower_call_args_for(FunctionDeclaration* fd, bool self_included,
                                     const std::vector<Value*>& values,
                                     std::vector<MIROperand>& args, std::string& error) {
    call_temps_.clear();
    const size_t offset = self_included ? 1 : 0;
    for (size_t i = 0; i < values.size(); ++i) {
        BaseType* param_type = nullptr;
        if (fd && i + offset < fd->params.size()) {
            param_type = fd->params[i + offset]->type;
        }
        MIRExprResult r = lower_arg_converted(values[i], param_type, error);
        if (!r.ok()) return false;
        // track destructible temporaries passed by value (destroyed after the call)
        if (r.kind == MIRExprKind::Place && values[i]) {
            Value* v0 = values[i];
            if (v0->val_kind() == ValueKind::AccessChain) {
                auto* c = v0->as_access_chain_unsafe();
                if (c->values.size() == 1) v0 = c->values[0];
            }
            const ValueKind vk = v0->val_kind();
            const bool by_value =
                !(param_type && (param_type->kind() == BaseTypeKind::Reference ||
                                 param_type->kind() == BaseTypeKind::Pointer));
            if (vk == ValueKind::StructValue || vk == ValueKind::FunctionCall) {
                BaseType* at = v0->getType();
                // only temporaries passed by reference are owned by the caller
                if (at && !by_value) call_temps_.emplace_back(static_cast<PlaceId>(r.id), at);
            } else if (vk == ValueKind::Identifier && by_value) {
                // a local passed by value is moved out
                mark_moved(static_cast<PlaceId>(r.id));
            }
        }
        if (!push_call_arg(r, args, error)) return false;
    }
    return true;
}

void MIRLowerer::destroy_call_temps(std::string& error) {
    // A temporary passed by reference (`&T`) is owned by the caller and must be
    // destroyed after the call; by-value temporaries are owned by the callee's
    // parameter instead.
    for (auto& [place, type] : call_temps_) {
        const SymbolId dtor = destructor_symbol(type);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(place, dtor);
    }
    call_temps_.clear();
    (void)error;
}

SymbolId MIRLowerer::destructor_symbol(BaseType* type) {
    if (!type) return MIR_INVALID_ID;
    BaseType* canon = type->canonical();
    if (!canon) return MIR_INVALID_ID;
    ASTNode* node = canon->get_direct_linked_canonical_node();
    if (!node) return MIR_INVALID_ID;
    FunctionDeclaration* dtor = nullptr;
    if (node->kind() == ASTNodeKind::StructDecl) {
        dtor = node->as_struct_def_unsafe()->destructor_func();
    } else if (node->kind() == ASTNodeKind::VariantDecl) {
        dtor = node->as_variant_def_unsafe()->destructor_func();
    } else if (node->kind() == ASTNodeKind::UnionDecl) {
        dtor = node->as_union_def_unsafe()->destructor_func();
    }
    if (!dtor || !dtor->body.has_value()) return MIR_INVALID_ID;
    return intern_function(dtor);
}

void MIRLowerer::register_destructible(PlaceId place, BaseType* type) {
    const SymbolId dtor = destructor_symbol(type);
    if (dtor == MIR_INVALID_ID) return;
    PlaceId flag = MIR_INVALID_ID;
    if (async_ && place < builder_->function().places.size()) {
        // a frame-resident destructible uses the frame drop flag the drop
        // function checks, so completion and cancellation agree on liveness
        const MIRPlaceDef& pd = builder_->function().places[place];
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FrameField) &&
            pd.frame_field < module_.constants.size()) {
            const MIRConstant& c = module_.constants.get(pd.frame_field);
            std::string fname(module_.constants.data.data() + c.data_offset, c.data_count);
            const std::string prefix = "__chx_slot_";
            if (fname.rfind(prefix, 0) == 0) {
                const std::string df = "__chx_drop_" + fname.substr(prefix.size());
                const ConstantId dfc = module_.constants.add_string(
                    MIR_INVALID_ID, df.data(), static_cast<uint32_t>(df.size()));
                flag = builder_->alloca_frame(types_.bool_type(), dfc);
            }
        }
    }
    if (flag == MIR_INVALID_ID) {
        flag = builder_->alloca(types_.bool_type(), MIRStorageClass::Local);
    }
    builder_->set_drop(flag, true);
    destructibles_.push_back({place, flag, dtor});
}

void MIRLowerer::mark_moved(PlaceId place) {
    for (auto& d : destructibles_) {
        if (d.place == place) {
            builder_->set_drop(d.flag, false);
            return;
        }
    }
}

void MIRLowerer::emit_drops() {
    for (auto& d : destructibles_) {
        const ValueId fv = builder_->load(d.flag, types_.bool_type());
        builder_->drop(d.place, d.dtor, fv);
    }
}

MIRExprResult MIRLowerer::lower_arg_converted(Value* arg, BaseType* param_type,
                                              std::string& error) {
    BaseType* conv_type = param_type;
    if (param_type) {
        if (param_type->kind() == BaseTypeKind::Reference) {
            conv_type = param_type->as_reference_type()->type;
        } else if (param_type->kind() == BaseTypeKind::Pointer) {
            conv_type = param_type->as_pointer_type()->type;
        }
    }
    // an untyped constructor call for the parameter's own type
    // (`std::string_view("...")`): the callee identifier may not be linked, so
    // match its name against the parameter type and construct directly.
    if (conv_type && arg && arg->val_kind() == ValueKind::FunctionCall) {
        auto* acall = arg->as_func_call_unsafe();
        std::string cname;
        bool have = false;
        if (acall->parent_val) {
            if (acall->parent_val->val_kind() == ValueKind::Identifier) {
                auto* id = acall->parent_val->as_identifier_unsafe();
                cname.assign(id->value.data(), id->value.size());
                have = true;
            } else if (acall->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* ch = acall->parent_val->as_access_chain_unsafe();
                std::string e;
                if (!ch->values.empty() && member_name(ch->values.back(), cname, e)) have = true;
            }
        }
        ASTNode* ln = conv_type->get_direct_linked_canonical_node();
        if (!ln) ln = conv_type->get_direct_linked_node();
        if ((!ln || ln->kind() != ASTNodeKind::StructDecl) &&
            conv_type->kind() == BaseTypeKind::Generic) {
            if (auto* g = conv_type->as_generic_type()) {
                if (g->referenced) {
                    ln = g->referenced->get_direct_linked_canonical_node();
                    if (!ln) ln = g->referenced->get_direct_linked_node();
                }
            }
        }
        if (have && ln && ln->kind() == ASTNodeKind::StructDecl) {
            auto* sd = ln->as_struct_def_unsafe();
            if (sd->name_view() ==
                chem::string_view(cname.data(), static_cast<unsigned>(cname.size()))) {
                FunctionDeclaration* ctor = sd->constructor_func(acall->values);
                if (!ctor && acall->values.empty()) ctor = conv_type->get_def_constructor();
                const TypeId mt = types_.map(conv_type);
                const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                std::vector<MIROperand> cargs;
                if (!lower_call_args_for(ctor, true, acall->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, acall->values.size(), true, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && ctor->is_comptime() && comptime_eval_) {
                    Value* ev = comptime_eval_(acall, ctor);
                    if (ev) return lower_expr(ev, error);
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    builder_->zero_init(tmp);
                    builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                }
                return MIRExprResult::place(tmp, mt);
            }
        }
    }
    if (conv_type) {
        FunctionDeclaration* imp = nullptr;
        ASTNode* ln = conv_type->get_direct_linked_canonical_node();
        if (ln && ln->kind() == ASTNodeKind::StructDecl) {
            imp = ln->as_struct_def_unsafe()->implicit_constructor_func(arg);
        }
        if (!imp) imp = conv_type->implicit_constructor_for(arg);
        if (imp) {
            if (imp->is_comptime() && comptime_ctor_eval_) {
                Value* ev = comptime_ctor_eval_(imp, arg);
                if (ev) return lower_expr(ev, error);
            }
            const TypeId mt = types_.map(conv_type);
            const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
            std::vector<MIROperand> cargs;
            std::vector<Value*> one{arg};
            if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
            const SymbolId sym = intern_function(imp);
            builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
            return MIRExprResult::place(tmp, mt);
        }
    }
    return lower_expr(arg, error);
}

bool MIRLowerer::push_call_arg(MIRExprResult r, std::vector<MIROperand>& args,
                               std::string& error) {
    if (r.kind == MIRExprKind::Place) {
        const PlaceId pid = static_cast<PlaceId>(r.id);
        const TypeId pt = builder_->function().places[pid].type;
        const MIRTypeRecord& prec = module_.types.get(pt);
        if (prec.kind == MIRTypeKind::Pointer || prec.kind == MIRTypeKind::Reference) {
            args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
        } else if (prec.kind == MIRTypeKind::Array) {
            // arrays decay to a pointer to their first element
            const TypeId eptr = types_.pointer_type(prec.element, false);
            args.push_back(MIROperand::value(builder_->address_of(pid, eptr), eptr));
        } else {
            const TypeId ptrt = types_.pointer_type(pt, false);
            args.push_back(MIROperand::value(builder_->address_of(pid, ptrt), ptrt));
        }
    } else if (r.kind == MIRExprKind::Address) {
        args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
    } else if (r.kind == MIRExprKind::Void) {
        error = "void expression used as a call argument";
        return false;
    } else {
        args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
    }
    return true;
}

bool MIRLowerer::append_default_args(FunctionDeclaration* fd, size_t provided,
                                     bool self_included, std::vector<MIROperand>& args,
                                     std::string& error) {
    if (!fd) return true;
    auto& params = fd->params;
    const size_t offset = self_included ? 1 : 0;
    for (size_t i = provided + offset; i < params.size(); ++i) {
        if (fd->isVariadic() && i + 1 == params.size()) break; // variadic tail
        Value* dv = params[i]->defValue;
        if (!dv) {
            error = "missing required argument '" +
                    std::string(params[i]->name.data(), params[i]->name.size()) + "'";
            return false;
        }
        MIRExprResult r = lower_arg_converted(dv, params[i]->type, error);
        if (!r.ok()) return false;
        if (!push_call_arg(r, args, error)) return false;
    }
    return true;
}

MIRExprResult MIRLowerer::lower_method_call(Value* receiver, FunctionCall* call, std::string& error) {
    ASTNode* lk = call->parent_val ? call->parent_val->linked_node() : nullptr;
    FunctionDeclaration* fd = lk ? lk->as_function() : nullptr;
    if (!fd && call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
        auto* ch = call->parent_val->as_access_chain_unsafe();
        if (!ch->values.empty()) {
            std::string mname, merr;
            if (member_name(ch->values.back(), mname, merr)) {
                fd = resolve_method_fallback(receiver->getType(), mname);
            }
        }
    }
    if (!fd) {
        error = "method call to unresolved function";
        return MIRExprResult::error();
    }
    if (FunctionParam* self = fd->get_self_param()) {
        if (self->type && self->type->kind() == BaseTypeKind::Reference) {
            BaseType* inner = self->type->as_reference_type()->type;
            if (inner && (inner->kind() == BaseTypeKind::Reference ||
                          inner->kind() == BaseTypeKind::Pointer)) {
                error = "reference-to-reference method receiver is not yet supported";
                return MIRExprResult::error();
            }
        }
    }

    MIRExprResult recv = lower_expr(receiver, error);
    if (!recv.ok()) return recv;
    std::vector<MIROperand> args;
    if (recv.kind == MIRExprKind::Place) {
        const PlaceId rp = static_cast<PlaceId>(recv.id);
        const TypeId rt = builder_->function().places[rp].type;
        const MIRTypeRecord& rr = module_.types.get(rt);
        if (rr.kind == MIRTypeKind::Pointer || rr.kind == MIRTypeKind::Reference) {
            args.push_back(MIROperand::value(builder_->load(rp, rt), rt));
        } else {
            const TypeId ptrt = types_.pointer_type(rt, true);
            args.push_back(MIROperand::value(builder_->address_of(rp, ptrt), ptrt));
        }
    } else if (recv.kind == MIRExprKind::Value || recv.kind == MIRExprKind::Address) {
        const MIRTypeRecord& rr = module_.types.get(recv.type);
        if (rr.kind != MIRTypeKind::Pointer && rr.kind != MIRTypeKind::Reference) {
            if (rr.kind == MIRTypeKind::Opaque && fd->get_self_param()) {
                // the receiver node is untyped (its declaration was not fully
                // resolved); trust the resolved method's receiver type
                const TypeId st = types_.map(fd->get_self_param()->type);
                args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), st));
            } else {
                error = "method receiver must be addressable (recv kind " +
                        std::to_string(static_cast<int>(recv.kind)) + ", type kind " +
                        std::to_string(static_cast<int>(rr.kind)) + ", ast kind " +
                        std::to_string(static_cast<int>(receiver->val_kind())) + ")";
                return MIRExprResult::error();
            }
        } else {
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), recv.type));
        }
    } else {
        error = "method receiver must be an addressable place";
        return MIRExprResult::error();
    }
    if (!lower_call_args_for(fd, true, call->values, args, error)) {
        return MIRExprResult::error();
    }
    if (!append_default_args(fd, call->values.size(), true, args, error)) {
        return MIRExprResult::error();
    }

    if (fd->is_comptime() && comptime_eval_) {
        Value* evaluated = comptime_eval_(call, fd);
        if (!evaluated) {
            error = "comptime call to '" + fd->name_str() + "' could not be evaluated";
            return MIRExprResult::error();
        }
        return lower_expr(evaluated, error);
    }

    const SymbolId sym = intern_function(fd);
    const TypeId type = types_.map(call->getType());
    if (is_void_type(module_, type)) {
        builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
        destroy_call_temps(error);
        return MIRExprResult::void_result();
    }
    if (needs_aggregate_path(module_, type)) {
        const PlaceId res = builder_->alloca(type, MIRStorageClass::Temporary);
        if (res == MIR_INVALID_ID) {
            error = "failed to allocate method result place";
            return MIRExprResult::error();
        }
        builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
        destroy_call_temps(error);
        return MIRExprResult::place(res, type);
    }
    const ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
    destroy_call_temps(error);
    return MIRExprResult::value(v, type);
}

SymbolId MIRLowerer::intern_global(VarInitStatement* vi) {
    auto it = global_symbols_.find(vi);
    if (it != global_symbols_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    rec.linkage = vi->is_extern() ? MIRLinkage::External : MIRLinkage::Internal;
    if (vi->getType()) rec.type = types_.map(vi->getType());
    std::string name;
    if (mangler_) name = mangler_(vi);
    if (name.empty()) {
        const chem::string_view nv = vi->name_view();
        name.assign(nv.data(), nv.size());
    }
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    global_symbols_[vi] = s;
    return s;
}

SymbolId MIRLowerer::intern_named_global(const std::string& name) {
    auto it = named_globals_.find(name);
    if (it != named_globals_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    rec.linkage = MIRLinkage::External;
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    named_globals_[name] = s;
    return s;
}

SymbolId MIRLowerer::intern_function(FunctionDeclaration* decl) {
    auto it = func_symbols_.find(decl);
    if (it != func_symbols_.end()) return it->second;

    TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    std::vector<TypeId> ptypes;
    ptypes.reserve(decl->params.size());
    for (FunctionParam* p : decl->params) {
        ptypes.push_back(p && p->type ? types_.map(p->type) : types_.opaque_type());
    }
    TypeId ftype = types_.function_signature(ret, ptypes);

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = decl->is_extern() ? MIRLinkage::External : MIRLinkage::Internal;
    rec.type = ftype;

    // NOTE: mangled name is filled by the serial symbol builder (PR3b) using
    // NameMangler; MIR must not retain FunctionDeclaration pointers.
    std::string name;
    if (mangler_) name = mangler_(decl);
    if (name.empty()) {
        const chem::string_view nv = decl->name_view();
        name.assign(nv.data(), nv.size());
    }
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    func_symbols_[decl] = s;
    return s;
}

MIRExprResult MIRLowerer::lower_expr(Value* value, std::string& error) {
    if (!value) {
        error = "null value node";
        return MIRExprResult::error();
    }

    const TypeId type = types_.map(value->getType());

    switch (value->val_kind()) {
        case ValueKind::IntN: {
            auto* n = value->as_int_num_value_unsafe();
            ConstantId c = module_.constants.add_int(type, n->get_num_value());
            return MIRExprResult::value(builder_->const_int(type, c), type);
        }
        case ValueKind::Bool: {
            auto* b = value->as_bool_unsafe();
            ConstantId c = module_.constants.add_bool(type, b->value);
            return MIRExprResult::value(builder_->const_bool(type, c), type);
        }
        case ValueKind::Float: {
            auto* f = value->as_float_unsafe();
            ConstantId c = module_.constants.add_float(type, f->value);
            return MIRExprResult::value(builder_->const_float(type, c), type);
        }
        case ValueKind::Double: {
            auto* d = value->as_double_unsafe();
            ConstantId c = module_.constants.add_double(type, d->value);
            return MIRExprResult::value(builder_->const_double(type, c), type);
        }
        case ValueKind::String: {
            auto* s = value->as_string_unsafe();
            ConstantId c = module_.constants.add_string(type, s->value.data(), s->length);
            return MIRExprResult::value(builder_->const_string(type, c), type);
        }
        case ValueKind::Identifier: {
            auto* id = value->as_identifier_unsafe();
            if (Value* cv = captured_comptime_value(id->linked)) {
                return lower_expr(cv, error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::FunctionDecl) {
                // a function used as a value (function pointer)
                FunctionDeclaration* fd = id->linked->as_function_unsafe();
                const SymbolId sym = intern_function(fd);
                return MIRExprResult::value(builder_->function_addr(sym, type), type);
            }
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID) place = place_for_name(id->value);
            if (place != MIR_INVALID_ID) {
                if (needs_aggregate_path(module_, type)) {
                    return MIRExprResult::place(place, type);
                }
                return MIRExprResult::value(builder_->load(place, type), type);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* vi = id->linked->as_var_init();
                if (vi->is_comptime() && vi->value) {
                    // module-level comptime constants are inlined at their use
                    return lower_expr(vi->value, error);
                }
                const SymbolId gs = intern_global(vi);
                // an array global decays to a pointer to its first element
                const TypeId ptrt = (type < module_.types.size() &&
                                     module_.types.get(type).kind == MIRTypeKind::Array)
                                        ? types_.pointer_type(module_.types.get(type).element, false)
                                        : types_.pointer_type(type, false);
                const ValueId addr = builder_->global_addr(gs, ptrt);
                if (needs_aggregate_path(module_, type)) {
                    return MIRExprResult::address(addr, ptrt);
                }
                return MIRExprResult::value(builder_->load_indirect(addr, type), type);
            }
            // unresolved identifier: treat as a module-level / extern global
            {
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId ptrt = (type < module_.types.size() &&
                                         module_.types.get(type).kind == MIRTypeKind::Array)
                                            ? types_.pointer_type(module_.types.get(type).element, false)
                                            : types_.pointer_type(type, false);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    if (needs_aggregate_path(module_, type)) {
                        return MIRExprResult::address(addr, ptrt);
                    }
                    return MIRExprResult::value(builder_->load_indirect(addr, type), type);
                }
            }
            error = "identifier does not resolve to a local place (name '" +
                    std::string(id->value.data(), id->value.size()) + "')";
            return MIRExprResult::error();
        }
        case ValueKind::NegativeValue: {
            auto* n = value->as_negative_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Neg));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::NotValue: {
            auto* n = value->as_not_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Not));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::BitwiseNot: {
            auto* n = value->as_bitwise_not_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::BitNot));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::Expression: {
            auto* e = value->as_expression_unsafe();
            if (e->operation == Operation::LogicalAND || e->operation == Operation::LogicalOR) {
                const TypeId bool_t = types_.bool_type();
                MIRExprResult a = lower_expr(e->firstValue, error);
                if (!a.ok()) return a;
                const PlaceId res = builder_->alloca(bool_t, MIRStorageClass::Temporary);
                builder_->store(res, a.id);
                const BlockId rhs_b = builder_->create_block();
                const BlockId end_b = builder_->create_block();
                if (e->operation == Operation::LogicalAND) {
                    builder_->cond_br(a.id, rhs_b, end_b);
                } else {
                    builder_->cond_br(a.id, end_b, rhs_b);
                }
                builder_->set_block(rhs_b);
                MIRExprResult b = lower_expr(e->secondValue, error);
                if (!b.ok()) return b;
                builder_->store(res, b.id);
                builder_->br(end_b);
                builder_->set_block(end_b);
                return MIRExprResult::value(builder_->load(res, bool_t), bool_t);
            }
            MIRExprResult lhs = lower_expr(e->firstValue, error);
            if (!lhs.ok()) return lhs;
            MIRExprResult rhs = lower_expr(e->secondValue, error);
            if (!rhs.ok()) return rhs;
            MIROpcode op;
            MIRBinaryOp bop;
            if (!map_binary(e->operation, op, bop)) {
                error = "unsupported binary operator (short-circuit/logical operators need CFG)";
                return MIRExprResult::error();
            }
            ConstantId opc = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(bop));
            if (op == MIROpcode::Compare) {
                return MIRExprResult::value(builder_->compare(lhs.id, rhs.id, opc, type), type);
            }
            return MIRExprResult::value(builder_->binary(lhs.id, rhs.id, opc, type), type);
        }
        case ValueKind::CastedValue: {
            auto* c = value->as_casted_value_unsafe();
            MIRExprResult in = lower_expr(c->value, error);
            if (!in.ok()) return in;
            if (type != MIR_INVALID_ID && type < module_.types.size() &&
                module_.types.get(type).kind == MIRTypeKind::Array) {
                // casting a pointer to an array: copy the bytes into a temp array
                const PlaceId tmp = builder_->alloca(type, MIRStorageClass::Temporary);
                if (in.kind == MIRExprKind::Value || in.kind == MIRExprKind::Address) {
                    builder_->copy_ptr_to_place(tmp, static_cast<ValueId>(in.id));
                } else if (in.kind == MIRExprKind::Place) {
                    const TypeId pt = builder_->function().places[in.id].type;
                    const ValueId ptr = builder_->address_of(
                        static_cast<PlaceId>(in.id), types_.pointer_type(pt, false));
                    builder_->copy_ptr_to_place(tmp, ptr);
                }
                return MIRExprResult::place(tmp, type);
            }
            if (in.type == type) return in;
            return MIRExprResult::value(builder_->cast(in.id, type), type);
        }
        case ValueKind::AccessChain: {
            auto* chain = value->as_access_chain_unsafe();
            if (chain->values.size() == 1) {
                return lower_expr(chain->values[0], error);
            }
            // enum member access: `EnumName.Member` -> its constant value
            if (ASTNode* lk = chain->linked_node()) {
                if (lk->kind() == ASTNodeKind::EnumMember) {
                    auto* em = lk->as_enum_member();
                    if (em && em->init_value) return lower_expr(em->init_value, error);
                    const uint64_t idx =
                        em ? static_cast<uint64_t>(em->get_default_index()) : 0;
                    const ConstantId c = module_.constants.add_int(type, idx);
                    return MIRExprResult::value(builder_->const_int(type, c), type);
                }
            }
            // static method reference: `Type::method` -> a function pointer
            if (chain->values.size() >= 2) {
                if (ASTNode* lk = chain->values.back()->linked_node()) {
                    if (lk->kind() == ASTNodeKind::VarInitStmt) {
                        // qualified module-level constant/global: resolve the leaf
                        return lower_expr(chain->values.back(), error);
                    }
                    if (lk->kind() == ASTNodeKind::FunctionDecl) {
                        auto* fd = lk->as_function();
                        ASTNode* base = chain->values[0]->linked_node();
                        const bool base_is_type =
                            base && (base->kind() == ASTNodeKind::StructDecl ||
                                     base->kind() == ASTNodeKind::ImplDecl ||
                                     base->kind() == ASTNodeKind::InterfaceDecl ||
                                     base->kind() == ASTNodeKind::NamespaceDecl ||
                                     base->kind() == ASTNodeKind::VariantDecl ||
                                     base->kind() == ASTNodeKind::UnionDecl ||
                                     base->kind() == ASTNodeKind::EnumDecl ||
                                     base->kind() == ASTNodeKind::GenericStructDecl);
                        if (base_is_type) {
                            const SymbolId sym = intern_function(fd);
                            return MIRExprResult::value(builder_->function_addr(sym, type),
                                                        type);
                        }
                    }
                }
            }
            // member access: walk the chain from the base, taking the address of
            // each intermediate field and loading the last
            MIRExprResult first = lower_expr(chain->values[0], error);
            if (!first.ok()) return first;
            PlaceId cur_place = MIR_INVALID_ID;
            ValueId cur_ptr = MIR_NULL;
            bool have_ptr = false;
            if (first.kind == MIRExprKind::Place) {
                cur_place = static_cast<PlaceId>(first.id);
            } else if (first.kind == MIRExprKind::Value ||
                       first.kind == MIRExprKind::Address) {
                cur_ptr = static_cast<ValueId>(first.id);
                have_ptr = true;
            } else {
                error = "member access base is not a place or pointer";
                return MIRExprResult::error();
            }
            const size_t n = chain->values.size();
            for (size_t i = 1; i < n; ++i) {
                std::string fname;
                if (!member_name(chain->values[i], fname, error)) return MIRExprResult::error();
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                const TypeId ft = types_.map(chain->values[i]->getType());
                const bool last = (i + 1 == n);
                if (!have_ptr) {
                    if (last) {
                        if (needs_aggregate_path(module_, ft)) {
                            // aggregate fields are returned by address (lvalue)
                            return MIRExprResult::address(builder_->field_addr(cur_place, fc, ft),
                                                          ft);
                        }
                        return MIRExprResult::value(builder_->field_load(cur_place, fc, ft), ft);
                    }
                    const ValueId faddr = builder_->field_addr(cur_place, fc, ft);
                    const MIRTypeRecord& ftr = module_.types.get(ft);
                    cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                               ftr.kind == MIRTypeKind::Reference)
                                  ? builder_->load_indirect(faddr, ft)
                                  : faddr;
                    have_ptr = true;
                } else {
                    if (last) {
                        if (needs_aggregate_path(module_, ft)) {
                            return MIRExprResult::address(builder_->field_addr_ptr(cur_ptr, fc, ft),
                                                          ft);
                        }
                        return MIRExprResult::value(builder_->field_load_ptr(cur_ptr, fc, ft), ft);
                    }
                    const ValueId faddr = builder_->field_addr_ptr(cur_ptr, fc, ft);
                    const MIRTypeRecord& ftr = module_.types.get(ft);
                    cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                               ftr.kind == MIRTypeKind::Reference)
                                  ? builder_->load_indirect(faddr, ft)
                                  : faddr;
                }
            }
            error = "empty member chain";
            return MIRExprResult::error();
        }
        case ValueKind::StructValue: {
            auto* sv = value->as_struct_value_unsafe();
            const TypeId st = type;
            const PlaceId temp = builder_->alloca(st, MIRStorageClass::Temporary);
            if (temp == MIR_INVALID_ID) {
                error = "failed to allocate struct temporary";
                return MIRExprResult::error();
            }
            // zero the temp so members that are not explicitly set are
            // well-defined (matches the legacy backend's default initialization)
            builder_->zero_init(temp);
            StructDefinition* sd = nullptr;
            if (BaseType* ctype = value->getType()) {
                ASTNode* cn = ctype->get_direct_linked_canonical_node();
                if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                    sd = cn->as_struct_def_unsafe();
                }
            }
            auto it = sv->values.begin();
            while (it != sv->values.end()) {
                auto& init = it.value();
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, init.name.data(), static_cast<uint32_t>(init.name.size()));
                // implicit conversion: if the member type has an implicit
                // constructor for the initializer value, construct into the field
                FunctionDeclaration* implicit = nullptr;
                BaseType* member_type = nullptr;
                {
                    auto mtw = sv->child_type_w_index(init.name);
                    if (mtw.second != -1) {
                        member_type = mtw.first;
                        ASTNode* ln = member_type->get_direct_linked_canonical_node();
                        if (ln && ln->kind() == ASTNodeKind::StructDecl) {
                            implicit = ln->as_struct_def_unsafe()->implicit_constructor_func(init.value);
                        }
                        if (!implicit) implicit = member_type->implicit_constructor_for(init.value);
                    }
                }
                if (implicit) {
                    if (implicit->is_comptime() && comptime_ctor_eval_) {
                        Value* evaluated = comptime_ctor_eval_(implicit, init.value);
                        if (evaluated) {
                            MIRExprResult er = lower_expr(evaluated, error);
                            if (!er.ok()) return er;
                            if (er.kind == MIRExprKind::Place) {
                                builder_->field_store_place(temp, fc, static_cast<PlaceId>(er.id));
                            } else if (er.kind == MIRExprKind::Value) {
                                builder_->field_store(temp, fc, static_cast<ValueId>(er.id));
                            } else {
                                error = "comptime constructor did not produce a value";
                                return MIRExprResult::error();
                            }
                            ++it;
                            continue;
                        }
                    }
                    const TypeId mt = types_.map(member_type);
                    const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                    std::vector<MIROperand> cargs;
                    std::vector<Value*> one{init.value};
                    if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
                    const SymbolId sym = intern_function(implicit);
                    builder_->zero_init(tmp);
                    builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                    builder_->field_store_place(temp, fc, tmp);
                    ++it;
                    continue;
                }
                MIRExprResult r = lower_expr(init.value, error);
                if (!r.ok()) return r;
                if (r.kind == MIRExprKind::Place) {
                    builder_->field_store_place(temp, fc, static_cast<PlaceId>(r.id));
                    ++it;
                    continue;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "unsupported struct field initializer";
                    return MIRExprResult::error();
                }
                // if the field is an aggregate value but the initializer is a
                // pointer/reference, dereference to get the value
                if (!member_type && sd) {
                    auto mtw2 = sd->variable_type_w_index(init.name);
                    if (mtw2.second != -1) member_type = mtw2.first;
                }
                if (member_type) {
                    const MIRTypeRecord& rt = module_.types.get(r.type);
                    if ((rt.kind == MIRTypeKind::Pointer ||
                         rt.kind == MIRTypeKind::Reference) &&
                        needs_aggregate_path(module_, types_.map(member_type))) {
                        builder_->field_store(
                            temp, fc,
                            builder_->load_indirect(r.id, types_.map(member_type)));
                        ++it;
                        continue;
                    }
                }
                builder_->field_store(temp, fc, r.id);
                ++it;
            }
            // initialize members that were not explicitly set with their default
            // values (the struct temp was zeroed above, so scalars are already 0)
            if (sd) {
                for (BaseDefMember* member : sd->variables()) {
                    if (!member) continue;
                    const chem::string_view mname = member->name;
                    if (sv->values.find(mname) != sv->values.end()) continue;
                    Value* dv = member->default_value();
                    if (!dv) continue;
                    MIRExprResult r = lower_expr(dv, error);
                    if (!r.ok()) return r;
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, mname.data(), static_cast<uint32_t>(mname.size()));
                    if (r.kind == MIRExprKind::Place) {
                        builder_->field_store_place(temp, fc, static_cast<PlaceId>(r.id));
                    } else if (r.kind == MIRExprKind::Value) {
                        builder_->field_store(temp, fc, r.id);
                    }
                }
            }
            return MIRExprResult::place(temp, st);
        }
        case ValueKind::FunctionCall: {
            auto* call = value->as_func_call_unsafe();
            if (call->captured_ref_owner && call->captured_ref_index >= 0 &&
                call->captured_ref_index <
                    static_cast<int>(call->captured_ref_owner->refs.size())) {
                Value* ev =
                    call->captured_ref_owner->refs[call->captured_ref_index].evaluated;
                if (ev) return lower_expr(ev, error);
            }
            FunctionDeclaration* fd = nullptr;
            std::vector<MIROperand> args;

            // aggregate construction: `Type(args)` / `ns.Type(args)` /
            // `ns.Generic<...>(args)`
            ASTNode* ctor_node = nullptr;
            if (call->parent_val) {
                ASTNode* plk = call->parent_val->linked_node();
                if (plk && plk->as_struct_def()) {
                    ctor_node = plk;
                } else if (call->parent_val->val_kind() == ValueKind::AccessChain) {
                    auto* ch = call->parent_val->as_access_chain_unsafe();
                    if (!ch->values.empty()) {
                        ASTNode* lk = ch->values.back()->linked_node();
                        if (lk && (lk->as_struct_def() || lk->kind() == ASTNodeKind::GenericStructDecl)) {
                            ctor_node = lk;
                        }
                    }
                }
            }
            if (!ctor_node && call->getType() && call->parent_val &&
                call->parent_val->val_kind() == ValueKind::AccessChain) {
                // unlinked constructor call (`std::string_view(...)`): match the
                // callee name against the call's aggregate type name
                auto* ch = call->parent_val->as_access_chain_unsafe();
                std::string mname, merr;
                if (!ch->values.empty() && member_name(ch->values.back(), mname, merr)) {
                    ASTNode* cn = call->getType()->get_direct_linked_canonical_node();
                    if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                        auto* sd = cn->as_struct_def_unsafe();
                        if (sd->name_view() ==
                            chem::string_view(mname.data(), static_cast<unsigned>(mname.size()))) {
                            ctor_node = cn;
                        }
                    }
                }
            }
            if (ctor_node) {
                BaseType* ctype = call->getType();
                const TypeId st = types_.map(ctype);
                const PlaceId place = builder_->alloca(st, MIRStorageClass::Temporary);
                StructDefinition* sd = nullptr;
                if (ctype) {
                    ASTNode* cn = ctype->get_direct_linked_canonical_node();
                    if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                        sd = cn->as_struct_def_unsafe();
                    }
                }
                if (!sd) sd = ctor_node->as_struct_def();
                FunctionDeclaration* ctor = sd ? sd->constructor_func(call->values) : nullptr;
                if (!ctor && ctype && call->values.empty()) ctor = ctype->get_def_constructor();
                std::vector<MIROperand> cargs;
                if (!lower_call_args_for(ctor, true, call->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, call->values.size(), true, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && ctor->is_comptime() && comptime_eval_) {
                    Value* ev = comptime_eval_(call, ctor);
                    if (ev) return lower_expr(ev, error);
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    builder_->zero_init(place);
                    builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                } else if (!cargs.empty()) {
                    error = "no matching constructor for aggregate construction";
                    return MIRExprResult::error();
                }
                return MIRExprResult::place(place, st);
            }

            // method call: `receiver.method(args)`
            if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* chain = call->parent_val->as_access_chain_unsafe();
                if (chain->values.size() >= 2) {
                    // resolve the callee first: a namespace-qualified function
                    // (e.g. `lab::curr_dir()`) must not be mistaken for a
                    // constructor even when it returns an aggregate
                    ASTNode* callee_node = call->parent_val->linked_node();
                    FunctionDeclaration* mfd = callee_node ? callee_node->as_function() : nullptr;
                    if (!mfd) {
                        // fallback: the method identifier may not have been linked
                        // (extension methods on an interface receiver); resolve it
                        // by name on the receiver's canonical container.
                        std::string mname, merr;
                        if (member_name(chain->values.back(), mname, merr)) {
                            mfd = resolve_method_fallback(chain->values[0]->getType(), mname);
                        }
                    }
                    // namespace-qualified aggregate construction: the callee is
                    // not a function, the receiver is not an addressable place,
                    // and the result is an aggregate
                    std::string probe;
                    const bool recv_is_place =
                        resolve_place(chain->values[0], probe) != MIR_INVALID_ID;
                    const TypeId ct = types_.map(call->getType());
                    if (!mfd && !recv_is_place && needs_aggregate_path(module_, ct)) {
                        const PlaceId place = builder_->alloca(ct, MIRStorageClass::Temporary);
                        FunctionDeclaration* ctor = nullptr;
                        if (call->getType()) {
                            ASTNode* cn = call->getType()->get_direct_linked_canonical_node();
                            if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                                ctor = cn->as_struct_def_unsafe()->constructor_func(call->values);
                            }
                        }
                        if (!ctor && call->getType() && call->values.empty()) {
                            ctor = call->getType()->get_def_constructor();
                        }
                        std::vector<MIROperand> cargs;
                        if (!lower_call_args_for(ctor, true, call->values, cargs, error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && !append_default_args(ctor, call->values.size(), true, cargs,
                                                         error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && ctor->is_comptime() && comptime_eval_) {
                            Value* ev = comptime_eval_(call, ctor);
                            if (ev) return lower_expr(ev, error);
                        }
                        if (ctor) {
                            const SymbolId sym = intern_function(ctor);
                            builder_->zero_init(place);
                            builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                        } else if (!cargs.empty()) {
                            error = "no matching constructor for aggregate construction";
                            return MIRExprResult::error();
                        }
                        return MIRExprResult::place(place, ct);
                    }
                    const bool needs_receiver =
                        mfd && (mfd->get_self_param() != nullptr || mfd->isExtensionFn());
                    if (needs_receiver) {
                        if (chain->values.size() != 2) {
                            error = "chained method calls are not yet supported";
                            return MIRExprResult::error();
                        }
                        return lower_method_call(chain->values[0], call, error);
                    }
                    // otherwise: namespace/type-qualified free function or a
                    // static method -- fall through to the direct-call path,
                    // which passes no receiver.
                }
            }
            {
                ASTNode* linked_fn = call->parent_val ? call->parent_val->linked_node() : nullptr;
                fd = linked_fn ? linked_fn->as_function() : nullptr;
                if (!fd) {
                    // function-pointer variable/parameter call -> call_indirect
                    PlaceId fp = linked_fn ? place_for_linked(linked_fn) : MIR_INVALID_ID;
                    if (fp == MIR_INVALID_ID && call->parent_val &&
                        call->parent_val->val_kind() == ValueKind::Identifier) {
                        fp = place_for_linked(call->parent_val->as_identifier_unsafe()->linked);
                    }
                    if (fp == MIR_INVALID_ID) {
                        std::string callee;
                        int last_linked_kind = -2;
                        int pv_kind = -1;
                        if (call->parent_val) {
                            Value* pv = call->parent_val;
                            pv_kind = static_cast<int>(pv->val_kind());
                            if (pv->val_kind() == ValueKind::AccessChain) {
                                auto* ch = pv->as_access_chain_unsafe();
                                if (!ch->values.empty()) {
                                    std::string n, e;
                                    if (member_name(ch->values.back(), n, e)) callee = n;
                                    ASTNode* ll = ch->values.back()->linked_node();
                                    last_linked_kind = ll ? static_cast<int>(ll->kind()) : -1;
                                }
                            } else if (pv->val_kind() == ValueKind::Identifier) {
                                auto* id = pv->as_identifier_unsafe();
                                callee.assign(id->value.data(), id->value.size());
                                last_linked_kind = id->linked ? static_cast<int>(id->linked->kind()) : -1;
                            }
                        }
                        error = "call to unresolved function '" + callee + "' (linked kind " +
                                std::to_string(linked_fn
                                                   ? static_cast<int>(linked_fn->kind())
                                                   : -1) +
                                ", pv kind " + std::to_string(pv_kind) + ", last linked " +
                                std::to_string(last_linked_kind) + ")";
                        return MIRExprResult::error();
                    }
                    const TypeId ft = builder_->function().places[fp].type;
                    if (ft != MIR_INVALID_ID && ft < module_.types.size()) {
                        const MIRTypeRecord& ftr = module_.types.get(ft);
                        const MIRTypeRecord* eff = &ftr;
                        if ((ftr.kind == MIRTypeKind::Pointer ||
                             ftr.kind == MIRTypeKind::Reference) &&
                            ftr.element < module_.types.size()) {
                            eff = &module_.types.get(ftr.element);
                        }
                        if (eff->kind == MIRTypeKind::Struct) {
                            // capturing function instance (`std::function`) call
                            error = "capturing function call is not yet supported";
                            return MIRExprResult::error();
                        }
                    }
                    const ValueId fnptr = builder_->load(fp, ft);
                    std::vector<MIROperand> cargs;
                    if (!lower_call_args(call->values, cargs, error)) return MIRExprResult::error();
                    const TypeId rt = types_.map(call->getType());
                    if (is_void_type(module_, rt)) {
                        builder_->call_indirect(fnptr, rt, cargs.data(), static_cast<uint32_t>(cargs.size()));
                        return MIRExprResult::void_result();
                    }
                    if (needs_aggregate_path(module_, rt)) {
                        // aggregate return: use the sret convention (the callee
                        // returns void and writes through a leading pointer)
                        const PlaceId tmp = builder_->alloca(rt, MIRStorageClass::Temporary);
                        builder_->zero_init(tmp);
                        builder_->call_indirect_sret(fnptr, tmp, cargs.data(),
                                                     static_cast<uint32_t>(cargs.size()));
                        return MIRExprResult::place(tmp, rt);
                    }
                    const ValueId v = builder_->call_indirect(fnptr, rt, cargs.data(),
                                                              static_cast<uint32_t>(cargs.size()));
                    return MIRExprResult::value(v, rt);
                }
            }
            if (fd->is_comptime() && comptime_eval_) {
                Value* evaluated = comptime_eval_(call, fd);
                if (!evaluated) {
                    error = "comptime call to '" + fd->name_str() + "' could not be evaluated";
                    return MIRExprResult::error();
                }
                return lower_expr(evaluated, error);
            }
            SymbolId sym = intern_function(fd);

            args.reserve(args.size() + call->values.size());
            if (!lower_call_args_for(fd, false, call->values, args, error)) {
                return MIRExprResult::error();
            }
            if (!append_default_args(fd, call->values.size(), false, args, error)) {
                return MIRExprResult::error();
            }

            if (is_void_type(module_, type)) {
                builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
                destroy_call_temps(error);
                return MIRExprResult::void_result();
            }
            if (needs_aggregate_path(module_, type)) {
                const PlaceId res = builder_->alloca(type, MIRStorageClass::Temporary);
                if (res == MIR_INVALID_ID) {
                    error = "failed to allocate struct-return result place";
                    return MIRExprResult::error();
                }
                builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
                destroy_call_temps(error);
                return MIRExprResult::place(res, type);
            }
            ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
            destroy_call_temps(error);
            return MIRExprResult::value(v, type);
        }
        case ValueKind::AddrOfValue: {
            auto* a = value->as_addr_of_value_unsafe();
            return lower_address_of(a->value, error);
        }
        case ValueKind::ReferenceOfValue: {
            auto* r = value->as_reference_of_value_unsafe();
            return lower_address_of(r->value, error);
        }
        case ValueKind::ArrayValue: {
            auto* arr = value->as_array_value_unsafe();
            const TypeId at = type;
            if (at == MIR_INVALID_ID || at >= module_.types.size() ||
                module_.types.get(at).kind != MIRTypeKind::Array) {
                error = "array literal without an array type";
                return MIRExprResult::error();
            }
            const PlaceId place = builder_->alloca(at, MIRStorageClass::Temporary);
            // zero the array so elements not covered by the literal are 0
            builder_->zero_init(place);
            const TypeId ut = types_.u32_type();
            for (size_t i = 0; i < arr->values.size(); ++i) {
                MIRExprResult e = lower_expr(arr->values[i], error);
                if (!e.ok()) return e;
                ValueId val = MIR_NULL;
                if (e.kind == MIRExprKind::Place) {
                    const PlaceId pid = static_cast<PlaceId>(e.id);
                    const TypeId pt = builder_->function().places[pid].type;
                    const ValueId idx = builder_->const_int(
                        ut, module_.constants.add_int(ut, static_cast<uint64_t>(i)));
                    if (needs_aggregate_path(module_, pt)) {
                        builder_->element_store_place(place, idx, pid);
                        continue;
                    }
                    val = builder_->load(pid, pt);
                    builder_->element_store(place, idx, val);
                    continue;
                }
                if (e.kind == MIRExprKind::Value || e.kind == MIRExprKind::Address) {
                    val = static_cast<ValueId>(e.id);
                } else {
                    error = "unsupported array element expression";
                    return MIRExprResult::error();
                }
                const ValueId idx = builder_->const_int(
                    ut, module_.constants.add_int(ut, static_cast<uint64_t>(i)));
                builder_->element_store(place, idx, val);
            }
            return MIRExprResult::place(place, at);
        }
        case ValueKind::IndexOperator: {
            auto* io = value->as_index_op_unsafe();
            MIRExprResult base = lower_expr(io->parent_val, error);
            if (!base.ok()) return base;
            MIRExprResult idx = lower_expr(io->idx, error);
            if (!idx.ok()) return idx;
            if (needs_aggregate_path(module_, type)) {
                error = "indexing to an aggregate element is not yet supported";
                return MIRExprResult::error();
            }
            if (base.kind == MIRExprKind::Place) {
                return MIRExprResult::value(
                    builder_->element_load(static_cast<PlaceId>(base.id),
                                           static_cast<ValueId>(idx.id), type), type);
            }
            if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
                return MIRExprResult::value(
                    builder_->index_load(static_cast<ValueId>(base.id),
                                         static_cast<ValueId>(idx.id), type), type);
            }
            error = "index base is not a place or pointer value";
            return MIRExprResult::error();
        }
        case ValueKind::NullValue: {
            const ConstantId c = module_.constants.add_null(type);
            return MIRExprResult::value(builder_->const_null(type, c), type);
        }
        case ValueKind::DereferenceValue: {
            auto* d = value->as_dereference_value_unsafe();
            MIRExprResult in = lower_expr(d->getValue(), error);
            if (!in.ok()) return in;
            if (in.kind != MIRExprKind::Value && in.kind != MIRExprKind::Address) {
                error = "dereference operand is not a pointer value";
                return MIRExprResult::error();
            }
            if (needs_aggregate_path(module_, type)) {
                return MIRExprResult::address(static_cast<ValueId>(in.id), in.type);
            }
            return MIRExprResult::value(
                builder_->load_indirect(static_cast<ValueId>(in.id), type), type);
        }
        case ValueKind::IncDecValue: {
            auto* n = value->as_inc_dec_value_unsafe();
            ValueId out = MIR_NULL;
            if (!lower_incdec_value(n->getValue(), n->increment, n->post, error, out)) {
                return MIRExprResult::error();
            }
            return MIRExprResult::value(out, type);
        }
        case ValueKind::SizeOfValue: {
            auto* s = value->as_sizeof_value_unsafe();
            const TypeId ft =
                s->for_type.getType() ? types_.map(const_cast<BaseType*>(s->for_type.getType())) : types_.opaque_type();
            return MIRExprResult::value(builder_->size_of(ft, type), type);
        }
        case ValueKind::AlignOfValue: {
            auto* a = static_cast<AlignOfValue*>(value);
            const TypeId ft =
                a->for_type.getType() ? types_.map(const_cast<BaseType*>(a->for_type.getType())) : types_.opaque_type();
            return MIRExprResult::value(builder_->align_of(ft, type), type);
        }
        case ValueKind::OffsetOfValue: {
            auto* o = value->as_offset_of_value_unsafe();
            const TypeId ft =
                o->for_type.getType() ? types_.map(const_cast<BaseType*>(o->for_type.getType())) : types_.opaque_type();
            const std::string mn(o->member_name.data(), o->member_name.size());
            const ConstantId fc =
                module_.constants.add_string(MIR_INVALID_ID, mn.data(),
                                            static_cast<uint32_t>(mn.size()));
            return MIRExprResult::value(builder_->offset_of(ft, fc, type), type);
        }
        case ValueKind::UnsafeValue: {
            // `unsafe(expr)` is a compile-time safety marker only
            auto* u = static_cast<UnsafeValue*>(value);
            if (u->getValue()) return lower_expr(u->getValue(), error);
            error = "unsafe value has no inner expression";
            return MIRExprResult::error();
        }
        case ValueKind::ZeroedValue: {
            const TypeId zt = type;
            const PlaceId tmp = builder_->alloca(zt, MIRStorageClass::Temporary);
            builder_->zero_init(tmp);
            if (needs_aggregate_path(module_, zt)) return MIRExprResult::place(tmp, zt);
            return MIRExprResult::value(builder_->load(tmp, zt), zt);
        }
        case ValueKind::RuntimeValue:
            return lower_expr(static_cast<RuntimeValue*>(value)->underlying, error);
        case ValueKind::ComptimeValue:
            return lower_expr(static_cast<ComptimeValue*>(value)->getValue(), error);
        default:
            error = "unsupported value kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(value->val_kind())) + ")";
            return MIRExprResult::error();
    }
}

bool MIRLowerer::lower_stmt(ASTNode* node, std::string& error) {
    if (!node) return true;

    switch (node->kind()) {
        case ASTNodeKind::VarInitStmt: {
            auto* vi = node->as_var_init();
            TypeId vt = types_.map(vi->known_type());
            PlaceId place = MIR_INVALID_ID;
            if (async_) {
                const std::string rf = async_resident_field(vi);
                if (!rf.empty()) {
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, rf.data(), static_cast<uint32_t>(rf.size()));
                    place = builder_->alloca_frame(vt, fc);
                }
            }
            if (place == MIR_INVALID_ID) place = builder_->alloca(vt, MIRStorageClass::Local);
            if (place == MIR_INVALID_ID) {
                error = "failed to allocate place for variable";
                return false;
            }
            bind(vi, place);
            bind_name(vi->name_view(), place);
            if (async_ && vi->value && vi->value->kind() == ValueKind::AwaitExpr) {
                auto it = async_site_index_.find(vi);
                if (it == async_site_index_.end()) {
                    error = "await var init has no matching site";
                    return false;
                }
                async_result_places_[it->second] = place;
                return async_lower_await_var_init(vi, it->second, error);
            }
            if (vi->known_type()) {
                const MIRTypeRecord& vr = module_.types.get(vt);
                if (!(vr.flags & TF_TYPEDEF) && needs_aggregate_path(module_, vt)) {
                    register_destructible(place, vi->known_type());
                }
            }
            if (vi->value) {
                // moving a destructible value out of another local/param
                {
                    Value* iv = vi->value;
                    if (iv->val_kind() == ValueKind::AccessChain) {
                        auto* c = iv->as_access_chain_unsafe();
                        if (c->values.size() == 1) iv = c->values[0];
                    }
                    if (iv->val_kind() == ValueKind::Identifier) {
                        auto* iid = iv->as_identifier_unsafe();
                        PlaceId src = place_for_linked(iid->linked);
                        if (src == MIR_INVALID_ID) src = place_for_name(iid->value);
                        if (src != MIR_INVALID_ID) mark_moved(src);
                    }
                }
                MIRExprResult r = lower_expr(vi->value, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    // aggregate initializer: move the temporary into the variable
                    builder_->move_init(place, static_cast<PlaceId>(r.id), vt);
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "invalid initializer expression";
                    return false;
                }
                if (vt != MIR_INVALID_ID && vt < module_.types.size() &&
                    module_.types.get(vt).kind == MIRTypeKind::Array) {
                    // initialize an array from a pointer (e.g. a string literal)
                    builder_->copy_ptr_to_place(place, static_cast<ValueId>(r.id));
                    return true;
                }
                ValueId val = r.id;
                if (r.type != vt) val = builder_->cast(val, vt);
                builder_->store(place, val);
            }
            return true;
        }
        case ASTNodeKind::AssignmentStmt: {
            auto* as = node->as_assignment();
            if (!as->lhs) {
                error = "assignment has no target";
                return false;
            }
            Value* lhs = as->lhs;
            if (lhs->val_kind() == ValueKind::AccessChain) {
                auto* chain = lhs->as_access_chain_unsafe();
                if (chain->values.size() == 1) {
                    lhs = chain->values[0];
                } else if (chain->values.size() >= 2) {
                    const size_t n = chain->values.size();
                    // walk to the base of the last field
                    MIRExprResult first = lower_expr(chain->values[0], error);
                    if (!first.ok()) return false;
                    PlaceId cur_place = MIR_INVALID_ID;
                    ValueId cur_ptr = MIR_NULL;
                    bool have_ptr = false;
                    if (first.kind == MIRExprKind::Place) {
                        cur_place = static_cast<PlaceId>(first.id);
                    } else if (first.kind == MIRExprKind::Value ||
                               first.kind == MIRExprKind::Address) {
                        cur_ptr = static_cast<ValueId>(first.id);
                        have_ptr = true;
                    } else {
                        error = "field assignment base is not a place or pointer";
                        return false;
                    }
                    for (size_t i = 1; i + 1 < n; ++i) {
                        std::string fname;
                        if (!member_name(chain->values[i], fname, error)) return false;
                        const ConstantId fc = module_.constants.add_string(
                            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                        const TypeId ft = types_.map(chain->values[i]->getType());
                        if (!have_ptr) {
                            cur_ptr = builder_->field_addr(cur_place, fc, ft);
                            have_ptr = true;
                        } else {
                            cur_ptr = builder_->field_addr_ptr(cur_ptr, fc, ft);
                        }
                    }
                    std::string fname;
                    if (!member_name(chain->values.back(), fname, error)) return false;
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind == MIRExprKind::Value &&
                        as->assOp != Operation::Assignment) {
                        // compound assignment on a struct field: load, apply, store
                        MIRBinaryOp bop = MIRBinaryOp::Add;
                        if (!compound_binary(as->assOp, bop)) {
                            error = "unsupported compound field assignment operator";
                            return false;
                        }
                        const TypeId ft = types_.map(chain->values.back()->getType());
                        const ValueId cur = have_ptr ? builder_->field_load_ptr(cur_ptr, fc, ft)
                                                     : builder_->field_load(cur_place, fc, ft);
                        const ConstantId opc = module_.constants.add_int(
                            MIR_INVALID_ID, static_cast<uint64_t>(bop));
                        const ValueId nv = builder_->binary(cur, r.id, opc, ft);
                        if (!have_ptr) {
                            builder_->field_store(cur_place, fc, nv);
                        } else {
                            builder_->field_store_ptr(cur_ptr, fc, nv);
                        }
                        return true;
                    }
                    if (r.kind == MIRExprKind::Place) {
                        if (!have_ptr) {
                            builder_->field_store_place(cur_place, fc,
                                                        static_cast<PlaceId>(r.id));
                        } else {
                            const TypeId vt = builder_->function().places[r.id].type;
                            const ValueId sptr = builder_->address_of(
                                static_cast<PlaceId>(r.id), types_.pointer_type(vt, false));
                            const ValueId sv = builder_->load_indirect(sptr, vt);
                            builder_->field_store_ptr(cur_ptr, fc, sv);
                        }
                        return true;
                    }
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate field assignment not yet supported";
                        return false;
                    }
                    if (!have_ptr) {
                        builder_->field_store(cur_place, fc, r.id);
                    } else {
                        builder_->field_store_ptr(cur_ptr, fc, r.id);
                    }
                    return true;
                }
            }
            if (lhs->val_kind() == ValueKind::DereferenceValue) {
                auto* d = lhs->as_dereference_value_unsafe();
                MIRExprResult p = lower_expr(d->getValue(), error);
                if (!p.ok()) return false;
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                if (r.kind != MIRExprKind::Value) {
                    error = "aggregate dereference assignment not yet supported";
                    return false;
                }
                if (as->assOp != Operation::Assignment) {
                    MIRBinaryOp bop = MIRBinaryOp::Add;
                    if (!compound_binary(as->assOp, bop)) {
                        error = "unsupported compound dereference assignment operator";
                        return false;
                    }
                    const TypeId et = types_.map(lhs->getType());
                    const ConstantId opc = module_.constants.add_int(
                        MIR_INVALID_ID, static_cast<uint64_t>(bop));
                    const ValueId cur =
                        builder_->load_indirect(static_cast<ValueId>(p.id), et);
                    const ValueId nv = builder_->binary(cur, r.id, opc, et);
                    builder_->store_indirect(static_cast<ValueId>(p.id), nv);
                } else {
                    builder_->store_indirect(static_cast<ValueId>(p.id), r.id);
                }
                return true;
            }
            if (lhs->val_kind() == ValueKind::IndexOperator) {
                auto* io = lhs->as_index_op_unsafe();
                MIRExprResult base = lower_expr(io->parent_val, error);
                if (!base.ok()) return false;
                MIRExprResult idx = lower_expr(io->idx, error);
                if (!idx.ok()) return false;
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                if (r.kind != MIRExprKind::Value) {
                    error = "aggregate index assignment not yet supported";
                    return false;
                }
                const TypeId et = types_.map(io->getType());
                const bool compound = (as->assOp != Operation::Assignment);
                ConstantId opc = MIR_INVALID_ID;
                if (compound) {
                    MIRBinaryOp bop = MIRBinaryOp::Add;
                    if (!compound_binary(as->assOp, bop)) {
                        error = "unsupported compound index assignment operator";
                        return false;
                    }
                    opc = module_.constants.add_int(MIR_INVALID_ID,
                                                   static_cast<uint64_t>(bop));
                }
                if (base.kind == MIRExprKind::Place) {
                    const PlaceId bp = static_cast<PlaceId>(base.id);
                    ValueId val = r.id;
                    if (compound) {
                        const ValueId cur =
                            builder_->element_load(bp, static_cast<ValueId>(idx.id), et);
                        val = builder_->binary(cur, r.id, opc, et);
                    }
                    builder_->element_store(bp, static_cast<ValueId>(idx.id), val);
                } else {
                    const ValueId bv = static_cast<ValueId>(base.id);
                    ValueId val = r.id;
                    if (compound) {
                        const ValueId cur =
                            builder_->index_load(bv, static_cast<ValueId>(idx.id), et);
                        val = builder_->binary(cur, r.id, opc, et);
                    }
                    builder_->index_store(bv, static_cast<ValueId>(idx.id), val);
                }
                return true;
            }
            if (lhs->val_kind() != ValueKind::Identifier) {
                error = "assignment target is not a simple local variable yet";
                return false;
            }
            auto* id = lhs->as_identifier_unsafe();
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID) {
                if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                    auto* vi = id->linked->as_var_init();
                    const SymbolId gs = intern_global(vi);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate global assignment not yet supported";
                        return false;
                    }
                    builder_->store_indirect(addr, r.id);
                    return true;
                }
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate global assignment not yet supported";
                        return false;
                    }
                    builder_->store_indirect(addr, r.id);
                    return true;
                }
                error = "assignment target does not resolve to a local place";
                return false;
            }
            MIRExprResult r = lower_expr(as->value, error);
            if (!r.ok()) return false;
            if (r.kind != MIRExprKind::Value) {
                error = "aggregate assignment not yet supported";
                return false;
            }
            const TypeId place_type = builder_->function().places[place].type;
            ValueId val = r.id;
            if (as->assOp != Operation::Assignment) {
                MIROpcode cop = MIROpcode::Nop;
                MIRBinaryOp bop = MIRBinaryOp::Add;
                if (!(map_binary(as->assOp, cop, bop) && cop == MIROpcode::Binary) &&
                    !compound_binary(as->assOp, bop)) {
                    error = "unsupported compound assignment operator (" +
                            std::to_string(static_cast<int>(as->assOp)) + ")";
                    return false;
                }
                const ValueId cur = builder_->load(place, place_type);
                const ConstantId opc = module_.constants.add_int(
                    MIR_INVALID_ID, static_cast<uint64_t>(bop));
                val = builder_->binary(cur, val, opc, place_type);
            } else if (r.type != place_type) {
                val = builder_->cast(val, place_type);
            }
            builder_->store(place, val);
            return true;
        }
        case ASTNodeKind::ReturnStmt: {
            auto* rs = node->as_return();
            if (async_) {
                const TypeId inner_t = types_.map(async_inner_);
                const ConstantId res_fc =
                    module_.constants.add_string(MIR_INVALID_ID, "__result", 8);
                if (async_in_ramp_) {
                    if (rs->value) {
                        MIRExprResult r = lower_expr(rs->value, error);
                        if (!r.ok()) return false;
                        ValueId v = (r.kind == MIRExprKind::Place)
                                        ? builder_->load(static_cast<PlaceId>(r.id), inner_t)
                                        : static_cast<ValueId>(r.id);
                        builder_->field_store_ptr(async_ramp_af_val_, res_fc, v);
                    }
                    emit_drops();
                    builder_->br(async_ramp_done_);
                    return true;
                }
                if (rs->value) {
                    MIRExprResult r = lower_expr(rs->value, error);
                    if (!r.ok()) return false;
                    ValueId v = (r.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(r.id), inner_t)
                                    : static_cast<ValueId>(r.id);
                    builder_->field_store_ptr(async_poll_af_val_, res_fc, v);
                }
                emit_drops();
                builder_->emit(
                    MIROpcode::AsyncFinish, MIR_NULL,
                    {MIROperand::place(async_poll_frame_, async_frame_ptr_type_),
                     MIROperand::place(async_poll_ret_,
                                       types_.pointer_type(types_.map(async_poll_base_), true)),
                     MIROperand::type(types_.map(async_poll_base_))});
                return true;
            }
            // returning a local destructible moves it out (do not drop it here)
            if (rs->value) {
                Value* rv = rs->value;
                // unwrap `unsafe(...)`, a 1-element chain, etc. to find the moved local
                for (int guard = 0; guard < 4 && rv; ++guard) {
                    if (rv->val_kind() == ValueKind::AccessChain) {
                        auto* c = rv->as_access_chain_unsafe();
                        if (c->values.size() == 1) { rv = c->values[0]; continue; }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::UnsafeValue) {
                        auto* u = static_cast<UnsafeValue*>(rv);
                        if (u->getValue()) { rv = u->getValue(); continue; }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::FunctionCall) {
                        auto* fc = rv->as_func_call_unsafe();
                        std::string cn;
                        if (fc->parent_val && fc->parent_val->val_kind() == ValueKind::Identifier) {
                            auto* cid = fc->parent_val->as_identifier_unsafe();
                            cn.assign(cid->value.data(), cid->value.size());
                        }
                        if (cn == "unsafe" && fc->values.size() == 1) { rv = fc->values[0]; continue; }
                        break;
                    }
                    break;
                }
                if (rv->val_kind() == ValueKind::Identifier) {
                    auto* id = rv->as_identifier_unsafe();
                    PlaceId rp = place_for_linked(id->linked);
                    if (rp == MIR_INVALID_ID) rp = place_for_name(id->value);
                    if (rp != MIR_INVALID_ID) mark_moved(rp);
                }
            }
            if (!rs->value) {
                emit_drops();
                builder_->ret_void();
                return true;
            }
            MIRExprResult r = lower_expr(rs->value, error);
            if (!r.ok()) return false;
            emit_drops();
            if (sret_) {
                if (r.kind != MIRExprKind::Place) {
                    error = "returning an aggregate rvalue is not yet supported (name it first)";
                    return false;
                }
                builder_->emit(MIROpcode::Store, MIR_NULL,
                               {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                MIROperand::place(static_cast<PlaceId>(r.id), sret_ret_type_)});
                builder_->ret_void();
                return true;
            }
            if (r.kind != MIRExprKind::Value) {
                error = "returning an aggregate value is not yet supported";
                return false;
            }
            builder_->ret(r.id);
            return true;
        }
        case ASTNodeKind::ValueWrapper: {
            auto* vw = node->as_value_wrapper();
            MIRExprResult r = lower_expr(vw->value, error);
            return r.ok();
        }
        case ASTNodeKind::ValueNode: {
            auto* vn = node->as_value_node_unsafe();
            MIRExprResult r = lower_expr(vn->value, error);
            return r.ok();
        }
        case ASTNodeKind::IfStmt: {
            auto* ifs = node->as_if_stmt_unsafe();
            if (ifs->is_comptime()) {
                // compile-time condition: lower only the taken scope
                std::string rerr;
                Scope* taken = comptime_if_resolver_ ? comptime_if_resolver_(ifs, rerr) : nullptr;
                if (taken) return lower_scope(*taken, error);
                // if the condition cannot be resolved at comptime, fall back to
                // lowering it as a runtime branch (matches the legacy backend)
            }
            const BlockId merge = builder_->create_block();
            if (!lower_if(ifs, merge, error)) return false;
            builder_->set_block(merge);
            return true;
        }
        case ASTNodeKind::WhileLoopStmt:
            return lower_while(node->as_while_loop_unsafe(), error);
        case ASTNodeKind::ForLoopStmt:
            return lower_for(node->as_for_loop_unsafe(), error);
        case ASTNodeKind::BreakStmt: {
            if (break_targets_.empty()) {
                error = "break outside a loop";
                return false;
            }
            builder_->br(break_targets_.back());
            return true;
        }
        case ASTNodeKind::ContinueStmt: {
            if (continue_targets_.empty()) {
                error = "continue outside a loop";
                return false;
            }
            builder_->br(continue_targets_.back());
            return true;
        }
        case ASTNodeKind::IncDecNode: {
            auto* n = node->as_inc_dec_node_unsafe();
            ValueId out = MIR_NULL;
            return lower_incdec_value(n->value.getValue(), n->value.increment, n->value.post,
                                      error, out);
        }
        case ASTNodeKind::AccessChainNode: {
            auto* acn = node->as_access_chain_node_unsafe();
            auto& vals = acn->chain.values;
            if (vals.size() == 2 && vals[1] && vals[1]->val_kind() == ValueKind::FunctionCall) {
                MIRExprResult r = lower_method_call(vals[0], vals[1]->as_func_call_unsafe(), error);
                return r.ok();
            }
            MIRExprResult r = lower_expr(&acn->chain, error);
            return r.ok();
        }
        case ASTNodeKind::SwitchStmt: {
            auto* sw = node->as_switch_stmt_unsafe();
            MIRExprResult scrut = lower_expr(sw->expression, error);
            if (!scrut.ok()) return false;

            const BlockId end_b = builder_->create_block();
            std::vector<BlockId> scope_blocks(sw->scopes.size());
            for (size_t i = 0; i < sw->scopes.size(); ++i) {
                scope_blocks[i] = builder_->create_block();
            }
            const BlockId default_b =
                (sw->defScopeInd >= 0) ? scope_blocks[sw->defScopeInd] : end_b;

            const TypeId bool_t = types_.bool_type();
            const ConstantId eqc = module_.constants.add_int(
                MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));

            const size_t n = sw->cases.size();
            for (size_t i = 0; i < n; ++i) {
                auto& c = sw->cases[i];
                const BlockId target = (c.second >= 0) ? scope_blocks[c.second] : default_b;
                if (c.first == nullptr) {
                    builder_->br(target);
                    break;
                }
                const BlockId next_b = (i + 1 < n) ? builder_->create_block() : default_b;
                MIRExprResult cv = lower_expr(c.first, error);
                if (!cv.ok()) return false;
                const ValueId cond = builder_->compare(scrut.id, cv.id, eqc, bool_t);
                builder_->cond_br(cond, target, next_b);
                if (i + 1 < n) {
                    builder_->set_block(next_b);
                } else {
                    break;
                }
            }
            if (n == 0) builder_->br(default_b);

            for (size_t i = 0; i < sw->scopes.size(); ++i) {
                builder_->set_block(scope_blocks[i]);
                if (!lower_scope(sw->scopes[i], error)) return false;
                if (!builder_->current_block_terminated()) builder_->br(end_b);
            }
            builder_->set_block(end_b);
            return true;
        }
        case ASTNodeKind::Block: {
            auto* b = node->as_block_scope_unsafe();
            return lower_scope_nodes(b->nodes, error);
        }
        case ASTNodeKind::Scope: {
            auto* s = node->as_scope_unsafe();
            return lower_scope_nodes(s->nodes, error);
        }
        case ASTNodeKind::UnsafeBlock: {
            auto* u = node->as_unsafe_block_unsafe();
            return lower_scope_nodes(u->scope.nodes, error);
        }
        default:
            error = "unsupported statement kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(node->kind())) + ")";
            return false;
    }
}

bool MIRLowerer::lower_incdec_value(Value* target, bool increment, bool post, std::string& error,
                                    ValueId& out) {
    Value* t = target;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t || t->val_kind() != ValueKind::Identifier) {
        error = "increment/decrement target must be a local variable";
        return false;
    }
    auto* id = t->as_identifier_unsafe();
    const PlaceId place = place_for_linked(id->linked);
    if (place == MIR_INVALID_ID) {
        error = "increment/decrement target does not resolve to a local place";
        return false;
    }
    const TypeId vt = types_.map(t->getType());
    const ValueId cur = builder_->load(place, vt);
    const ConstantId opc = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(increment ? MIRBinaryOp::Add : MIRBinaryOp::Sub));
    const TypeId it = types_.u32_type();
    const ValueId one = builder_->const_int(it, module_.constants.add_int(it, 1));
    const ValueId nv = builder_->binary(cur, one, opc, vt);
    builder_->store(place, nv);
    // a post-increment/decrement expression yields the *old* value
    out = post ? cur : nv;
    return true;
}

bool MIRLowerer::lower_scope(Scope& scope, std::string& error) {
    return lower_scope_nodes(scope.nodes, error);
}

bool MIRLowerer::lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error) {
    const size_t mark = destructibles_.size();
    for (ASTNode* n : nodes) {
        if (!lower_stmt(n, error)) return false;
    }
    // destroy locals declared in this scope (unless they were moved out)
    if (!builder_->current_block_terminated()) {
        for (size_t i = mark; i < destructibles_.size(); ++i) {
            const ValueId fv = builder_->load(destructibles_[i].flag, types_.bool_type());
            builder_->drop(destructibles_[i].place, destructibles_[i].dtor, fv);
        }
    }
    destructibles_.resize(mark);
    return true;
}

bool MIRLowerer::lower_if(IfStatement* stmt, BlockId merge, std::string& error) {
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();

    MIRExprResult c = lower_expr(stmt->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, then_b, else_b);

    builder_->set_block(then_b);
    if (!lower_scope(stmt->ifBody, error)) return false;
    if (!builder_->current_block_terminated()) builder_->br(merge);

    BlockId current = else_b;
    for (size_t i = 0; i < stmt->elseIfs.size(); ++i) {
        const BlockId body_b = builder_->create_block();
        const BlockId next_else = builder_->create_block();
        builder_->set_block(current);
        MIRExprResult ec = lower_expr(stmt->elseIfs[i].first, error);
        if (!ec.ok()) return false;
        builder_->cond_br(ec.id, body_b, next_else);
        builder_->set_block(body_b);
        if (!lower_scope(stmt->elseIfs[i].second, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(merge);
        current = next_else;
    }

    builder_->set_block(current);
    if (stmt->elseBody.has_value()) {
        if (!lower_scope(stmt->elseBody.value(), error)) return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(merge);
    return true;
}

bool MIRLowerer::lower_while(WhileLoop* loop, std::string& error) {
    const BlockId cond_b = builder_->create_block();
    const BlockId body_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(cond_b);
    builder_->set_block(cond_b);
    MIRExprResult c = lower_expr(loop->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, body_b, exit_b);

    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(cond_b);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_for(ForLoop* loop, std::string& error) {
    if (loop->initializer) {
        if (!lower_stmt(loop->initializer, error)) return false;
    }

    const BlockId cond_b = builder_->create_block();
    const BlockId body_b = builder_->create_block();
    const BlockId step_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(cond_b);
    builder_->set_block(cond_b);
    if (loop->conditionExpr) {
        MIRExprResult c = lower_expr(loop->conditionExpr, error);
        if (!c.ok()) return false;
        builder_->cond_br(c.id, body_b, exit_b);
    } else {
        builder_->br(body_b);
    }

    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(step_b);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(step_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();

    builder_->set_block(step_b);
    if (loop->incrementerExpr) {
        if (!lower_stmt(loop->incrementerExpr, error)) return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);

    builder_->set_block(exit_b);
    return true;
}

// ── async / coroutine lowering ─────────────────────────────────────────────

static bool mir_slot_is_destructible(BaseType* type) {
    if (type == nullptr) return false;
    if (type->get_destructor() != nullptr) return true;
    const auto canonical = type->canonical();
    if (canonical->kind() != BaseTypeKind::Array) return false;
    const auto arr = canonical->as_array_type_unsafe();
    if (!arr->has_array_size() || arr->elem_type == nullptr) return false;
    return arr->elem_type->canonical()->get_destructor() != nullptr;
}

std::string MIRLowerer::async_slot_field(unsigned id) const {
    return "__chx_slot_" + std::to_string(id);
}
std::string MIRLowerer::async_child_field(unsigned id) const {
    return "__chx_child_" + std::to_string(id);
}
std::string MIRLowerer::async_drop_flag_field(unsigned id) const {
    return "__chx_drop_" + std::to_string(id);
}
std::string MIRLowerer::async_resident_field(ASTNode* node) const {
    auto it = async_resident_.find(node);
    if (it == async_resident_.end()) return std::string();
    return async_slot_field(it->second);
}
PlaceId MIRLowerer::async_resident_place(ASTNode* node) const {
    auto it = var_places_.find(node);
    return it == var_places_.end() ? MIR_INVALID_ID : it->second;
}
void MIRLowerer::async_emit_spill(const AwaitSite&) {}
void MIRLowerer::async_emit_reload(const AwaitSite&) {}
void MIRLowerer::async_emit_pending_return() {}

static std::string mir_sym_name(const MIRModule& module, SymbolId sym) {
    if (sym == MIR_INVALID_ID || sym >= module.symbols.size()) return std::string();
    const MIRSymbolRecord& r = module.symbols.get(sym);
    return std::string(module.symbols.name_data(r), r.name_length);
}

bool MIRLowerer::lower_async_function(FunctionDeclaration* decl, MIRArena& arena,
                                      MIRFunction& func, std::string& error) {
    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    async_ = true;
    arena_ = &arena;
    async_pre_decls_.clear();
    async_post_decls_.clear();
    async_resident_.clear();
    async_drop_flag_slots_.clear();
    async_site_index_.clear();
    async_plan_ = build_async_plan(decl);
    const AsyncCTypes t = resolve_async_c_types(decl);
    if (!t.ok) {
        error = "async lowering could not resolve the core::async protocol types";
        async_ = false;
        return false;
    }
    async_inner_ = t.inner;
    async_handle_ = t.handle;
    async_table_ = t.table;
    async_poll_base_ = t.poll;
    async_context_ = t.context_ptr;

    async_mangled_.clear();
    if (mangler_) async_mangled_ = mangler_(decl);
    if (async_mangled_.empty()) {
        const chem::string_view nv = decl->name_view();
        async_mangled_.assign(nv.data(), nv.size());
    }
    async_frame_type_ = types_.named_struct(async_mangled_ + "__frame");
    async_frame_ptr_type_ = types_.pointer_type(async_frame_type_, true);

    // resident slots (parameters + locals that cross a suspension) and drop flags
    std::vector<bool> crosses(async_plan_.slots.size(), false);
    for (auto& site : async_plan_.sites) {
        for (auto id : site.live_slots) {
            if (id < crosses.size()) crosses[id] = true;
        }
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node == nullptr) continue;
        const bool is_param = slot.node->kind() == ASTNodeKind::FunctionParam;
        if (!crosses[id] && !is_param) continue;
        if (slot.type == nullptr) continue;
        async_resident_[slot.node] = static_cast<unsigned>(id);
    }
    for (auto& site : async_plan_.sites) {
        for (auto id : site.live_drops) async_drop_flag_slots_.insert(id);
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node != nullptr && slot.node->kind() == ASTNodeKind::FunctionParam &&
            async_resident_.count(slot.node) != 0 && slot.destructible) {
            async_drop_flag_slots_.insert(static_cast<unsigned>(id));
        }
    }

    // per-site poll types
    async_site_poll_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        const AsyncCTypes st = resolve_async_c_types_from_handle(
            const_cast<BaseType*>(async_plan_.sites[i].awaited_handle_type));
        async_site_poll_[i] = st.ok ? types_.map(st.poll) : types_.map(async_poll_base_);
    }
    async_site_index_.clear();
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        if (async_plan_.sites[i].var_init != nullptr) {
            async_site_index_[async_plan_.sites[i].var_init] = i;
        }
    }

    async_emit_frame_helpers(decl);
    if (!async_lower_poll_body(decl, error)) { async_ = false; return false; }
    if (!async_lower_ramp(decl, func, error)) { async_ = false; return false; }
    async_ = false;
    return true;
}

void MIRLowerer::async_emit_frame_helpers(FunctionDeclaration* decl) {
    const std::string frame = async_mangled_ + "__frame";
    const std::string inner_c = c_type_of(module_, types_.map(async_inner_));
    const std::string ctx_c = c_type_of(module_, types_.map(async_context_));
    const std::string table_c = c_type_of(module_, types_.map(async_handle_));
    // (the table type is not directly needed; the vtable is spelled below)
    (void)table_c;

    async_pre_decls_ += "struct " + frame + ";\n";
    async_pre_decls_ += "struct " + frame + " { uint32_t __state; " + inner_c + " __result; " +
                        ctx_c + " __cx;";
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        BaseType* ct = async_plan_.sites[i].awaited_handle_type;
        async_pre_decls_ += " " +
                            (ct ? c_type_of(module_, types_.map(ct)) : std::string("void*")) + " " +
                            async_child_field(static_cast<unsigned>(i)) + ";";
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        BaseType* st = async_plan_.slots[id].type;
        const std::string nm = async_slot_field(static_cast<unsigned>(id));
        if (st != nullptr && st->kind() == BaseTypeKind::Array) {
            auto* arr = st->as_array_type_unsafe();
            async_pre_decls_ += " " + c_type_of(module_, types_.map(arr->elem_type)) + " " + nm +
                                "[" + std::to_string(arr->get_array_size()) + "];";
        } else {
            async_pre_decls_ += " " + (st ? c_type_of(module_, types_.map(st))
                                          : std::string("void*")) + " " + nm + ";";
        }
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        if (async_drop_flag_slots_.count(static_cast<unsigned>(id)) != 0) {
            async_pre_decls_ += " uint8_t " + async_drop_flag_field(static_cast<unsigned>(id)) +
                                ";";
        }
    }
    async_pre_decls_ += " };\n";

    const std::string poll_ptr = c_type_of(module_, types_.pointer_type(types_.map(async_poll_base_), true));
    const std::string handle_ptr = c_type_of(module_, types_.pointer_type(types_.map(async_handle_), true));
    async_pre_decls_ += "static void " + async_mangled_ + "__poll(" + poll_ptr +
                        " __chx__async_ret, void* __frame, " + ctx_c + " __cx);\n";
    async_pre_decls_ += "static void " + async_mangled_ + "__drop(void* __frame);\n";

    // drop definition
    async_post_decls_ += "static void " + async_mangled_ + "__drop(void* __frame) {\n";
    async_post_decls_ += "    struct " + frame + "* __chx__af = (struct " + frame +
                         "*) __frame;\n";
    async_post_decls_ += "    switch(__chx__af->__state) {\n";
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        async_post_decls_ += "        case " + std::to_string(i + 1) + ":\n";
        for (auto id : async_plan_.sites[i].live_drops) {
            const auto& slot = async_plan_.slots[id];
            if (slot.type == nullptr) continue;
            SymbolId dtor = destructor_symbol(slot.type);
            std::string dname = mir_sym_name(module_, dtor);
            if (dname.empty()) continue;
            const std::string field = "__chx__af->" + async_slot_field(id);
            if (async_drop_flag_slots_.count(id) != 0) {
                async_post_decls_ += "            if(__chx__af->" + async_drop_flag_field(id) +
                                     ") { " + dname + "(&" + field + "); }\n";
            } else {
                async_post_decls_ += "            " + dname + "(&" + field + ");\n";
            }
        }
        const std::string child = "__chx__af->" + async_child_field(static_cast<unsigned>(i));
        async_post_decls_ += "            if(" + child + ".frame != (void*) 0) { " + child +
                             ".vtbl->drop(" + child + ".frame); " + child +
                             ".frame = (void*) 0; }\n";
        async_post_decls_ += "            break;\n";
    }
    async_post_decls_ += "        default: break;\n    }\n";
    async_post_decls_ += "    chemical_async_frame_free(__frame, sizeof(struct " + frame +
                         "), _Alignof(struct " + frame + "));\n}\n";
    // vtable definition (references __poll and __drop, both defined above)
    const std::string table_t = c_type_of(module_, types_.map(async_table_));
    async_post_decls_ += "static " + table_t + " " + async_mangled_ + "__vtbl = (" + table_t +
                         "){ .poll = " + async_mangled_ + "__poll, .drop = " + async_mangled_ +
                         "__drop };\n";
    (void)handle_ptr;
}

bool MIRLowerer::async_lower_await_var_init(VarInitStatement* stmt, size_t site_index,
                                            std::string& error) {
    if (site_index >= async_plan_.sites.size()) {
        error = "async await has no matching site";
        return false;
    }
    const AwaitSite& site = async_plan_.sites[site_index];
    auto* await = stmt->value ? stmt->value->as_await_expression_unsafe() : nullptr;
    if (await == nullptr || await->getInner() == nullptr) {
        error = "async await has no operand";
        return false;
    }
    // The child future lives in a frame field so it survives suspension.
    const std::string child_name = async_child_field(static_cast<unsigned>(site_index));
    const ConstantId child_fc = module_.constants.add_string(MIR_INVALID_ID, child_name.data(),
                                                             static_cast<uint32_t>(child_name.size()));
    PlaceId child_place = async_child_places_[site_index];
    if (child_place == MIR_INVALID_ID) {
        child_place = builder_->alloca_frame(types_.map(site.awaited_handle_type), child_fc);
        async_child_places_[site_index] = child_place;
    }
    // result place (the awaited value)
    PlaceId result_place = async_result_places_[site_index];
    // lower the operand and store it in the child slot
    MIRExprResult child = lower_expr(await->getInner(), error);
    if (!child.ok()) return false;
    if (child.kind == MIRExprKind::Place) {
        const PlaceId cp = static_cast<PlaceId>(child.id);
        const TypeId cpt = builder_->function().places[cp].type;
        builder_->field_store_ptr(async_poll_af_val_, child_fc, builder_->load(cp, cpt));
        // the child is consumed by the await; don't destroy the source again
        mark_moved(cp);
    } else {
        builder_->field_store_ptr(async_poll_af_val_, child_fc,
                                  static_cast<ValueId>(child.id));
    }
    // poll
    std::vector<MIROperand> ops;
    ops.push_back(MIROperand::place(async_poll_frame_, async_frame_ptr_type_));
    ops.push_back(MIROperand::place(child_place, types_.map(site.awaited_handle_type)));
    ops.push_back(MIROperand::place(result_place, types_.map(site.awaited_type)));
    ops.push_back(MIROperand::type(async_site_poll_[site_index]));
    ops.push_back(MIROperand::constant(
        module_.constants.add_int(MIR_INVALID_ID, site_index + 1), MIR_INVALID_ID));
    ops.push_back(MIROperand::place(async_poll_ret_, types_.pointer_type(types_.map(async_poll_base_), true)));
    ops.push_back(MIROperand::type(types_.map(async_poll_base_)));
    // spills
    std::vector<std::pair<PlaceId, ConstantId>> spills;
    for (auto id : site.live_slots) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node == site.var_init) continue;
        if (async_resident_.count(slot.node) != 0) continue;
        PlaceId lp = place_for_linked(slot.node);
        if (lp == MIR_INVALID_ID) continue;
        const std::string fname = async_slot_field(id);
        const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, fname.data(),
                                                           static_cast<uint32_t>(fname.size()));
        spills.emplace_back(lp, fc);
    }
    async_spills_[site_index] = spills;
    ops.push_back(MIROperand::constant(
        module_.constants.add_int(MIR_INVALID_ID, spills.size()), MIR_INVALID_ID));
    for (auto& sp : spills) {
        ops.push_back(MIROperand::place(sp.first, MIR_INVALID_ID));
        ops.push_back(MIROperand::constant(sp.second, MIR_INVALID_ID));
    }
    builder_->emit(MIROpcode::AsyncAwait, MIR_NULL, ops.data(), static_cast<uint32_t>(ops.size()));
    // the continuation after the await is reached from both the first pass and
    // the resume block, so give it its own block
    const BlockId cont = builder_->create_block();
    builder_->br(cont);
    builder_->set_block(cont);
    async_cont_blocks_[site_index] = cont;
    return true;
}

bool MIRLowerer::async_lower_poll_body(FunctionDeclaration* decl, std::string& error) {
    MIRFunction& fn = async_poll_;
    fn.clear();
    const std::string poll_name = async_mangled_ + "__poll";
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    SymbolId sym = module_.symbols.add(rec, poll_name.data(),
                                       static_cast<uint32_t>(poll_name.size()), poll_name.data(),
                                       static_cast<uint32_t>(poll_name.size()));
    fn.symbol = sym;
    const TypeId poll_ptr = types_.pointer_type(types_.map(async_poll_base_), true);
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    const TypeId ctx_ptr = types_.map(async_context_);
    std::vector<TypeId> pts{poll_ptr, void_ptr, ctx_ptr};
    const TypeId ftype = types_.function_signature(types_.void_type(), pts);
    module_.symbols.symbols[sym].type = ftype;
    fn.function_type = ftype;

    MIRBuilder b(*arena_, module_, fn);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    ValueId p0 = b.param(poll_ptr);
    PlaceId ret_place = b.alloca(poll_ptr, MIRStorageClass::Parameter);
    b.store(ret_place, p0);
    ValueId p1 = b.param(void_ptr);
    PlaceId frame_arg = b.alloca(void_ptr, MIRStorageClass::Parameter);
    b.store(frame_arg, p1);
    ValueId p2 = b.param(ctx_ptr);
    PlaceId cx_place = b.alloca(ctx_ptr, MIRStorageClass::Parameter);
    b.store(cx_place, p2);
    async_poll_ret_ = ret_place;
    async_poll_cx_ = cx_place;

    ValueId fv = b.load(frame_arg, void_ptr);
    ValueId af = b.cast(fv, async_frame_ptr_type_);
    PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);
    async_poll_frame_ = af_place;
    async_poll_af_val_ = af;
    // stash the context in the frame so child polls can reach the waker
    {
        const ConstantId cx_fc = module_.constants.add_string(MIR_INVALID_ID, "__cx", 4);
        ValueId cxv = b.load(cx_place, ctx_ptr);
        b.field_store_ptr(af, cx_fc, cxv);
    }

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    async_child_places_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    async_result_places_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    async_spills_.assign(async_plan_.sites.size(), {});
    async_cont_blocks_.assign(async_plan_.sites.size(), MIR_INVALID_ID);

    const size_t nsites = async_plan_.sites.size();
    const BlockId l0_b = b.create_block();
    std::vector<BlockId> resume_b(nsites);
    for (size_t i = 0; i < nsites; ++i) resume_b[i] = b.create_block();
    const BlockId default_b = b.create_block();
    std::vector<BlockId> chain(nsites + 1);
    for (size_t i = 0; i <= nsites; ++i) chain[i] = b.create_block();

    const TypeId u32 = types_.u32_type();
    const ConstantId state_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    ValueId st = b.field_load_ptr(af, state_fc, u32);
    const ConstantId eqc = module_.constants.add_int(MIR_INVALID_ID,
                                                     static_cast<uint64_t>(MIRBinaryOp::Eq));
    for (size_t i = 0; i <= nsites; ++i) {
        const ValueId cval = b.const_int(u32, module_.constants.add_int(u32, i));
        ValueId iseq = b.compare(st, cval, eqc, types_.bool_type());
        const BlockId target = (i == 0) ? l0_b : resume_b[i - 1];
        b.cond_br(iseq, target, chain[i]);
        b.set_block(chain[i]);
    }
    b.br(default_b);

    // default: Ready(af->__result)
    b.set_block(default_b);
    b.emit(MIROpcode::AsyncFinish, MIR_NULL,
           {MIROperand::place(af_place, async_frame_ptr_type_),
            MIROperand::place(ret_place, poll_ptr),
            MIROperand::type(types_.map(async_poll_base_))});

    // L0: body
    b.set_block(l0_b);
    for (FunctionParam* p : decl->params) {
        // the frame stores the declared type (aggregates by value), so the poll
        // parameter is the declared type too (not the hidden pointer form)
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        const std::string rf = async_resident_field(p);
        if (!rf.empty()) {
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, rf.data(),
                                                               static_cast<uint32_t>(rf.size()));
            PlaceId place = b.alloca_frame(pt, fc);
            bind(p, place);
            bind_name(p->name, place);
        } else {
            PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
            // load the spilled value from the frame slot
            auto it = std::find_if(async_plan_.slots.begin(), async_plan_.slots.end(),
                                   [&](const AsyncFrameSlot& s) { return s.node == p; });
            if (it != async_plan_.slots.end()) {
                const unsigned sid = static_cast<unsigned>(it - async_plan_.slots.begin());
                const std::string sf = async_slot_field(sid);
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, sf.data(), static_cast<uint32_t>(sf.size()));
                ValueId sv = b.field_load_ptr(af, fc, pt);
                b.store(place, sv);
            }
            bind(p, place);
            bind_name(p->name, place);
        }
    }
    if (decl->body.has_value()) {
        if (!lower_scope_nodes(decl->body->nodes, error)) {
            builder_ = nullptr;
            return false;
        }
    }
    if (!b.current_block_terminated()) {
        emit_drops();
        b.emit(MIROpcode::AsyncFinish, MIR_NULL,
               {MIROperand::place(af_place, async_frame_ptr_type_),
                MIROperand::place(ret_place, poll_ptr),
                MIROperand::type(types_.map(async_poll_base_))});
    }

    // resume blocks
    for (size_t i = 0; i < nsites; ++i) {
        b.set_block(resume_b[i]);
        const AwaitSite& site = async_plan_.sites[i];
        // reload spilled slots
        for (auto& sp : async_spills_[i]) {
            // sp.first is the local place; the frame field name is the constant
            const MIRConstant& c = module_.constants.get(sp.second);
            std::string fname(module_.constants.data.data() + c.data_offset, c.data_count);
            const TypeId lpt = builder_->function().places[sp.first].type;
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, fname.data(),
                                                               static_cast<uint32_t>(fname.size()));
            ValueId sv = b.field_load_ptr(async_poll_af_val_, fc, lpt);
            b.store(sp.first, sv);
        }
        // AsyncAwait with resume_state = 0
        std::vector<MIROperand> ops;
        ops.push_back(MIROperand::place(async_poll_frame_, async_frame_ptr_type_));
        ops.push_back(MIROperand::place(async_child_places_[i],
                                        types_.map(site.awaited_handle_type)));
        ops.push_back(MIROperand::place(async_result_places_[i], types_.map(site.awaited_type)));
        ops.push_back(MIROperand::type(async_site_poll_[i]));
        ops.push_back(MIROperand::constant(module_.constants.add_int(MIR_INVALID_ID, 0),
                                           MIR_INVALID_ID));
        ops.push_back(MIROperand::place(async_poll_ret_,
                                        types_.pointer_type(types_.map(async_poll_base_), true)));
        ops.push_back(MIROperand::type(types_.map(async_poll_base_)));
        ops.push_back(MIROperand::constant(
            module_.constants.add_int(MIR_INVALID_ID, async_spills_[i].size()), MIR_INVALID_ID));
        for (auto& sp : async_spills_[i]) {
            ops.push_back(MIROperand::place(sp.first, MIR_INVALID_ID));
            ops.push_back(MIROperand::constant(sp.second, MIR_INVALID_ID));
        }
        b.emit(MIROpcode::AsyncAwait, MIR_NULL, ops.data(), static_cast<uint32_t>(ops.size()));
        const BlockId cont = async_cont_blocks_[i];
        b.br(cont != MIR_INVALID_ID ? cont : default_b);
    }

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async poll builder failure";
        return false;
    }
    return true;
}

bool MIRLowerer::async_lower_ramp(FunctionDeclaration* decl, MIRFunction& func,
                                  std::string& error) {
    // The ramp returns the handle; for the eager path it runs the body inline.
    const bool eager = async_plan_.sites.empty();
    const TypeId handle_t = types_.map(async_handle_);
    sret_ = true;
    sret_ret_type_ = handle_t;
    sret_ptr_type_ = types_.pointer_type(handle_t, true);
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) { self_in_params = true; break; }
    }
    if (self_in_params) self_param = nullptr;
    {
        std::vector<TypeId> ptypes;
        ptypes.push_back(sret_ptr_type_);
        if (self_param) {
            TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                pt = types_.pointer_type(module_.types.get(pt).element, true);
            } else if (needs_aggregate_path(module_, pt)) {
                pt = types_.pointer_type(pt, true);
            }
            ptypes.push_back(pt);
        }
        for (FunctionParam* p : decl->params) {
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                pt = types_.pointer_type(module_.types.get(pt).element, true);
            } else if (needs_aggregate_path(module_, pt)) {
                pt = types_.pointer_type(pt, true);
            }
            ptypes.push_back(pt);
        }
        const TypeId ftype = types_.function_signature(types_.void_type(), ptypes);
        func.symbol = intern_function(decl);
        module_.symbols.symbols[func.symbol].type = ftype;
        func.function_type = ftype;
    }

    MIRBuilder b(*arena_, module_, func);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;
    sret_ptr_ = b.param(sret_ptr_type_);
    const PlaceId sret_place = b.alloca(sret_ptr_type_, MIRStorageClass::Parameter);
    b.store(sret_place, sret_ptr_);

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();

    // frame alloc
    ValueId af = b.async_frame_alloc(async_frame_type_, async_frame_ptr_type_);
    PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);
    async_ramp_af_ = af_place;
    async_ramp_af_val_ = af;
    // set state = 0 and cx = 0
    const ConstantId st_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    const ConstantId cx_fc = module_.constants.add_string(MIR_INVALID_ID, "__cx", 4);
    ValueId zero = b.const_int(types_.u32_type(), module_.constants.add_int(types_.u32_type(), 0));
    b.field_store_ptr(af, st_fc, zero);
    ValueId nullv = b.const_null(types_.map(async_context_),
                                 module_.constants.add_int(types_.map(async_context_), 0));
    b.field_store_ptr(af, cx_fc, nullv);

    // bind self/params and spill parameters into the frame
    if (self_param) {
        TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(self_param, place);
        bind_name(self_param->name, place);
    }
    for (FunctionParam* p : decl->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        // store the parameter into its frame slot
        auto it = std::find_if(async_plan_.slots.begin(), async_plan_.slots.end(),
                               [&](const AsyncFrameSlot& s) { return s.node == p; });
        if (it != async_plan_.slots.end()) {
            const unsigned sid = static_cast<unsigned>(it - async_plan_.slots.begin());
            const std::string sf = async_slot_field(sid);
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, sf.data(),
                                                               static_cast<uint32_t>(sf.size()));
            const MIRTypeRecord& pr = module_.types.get(pt);
            ValueId src = b.load(place, pt);
            if (pr.kind == MIRTypeKind::Pointer || pr.kind == MIRTypeKind::Reference) {
                // aggregate parameter arrives as a hidden pointer: copy through it
                src = b.load_indirect(src, types_.map(p->type));
            }
            b.field_store_ptr(af, fc, src);
            if (async_drop_flag_slots_.count(sid) != 0) {
                const std::string df = async_drop_flag_field(sid);
                const ConstantId dfc = module_.constants.add_string(MIR_INVALID_ID, df.data(),
                                                                    static_cast<uint32_t>(df.size()));
                ValueId one = b.const_int(types_.u32_type(),
                                          module_.constants.add_int(types_.u32_type(), 1));
                b.field_store_ptr(af, dfc, one);
            }
        }
    }

    if (eager) {
        // run the body inline, redirecting `return e` into the frame result
        async_in_ramp_ = true;
        async_ramp_result_ = MIR_INVALID_ID;
        async_ramp_done_ = b.create_block();
        if (decl->body.has_value()) {
            if (!lower_scope_nodes(decl->body->nodes, error)) {
                builder_ = nullptr;
                return false;
            }
        }
        if (!b.current_block_terminated()) b.br(async_ramp_done_);
        b.set_block(async_ramp_done_);
        async_in_ramp_ = false;
    }
    // return the handle
    const ConstantId vtbl_c = module_.constants.add_string(
        MIR_INVALID_ID, (async_mangled_ + "__vtbl").data(),
        static_cast<uint32_t>((async_mangled_ + "__vtbl").size()));
    ValueId state_one = b.const_int(types_.u32_type(),
                                    module_.constants.add_int(types_.u32_type(), eager ? 1 : 0));
    (void)state_one;
    const ConstantId ramp_state_c =
        module_.constants.add_int(types_.u32_type(), eager ? 1 : 0);
    b.emit(MIROpcode::AsyncRampFinish, MIR_NULL,
           {MIROperand::place(af_place, async_frame_ptr_type_),
            MIROperand::place(sret_place, sret_ptr_type_),
            MIROperand::constant(ramp_state_c, MIR_INVALID_ID),
            MIROperand::constant(vtbl_c, MIR_INVALID_ID),
            MIROperand::type(handle_t)});

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async ramp builder failure";
        return false;
    }
    return true;
}

bool MIRLowerer::lower_function(FunctionDeclaration* decl, MIRArena& arena,
                                MIRFunction& func, std::string& error) {
    if (decl->is_async()) {
        return lower_async_function(decl, arena, func, error);
    }
    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    // func_symbols_ persists across functions in a module so callees keep stable ids

    func.symbol = intern_function(decl);
    const TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    sret_ = needs_aggregate_path(module_, ret);
    sret_ret_type_ = ret;
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) { self_in_params = true; break; }
    }
    if (self_in_params) self_param = nullptr; // already bound by the params loop
    {
        std::vector<TypeId> ptypes;
        if (sret_) {
            sret_ptr_type_ = types_.pointer_type(ret, true);
            ptypes.push_back(sret_ptr_type_);
        }
        if (self_param) {
            TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
            ptypes.push_back(pt);
        }
        for (FunctionParam* p : decl->params) {
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
            ptypes.push_back(pt);
        }
        const TypeId ftype = types_.function_signature(sret_ ? types_.void_type() : ret, ptypes);
        module_.symbols.symbols[func.symbol].type = ftype;
        func.function_type = ftype;
    }

    MIRBuilder b(arena, module_, func);
    builder_ = &b;

    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;

    if (sret_) {
        sret_ptr_ = b.param(sret_ptr_type_);
    }

    if (self_param) {
        TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(self_param, place);
        bind_name(self_param->name, place);
    }

    // parameters: materialize an SSA param value, spill to a place, bind it
    for (FunctionParam* p : decl->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        if (place == MIR_INVALID_ID) {
            error = "failed to allocate parameter place";
            builder_ = nullptr;
            return false;
        }
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        // by-value struct params own their value and are destroyed at exit
        if (p->type) {
            const TypeId ptt = types_.map(p->type);
            const MIRTypeRecord& prr = module_.types.get(ptt);
            if (!(prr.flags & TF_TYPEDEF) && needs_aggregate_path(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (decl->body.has_value()) {
        for (ASTNode* node : decl->body->nodes) {
            if (!lower_stmt(node, error)) {
                builder_ = nullptr;
                return false;
            }
        }
    }

    if (!b.current_block_terminated()) {
        emit_drops();
        b.ret_void();
    }

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "MIR builder failure";
        return false;
    }
    return true;
}

} // namespace mir
