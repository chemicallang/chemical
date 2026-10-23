// Copyright (c) Chemical Language Foundation 2026.

#include "SafetyReport.h"

#include "compiler/lab/LabModule.h"
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/processor/ASTFileResult.h"
#include "ast/statements/Unreachable.h"
#include "core/source/LocationManager.h"
#include "preprocess/visitors/RecursiveVisitor.h"

#include <algorithm>
#include <iostream>

namespace chem::safety {

void SafetyCounts::add(const SafetyCounts& other) {
    raw_pointer_types += other.raw_pointer_types;
    raw_pointer_derefs += other.raw_pointer_derefs;
    raw_pointer_addrs += other.raw_pointer_addrs;
    raw_pointer_casts += other.raw_pointer_casts;
    unsafe_blocks += other.unsafe_blocks;
    unsafe_values += other.unsafe_values;
    extern_decls += other.extern_decls;
    extern_calls += other.extern_calls;
    variadic_calls += other.variadic_calls;
    panic_calls += other.panic_calls;
    abort_calls += other.abort_calls;
    unreachable_stmts += other.unreachable_stmts;
    new_values += other.new_values;
    delete_stmts += other.delete_stmts;
    dealloc_stmts += other.dealloc_stmts;
    destruct_stmts += other.destruct_stmts;
    allocator_calls += other.allocator_calls;
    memory_calls += other.memory_calls;
    inline_asm += other.inline_asm;
    mutable_globals += other.mutable_globals;
    extern_globals += other.extern_globals;
    union_field_accesses += other.union_field_accesses;
}

uint64_t SafetyCounts::total() const {
    return raw_pointer_types + raw_pointer_derefs + raw_pointer_addrs +
           raw_pointer_casts + unsafe_blocks + unsafe_values + extern_decls +
           extern_calls + variadic_calls + panic_calls + abort_calls +
           unreachable_stmts + new_values + delete_stmts + dealloc_stmts +
           destruct_stmts + allocator_calls + memory_calls + inline_asm +
           mutable_globals + extern_globals + union_field_accesses;
}

void SafetyReport::add(const std::string& path, const SafetyCounts& counts) {
    files[path].add(counts);
}

SafetyCounts SafetyReport::totals() const {
    SafetyCounts total;
    for (const auto& entry : files) {
        total.add(entry.second);
    }
    return total;
}

namespace {

const char* const kCountNames[] = {
    "raw_pointer_types", "raw_pointer_derefs", "raw_pointer_addrs",
    "raw_pointer_casts", "unsafe_blocks", "unsafe_values", "extern_decls",
    "extern_calls", "variadic_calls", "panic_calls", "abort_calls",
    "unreachable_stmts", "new_values", "delete_stmts", "dealloc_stmts",
    "destruct_stmts", "allocator_calls", "memory_calls", "inline_asm",
    "mutable_globals", "extern_globals", "union_field_accesses"
};

const uint32_t SafetyCounts::* const kCountFields[] = {
    &SafetyCounts::raw_pointer_types, &SafetyCounts::raw_pointer_derefs,
    &SafetyCounts::raw_pointer_addrs, &SafetyCounts::raw_pointer_casts,
    &SafetyCounts::unsafe_blocks, &SafetyCounts::unsafe_values,
    &SafetyCounts::extern_decls, &SafetyCounts::extern_calls,
    &SafetyCounts::variadic_calls, &SafetyCounts::panic_calls,
    &SafetyCounts::abort_calls, &SafetyCounts::unreachable_stmts,
    &SafetyCounts::new_values, &SafetyCounts::delete_stmts,
    &SafetyCounts::dealloc_stmts, &SafetyCounts::destruct_stmts,
    &SafetyCounts::allocator_calls, &SafetyCounts::memory_calls,
    &SafetyCounts::inline_asm, &SafetyCounts::mutable_globals,
    &SafetyCounts::extern_globals, &SafetyCounts::union_field_accesses
};

constexpr size_t kCountCount = sizeof(kCountNames) / sizeof(kCountNames[0]);

std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

class SafetyReportVisitor : public RecursiveVisitor<SafetyReportVisitor> {
public:
    LocationManager& loc_man;
    SafetyReport& report;
    SafetyCounts fallback;
    SourceLocation current_loc = 0;

    SafetyReportVisitor(LocationManager& loc_man, SafetyReport& report)
        : loc_man(loc_man), report(report) {}

    SafetyCounts& counts_for(SourceLocation loc) {
        if (!loc.isValid()) return fallback;
        const auto pos = loc_man.getLocationPos(loc);
        const auto path = loc_man.getPathForFileId(pos.fileId);
        if (path.empty()) return fallback;
        return report.files[std::string(path)];
    }

