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
    const chem::string_view name = decl->name_view();
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
            if (place == MIR_INVALID_ID) {
                error = "identifier does not resolve to a local place (unsupported: globals/params)";
                return MIRExprResult::error();
            }
            if (needs_aggregate_path(module_, type)) {
                return MIRExprResult::place(place, type);
            }
            return MIRExprResult::value(builder_->load(place, type), type);
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
            ASTNode* linked = call->parent_val ? call->parent_val->linked_node() : nullptr;
            FunctionDeclaration* fd = linked ? linked->as_function() : nullptr;
            if (!fd) {
                error = "call to unresolved function";
                return MIRExprResult::error();
            }
            SymbolId sym = intern_function(fd);

            std::vector<MIROperand> args;
            args.reserve(call->values.size());
            for (Value* a : call->values) {
                MIRExprResult r = lower_expr(a, error);
                if (!r.ok()) return r;
                if (r.kind == MIRExprKind::Place) {
                    args.push_back(MIROperand::place(static_cast<PlaceId>(r.id), r.type));
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
                error = "struct-returning call requires an explicit result place (aggregate milestone)";
                return MIRExprResult::error();
            }
            ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
            return MIRExprResult::value(v, type);
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
            if (as->assOp != Operation::Assignment) {
                error = "compound assignment not yet supported";
                return false;
            }
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
            if (lhs->val_kind() != ValueKind::Identifier) {
                error = "assignment target is not a simple local variable yet";
                return false;
            }
            auto* id = lhs->as_identifier_unsafe();
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID) {
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
        default:
            error = "unsupported statement kind during MIR lowering";
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
    {
        // ensure the symbol's signature type is set even when the symbol was
        // pre-declared by the module builder (which does not know the types yet)
        TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
        std::vector<TypeId> ptypes;
        ptypes.reserve(decl->params.size());
        for (FunctionParam* p : decl->params) {
            ptypes.push_back(p && p->type ? types_.map(p->type) : types_.opaque_type());
        }
        const TypeId ftype = types_.function_signature(ret, ptypes);
        module_.symbols.symbols[func.symbol].type = ftype;
        func.function_type = ftype;
    }

    MIRBuilder b(arena, module_, func);
    builder_ = &b;

    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;

    // parameters: materialize an SSA param value, spill to a place, bind it
    for (FunctionParam* p : decl->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
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
