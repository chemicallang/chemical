// Copyright (c) Qinetik 2024.

#include "ChainValue.h"
#include "ast/values/StructValue.h"
#include "ast/statements/Assignment.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/IntValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/PointerType.h"
#include "ast/values/UIntValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IndexOperator.h"
#include "ast/types/ArrayType.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/VariantCase.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/If.h"
#include <ranges>
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/structures/StructMember.h"

llvm::AllocaInst* Value::llvm_allocate_with(Codegen& gen, llvm::Value* value, llvm::Type* type) {
    auto x = gen.builder->CreateAlloca(type, nullptr);
    gen.builder->CreateStore(value, x);
    return x;
}

llvm::AllocaInst *Value::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    return llvm_allocate_with(gen, llvm_value(gen, expected_type), expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
}

llvm::AllocaInst* ChainValue::access_chain_allocate(Codegen& gen, std::vector<ChainValue*>& values, unsigned int until, BaseType* expected_type) {
    return llvm_allocate_with(gen, values[until]->access_chain_value(gen, values, until, expected_type), values[until]->llvm_type(gen));
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
        Value* parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    const auto value = llvm_value(gen, expected_type);
    if(!gen.assign_dyn_obj(this, expected_type, elementPtr, value)) {
        gen.builder->CreateStore(value, elementPtr);
    }
    return index + 1;
}

unsigned int Value::store_in_array(
        Codegen& gen,
        Value* parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    const auto value = llvm_value(gen, expected_type);
    if(!gen.assign_dyn_obj(this, expected_type, elementPtr, value)) {
        gen.builder->CreateStore(value, elementPtr);
    }
    return index + 1;
}

void Value::destruct(Codegen& gen, std::vector<std::pair<Value*, llvm::Value*>>& destructibles) {
    for(auto& val : std::ranges::reverse_view(destructibles)) {
        val.first->llvm_destruct(gen, val.second);
    }
}

llvm::Value* Value::load_value(Codegen& gen, BaseType* known_t, llvm::Type* type, llvm::Value* ptr) {
    if(known_t->value_type() == ValueType::Struct) {
        return ptr;
    }
    return gen.builder->CreateLoad(type, ptr);
}

llvm::Value* ChainValue::access_chain_value(Codegen &gen, std::vector<ChainValue*>& values, unsigned int until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles, BaseType* expected_type) {
    if(until == 0) return values[0]->llvm_value(gen, expected_type);
    return Value::load_value(gen, values[until], access_chain_pointer(gen, values, destructibles, until));
}

/**
 * should llvm_chain_type be used to get the llvm_type
 * llvm_chain_type checks each member in the access chain
 * for example, in a.b.c, 'b' could be a unnamed union, or anonymous union declaration
 * now 'c' is accessing member of a union, to be able to access 'c' we emit a get element ptr instruction
 * which must have correct parent type b, now if llvm_type were to be called on 'b' it would consider it's largest
 * member, because that's what unions do, their storage type is the largest member
 * now that largest member may or may not contain 'c' which is the next member, and we are giving llvm
 * the largest member type, so this method checks if the chain contains a union or unnamed union, and
 * one of largest member is not being accessed in the union, then we call llvm_chain_type
 */
bool should_use_chain_type(ASTAllocator& allocator, std::vector<ChainValue*>& values, unsigned index) {
    unsigned i = index;
    // why -1, last not checked, because last maybe a union but there's no member access into it
    const auto total = values.size() - 1;
    while(i < total - 1) {
        auto& value = values[i];
        auto base_type = value->create_type(allocator);
        auto node = base_type->get_direct_linked_node();
        if(node) {
            auto node_kind = node->kind();
            if(node_kind == ASTNodeKind::UnionDecl) {
                auto union_def = node->as_union_def_unsafe();
                auto& next = values[i + 1];
                auto next_node = next->linked_node();
                auto largest = union_def->largest_member();
                if(next_node != largest) {
                    return true;
                }
            } else if(node_kind == ASTNodeKind::UnnamedUnion) {
                auto unnamed_def = node->as_unnamed_union_unsafe();
                auto& next = values[i + 1];
                auto next_node = next->linked_node();
                auto largest = unnamed_def->largest_member();
                if(next_node != largest) {
                    return true;
                }
            }
        }
        i++;
    }
    return false;
}

