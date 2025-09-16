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
        return name == o.name && type_ptr == o.type_ptr && is_mutable == o.is_mutable;
    }
};

// splitmix64 as a small finalizer / scramble (cheap & good avalanche)
static inline uint64_t splitmix64(uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

struct ChildKeyHash {
    size_t operator()(ChildKey const& k) const noexcept {
        // 1) hash the string_view to uint64
        uint64_t h_name = static_cast<uint64_t>(FastHashInternedView{}(k.name));

        // 2) pointer -> integer (portable)
        auto p = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(k.type_ptr));

        // 3) pack small field(s) into a 64-bit word
        //    put bits in low byte; mix in a small constant to separate similar ptrs
        auto small = static_cast<uint64_t>(k.is_mutable);

        // 4) combine in a way that mixes locality while still letting splitmix finalize
        //    (we multiply h_name by golden constant to spread it, then add pointer and small)
        uint64_t combined = h_name * 0x9e3779b97f4a7c15ULL + p + (small << 1);

        // 5) final scramble
        uint64_t final_val = splitmix64(combined);

        // cast to size_t (size_t may be 32-bit on some platforms; acceptable)
        return static_cast<size_t>(final_val);
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
    static ASTNode* find_container_of(std::unordered_map<ChildKey, ASTNode*, ChildKeyHash>& map, BaseType* type_ptr, bool is_mutable, const chem::string_view& name) {
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

    ASTNode* find_container_of(PointerType* type, const chem::string_view& name) {
        if(type->type->kind() == BaseTypeKind::Pointer) {
            // impl Trait for **int (multiple indirections) NOT supported
            return nullptr;
        }
        return find_container_of(ptr_child_types, type->type, type->is_mutable, name);
    }

    ASTNode* find_container_of(ReferenceType* type, const chem::string_view& name) {
        if(type->type->kind() == BaseTypeKind::Reference) {
            // impl Trait for &&int (multiple indirections) NOT supported
            return nullptr;
        }
        return find_container_of(ref_child_types, type->type, type->is_mutable, name);
    }

};