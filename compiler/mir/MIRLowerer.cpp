// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"

#include "ast/base/BaseType.h"
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
#include "ast/values/StructValue.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/values/ValueNode.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/Scope.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/ForLoop.h"
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

namespace mir {

namespace {

bool is_void_type(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    return module.types.get(type).kind == MIRTypeKind::Void;
}

bool needs_aggregate_path(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeKind k = module.types.get(type).kind;
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
        error = "expression is not a simple place";
        return MIR_INVALID_ID;
    }
    auto* id = t->as_identifier_unsafe();
    const PlaceId p = place_for_linked(id->linked);
    if (p == MIR_INVALID_ID) {
        error = "identifier does not resolve to a local place";
        return MIR_INVALID_ID;
    }
    return p;
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
    for (Value* a : values) {
        MIRExprResult r = lower_expr(a, error);
        if (!r.ok()) return false;
        if (r.kind == MIRExprKind::Place) {
            const PlaceId pid = static_cast<PlaceId>(r.id);
            const TypeId pt = builder_->function().places[pid].type;
            const MIRTypeRecord& prec = module_.types.get(pt);
            if (prec.kind == MIRTypeKind::Pointer || prec.kind == MIRTypeKind::Reference) {
                args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
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
    }
    return true;
}

MIRExprResult MIRLowerer::lower_method_call(Value* receiver, FunctionCall* call, std::string& error) {
    ASTNode* lk = call->parent_val ? call->parent_val->linked_node() : nullptr;
    FunctionDeclaration* fd = lk ? lk->as_function() : nullptr;
    if (!fd) {
        error = "method call to unresolved function";
        return MIRExprResult::error();
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
            error = "method receiver must be addressable";
            return MIRExprResult::error();
        }
        args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), recv.type));
    } else {
        error = "method receiver must be an addressable place";
        return MIRExprResult::error();
    }
    if (!lower_call_args(call->values, args, error)) return MIRExprResult::error();

    const SymbolId sym = intern_function(fd);
    const TypeId type = types_.map(call->getType());
    if (is_void_type(module_, type)) {
        builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
        return MIRExprResult::void_result();
    }
    if (needs_aggregate_path(module_, type)) {
        const PlaceId res = builder_->alloca(type, MIRStorageClass::Temporary);
        if (res == MIR_INVALID_ID) {
            error = "failed to allocate method result place";
            return MIRExprResult::error();
        }
        builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
        return MIRExprResult::place(res, type);
    }
    const ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
    return MIRExprResult::value(v, type);
}

