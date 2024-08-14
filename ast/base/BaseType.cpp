// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/GenericType.h"
#include <sstream>
#include "ASTNode.h"

std::string BaseType::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

StructDefinition* BaseType::linked_struct_def() {
    const auto linked = linked_node();
    return linked ? linked->as_struct_def() : nullptr;
}

StructDefinition* BaseType::get_generic_struct() {
    auto linked_struct = linked_struct_def();
    if(linked_struct && !linked_struct->generic_params.empty()) {
        return linked_struct;
    } else {
        return nullptr;
    }
}

std::string& BaseType::ref_name() {
    if(kind() == BaseTypeKind::Referenced) {
        return ((ReferencedType*) (this))->type;
    } else if(kind() == BaseTypeKind::Generic) {
        return ((GenericType*) (this))->referenced->type;
    } else {
#ifdef DEBUG
        throw std::runtime_error("BaseType::ref_name called on unexpected type '" + representation() + "'");
#else
        std::cerr << "BaseType::ref_name called on unexpected type '" + representation() << "'" << std::endl;
        string x;
        return x;
#endif
    }
}

bool BaseType::is_ref_struct() {
    const auto k = kind();
    if(k == BaseTypeKind::Generic) {
        return true;
    } else if(k == BaseTypeKind::Referenced) {
        const auto linked = linked_node();
        return linked && linked->as_struct_def() != nullptr;
    } else {
        return false;
    }
}

FunctionDeclaration* BaseType::implicit_constructor_for(Value *value) {
    const auto linked_def = linked_struct_def();
    if(linked_def) {
        const auto prev_itr = linked_def->active_iteration;
        const auto itr = get_generic_iteration();
        if(itr != -1) {
            linked_def->set_active_iteration(itr);
        }
        const auto implicit_constructor = linked_def->implicit_constructor_func(value);
        if(itr != -1) {
            linked_def->set_active_iteration(prev_itr);
        }
        return implicit_constructor;
    }
    return nullptr;
}

int16_t BaseType::set_generic_iteration(int16_t iteration) {
    if(iteration != -1) {
        const auto generic_struct = get_generic_struct();
        if (generic_struct) {
            const auto prev_itr = generic_struct->active_iteration;
            generic_struct->set_active_iteration(iteration);
            return prev_itr;
        }
    }
    return -2;
}

BaseType::~BaseType() = default;