    void bump(SourceLocation loc, uint32_t SafetyCounts::*field) {
        ++(counts_for(loc).*field);
    }

    void VisitFunctionDecl(FunctionDeclaration* decl) {
        current_loc = decl->encoded_location();
        if (decl->is_extern()) {
            bump(decl->encoded_location(), &SafetyCounts::extern_decls);
        }
        RecursiveVisitor<SafetyReportVisitor>::VisitFunctionDecl(decl);
    }

    void VisitVarInitStmt(VarInitStatement* init) {
        current_loc = init->encoded_location();
        if (init->is_top_level()) {
            if (init->is_extern()) {
                bump(init->encoded_location(), &SafetyCounts::extern_globals);
            } else if (!init->attrs.is_const) {
                bump(init->encoded_location(), &SafetyCounts::mutable_globals);
            }
        }
        RecursiveVisitor<SafetyReportVisitor>::VisitVarInitStmt(init);
    }

    void VisitStructDecl(StructDefinition* decl) {
        current_loc = decl->encoded_location();
        RecursiveVisitor<SafetyReportVisitor>::VisitStructDecl(decl);
    }

    void VisitVariantDecl(VariantDefinition* decl) {
        current_loc = decl->encoded_location();
        RecursiveVisitor<SafetyReportVisitor>::VisitVariantDecl(decl);
    }

    void VisitUnionDecl(UnionDef* decl) {
        current_loc = decl->encoded_location();
        RecursiveVisitor<SafetyReportVisitor>::VisitUnionDecl(decl);
    }

    void VisitInterfaceDecl(InterfaceDefinition* decl) {
        current_loc = decl->encoded_location();
        RecursiveVisitor<SafetyReportVisitor>::VisitInterfaceDecl(decl);
    }

    void VisitImplDecl(ImplDefinition* def) {
        // Generic master impls can be finalized into concrete instantiations
        // whose master nodes are no longer safe to walk after codegen. The
        // concrete methods are reached through their owning module/type
        // declarations; skip the master impl here rather than dereferencing a
        // stale evaluated-container entry.
        (void) def;
    }

    void VisitPointerType(PointerType* type) {
        bump(current_loc, &SafetyCounts::raw_pointer_types);
        RecursiveVisitor<SafetyReportVisitor>::VisitPointerType(type);
    }

    void VisitDereferenceValue(DereferenceValue* value) {
        bump(value->encoded_location(), &SafetyCounts::raw_pointer_derefs);
        RecursiveVisitor<SafetyReportVisitor>::VisitDereferenceValue(value);
    }

    void VisitAddrOfValue(AddrOfValue* value) {
        bump(value->encoded_location(), &SafetyCounts::raw_pointer_addrs);
        RecursiveVisitor<SafetyReportVisitor>::VisitAddrOfValue(value);
    }

    void VisitCastedValue(CastedValue* casted) {
        const auto* type = casted->getType();
        if (type && (type->kind() == BaseTypeKind::Pointer || type->kind() == BaseTypeKind::Reference)) {
            bump(casted->encoded_location(), &SafetyCounts::raw_pointer_casts);
        }
        RecursiveVisitor<SafetyReportVisitor>::VisitCastedValue(casted);
    }

    void VisitUnsafeBlock(UnsafeBlock* block) {
        bump(block->encoded_location(), &SafetyCounts::unsafe_blocks);
        RecursiveVisitor<SafetyReportVisitor>::VisitUnsafeBlock(block);
    }

    void VisitUnsafeValue(UnsafeValue* value) {
        bump(value->encoded_location(), &SafetyCounts::unsafe_values);
        RecursiveVisitor<SafetyReportVisitor>::VisitUnsafeValue(value);
    }

    void VisitNewValue(NewValue* value) {
        bump(value->encoded_location(), &SafetyCounts::new_values);
        RecursiveVisitor<SafetyReportVisitor>::VisitNewValue(value);
    }

    void VisitDeleteStmt(DestructStmt* stmt) {
        bump(stmt->encoded_location(), stmt->free_after
            ? &SafetyCounts::delete_stmts
            : &SafetyCounts::destruct_stmts);
        RecursiveVisitor<SafetyReportVisitor>::VisitDeleteStmt(stmt);
    }

    void VisitDeallocStmt(DeallocStmt* stmt) {
        bump(stmt->encoded_location(), &SafetyCounts::dealloc_stmts);
        RecursiveVisitor<SafetyReportVisitor>::VisitDeallocStmt(stmt);
    }

