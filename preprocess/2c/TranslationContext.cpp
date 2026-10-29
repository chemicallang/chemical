// Copyright (c) Chemical Language Foundation 2025.

#include "TranslationContext.h"

#include "2cBackendContext.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/cbi/model/ASTBuilder.h"
#include "compiler/mangler/NameMangler.h"
#include "compiler/symres/CoreNodes.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/EmbeddedValue.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/FunctionCall.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/InterpretScope.h"
#include "ast/utils/ASTUtils.h"

#include <iostream>

CTranslationContext::CTranslationContext(
    CompilerBinder& binder,
    GlobalInterpretScope& scope,
    NameMangler& mangler,
    ASTAllocator& allocator,
    LocationManager& manager,
    CoreNodes& coreNodes,
    ImplementationsIndex& implsIndex,
    bool debug_info,
    bool minify
) : binder(binder), comptime_scope(scope), mangler(mangler), allocator(allocator),
    loc_man(manager), line_directives(debug_info), minify(minify),
    ASTDiagnoser(manager), coreNodes(coreNodes), implsIndex(implsIndex)
{

}

CTranslationContext::~CTranslationContext() = default;

void write_escape_encoded(BufferedWriter& stream, char value) {
    switch (value) {
        case '\a':
            stream << "\\a";
            return;
        case '\f':
            stream << "\\f";
            return;
        case '\r':
            stream << "\\r";
            return;
        case '\n':
            stream << "\\n";
            return;
        case '\0':
            stream << "\\0";
            return;
        case '\t':
            stream << "\\t";
            return;
        case '\v':
            stream << "\\v";
            return;
        case '\b':
            stream << "\\b";
            return;
        case '\"':
            stream << "\\\"";
            return;
        case '\?':
            stream << "\\?";
            return;
        case '\x1b':
            stream << "\\x1b";
            return;
        case '\'':
            stream << "\\'";
            return;
        case '\\':
            stream << "\\\\";
            return;
        default:
            stream << value;
    }
}

void write_encoded(CTranslationContext& visitor, const chem::string_view& value) {
    auto& out = visitor.writer;
    for(char c : value) {
        write_escape_encoded(out, c);
    }
}

void CTranslationContext::write_str_value(const chem::string_view& view) {
    write('"');
    write_encoded(*this, view);
    write('"');
}

void CTranslationContext::indent() {
    unsigned start = 0;
    while(start < indentation_level) {
        write('\t');
        start++;
    }
}

/**
 * evaluates the given comptime function call and returns its evaluated value.
 */
static Value* evaluate_comptime_func(
        CTranslationContext& visitor,
        FunctionDeclaration* func_decl,
        FunctionCall* call
) {
    const auto prev = visitor.comptime_scope.current_func_type;
    visitor.comptime_scope.current_func_type = visitor.current_func_type;
    const auto prev_runtime_call = visitor.comptime_scope.is_runtime_call;
    visitor.comptime_scope.is_runtime_call = true;
    const auto value = func_decl->call(&visitor.comptime_scope, visitor.allocator, call, build_parent_chain(call->parent_val, visitor.allocator), false);
    visitor.comptime_scope.is_runtime_call = prev_runtime_call;
    visitor.comptime_scope.current_func_type = prev;
    // put all diagnostics for this function inside the diagnoser
    auto& diags = visitor.comptime_scope.diagnostics;
    for(auto& diag : diags) {
        visitor.add_diag(diag);
    }
    visitor.comptime_scope.reset_diagnostics();
    if(!value) {
        return nullptr;
    }
    return value;
}

Value* CTranslationContext::eval_comptime_call(FunctionCall* call, FunctionDeclaration* decl) {
    return evaluate_comptime_func(*this, decl, call);
}

Value* CTranslationContext::eval_comptime_ctor(FunctionDeclaration* ctor, Value* arg) {
    BaseType* expected = ctor->params.empty() ? nullptr : ctor->params[0]->type;
    FunctionCall* call = call_with_arg(ctor, arg, expected, allocator, *this);
    if (!call) return nullptr;
    return evaluate_comptime_func(*this, ctor, call);
}

static ASTNode* embedded_repl(CTranslationContext& visitor, EmbeddedNode* node, CBIFunctionType type) {
    ASTAllocator& repl_alloc = visitor.embedded_replacement_allocator
        ? *visitor.embedded_replacement_allocator : visitor.allocator;
    ASTBuilder builder(&repl_alloc, visitor.comptime_scope.typeBuilder);
    const auto replacement_fn = visitor.binder.findHook(node->name, type);
    if(!replacement_fn) {
        visitor.error(node) << "couldn't find replacement function for embedded node with name '" << node->name << "'";
        return nullptr;
    }
    const auto repl = ((EmbeddedNodeReplacementFunc) replacement_fn)(&builder, &visitor, node);
    if(repl) {
        return repl;
    } else {
        visitor.error(node) << "couldn't replace embedded node with name '" << node->name << "'";
        return nullptr;
    }
}

ASTNode* CTranslationContext::resolve_embedded_node(EmbeddedNode* node) {
    return embedded_repl(*this, node, CBIFunctionType::ReplacementNode);
}