// this method could be put into access chain's llvm_type, when the need arises
llvm::Type* access_chain_llvm_type(Codegen& gen, BaseType* type, std::vector<ChainValue*>& values, unsigned index) {
    if(should_use_chain_type(gen.allocator, values, index)) {
        return type->llvm_chain_type(gen, values, index);
    } else {
        return type->llvm_type(gen);
    };
}

llvm::Value* create_gep(Codegen &gen, std::vector<ChainValue*>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList) {
    const auto parent = values[index];
    const auto linked = parent->linked_node();
    auto type = parent->create_type(gen.allocator);
    if(type) {
        auto type_kind = type->kind();
        if (type_kind == BaseTypeKind::Array && linked && linked->as_func_param()) {
            auto arr_type = (ArrayType*) type;
            return gen.builder->CreateGEP(access_chain_llvm_type(gen, arr_type->elem_type, values, index), pointer, idxList, "", gen.inbounds);
        } else if (type_kind == BaseTypeKind::Pointer) {
            return gen.builder->CreateGEP(access_chain_llvm_type(gen, ((PointerType*) (type))->type, values, index),
                                          pointer, idxList, "", gen.inbounds);
        }
    }
    return gen.builder->CreateGEP(parent->llvm_chain_type(gen, values, index), pointer, idxList, "", gen.inbounds);
}

std::pair<unsigned int, llvm::Value*> ChainValue::access_chain_parent_pointer(
        Codegen &gen,
        std::vector<ChainValue*>& values,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        unsigned int until,
        std::vector<llvm::Value*>& idxList
) {

#ifdef DEBUG
    if(until == 0) {
        throw std::runtime_error("index can't be zero, because it takes a parent pointer, parent exists at location zero");
    }
#endif

    // evaluate the last function in the access chain
    int j = (int) until;
    while(j >= 0) {
        const auto func_call = values[j]->as_func_call();
        if(func_call) {
            auto func_ret = func_call->access_chain_value(gen, values, j, destructibles, nullptr);
            if(j + 1 <= until) {
                destructibles.emplace_back(func_call, func_ret);
            }
            return { j, func_ret };
        }
        j--;
    }

    unsigned parent_index = 0;
    Value* parent = values[0];
    llvm::Value* pointer = parent->llvm_pointer(gen);

    unsigned i = 1;

    if(parent->is_stored_pointer() && i <= until) {
        pointer = gen.builder->CreateLoad(parent->llvm_type(gen), pointer);
    }

    while (i <= until) {
        if(i + 1 <= until && values[i]->is_stored_pointer()) {
            llvm::Value* gep;
            if(idxList.empty()) {
                gep = pointer;
            } else {
                gep = create_gep(gen, values, parent_index, pointer, idxList);
            }
            pointer = gen.builder->CreateLoad(values[i]->llvm_type(gen), gep);
            parent = values[i];
            parent_index = i;
            idxList.clear();
        } else {
            if (!values[i]->add_member_index(gen, values[i - 1], idxList)) {
                std::string err = "couldn't add member index for fragment '" + values[i]->representation() + "' in access chain ";
                bool is_first = true;
                for(auto& val : values) {
                    if(!is_first) {
                        err += ',';
                    }
                    err += '\'';
                    err += val->representation();
                    err += '\'';
                    is_first = false;
                }
                gen.error(err, values[i]);
            }
        }
        i++;
    }
    return { parent_index, pointer };
}

llvm::Value* ChainValue::pointer_from_parent_to_next(
        Codegen &gen,
        std::vector<ChainValue*>& values,
        std::vector<llvm::Value*>& idxList,
        std::pair<unsigned int, llvm::Value*>& parent_pointer
) {
    return create_gep(gen, values, parent_pointer.first, parent_pointer.second, idxList);
}

llvm::Value* ChainValue::access_chain_pointer(
        Codegen &gen,
        std::vector<ChainValue*>& values,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        unsigned int until
) {
    // a single value, we just return pointer to it
    if(until == 0) {
        return values[0]->llvm_pointer(gen);
    }
    // evaluate last comptime function call
    const auto last = values[until];
    const auto last_func_call = last->as_func_call();
    if(last_func_call) {
        const auto func_decl = last_func_call->safe_linked_func();
        if(func_decl && func_decl->has_annotation(AnnotationKind::CompTime)) {
            auto& ret_value = gen.eval_comptime(last_func_call, func_decl);
            if(ret_value) {
                return ret_value->llvm_pointer(gen);
            } else {
                return nullptr;
            }
        }
    }
    std::vector<llvm::Value*> idxList;
    auto parent_pointer = access_chain_parent_pointer(gen, values, destructibles, until, idxList);
    return create_gep(gen, values, parent_pointer.first, parent_pointer.second, idxList);
}

