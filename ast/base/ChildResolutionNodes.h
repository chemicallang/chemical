// Copyright (c) Chemical Language Foundation 2025.

#include "ASTNode.h"
#include <unordered_map>
#include "std/chem_string_view_fast.h"
#include <span>

// PTFK
struct PrimitiveTypeFunctionKey {
    chem::string_view name;
    BaseType* type;
    bool operator==(PrimitiveTypeFunctionKey const& b) const noexcept {
        return type == b.type && name == b.name;
    }
};

struct PTFKHash {
    std::size_t operator()(PrimitiveTypeFunctionKey const& k) const noexcept {
        size_t h_name = std::hash<chem::string_view>{}(k.name);
        // pointer identity for BaseType*
        size_t h_ptr = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(k.type));
        // combine (boost::hash_combine style)
        size_t seed = h_name;
        seed ^= h_ptr + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }
};

// Key describing a child lookup: (type name, identity pointer, up to 8 bits)
struct ChildKey {
    chem::string_view name;     // must outlive the map entry
    void* pointer;         // pointer identity (nullable)
    bool is_mutable;
    bool operator==(ChildKey const& o) const noexcept {
        return is_mutable == o.is_mutable && pointer == o.pointer && name == o.name;
    }
};

struct ChildKeyHash {
    size_t operator()(ChildKey const& k) const noexcept {
        size_t h_name = std::hash<chem::string_view>{}(k.name);
        // Hash the pointer identity
        size_t h_ptr = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(k.pointer));
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
    static ASTNode* find_child(std::unordered_map<ChildKey, ASTNode*, ChildKeyHash>& map, void* type_ptr, bool is_mutable, const chem::string_view& name) {
        auto key = ChildKey {
                .name = name,
                .pointer = type_ptr,
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

    ASTNode* find_child(std::unordered_map<ChildKey, ASTNode*, ChildKeyHash>& map, BaseType* type, bool is_mutable, const chem::string_view& name) {
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
                // the underlying pointer doesn't change for these types
                return find_child(map, type, is_mutable, name);
            case BaseTypeKind::Linked:
                // linked types are stored with pointer to linked (because that pointer doesn't change)
                return find_child(map, type->as_linked_type_unsafe()->linked, is_mutable, name);
            case BaseTypeKind::Generic:
                // generic types are stores similar to linked types
                return find_child(map, type->as_generic_type_unsafe()->referenced->linked, is_mutable, name);
            default:
                return nullptr;
        }
    }

    inline ASTNode* find_child(PointerType* type, const chem::string_view& name) {
        return find_child(ptr_child_types, type->type, type->is_mutable, name);
    }

    inline ASTNode* find_child(ReferenceType* type, const chem::string_view& name) {
        return find_child(ref_child_types, type->type, type->is_mutable, name);
    }

    inline void index_primitive_child(BaseType* type, const chem::string_view& name, ASTNode* node) {
        primitive_types_children.emplace(PrimitiveTypeFunctionKey{ .name = name, .type = type }, node);
    }

    void index_child(std::unordered_map<ChildKey, ASTNode*, ChildKeyHash>& map, BaseType* type, bool is_mutable, const chem::string_view& name, ASTNode* node) {
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
                // the underlying pointer doesn't change for these types
                map.emplace(ChildKey{.name = name, .pointer = type, .is_mutable = is_mutable}, node);
                return;
            case BaseTypeKind::Linked:
                // linked types are stored with pointer to linked (because that pointer doesn't change)
                map.emplace(ChildKey{.name = name, .pointer = type->as_linked_type_unsafe()->linked, .is_mutable = is_mutable}, node);
                return;
            case BaseTypeKind::Generic:
                // generic types are stores similar to linked types
                map.emplace(ChildKey{.name = name, .pointer = type->as_generic_type_unsafe()->referenced->linked, .is_mutable = is_mutable}, node);
                return;
            default:
                return;
        }
    }

    void index_ptr_child(PointerType* type, const chem::string_view& name, ASTNode* node) {
        index_child(ptr_child_types, type->type, type->is_mutable, name, node);
    }

    inline void index_ref_child(PointerType* type, const chem::string_view& name, ASTNode* node) {
        index_child(ref_child_types, type->type, type->is_mutable, name, node);
    }

};