Value* CTranslationContext::resolve_embedded_value(EmbeddedValue* value) {
    ASTAllocator& repl_alloc = embedded_replacement_allocator
        ? *embedded_replacement_allocator : allocator;
    ASTBuilder builder(&repl_alloc, comptime_scope.typeBuilder);
    const auto replacement_fn = binder.findHook(value->name, CBIFunctionType::ReplacementValue);
    if(!replacement_fn) {
        error(value) << "couldn't find replacement function for embedded value with name '" << value->name << "'";
        return nullptr;
    }
    return ((EmbeddedValueReplacementFunc) replacement_fn)(&builder, this, value);
}

static void declare_fat_pointer(CTranslationContext& visitor) {
    visitor.fat_pointer_type = "__chemical_fat_pointer__";
    visitor.write("typedef struct {");
    visitor.indentation_level+=1;
    visitor.new_line_and_indent();
    visitor.write("void* first;");
    visitor.new_line_and_indent();
    visitor.write("void* second;");
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    visitor.write("} ");
    visitor.write(visitor.fat_pointer_type);
    visitor.write(';');
}

void CTranslationContext::prepare_translate() {
    write("#ifndef __cplusplus\n"
          "    #ifndef bool\n"
          "        #define bool _Bool\n"
          "    #endif\n"
          "    #ifndef true\n"
          "        #define true 1\n"
          "    #endif\n"
          "    #ifndef false\n"
          "        #define false 0\n"
          "    #endif\n"
          "#endif\n"
          "#ifndef __bool_true_false_are_defined\n"
          "    #define __bool_true_false_are_defined 1\n"
          "#endif\n"
          "#ifndef NULL\n"
          "    #ifdef __cplusplus\n"
          "        #define NULL 0\n"
          "    #else\n"
          "        #define NULL ((void*)0)\n"
          "    #endif\n"
          "#endif\n"
          "#ifndef offsetof\n"
          "    #define offsetof(type, member) ((uint64_t)&(((type*)0)->member))\n"
          "#endif\n");
    const auto& target = comptime_scope.target_data;
    const char* int64_type = (target.is64Bit && !target.win64) ? "long" : "long long";
    const char* uint64_type = (target.is64Bit && !target.win64) ? "unsigned long" : "unsigned long long";
    write("typedef signed char int8_t;\n"
          "typedef unsigned char uint8_t;\n"
          "typedef short int16_t;\n"
          "typedef unsigned short uint16_t;\n"
          "typedef int int32_t;\n"
          "typedef unsigned int uint32_t;\n"
          "typedef ");
    write(int64_type);
    write(" int64_t;\n"
          "typedef ");
    write(uint64_type);
    write(" uint64_t;\n");
    declare_fat_pointer(*this);
    new_line_and_indent();
    if(comptime_scope.target_data.win64) {
        write("extern void* malloc(unsigned long long size);\n");
    } else {
        write("extern void* malloc(unsigned long size);\n");
    }
    write("extern void free(void* block);\n");
    // MIR inserts a fixed-array bounds check that calls __chc_bounds_fail.
    write("extern void abort(void);\n");
    // Panic/failure runtime ABI. Every generated C file carries the abort
    // profile: a failed check reports its reason and source location, then
    // terminates. The functions are `static inline` so a module that never
    // fails a check does not leave an unused external definition behind.
    // An unwinding profile can replace these entry points without changing
    // the MIR call sites.
    write("extern int printf(const char* format, ...);\n"
          "static inline void __chc_panic(const char* message, const char* file, uint32_t line, uint32_t column) {\n"
          "    if (message == 0) { message = \"(null)\"; }\n"
          "    if (file != 0) {\n"
          "        printf(\"panic: %s at %s:%u:%u\\n\", message, file, (unsigned)(line + 1u), (unsigned)(column + 1u));\n"
          "    } else {\n"
          "        printf(\"panic: %s\\n\", message);\n"
          "    }\n"
          "    abort();\n"
          "}\n"
          "static inline void __chc_bounds_fail(const char* file, uint32_t line, uint32_t column) {\n"
          "    __chc_panic(\"index out of bounds\", file, line, column);\n"
          "}\n"
          "static inline void __chc_overflow_fail(const char* file, uint32_t line, uint32_t column) {\n"
          "    __chc_panic(\"integer overflow\", file, line, column);\n"
          "}\n"
          "static inline void __chc_div_zero_fail(const char* file, uint32_t line, uint32_t column) {\n"
          "    __chc_panic(\"division by zero\", file, line, column);\n"
          "}\n"
          "static inline void __chc_shift_fail(const char* file, uint32_t line, uint32_t column) {\n"
          "    __chc_panic(\"shift amount out of range\", file, line, column);\n"
          "}\n"
          "static inline void __chc_alloc_fail(const char* file, uint32_t line, uint32_t column) {\n"
          "    __chc_panic(\"allocation failed\", file, line, column);\n"
          "}\n"
          "static inline void __chc_enum_fail(const char* file, uint32_t line, uint32_t column) {\n"
          "    __chc_panic(\"invalid enum value\", file, line, column);\n"
          "}\n");
    // The MIR emitter spells a struct copy and an aggregate zeroing as bare
    // `memcpy`/`memset` (the C compiler lowers them to width-sized stores or a
    // libcall). cstd declares the same functions, but the module that calls one
    // can be translated *before* cstd's declaration block, so the call would be
    // an implicit declaration later redefined incompatibly. Declare them up
    // front, with the same `size_t` spelling cstd uses for the target, so every
    // call sees a prototype and cstd's later declaration stays compatible.
    const char* size_type = (comptime_scope.target_data.win64)
                                ? "unsigned long long" : "unsigned long";
    write("extern void* memset(void* dest, int ch, ");
    write(size_type);
    write(" count);\n");
    write("extern void* memcpy(void* dest, const void* src, ");
    write(size_type);
    write(" count);\n");
    write("extern void* memmove(void* dest, const void* src, ");
    write(size_type);
    write(" count);\n");
    write("#if defined(_MSC_VER)\n"
          "    #define __chem_stdcall __stdcall\n"
          "    #define __chem_dllimport __declspec(\"dllimport\")\n"
          "#else\n"
          "    #define __chem_stdcall __attribute__((stdcall))\n"
          "    #define __chem_dllimport __attribute__((dllimport))\n"
          "#endif\n"
      );
    write("#ifndef __chx_align\n"
          "#if defined(_MSC_VER)\n"
          "  #define __chx_align(N) __declspec(align(N))\n"
          "  #define __chx_align_struct(N) __declspec(align(N)) struct\n"
          "#else\n"
          "  #define __chx_align(N) __attribute__((aligned(N)))\n"
          "  #define __chx_align_struct(N) struct __attribute__((aligned(N)))\n"
          "#endif\n"
          "#endif\n"
      );
    write("#ifndef __chx_thread_local\n"
        "#if defined(__TINYC__)\n"
        "  #define __chx_thread_local\n"
        "#elif defined(_MSC_VER)\n"
        "  #define __chx_thread_local __declspec(\"thread\")\n"
        "#elif defined(__cplusplus) && (__cplusplus >= 201103L)\n"
        "  #define __chx_thread_local thread_local\n"
        "#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_THREADS__)\n"
        "  #define __chx_thread_local _Thread_local\n"
        "#elif defined(__GNUC__) || defined(__clang__)\n"
        "  #define __chx_thread_local __thread\n"
        "#else\n"
        "  #define __chx_thread_local _Thread_local\n"
        "#endif\n"
        "#endif\n"
    );
    write("#if defined(__clang__)\n"
"#pragma clang diagnostic ignored \"-Wincompatible-pointer-types\"\n"
"#elif defined(__GNUC__)\n"
"#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n"
"#elif defined(_MSC_VER)\n"
"#pragma warning(disable: 4047 4024)\n"
"#endif\n");
}

