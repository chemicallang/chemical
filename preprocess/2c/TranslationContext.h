// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/ASTDiagnoser.h"

#include "ast/base/ASTAny.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "std/chem_string_view.h"
#include "std/chem_string.h"
#include "compiler/mangler/NameMangler.h"
#include "core/source/LocationManager.h"
#include "BufferedWriter.h"

class ImplementationsIndex;
class CoreNodes;
class GlobalInterpretScope;
class ASTAllocator;
class CompilerBinder;
class FunctionTypeBody;
class EmbeddedNode;
class EmbeddedValue;
class InterfaceDefinition;

/**
 * The shared state and utilities the C translation passes need. This replaces
 * the legacy `ToCAstVisitor`: the AST-walking codegen (`Visit*`,
 * `CDestructionVisitor`, `CTopLevelDeclVisitor`) is gone, and all function
 * bodies are lowered by MIR. What remains is the output writer, the name
 * mangler, the comptime-interpreter bridge, CBI embedded-node/value resolution,
 * and the job-level static-interface stub bookkeeping.
 */
class CTranslationContext : public ASTDiagnoser {
public:

    /**
     * the location manager is used to emit locations
     */
    LocationManager& loc_man;

    /**
     * compile time interpret scope
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * name mangler that would mangle the names
     */
    NameMangler& mangler;

    /**
     * required to replace embedded node and embedded values
     */
    CompilerBinder& binder;

    /**
     * nodes in the core module are indexed in this class
     */
    CoreNodes& coreNodes;

    /**
     * implementations of all interfaces are indexed in this class
     */
    ImplementationsIndex& implsIndex;

    /**
     * the writer is used to actually write to an in memory buffer
     */
    BufferedWriter writer;

    /**
     * the place where last top level node ended, you can write to this position
     */
    size_t top_level_position = 0;

    /**
     * map tells which static interfaces have implementations
     * at the end, we iterate over every static interface here and generate a stub
     */
    std::unordered_map<InterfaceDefinition*, bool> unimplemented_static_interfaces;

    /**
     * the function type for which code is being generated
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * allocator
     */
    ASTAllocator& allocator;

    /**
     * 0 means in root, no indentation, 1 means a single '\t' and so on
     */
    unsigned int indentation_level = 0;

    /**
     * a typedef struct containing two void pointers is prepared
     */
    std::string fat_pointer_type;

    /**
     * when on, line directives will be written
     */
    bool line_directives = false;

    /**
     * this minifies the C code, removing any indentation and new lines
     */
    bool minify = false;

    /**
     * When set, CBI macro (`#macro` / `#universal`) replacement AST is allocated
     * here instead of `allocator`. The MIR -> LLVM backend resolves an
     * `EmbeddedNode` and stores the result in `EmbeddedNode::replacement`, then
     * reads it again *after* every file has been translated - by which point
     * `file_level_reset()` has cleared the (file level) `allocator` and reused
     * its memory, leaving the stored replacement dangling. The module allocator
     * lives until the whole module is compiled, so the LLVM path points this at
     * it. The C backend consumes each replacement within the file that produced
     * it, so it keeps using `allocator`.
     */
    ASTAllocator* embedded_replacement_allocator = nullptr;

    CTranslationContext(
        CompilerBinder& binder,
        GlobalInterpretScope& global,
        NameMangler& mangler,
        ASTAllocator& allocator,
        LocationManager& manager,
        CoreNodes& coreNodes,
        ImplementationsIndex& implsIndex,
        bool debug_info,
        bool minify
    );

    ~CTranslationContext();

    /**
     * used to write a character to the stream
     */
    inline void write(char value) noexcept {
        writer.append_char(value);
    }

    /**
     * write a number
     */
    inline void write(unsigned int num) noexcept {
        writer << num;
    }