SymbolId MIRLowerer::intern_global(VarInitStatement* vi) {
    auto it = global_symbols_.find(vi);
    if (it != global_symbols_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    rec.linkage = vi->is_extern() ? MIRLinkage::External : MIRLinkage::Internal;
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
            PlaceId place = place_for_linked(id->linked);
            if (place != MIR_INVALID_ID) {
                if (needs_aggregate_path(module_, type)) {
                    return MIRExprResult::place(place, type);
                }
                return MIRExprResult::value(builder_->load(place, type), type);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* vi = id->linked->as_var_init();
                const SymbolId gs = intern_global(vi);
                const TypeId ptrt = types_.pointer_type(type, false);
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
                    const TypeId ptrt = types_.pointer_type(type, false);
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
            if (in.type == type) return in;
            return MIRExprResult::value(builder_->cast(in.id, type), type);
        }
        case ValueKind::AccessChain: {
            auto* chain = value->as_access_chain_unsafe();
            if (chain->values.size() == 1) {
                return lower_expr(chain->values[0], error);
            }
            // member access: base must resolve to a place; field name is the
            // last chain element's identifier text
            const PlaceId base = resolve_place(chain->values[0], error);
            if (base == MIR_INVALID_ID) return MIRExprResult::error();
            std::string fname;
            if (!member_name(chain->values.back(), fname, error)) return MIRExprResult::error();
            const ConstantId fc = module_.constants.add_string(
                MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
            return MIRExprResult::value(builder_->field_load(base, fc, type), type);
        }
        case ValueKind::StructValue: {
            auto* sv = value->as_struct_value_unsafe();
            const TypeId st = type;
            const PlaceId temp = builder_->alloca(st, MIRStorageClass::Temporary);
            if (temp == MIR_INVALID_ID) {
                error = "failed to allocate struct temporary";
                return MIRExprResult::error();
            }
            auto it = sv->values.begin();
            while (it != sv->values.end()) {
                auto& init = it.value();
                MIRExprResult r = lower_expr(init.value, error);
                if (!r.ok()) return r;
                if (r.kind != MIRExprKind::Value) {
                    error = "nested aggregate struct field initialization not yet supported";
                    return MIRExprResult::error();
                }
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, init.name.data(), static_cast<uint32_t>(init.name.size()));
                builder_->field_store(temp, fc, r.id);
                ++it;
            }
            return MIRExprResult::place(temp, st);
        }
        case ValueKind::FunctionCall: {
            auto* call = value->as_func_call_unsafe();
            FunctionDeclaration* fd = nullptr;
            std::vector<MIROperand> args;

            // method call: `receiver.method(args)`
            if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* chain = call->parent_val->as_access_chain_unsafe();
                if (chain->values.size() >= 2) {
                    if (chain->values.size() != 2) {
                        error = "chained method calls are not yet supported";
                        return MIRExprResult::error();
                    }
                    return lower_method_call(chain->values[0], call, error);
                }
            }
            {
                ASTNode* linked = call->parent_val ? call->parent_val->linked_node() : nullptr;
                fd = linked ? linked->as_function() : nullptr;
                if (!fd) {
                    error = "call to unresolved function";
                    return MIRExprResult::error();
                }
            }
            SymbolId sym = intern_function(fd);

            args.reserve(args.size() + call->values.size());
            for (Value* a : call->values) {
                MIRExprResult r = lower_expr(a, error);
                if (!r.ok()) return r;
                if (r.kind == MIRExprKind::Place) {
                    const PlaceId pid = static_cast<PlaceId>(r.id);
                    const TypeId pt = builder_->function().places[pid].type;
                    const MIRTypeRecord& prec = module_.types.get(pt);
                    if (prec.kind == MIRTypeKind::Pointer || prec.kind == MIRTypeKind::Reference) {
                        // already an address (e.g. an aggregate parameter)
                        args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
                    } else {
                        const TypeId ptrt = types_.pointer_type(pt, false);
                        args.push_back(MIROperand::value(builder_->address_of(pid, ptrt), ptrt));
                    }
                } else if (r.kind == MIRExprKind::Address) {
                    args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
                } else if (r.kind == MIRExprKind::Void) {
                    error = "void expression used as a call argument";
                    return MIRExprResult::error();
                } else {
                    args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
                }
            }

            if (is_void_type(module_, type)) {
                builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
                return MIRExprResult::void_result();
            }
            if (needs_aggregate_path(module_, type)) {
                const PlaceId res = builder_->alloca(type, MIRStorageClass::Temporary);
                if (res == MIR_INVALID_ID) {
                    error = "failed to allocate struct-return result place";
                    return MIRExprResult::error();
                }
                builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
                return MIRExprResult::place(res, type);
            }
            ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
            return MIRExprResult::value(v, type);
        }
        case ValueKind::IndexOperator: {
            auto* io = value->as_index_op_unsafe();
            MIRExprResult base = lower_expr(io->parent_val, error);
            if (!base.ok()) return base;
            MIRExprResult idx = lower_expr(io->idx, error);
            if (!idx.ok()) return idx;
            if (base.kind != MIRExprKind::Value && base.kind != MIRExprKind::Address) {
                error = "index base is not a pointer value";
                return MIRExprResult::error();
            }
            if (needs_aggregate_path(module_, type)) {
                error = "indexing to an aggregate element is not yet supported";
                return MIRExprResult::error();
            }
            return MIRExprResult::value(
                builder_->index_load(static_cast<ValueId>(base.id),
                                     static_cast<ValueId>(idx.id), type), type);
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
            if (!lower_incdec_value(n->getValue(), n->increment, error, out)) {
                return MIRExprResult::error();
            }
            return MIRExprResult::value(out, type);
        }
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
            PlaceId place = builder_->alloca(vt, MIRStorageClass::Local);
            if (place == MIR_INVALID_ID) {
                error = "failed to allocate place for variable";
                return false;
            }
            bind(vi, place);
            if (vi->value) {
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
                    const PlaceId base = resolve_place(chain->values[0], error);
                    if (base == MIR_INVALID_ID) return false;
                    std::string fname;
                    if (!member_name(chain->values.back(), fname, error)) return false;
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate field assignment not yet supported";
                        return false;
                    }
                    builder_->field_store(base, fc, r.id);
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
                builder_->store_indirect(static_cast<ValueId>(p.id), r.id);
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
                builder_->index_store(static_cast<ValueId>(base.id),
                                      static_cast<ValueId>(idx.id), r.id);
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
            if (r.type != place_type) val = builder_->cast(val, place_type);
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
            }
            builder_->store(place, val);
            return true;
        }
        case ASTNodeKind::ReturnStmt: {
            auto* rs = node->as_return();
            if (!rs->value) {
                builder_->ret_void();
                return true;
            }
            MIRExprResult r = lower_expr(rs->value, error);
            if (!r.ok()) return false;
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
                if (!taken) {
                    error = rerr.empty() ? "comptime if could not be resolved" : rerr;
                    return false;
                }
                return lower_scope(*taken, error);
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
            return lower_incdec_value(n->value.getValue(), n->value.increment, error, out);
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
        default:
            error = "unsupported statement kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(node->kind())) + ")";
            return false;
    }
}