llvm::Value* ChainValue::access_chain_value(
        Codegen &gen,
        std::vector<ChainValue*>& values,
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        BaseType* expected_type,
        llvm::Value*& parent_pointer_ref
) {
#ifdef DEBUG
    if(until == 0) {
        throw std::runtime_error("index can't be zero, because it takes a parent pointer, parent exists at location zero");
    }
#endif
    const auto func_call = values[until]->as_func_call();
    if(func_call) {
        return func_call->access_chain_value(gen, values, until, destructibles, expected_type);
    }
    std::vector<llvm::Value*> idxList;
    auto parent_pointer = ChainValue::access_chain_parent_pointer(gen, values, destructibles, until, idxList);
    parent_pointer_ref = parent_pointer.second;
    auto value_ptr = ChainValue::pointer_from_parent_to_next(gen, values, idxList, parent_pointer);
    return gen.builder->CreateLoad(llvm_type(gen), value_ptr);
}

void Value::llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) {
    gen.CreateCondBr(llvm_value(gen), then_block, otherwise_block);
}

llvm::Type* Value::llvm_elem_type(Codegen& gen) {
    throw std::runtime_error("llvm_elem_type called on bare Value of type " + std::to_string((int) value_type()));
};

llvm::Value* Value::llvm_pointer(Codegen& gen) {
    throw std::runtime_error("llvm_pointer called on bare Value of type " + std::to_string((int) value_type()));
}

llvm::Value* Value::llvm_value(Codegen& gen, BaseType* type) {
    throw std::runtime_error("Value::llvm_value called on bare Value " + representation() + " , type " + std::to_string((int) value_type()));
}

bool Value::add_member_index(Codegen& gen, Value* parent, std::vector<llvm::Value*>& indexes) {
#ifdef DEBUG
    throw std::runtime_error("Value::add_member_index called on a value");
#else
    std::cerr << "add_member_index called on base value " << representation();
#endif
}

bool Value::add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const std::string& name) {
#ifdef DEBUG
    throw std::runtime_error("Value::add_child_index called on a Value");
#else
    std::cerr << "add_child_index called on base Value " << representation();
#endif
}

llvm::Value* Value::llvm_arg_value(Codegen& gen, FunctionCall* call, unsigned int index) {
    return llvm_value(gen, call->get_arg_type(index));
}

/**
 * this method is called by return statement to get the return value for this Value
 * if this class defines specific behavior for return, it should override this method
 */
llvm::Value* Value::llvm_ret_value(Codegen& gen, ReturnStatement* returnStmt) {
    return llvm_value(gen, returnStmt->known_type());
}

/**
 * called by assignment, to assign the current value to left hand side
 */
llvm::Value* Value::llvm_assign_value(Codegen& gen, Value* lhs) {
    // TODO llvm_value expects a type
    return llvm_value(gen, lhs->known_type());
}

#endif

uint64_t Value::byte_size(bool is64Bit) {
#ifdef DEBUG
    throw std::runtime_error("byte_size called on base Value " + representation());
#else
    std::cerr << "Value::byte_size called on value " << representation() << std::endl;
#endif
}

// stored pointer into a variable, that must be loaded, before using
bool Value::is_stored_pointer() {
    auto linked = linked_node();
    return linked != nullptr && linked->is_stored_pointer();
}

bool Value::is_ref() {
    auto chain = as_access_chain();
    if(chain) {
        return true;
    } else {
        auto id = as_identifier();
        if(id) {
            return true;
        }
    }
    return false;
}

bool Value::is_func_call() {
    auto chain = as_access_chain();
    if(chain && chain->values.back()->as_func_call() != nullptr) {
        return true;
    }
    return false;
}

bool Value::is_ref_moved() {
    auto chain = as_access_chain();
    if(chain) {
        return chain->is_moved;
    } else {
        auto id = as_identifier();
        if(id) {
            return id->is_moved;
        }
    }
    return false;
}