    /**
     * used to write a string to a stream
     */
    inline void write(std::string& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * will write this string to stream
     */
    inline void write_str(const std::string& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write a chemical string
     */
    inline void write(chem::string& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write the string view to stream
     */
    inline void write(std::string_view& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write the string view to stream
     */
    inline void write(chem::string_view& str) {
        writer.append(str.data(), str.size());
    }

    /**
     * write this string view to stream
     */
    inline void write(const chem::string_view& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write a space
     */
    inline void space() noexcept {
        write(' ');
    }

    /**
     * mangle and write to output the runtime name of given node
     */
    inline void mangle(ASTNode* node) {
        mangler.mangle(writer, node);
    }

    /**
     * mangle and write to output the runtime name of given function
     */
    inline void mangle(FunctionDeclaration* decl) {
        mangler.mangle(writer, decl);
    }

    /**
     * get a local variable name, that is unique
     */

    /**
     * indentation of \t will be added for current indentation level
     */
    void indent();

    /**
     * write the view encoded as a C string literal (with quotes)
     */
    void write_str_value(const chem::string_view& view);

    /**
     * emits a new line, line directive if required
     */
    inline void new_line_no_check(SourceLocation location) {
        write('\n');
        if(line_directives) {
            write_line_directive(location);
            write('\n');
        }
    }

    inline void new_line(SourceLocation location) {
        if(minify) return;
        new_line_no_check(location);
    }

    inline void new_line_and_indent(SourceLocation location) {
        if(minify) return;
        new_line_no_check(location);
        indent();
    }

    inline void new_line() {
        if(minify) return;
        write('\n');
    }

    inline void new_line_and_indent() {
        if(minify) return;
        write('\n');
        indent();
    }

    /**
     * this writes a line directive
     */
    void write_line_directive(unsigned int lineNumber, const chem::string_view& file_path) {
        write("#line ");
        write(lineNumber);
        write(' ');
        write_str_value(file_path);
    }

    /**
     * writes the line directive for given source location
     */
    void write_line_directive(SourceLocation location) {
        auto loc_pos = loc_man.getLocationPos(location);
        auto filePath = loc_man.getPathForFileId(loc_pos.fileId);
        write_line_directive(loc_pos.start.line + 1, chem::string_view(filePath));
    }

#ifdef DEBUG

    void debug_comment(const chem::string_view& value, bool new_line = true) {
        if(minify) return;
        if(new_line) new_line_and_indent();
        write("/** ");
        write(value);
        write(" **/");
    }

#else

    inline void debug_comment(const chem::string_view& value, bool new_line = true) {

    }

#endif

    /**
     * store this static interface, for generation of stub implementation of it
     * unless it gets implemented by the user
     */
    void store_static_interface_for_stub_impl(InterfaceDefinition* def) {
        auto found = unimplemented_static_interfaces.find(def);
        if(found == unimplemented_static_interfaces.end()) {
            unimplemented_static_interfaces[def] = true;
        }
    }

    /**
     * remove this static interface from storage, so stub implementation for it
     * should not be generated
     */
    void remove_static_interface_for_stub_impl(InterfaceDefinition* def) {
        auto found = unimplemented_static_interfaces.find(def);
        if(found != unimplemented_static_interfaces.end()) {
            unimplemented_static_interfaces.erase(found);
        }
    }

    /**
     * Evaluate a comptime function call at compile time using the interpreter.
     */
    Value* eval_comptime_call(FunctionCall* call, FunctionDeclaration* decl);

    /**
     * Evaluate an implicit-constructor comptime call.
     */
    Value* eval_comptime_ctor(FunctionDeclaration* ctor, Value* arg);

    /**
     * Resolve a CBI embedded node to the AST node its plugin replacement hook
     * produces.
     */
    ASTNode* resolve_embedded_node(EmbeddedNode* node);

    /**
     * Resolve a CBI embedded value to the AST value its plugin hook produces.
     */
    Value* resolve_embedded_value(EmbeddedValue* value);

    /**
     * this should be called before calling translate
     */
    void prepare_translate();

    /**
     * end the translation (static-interface stubs are now emitted by
     * ASTProcessor::emit_static_interface_stubs)
     */
    void end_translate();

    /**
     * this is file level reset, it will free the file level allocator
     */
    void file_level_reset();

    /**
     * this should be called after translating a single module so the context
     * can be reused to translate another module
     */
    void reset();
};