    void VisitInlineAsmStmt(InlineAsmStatement* stmt) {
        bump(stmt->encoded_location(), &SafetyCounts::inline_asm);
        RecursiveVisitor<SafetyReportVisitor>::VisitInlineAsmStmt(stmt);
    }

    void VisitUnreachableStmt(UnreachableStmt* stmt) {
        bump(stmt->encoded_location(), &SafetyCounts::unreachable_stmts);
        RecursiveVisitor<SafetyReportVisitor>::VisitUnreachableStmt(stmt);
    }

    void VisitFunctionCall(FunctionCall* call) {
        RecursiveVisitor<SafetyReportVisitor>::VisitFunctionCall(call);
        if (!call->parent_val) return;
        auto* linked = call->parent_val->get_chain_last_linked();
        if (!linked || linked->kind() != ASTNodeKind::FunctionDecl) return;
        auto* fn = linked->as_function_unsafe();
        const auto name = fn->name_view();
        if (fn->is_extern()) {
            bump(call->encoded_location(), &SafetyCounts::extern_calls);
        }
        if (fn->isVariadic()) {
            bump(call->encoded_location(), &SafetyCounts::variadic_calls);
        }
        if (name == chem::string_view("panic")) {
            bump(call->encoded_location(), &SafetyCounts::panic_calls);
        } else if (name == chem::string_view("abort")) {
            bump(call->encoded_location(), &SafetyCounts::abort_calls);
        } else if (name == chem::string_view("malloc") ||
                   name == chem::string_view("calloc") ||
                   name == chem::string_view("realloc") ||
                   name == chem::string_view("free") ||
                   name == chem::string_view("aligned_alloc") ||
                   name == chem::string_view("posix_memalign")) {
            bump(call->encoded_location(), &SafetyCounts::allocator_calls);
        } else if (name == chem::string_view("memcpy") ||
                   name == chem::string_view("memmove") ||
                   name == chem::string_view("memset") ||
                   name == chem::string_view("memcmp") ||
                   name == chem::string_view("strcpy") ||
                   name == chem::string_view("strncpy")) {
            bump(call->encoded_location(), &SafetyCounts::memory_calls);
        }
    }
};

} // namespace

void SafetyReport::print_text(std::ostream& os) const {
    os << "safety report\n";
    os << "file\t" << "total";
    for (size_t i = 0; i < kCountCount; ++i) os << '\t' << kCountNames[i];
    os << '\n';
    for (const auto& entry : files) {
        const auto& c = entry.second;
        os << entry.first << '\t' << c.total();
        for (size_t i = 0; i < kCountCount; ++i) os << '\t' << (c.*(kCountFields[i]));
        os << '\n';
    }
    const auto total = totals();
    os << "TOTAL\t" << total.total();
    for (size_t i = 0; i < kCountCount; ++i) os << '\t' << (total.*(kCountFields[i]));
    os << '\n';
}

void SafetyReport::print_json(std::ostream& os) const {
    os << "{\"files\":[";
    bool first = true;
    for (const auto& entry : files) {
        if (!first) os << ',';
        first = false;
        os << "{\"path\":\"" << json_escape(entry.first) << "\",\"counts\":{";
        for (size_t i = 0; i < kCountCount; ++i) {
            if (i) os << ',';
            os << '"' << kCountNames[i] << "\":" << (entry.second.*(kCountFields[i]));
        }
        os << "}}";
    }
    os << "],\"totals\":{";
    const auto total = totals();
    for (size_t i = 0; i < kCountCount; ++i) {
        if (i) os << ',';
        os << '"' << kCountNames[i] << "\":" << (total.*(kCountFields[i]));
    }
    os << "}}\n";
}

void collect_module_report(LabModule& module, LocationManager& loc_man, SafetyReport& report) {
    SafetyReportVisitor visitor(loc_man, report);
    for (auto& file : module.direct_files) {
        if (!file.result) continue;
        // Module/build descriptors are not Chemical source; the parser stores
        // them as files too. They contribute no user safety surface.
        const auto& path = file.abs_path;
        const auto ends_with = [&path](const char* suffix) {
            const size_t n = std::char_traits<char>::length(suffix);
            return path.size() >= n && path.compare(path.size() - n, n, suffix) == 0;
        };
        if (ends_with(".mod") || ends_with(".lab")) continue;
        for (ASTNode* node : file.result->unit.scope.body.nodes) {
            if (node) visitor.visit(node);
        }
    }
}

} // namespace chem::safety