bool Value::requires_memcpy_ref_struct(BaseType* known_type) {
    // is referencing another struct, that is non movable and must be mem copied into the pointer
    const auto chain = as_access_chain();
    const auto id = as_identifier();
    if(id || (chain && chain->values.back()->as_func_call() == nullptr)) {
        auto linked = known_type->get_direct_linked_node();
        if (linked) {
            auto k = linked->kind();
            if(k == ASTNodeKind::UnnamedStruct || k == ASTNodeKind::UnnamedUnion) {
                return true;
            } else if(k == ASTNodeKind::StructDecl || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::UnionDecl) {
                const auto container = linked->as_members_container();
                return container->pre_move_func() == nullptr && container->destructor_func() == nullptr && container->clear_func() == nullptr;
            }
        }
    }
    return false;
}

Value* Value::child(InterpretScope& scope, const std::string& name) {
#ifdef DEBUG
    std::cerr << "Value::child called on base value " + representation();
#endif
    return nullptr;
}

Value* Value::call_member(InterpretScope& scope, const std::string& name, std::vector<Value*>& values) {
#ifdef DEBUG
    std::cerr << "Value::call_member called on base value " + representation() + " with name " + name;
#endif
    return nullptr;
}

Value* Value::index(InterpretScope& scope, int i) {
#ifdef DEBUG
    std::cerr << "Value::index called on base value " + representation();
#endif
    return nullptr;
}

void Value::set_child_value(const std::string& name, Value* value, Operation op) {
#ifdef DEBUG
    std::cerr << "Value::set_child_value called on base value " + representation();
#endif
}

Value* Value::find_in(InterpretScope& scope, Value* parent) {
#ifdef DEBUG
    std::cerr << "Value::find_in called on base value " + representation();
#endif
    return nullptr;
}

void Value::set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op) {
    scope.error("Value::set_value_in called on base value " + representation());
}

StructDefinition* Value::get_param_linked_struct() {
    const auto linked = linked_node();
    if(!linked) return nullptr;
    const auto linked_kind = linked->kind();
    if(linked_kind == ASTNodeKind::FunctionParam) {
        return linked->as_func_param_unsafe()->type->get_direct_linked_struct();
    }
    return nullptr;
}

BaseType* Value::create_type(ASTAllocator& allocator) {
    return nullptr;
}

Value* Value::evaluated_chain_value(InterpretScope& scope, Value* parent) {
    throw std::runtime_error("evaluated chain value called on base value");
}

Value* Value::copy(ASTAllocator& allocator) {
#ifdef DEBUG
    std::cerr << "copy called on base Value, representation : " << representation();
#endif
    return nullptr;
}

unsigned Value::as_uint() {
    return ((UIntValue*) this)->value;
}

char Value::as_char() {
    return ((CharValue*) this)->value;
}

bool Value::as_bool() {
    return ((BoolValue*) this)->value;
}

std::string Value::as_string() {
    return ((StringValue*) this)->value;
}

int Value::as_int() {
    switch(val_kind()) {
        case ValueKind::Int:
            return ((IntValue*) this)->value;
        case ValueKind::NumberValue:
            return (int) ((NumberValue*) this)->value;
        default:
#ifdef DEBUG
            throw std::runtime_error("unknown value for as_int");
#endif
            std::cerr << "unknown value for as_int" << std::endl;
            return -1;
    }
}

float Value::as_float() {
    return ((FloatValue*) this)->value;
}

double Value::as_double() {
    return ((DoubleValue*) this)->value;
}

StringValue* Value::as_string_value() {
    if(val_kind() == ValueKind::String) {
        return (StringValue*) this;
    } else {
        return nullptr;
    }
}

Value* Value::get_first_value_from_value_node(ASTNode* node) {
    const auto k = node->kind();
    switch(k) {
        case ASTNodeKind::AccessChain:
            return node->as_access_chain_unsafe();
        case ASTNodeKind::ValueNode:
            return node->holding_value();
        case ASTNodeKind::IfStmt:
            return get_first_value_from_value_node(((IfStatement*) node)->ifBody.nodes.back());
        case ASTNodeKind::SwitchStmt:
            return get_first_value_from_value_node(((SwitchStatement*) node)->scopes.front().second.nodes.back());
        default:
            return nullptr;
    }
}

std::string Value::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

hybrid_ptr<BaseType> Value::get_child_type() {
    auto base_type = known_type();
    return hybrid_ptr<BaseType> { base_type->known_child_type(), false };
}

Value* Value::scope_value(InterpretScope& scope) {
    return copy(scope.allocator);
}