bool MIRLowerer::lower_incdec_value(Value* target, bool increment, std::string& error, ValueId& out) {
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
    const ValueId one = builder_->const_int(vt, module_.constants.add_int(vt, 1));
    const ValueId nv = builder_->binary(cur, one, opc, vt);
    builder_->store(place, nv);
    out = nv;
    return true;
}

bool MIRLowerer::lower_scope(Scope& scope, std::string& error) {
    for (ASTNode* n : scope.nodes) {
        if (!lower_stmt(n, error)) return false;
    }
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

bool MIRLowerer::lower_function(FunctionDeclaration* decl, MIRArena& arena,
                                MIRFunction& func, std::string& error) {
    var_places_.clear();
    // func_symbols_ persists across functions in a module so callees keep stable ids

    func.symbol = intern_function(decl);
    const TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    sret_ = needs_aggregate_path(module_, ret);
    sret_ret_type_ = ret;
    {
        std::vector<TypeId> ptypes;
        if (sret_) {
            sret_ptr_type_ = types_.pointer_type(ret, true);
            ptypes.push_back(sret_ptr_type_);
        }
        for (FunctionParam* p : decl->params) {
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (needs_aggregate_path(module_, pt)) pt = types_.pointer_type(pt, true);
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

    // parameters: materialize an SSA param value, spill to a place, bind it
    for (FunctionParam* p : decl->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (needs_aggregate_path(module_, pt)) pt = types_.pointer_type(pt, true);
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        if (place == MIR_INVALID_ID) {
            error = "failed to allocate parameter place";
            builder_ = nullptr;
            return false;
        }
        b.store(place, pv);
        bind(p, place);
    }

    if (decl->body.has_value()) {
        for (ASTNode* node : decl->body->nodes) {
            if (!lower_stmt(node, error)) {
                builder_ = nullptr;
                return false;
            }
        }
    }

    if (!b.current_block_terminated()) b.ret_void();

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "MIR builder failure";
        return false;
    }
    return true;
}

} // namespace mir
