// Copyright (c) Chemical Language Foundation 2025.

#include "ASTNode.h"
#include <unordered_map>
#include "std/chem_string_view_fast.h"

// PTFK
struct PrimitiveTypeFunctionKey {
    chem::string_view name;
    BaseType* type;
    bool operator==(PrimitiveTypeFunctionKey const& b) const noexcept {
        return name.data() == b.name.data() && name.size() == b.name.size() && type == b.type;
    }
};

struct PTFKHash {
    std::size_t operator()(PrimitiveTypeFunctionKey const& k) const noexcept {
        return FastHashInternedView{}(k.name) ^ (std::hash<BaseType*>{}(k.type) << 1);
    }
};

// Key describing a child lookup: (type name, identity pointer, up to 8 bits)
struct ChildKey {
    chem::string_view name;     // must outlive the map entry
    BaseType* type_ptr;         // pointer identity (nullable)
    bool is_mutable;
    bool operator==(ChildKey const& o) const noexcept {
        return is_mutable == o.is_mutable && type_ptr == o.type_ptr && name == o.name;
    }
};

struct ChildKeyHash {
    size_t operator()(ChildKey const& k) const noexcept {
        // Hash the string contents (std::hash<string_view> is content-based)
        size_t h_name = std::hash<chem::string_view>{}(k.name);

        // Hash the pointer identity
        size_t h_ptr = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(k.type_ptr));

        // Small integer for the mutable flag
        size_t h_mut = k.is_mutable ? 0x9e3779b97f4a7c15ULL : 0xC13FA9A902A6328FULL;

        // combine (boost::hash_combine style)
        size_t seed = h_name;
        seed ^= h_ptr + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        seed ^= h_mut + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);

        return seed;
    }
};


// the ChildResolver helps us resolve children for primitive types (that user adds)
// the ChildResolver helps us resolve children for pointer / reference to primitive or non-primitive
// types. It should be noted that container types like struct/union/variant/interface aren't covered
// even pointer to container types aren't resolved here, their children are stored directly inside
// their MembersContainer class, indexed in VariablesContainer class
class ChildResolver {
public:

    // all the primitive types are indexed directly
    // CAREFUL: primitive types should be initialized once, and must not be changed after-wards
    // because we use pointers to its values (which are nodes)
    std::unordered_map<PrimitiveTypeFunctionKey, ASTNode*, PTFKHash> primitive_types_children;

    // all the reference types, their children are stored here
    std::unordered_map<ChildKey, ASTNode*, ChildKeyHash> ref_child_types;

    // all the pointer types, their children are stored here
    std::unordered_map<ChildKey, ASTNode*, ChildKeyHash> ptr_child_types;

    // constructor
    ChildResolver() {}

    /**
     * container of
     */
    static ASTNode* find_child(std::unordered_map<ChildKey, ASTNode*, ChildKeyHash>& map, BaseType* type_ptr, bool is_mutable, const chem::string_view& name) {
        auto key = ChildKey {
                .name = name,
                .type_ptr = type_ptr,
                .is_mutable = is_mutable
        };
        auto found = map.find(key);
        if(found == map.end()) {
            if(is_mutable) {
                // If not found and type is mutable, try const fallback (*mut T -> *const T)
                key.is_mutable = false;
                auto second_found = map.find(key);
                return second_found != map.end() ? second_found->second : nullptr;
            } else {
                // pointer/reference type is not mutable, don't search further
                return nullptr;
            }
        }
        // found exact match
        return found->second;
    }

    // only types that can be stored inside type builder
    // are allowed
    static bool isNestedTypeAllowed(BaseType* type) {
        switch(type->kind()) {
            case BaseTypeKind::IntN:
            case BaseTypeKind::String:
            case BaseTypeKind::ExpressiveString:
            case BaseTypeKind::Double:
            case BaseTypeKind::Float:
            case BaseTypeKind::Float128:
            case BaseTypeKind::LongDouble:
            case BaseTypeKind::Any:
            case BaseTypeKind::Void:
            case BaseTypeKind::NullPtr:
            case BaseTypeKind::Bool:
                return true;
            default:
                return false;
        }
    }

    ASTNode* find_child(PointerType* type, const chem::string_view& name) {
        if(!isNestedTypeAllowed(type->type)) {
            return nullptr;
        }
        return find_child(ptr_child_types, type->type, type->is_mutable, name);
    }

    ASTNode* find_child(ReferenceType* type, const chem::string_view& name) {
        if(!isNestedTypeAllowed(type->type)) {
            return nullptr;
        }
        return find_child(ref_child_types, type->type, type->is_mutable, name);
    }

};