// Copyright (c) Chemical Language Foundation 2025.
//
// Maps resolved AST `BaseType`s to canonical MIR `TypeId`s. This is the one
// MIR component allowed to depend on AST types (it is the lowering boundary);
// the core MIR headers stay AST-free. See mir-implementation-plan.md §2.6 and
// §4 Stage 2.5.

#pragma once

#include "MIRModule.h"

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

class BaseType;
class ASTNode;

namespace mir {

class MIRTypeBuilder {
public:
    explicit MIRTypeBuilder(MIRModule& module) : module_(module) {}

    /**
     * Resolver for the C-emittable name of a named aggregate declaration. Set
     * by the ASTProcessor so aggregates referenced from any file get a name
     * even when their declaration lives in a different file of the module.
     */
    void set_name_resolver(std::function<std::string(ASTNode*)> fn) {
        name_resolver_ = std::move(fn);
    }

    /** Map an AST type to a canonical MIR type id. Never returns INVALID. */
    TypeId map(BaseType* type);

    /** Build a function type from a return type and parameter type ids. */
    TypeId function_signature(TypeId return_type, const std::vector<TypeId>& params,
                              bool variadic = false);

    /** Create an opaque named struct type (e.g. a coroutine frame). */
    TypeId named_struct(const std::string& name) {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Struct;
        r.size = 0;
        r.alignment = 1;
        const TypeId id = module_.types.push(r);
        module_.types.update(id, [&](MIRTypeRecord& rec) {
            rec.decl = id + 0x40000000u;
        });
        module_.types.set_name(id, name.data(), static_cast<uint32_t>(name.size()));
        return id;
    }

    /** Pointer to `pointee`. */
    TypeId pointer_type(TypeId pointee, bool is_mutable = false);

    /** An unsized array type (spelled `T[]`, e.g. a module-level `char[]`). */
    TypeId unsized_array_type(TypeId element) {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Array;
        r.element = element;
        r.data_count = 0;
        const MIRTypeRecord& elem = module_.types.get(element);
        r.size = 0;
        r.alignment = elem.alignment;
        return intern(r);
    }

    /** A sized array type `[count]element`. */
    TypeId sized_array_type(TypeId element, uint32_t count) {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Array;
        r.element = element;
        r.data_count = count;
        const MIRTypeRecord& elem = module_.types.get(element);
        r.size = static_cast<uint32_t>(elem.size * count);
        r.alignment = elem.alignment;
        return intern(r);
    }

    /** Unsigned 32-bit integer type (loop indices, array indices). */
    TypeId u32_type() { return int_type(false, 32); }

    /**
     * The C `int` type (`IK_INT`, so it is spelled `int` rather than
     * `int32_t`). Used for the synchronous `int main` entry point an
     * application's `async func main` gets (see
     * `MIRLowerer::lower_async_main_trampoline`).
     */
    TypeId int_c_type() {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Int;
        r.flags = TF_SIGNED;
        r.size = 4;
        r.alignment = 4;
        r.int_kind = IK_INT;
        return intern(r);
    }

    /** Unsigned 64-bit integer type (`size_t`-sized values). */
    TypeId u64_type() { return int_type(false, 64); }

    /** The C `char` type (string literals and `char` arithmetic). */
    TypeId char_type() {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Int;
        r.flags = TF_SIGNED;
        r.size = 1;
        r.alignment = 1;
        r.int_kind = IK_CHAR;
        return intern(r);
    }

    /** The dynamic-dispatch fat pointer type (`__chemical_fat_pointer__`). */
    TypeId fat_pointer_type() {
        if (fat_pointer_id_ != MIR_INVALID_ID) return fat_pointer_id_;
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Struct;
        r.flags = TF_TYPEDEF;
        r.size = module_.layout.pointer_size * 2;
        r.alignment = module_.layout.pointer_alignment;
        fat_pointer_id_ = intern(r);
        module_.types.set_name(fat_pointer_id_, "__chemical_fat_pointer__", 24);
        // two pointer fields: `first` (object) and `second` (vtable)
        std::vector<MIRFieldInfo> fs;
        for (const char* n : {"first", "second"}) {
            const size_t nlen = std::strlen(n);
            MIRFieldInfo fi;
            fi.type = pointer_type(void_type(), true);
            fi.kind = static_cast<uint8_t>(MIRFieldKind::Direct);
            fi.index = static_cast<uint32_t>(fs.size());
            fi.name_offset = module_.types.append_field_name(n, static_cast<uint32_t>(nlen));
            fi.name_length = static_cast<uint32_t>(nlen);
            fs.push_back(fi);
        }
        const uint32_t fidx = module_.types.append_fields(fs);
        module_.types.update(fat_pointer_id_, [&](MIRTypeRecord& rec) {
            rec.data_count = 2;
            rec.field_index = fidx;
        });
        return fat_pointer_id_;
    }

    /** The module type table this maps into. */
    MIRModule& module() { return module_; }

    TypeId void_type() { return intern_simple(MIRTypeKind::Void, 0, 1); }
    TypeId opaque_type() { return intern_simple(MIRTypeKind::Opaque, 0, 1); }
    TypeId bool_type() {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Bool;
        r.size = 1;
        r.alignment = 1;
        return intern(r);
    }

private:
    TypeId intern(const MIRTypeRecord& rec) { return module_.types.intern(rec); }

    TypeId intern_simple(MIRTypeKind kind, uint32_t size, uint32_t align) {
        MIRTypeRecord r;
        r.kind = kind;
        r.size = size;
        r.alignment = align;
        return intern(r);
    }

    TypeId int_type(bool is_unsigned, uint32_t bits);
    TypeId pointer_like(MIRTypeKind kind, TypeId child, bool is_mutable);
    TypeId aggregate_type(MIRTypeKind kind, const void* decl, BaseType* type);
    TypeId function_type(BaseType* type);
    /** Give an inline (anonymous/locally-named) struct type a stable C name. */
    void ensure_inline_struct_name(TypeId id, BaseType* struct_type);
    /** Fill the ordered runtime fields (and size/alignment) of an aggregate. */
    void fill_aggregate_fields(TypeId id, MIRTypeKind kind, ASTNode* decl, BaseType* type);

public:
    /**
     * Drop the `BaseType*`-keyed memo. The AST file allocator is reset between
     * files, so an address can be reused for a different type; a stale memo
     * would then return the wrong `TypeId`. Call this after each file.
     */
    void clear_memo() {
        std::lock_guard<std::recursive_mutex> lk(builder_mtx_);
        memo_.clear();
    }

private:
    MIRModule& module_;
    std::unordered_map<const BaseType*, TypeId> memo_;
    // Keyed by (declaration node, aggregate kind): the AST arena reuses
    // addresses after a file allocator reset, so keying on the bare pointer
    // would collide a struct and a variant that happened to land on the same
    // address.
    std::map<std::pair<const void*, uint8_t>, uint32_t> decl_ids_;
    std::map<std::pair<const void*, uint8_t>, TypeId> decl_types_;
    std::function<std::string(ASTNode*)> name_resolver_;
    TypeId fat_pointer_id_ = MIR_INVALID_ID;
    uint32_t inline_counter_ = 0;
    /**
     * Guards `memo_`, `decl_ids_`, `decl_types_`, `fat_pointer_id_` and
     * `inline_counter_` (action E3d): several lowering workers share one builder,
     * so its caches must not be raced. Recursive because `map` recurses.
     */
    mutable std::recursive_mutex builder_mtx_;
};

} // namespace mir