hybrid_ptr<BaseType> Value::get_pure_type() {
    // TODO check this method, it should free types properly
    auto base_type = known_type();
    auto pure_type = base_type->pure_type();
    if(pure_type == base_type) {;
        return hybrid_ptr<BaseType> { base_type, false };
    } else {
        return hybrid_ptr<BaseType> { pure_type, false };
    }
}

BaseType* Value::pure_type_ptr() {
    return known_type()->pure_type();
}

bool Value::should_build_chain_type(std::vector<Value*>& chain, unsigned index) {
    ASTNode* linked;
    VariablesContainer* union_container = nullptr;
    while(index < chain.size()) {
        linked = chain[index]->linked_node();
        if(linked && (linked->as_union_def() || linked->as_unnamed_union())) {
            union_container = linked->as_variables_container();
        } else if(union_container && linked != union_container->largest_member()) {
            return true;
        }
        index++;
    }
    return false;
}

void Value::set_identifier_value(InterpretScope& scope, Value* rawValue, Operation op) {
    scope.error("set_identifier_value called on base value");
}

int16_t ChainValue::set_generic_iteration(ASTAllocator& allocator) {
//    const auto linked = linked_node();
//    if(linked) {
//        const auto case_var = linked->as_variant_case_var();
//        if (case_var) {
//            const auto known_t = case_var->variant_case->switch_statement->expression->known_type();
//            if (known_t) {
//                const auto prev_itr = known_t->set_generic_iteration(known_t->get_generic_iteration());
//                if (prev_itr > -2) {
//                    return prev_itr;
//                }
//            }
//        }
//    }
    const auto type = create_type(allocator);
    if (type) {
        const auto prev_itr = type->set_generic_iteration(type->get_generic_iteration());
        if(prev_itr > -2) {
            return prev_itr;
        }
    }
    return -2;
}

bool ChainValue::is_equal(ChainValue* other, ValueKind kind, ValueKind other_kind) {
    if(kind == other_kind) {
        switch(kind) {
            case ValueKind::AccessChain: {
                auto this_chain = as_access_chain();
                auto other_chain = other->as_access_chain();
                const auto siz = this_chain->values.size();
                if(siz != other_chain->values.size()) {
                    return false;
                }
                unsigned i = 0;
                while(i < siz) {
                    if(!this_chain->values[i]->is_equal(other_chain->values[i])) {
                        return false;
                    }
                    i++;
                }
                return true;
            }
            case ValueKind::Identifier:
                return as_identifier()->linked == other->as_identifier()->linked;
            case ValueKind::IndexOperator: {
                const auto this_index_op = as_index_op();
                const auto other_index_op = other->as_index_op();
                const auto siz = this_index_op->values.size();
                if(siz != other_index_op->values.size()) {
                    return false;
                }
                unsigned i = 0;
                while(i < siz) {
                    if(this_index_op->values[i]->is_int_n() && other_index_op->values[i]->is_int_n()) {
                        auto this_value = ((IntNumValue*) this_index_op->values[i])->get_num_value();
                        auto other_value = ((IntNumValue*) other_index_op->values[i])->get_num_value();
                        if(this_value != other_value) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                    i++;
                }
                return true;
            }
            case ValueKind::FunctionCall:{
                return false;
            }
            default:
                return false;
        }
    }
    return false;
}

bool ChainValue::link(
    SymbolResolver& linker,
    ChainValue* parent,
    std::vector<ChainValue*>& values,
    unsigned index,
    BaseType* expected_type
) {
    if(parent) {
        if(index == values.size() - 1) {
            return find_link_in_parent(parent, linker, expected_type);
        } else {
            return find_link_in_parent(parent, linker);
        }
    } else {
        return link(linker, (Value*&) values[index]);
    }
}

void ChainValue::relink_parent(ChainValue* parent) {
    throw std::runtime_error("relink_parent called on base chain value");
}

BaseType* implicit_constructor_type(BaseType* return_type, Value* value) {
    auto k = return_type->kind();
    if(k == BaseTypeKind::Linked || k == BaseTypeKind::Generic) {
        const auto linked = return_type->linked_node();
        const auto struc = linked->as_struct_def();
        if(struc) {
            const auto constr = struc->implicit_constructor_for(value);
            if(constr) {
                return constr->func_param_for_arg_at(0)->type;
            }
        }
    }
    return return_type;
}

Value::~Value() = default;