void CTranslationContext::end_translate() {
    // static-interface stubs are emitted by
    // ASTProcessor::emit_static_interface_stubs (once per job, through MIR)
}

void CTranslationContext::file_level_reset() {
    allocator.clear();
}

void CTranslationContext::reset() {
    current_func_type = nullptr;
    indentation_level = 0;
}

// ---------------------------------------------------------------------------
// ToCBackendContext hooks (comptime interpreter bridge)
// ---------------------------------------------------------------------------

bool ToCBackendContext::forget(ASTNode* targetNode) {
    // MIR owns destruction via drop flags; nothing to forget here.
    return false;
}

void ToCBackendContext::mem_copy(Value *lhs, Value *rhs) {
    // MIR emits struct copies directly; the legacy comptime hook is unused.
}

bool ToCBackendContext::supports(CompilerFeatureKind kind) {
    const auto& target = visitor->comptime_scope.target_data;
    switch(kind) {
        case CompilerFeatureKind::Float128:
            return !target.tcc;
        case CompilerFeatureKind::AtomicBuiltins:
            return true;
        case CompilerFeatureKind::InlineAsm:
            return true;
        case CompilerFeatureKind::Volatile:
            return true;
        default:
            return false;
    }
}

Value* ToCBackendContext::atomic_load(Value* ptr, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) {
    visitor->error(ptr) << "atomic_load is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}

Value* ToCBackendContext::atomic_store(Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) {
    visitor->error(ptr) << "atomic_store is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}

Value* ToCBackendContext::atomic_fence(BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope, SourceLocation location) {
    visitor->error(location) << "atomic_fence is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}

Value* ToCBackendContext::atomic_cmp_exch_weak(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) {
    visitor->error(ptr) << "atomic_cmp_exch_weak is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}

Value* ToCBackendContext::atomic_cmp_exch_strong(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) {
    visitor->error(ptr) << "atomic_cmp_exch_strong is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}

Value* ToCBackendContext::atomic_op(BackendAtomicOp op, Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) {
    visitor->error(ptr) << "atomic_op is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}

Value* ToCBackendContext::signal_fence(BackendAtomicMemoryOrder order) {
    visitor->error(SourceLocation(ZERO_LOC)) << "signal_fence is unsupported on C backend (use %runtime_value() inline asm)";
    return nullptr;
}
