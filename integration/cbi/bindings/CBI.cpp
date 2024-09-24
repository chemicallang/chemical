// Copyright (c) Qinetik 2024.

#include "LexerCBI.h"
#include "lexer/Lexer.h"
#include "stream/SourceProvider.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/PathUtils.h"
#include "utils/ProcessUtils.h"
#include "compiler/InvokeUtils.h"
#include "compiler/Lab/Utils.h"

#ifdef COMPILER_BUILD
int llvm_ar_main2(const std::vector<std::string> &command_args);
#endif

class dispose_string {
public:
    chem::string* ptr;
    ~dispose_string() {
        ptr->~string();
    }
};

chem::string* init_chem_string(chem::string* str) {
    str->storage.constant.data = nullptr;
    str->storage.constant.length = 0;
    str->state = '0';
    return str;
}

constexpr std::string bc_func(const std::string& name) {
    return "BuildContext" + name;
}

void build_context_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
        {bc_func("files_module"), [](LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies) -> LabModule* {
            dispose_string _x{name};
            return self->files_module(name, path, path_len, dependencies->ptr, dependencies->size);
        }},
        {bc_func("chemical_files_module"), [](LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies) -> LabModule* {
            dispose_string _x{name};
            return self->chemical_files_module(name, path, path_len, dependencies->ptr, dependencies->size);
        }},
        {bc_func("chemical_dir_module"), [](LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies) -> LabModule* {
            dispose_string _x{name};
            dispose_string _y{path};
            return self->chemical_dir_module(name, path, dependencies->ptr, dependencies->size);
        }},
        {bc_func("c_file_module"), [](LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies) -> LabModule* {
            dispose_string _x{name};
            dispose_string _y{path};
            return self->c_file_module(name, path, dependencies->ptr, dependencies->size);
        }},
        {bc_func("cpp_file_module"), [](LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies) -> LabModule* {
            dispose_string _x{name};
            dispose_string _y{path};
            return self->cpp_file_module(name, path, dependencies->ptr, dependencies->size);
        }},
        {bc_func("object_module"), [](LabBuildContext* self, chem::string* name, chem::string* path) -> LabModule* {
            dispose_string _x{name};
            dispose_string _y{path};
            return self->obj_file_module(name, path);
        }},
        {bc_func("translate_to_chemical"), [](LabBuildContext* self, chem::string* c_path, chem::string* output_path) -> LabJob* {
            dispose_string _x{c_path};
            dispose_string _y{output_path};
            return self->translate_to_chemical(c_path, output_path);
        }},
        {bc_func("translate_to_c"), [](LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies, chem::string* output_dir) -> LabJob* {
            dispose_string _x{name};
            dispose_string _y{output_dir};
            return self->translate_to_c(name, dependencies->ptr, dependencies->size, output_dir);
        }},
        {bc_func("build_exe"), [](LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies) -> LabJob* {
            return self->build_exe(name, dependencies->ptr, dependencies->size);
        }},
        {bc_func("build_dynamic_lib"), [](LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies) -> LabJob* {
            return self->build_dynamic_lib(name, dependencies->ptr, dependencies->size);
        }},
        {bc_func("build_cbi"), [](LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies, CBIImportKind kind) -> LabJob* {
            return self->build_cbi(name, dependencies->ptr, dependencies->size, kind);
        }},
        {bc_func("add_object"), [](LabBuildContext* self, LabJob* job, chem::string* path) {
            dispose_string _x{path};
            job->linkables.emplace_back(path->copy());
        }},
        {bc_func("declare_alias"), [](LabBuildContext* self, LabJob* job, chem::string* alias, chem::string* path) {
            dispose_string _x{alias};
            dispose_string _y{path};
            return self->declare_user_alias(job, alias->to_std_string(), path->to_std_string());
        }},
        {bc_func("build_path"), [](chem::string* str, LabBuildContext* self) {
            init_chem_string(str)->append(self->build_dir);
        }},
        {bc_func("has_arg"), [](LabBuildContext* self, chem::string* name) -> bool {
            return self->has_arg(name);
        }},
        {bc_func("get_arg"), [](chem::string* str, LabBuildContext* self, chem::string* name) {
            return self->get_arg(init_chem_string(str), name);
        }},
        {bc_func("remove_arg"), [](LabBuildContext* self, chem::string* name) {
            return self->remove_arg(name);
        }},
        {bc_func("define"), [](LabBuildContext* self, LabJob* job, chem::string* name) -> bool {
            auto def_name = name->to_std_string();
            auto& definitions = job->definitions;
            auto got = definitions.find(def_name);
            if(got == definitions.end()) {
                definitions[def_name] = true;
                return true;
            } else {
                return false;
            }
        }},
        {bc_func("undefine"), [](LabBuildContext* self, LabJob* job, chem::string* name) -> bool {
            auto def_name = name->to_std_string();
            auto& definitions = job->definitions;
            auto got = definitions.find(def_name);
            if(got != definitions.end()) {
                definitions.erase(got);
                return true;
            } else {
                return false;
            }
        }},
        {bc_func("launch_executable"), [](LabBuildContext* self, chem::string* path, bool same_window) {
            dispose_string _x{path};
            auto copied = path->to_std_string();
            copied = absolute_path(copied);
            if(same_window) {
                copied = '\"' + copied + '\"';
            }
            return launch_executable(copied.data(), same_window);
        }},
        {bc_func("on_finished"), [](LabBuildContext* self, void(*lambda)(void*), void* data) {
            self->on_finished = lambda;
            self->on_finished_data = data;
        }},
        {bc_func("link_objects"), [](LabBuildContext* self, StringArrayRef* string_arr, chem::string* output_path) -> int {
            dispose_string _x{output_path};
            std::vector<chem::string> linkables;
            for(auto i = 0; i < string_arr->size; i++) {
                linkables.emplace_back(string_arr->ptr[i].copy());
            }
            return link_objects(self->options->exe_path, linkables, output_path->to_std_string());
        }},
        {bc_func("invoke_dlltool"), [](LabBuildContext* self, StringArrayRef* string_arr) -> int {
#ifdef COMPILER_BUILD
            std::vector<std::string> arr;
            arr.emplace_back("dlltool");
            for(auto i = 0; i < string_arr->size; i++) {
                arr.emplace_back(string_arr->ptr[i].to_std_string());
            }
            return llvm_ar_main2(arr);
#else
            return -1;
#endif
        }},
        {bc_func("invoke_ranlib"), [](LabBuildContext* self, StringArrayRef* string_arr) -> int {
#ifdef COMPILER_BUILD
            std::vector<std::string> arr;
            arr.emplace_back("ranlib");
            for(auto i = 0; i < string_arr->size; i++) {
                arr.emplace_back(string_arr->ptr[i].to_std_string());
            }
            return llvm_ar_main2(arr);
#else
            return -1;
#endif
        }},
        {bc_func("invoke_lib"), [](LabBuildContext* self, StringArrayRef* string_arr) -> int {
#ifdef COMPILER_BUILD
            std::vector<std::string> arr;
            arr.emplace_back("lib");
            for(auto i = 0; i < string_arr->size; i++) {
                arr.emplace_back(string_arr->ptr[i].to_std_string());
            }
            return llvm_ar_main2(arr);
#else
            return -1;
#endif
        }},
        {bc_func("invoke_ar"), [](LabBuildContext* self, StringArrayRef* string_arr) -> int {
#ifdef COMPILER_BUILD
            std::vector<std::string> arr;
            arr.emplace_back("ar");
            for(auto i = 0; i < string_arr->size; i++) {
                arr.emplace_back(string_arr->ptr[i].to_std_string());
            }
            return llvm_ar_main2(arr);
#else
            return -1;
#endif
        }}
    };
}

constexpr std::string sp_func(const std::string& string) {
    return "SourceProvider" + string;
}

void source_provider_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    sym_map = {
            {sp_func("readCharacter"), [](SourceProvider* provider) -> char {
                return provider->readCharacter();
            }},
            {sp_func("eof"), [](SourceProvider* provider) -> bool {
                return provider->eof();
            }},
            {sp_func("peek"), [](SourceProvider* provider) -> char {
                return provider->peek();
            }},
            {sp_func("readUntil"), [](SourceProvider* provider, chem::string* into, char stop) -> void {
                return provider->readUntil(into, stop);
            }},
            {sp_func("increment"), [](SourceProvider* provider, chem::string* text, bool peek) -> bool {
                return provider->increment({ text->data(), text->size() }, peek);
            }},
            {sp_func("increment_char"), [](SourceProvider* provider, char c) -> bool {
                return provider->increment(c);
            }},
            {sp_func("getLineNumber"), [](SourceProvider* provider) -> unsigned int {
                return provider->getLineNumber();
            }},
            {sp_func("getLineCharNumber"), [](SourceProvider* provider) -> unsigned int {
                return provider->getLineCharNumber();
            }},
            {sp_func("readEscaping"), [](SourceProvider* provider, chem::string* value, char stopAt) -> void {
                return provider->readEscaping(value, stopAt);
            }},
            {sp_func("readAnything"), [](SourceProvider* provider, chem::string* value, char until) -> void {
                return provider->readAnything(value, until);
            }},
            {sp_func("readAlpha"), [](SourceProvider* provider, chem::string* value) -> void {
                return provider->readAlpha(value);
            }},
            {sp_func("readUnsignedInt"), [](SourceProvider* provider, chem::string* value) -> void {
                return provider->readUnsignedInt(value);
            }},
            {sp_func("readNumber"), [](SourceProvider* provider, chem::string* value) -> void {
                return provider->readNumber(value);
            }},
            {sp_func("readAlphaNum"), [](SourceProvider* provider, chem::string* value) -> void {
                return provider->readAlphaNum(value);
            }},
            {sp_func("readIdentifier"), [](SourceProvider* provider, chem::string* value) -> void {
                return provider->readIdentifier(value);
            }},
            {sp_func("readAnnotationIdentifier"), [](SourceProvider* provider, chem::string* value) -> void {
                return provider->readAnnotationIdentifier(value);
            }},
            {sp_func("readWhitespaces"), [](SourceProvider* provider) -> unsigned int {
                return provider->readWhitespaces();
            }},
            {sp_func("hasNewLine"), [](SourceProvider* provider) -> bool {
                return provider->hasNewLine();
            }},
            {sp_func("readNewLineChars"), [](SourceProvider* provider) -> bool {
                return provider->readNewLineChars();
            }},
            {sp_func("readWhitespacesAndNewLines"), [](SourceProvider* provider) -> void {
                return provider->readWhitespacesAndNewLines();
            }},

    };
}

constexpr std::string lexer_func(const std::string& name) {
    return "Lexer" + name;
};

void lexer_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    // TODO this
    sym_map = {
            {lexer_func("tokens_size"), [](Lexer* lexer) -> std::size_t {
                return lexer->tokens_size();
            }},
            {lexer_func("storeVariable"), [](Lexer* lexer, chem::string* str) -> bool {
                return lexer->storeVariable(str);
            }},
            {lexer_func("storeIdentifier"), [](Lexer* lexer, chem::string* str) -> bool {
                return lexer->storeIdentifier(str);
            }},
            {lexer_func("lexGenericArgsList"), [](Lexer* lexer) -> void {
                return lexer->lexGenericArgsList();
            }},
            {lexer_func("lexGenericArgsListCompound"), [](Lexer* lexer) -> bool {
                return lexer->lexGenericArgsListCompound();
            }},
            {lexer_func("lexFunctionCallWithGenericArgsList"), [](Lexer* lexer) -> void {
                return lexer->lexFunctionCallWithGenericArgsList();
            }},
            {lexer_func("lexFunctionCall"), [](Lexer* lexer, unsigned int back_start) -> bool {
                return lexer->lexFunctionCall(back_start);
            }},
            {lexer_func("lexAccessSpecifier"), [](Lexer* lexer, bool internal, bool protect) -> bool {
                return lexer->lexAccessSpecifier(internal, protect);
            }},
            {lexer_func("lexAccessChainAfterId"), [](Lexer* lexer, bool lexStruct, unsigned int chain_length) -> bool {
                return lexer->lexAccessChainAfterId(lexStruct, chain_length);
            }},
            {lexer_func("lexAccessChainRecursive"), [](Lexer* lexer, bool lexStruct, unsigned int chain_length) -> bool {
                return lexer->lexAccessChainRecursive(lexStruct, chain_length);
            }},
            {lexer_func("lexAccessChain"), [](Lexer* lexer, bool lexStruct, bool lex_as_node) -> bool {
                return lexer->lexAccessChain(lexStruct, lex_as_node);
            }},
            {lexer_func("lexAccessChainOrAddrOf"), [](Lexer* lexer, bool lexStruct) -> bool {
                return lexer->lexAccessChainOrAddrOf(lexStruct);
            }},
            {lexer_func("lexVarInitializationTokens"), [](Lexer* lexer, unsigned start, bool allowDeclarations, bool requiredType) -> bool {
                return lexer->lexVarInitializationTokens(start, allowDeclarations, requiredType);
            }},
            {lexer_func("lexAssignmentTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexAssignmentTokens();
            }},
            {lexer_func("lexDivisionOperatorToken"), [](Lexer* lexer) -> bool {
                return lexer->lexDivisionOperatorToken();
            }},
            {lexer_func("lexLanguageOperatorToken"), [](Lexer* lexer) -> bool {
                return lexer->lexLanguageOperatorToken();
            }},
            {lexer_func("isGenericEndAhead"), [](Lexer* lexer) -> bool {
                return lexer->isGenericEndAhead();
            }},
            {lexer_func("lexAssignmentOperatorToken"), [](Lexer* lexer) -> bool {
                return lexer->lexAssignmentOperatorToken();
            }},
            {lexer_func("lexLambdaTypeTokens"), [](Lexer* lexer, unsigned int start) -> bool {
                return lexer->lexLambdaTypeTokens(start);
            }},
            {lexer_func("lexGenericTypeAfterId"), [](Lexer* lexer, unsigned int start) -> bool {
                return lexer->lexLambdaTypeTokens(start);
            }},
            {lexer_func("lexRefOrGenericType"), [](Lexer* lexer) -> bool {
                return lexer->lexRefOrGenericType();
            }},
            {lexer_func("lexArrayAndPointerTypesAfterTypeId"), [](Lexer* lexer, unsigned int start) -> void {
                return lexer->lexArrayAndPointerTypesAfterTypeId(start);
            }},
            {lexer_func("lexTypeTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexTypeTokens();
            }},
            {lexer_func("lexTopLevelAccessSpecifiedDecls"), [](Lexer* lexer) -> bool {
                return lexer->lexTopLevelAccessSpecifiedDecls();
            }},
            {lexer_func("lexTopLevelStatementTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexTopLevelStatementTokens();
            }},
            {lexer_func("lexNestedLevelStatementTokens"), [](Lexer* lexer, bool is_value, bool lex_value_node) -> bool {
                return lexer->lexNestedLevelStatementTokens(is_value, lex_value_node);
            }},
            {lexer_func("lexStatementTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexStatementTokens();
            }},
            {lexer_func("lexThrowStatementTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexThrowStatementTokens();
            }},
            {lexer_func("lexOperatorToken"), [](Lexer* lexer, char op) -> bool {
                return lexer->lexOperatorToken(op);
            }},
            {lexer_func("lexOperatorTokenStr"), [](Lexer* lexer, chem::string* str) -> bool {
                return lexer->lexOperatorToken({ str->data(), str->size() });
            }},
            {lexer_func("storeOperationToken"), [](Lexer* lexer, char token, Operation op) -> void {
                return lexer->storeOperationToken(token, op);
            }},
            {lexer_func("lexOperationToken"), [](Lexer* lexer, char token, Operation op) -> bool {
                return lexer->lexOperationToken(token, op);
            }},
            {lexer_func("lexOperatorTokenStr2"), [](Lexer* lexer, chem::string* str, Operation op) -> bool {
                return lexer->lexOperatorToken({ str->data(), str->size() }, op);
            }},
            {lexer_func("lexKeywordToken"), [](Lexer* lexer, chem::string* str) -> bool {
                return lexer->lexKeywordToken({ str->data(), str->size() });
            }},
            {lexer_func("lexWSKeywordToken"), [](Lexer* lexer, chem::string* str) -> bool {
                return lexer->lexWSKeywordToken({ str->data(), str->size() });
            }},
            {lexer_func("lexWSKeywordToken2"), [](Lexer* lexer, chem::string* str, char may_end_at) -> bool {
                return lexer->lexWSKeywordToken({ str->data(), str->size() }, may_end_at);
            }},
            {lexer_func("lexTopLevelMultipleStatementsTokens"), [](Lexer* lexer, bool break_at_no_stmt) -> void {
                return lexer->lexTopLevelMultipleStatementsTokens(break_at_no_stmt);
            }},
            {lexer_func("lexTopLevelMultipleImportStatements"), [](Lexer* lexer) -> void {
                return lexer->lexTopLevelMultipleImportStatements();
            }},
            {lexer_func("lexNestedLevelMultipleStatementsTokens"), [](Lexer* lexer, bool is_value, bool lex_value_node) -> void {
                return lexer->lexNestedLevelMultipleStatementsTokens(is_value, lex_value_node);
            }},
            {lexer_func("lexMultipleStatementsTokens"), [](Lexer* lexer) -> void {
                return lexer->lexNestedLevelMultipleStatementsTokens();
            }},
            {lexer_func("lexSingleLineCommentTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexSingleLineCommentTokens();
            }},
            {lexer_func("lexMultiLineCommentTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexMultiLineCommentTokens();
            }},
            {lexer_func("lexBraceBlock"), [](Lexer* lexer, void(*nested_lexer)(Lexer*)) -> bool {
                return lexer->lexBraceBlock("cbi", nested_lexer);
            }},
            {lexer_func("lexTopLevelBraceBlock"), [](Lexer* lexer) -> bool {
                return lexer->lexTopLevelBraceBlock("cbi");
            }},
            {lexer_func("lexBraceBlockStmts"), [](Lexer* lexer) -> bool {
                return lexer->lexBraceBlock("cbi");
            }},
            {lexer_func("lexBraceBlockOrSingleStmt"), [](Lexer* lexer, bool is_value, bool lex_value_node) -> bool {
                return lexer->lexBraceBlockOrSingleStmt("cbi", is_value, lex_value_node);
            }},
            {lexer_func("lexImportIdentifierList"), [](Lexer* lexer) -> bool {
                return lexer->lexImportIdentifierList();
            }},
            {lexer_func("lexImportStatement"), [](Lexer* lexer) -> bool {
                return lexer->lexImportStatement();
            }},
            {lexer_func("lexDestructStatement"), [](Lexer* lexer) -> bool {
                return lexer->lexDestructStatement();
            }},
            {lexer_func("lexReturnStatement"), [](Lexer* lexer) -> bool {
                return lexer->lexReturnStatement();
            }},
            {lexer_func("lexConstructorInitBlock"), [](Lexer* lexer) -> bool {
                return lexer->lexConstructorInitBlock();
            }},
            {lexer_func("lexUnsafeBlock"), [](Lexer* lexer) -> bool {
                return lexer->lexUnsafeBlock();
            }},
            {lexer_func("lexBreakStatement"), [](Lexer* lexer) -> bool {
                return lexer->lexBreakStatement();
            }},
            {lexer_func("lexTypealiasStatement"), [](Lexer* lexer, unsigned start) -> bool {
                return lexer->lexTypealiasStatement(start);
            }},
            {lexer_func("lexContinueStatement"), [](Lexer* lexer) -> bool {
                return lexer->lexContinueStatement();
            }},
            {lexer_func("lexIfExprAndBlock"), [](Lexer* lexer, bool is_value, bool lex_value_node, bool top_level) -> void {
                return lexer->lexIfExprAndBlock(is_value, lex_value_node, top_level);
            }},
            {lexer_func("lexIfBlockTokens"), [](Lexer* lexer, bool is_value, bool lex_value_node, bool top_level) -> bool {
                return lexer->lexIfBlockTokens(is_value, lex_value_node, top_level);
            }},
            {lexer_func("lexDoWhileBlockTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexDoWhileBlockTokens();
            }},
            {lexer_func("lexWhileBlockTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexWhileBlockTokens();
            }},
            {lexer_func("lexForBlockTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexForBlockTokens();
            }},
            {lexer_func("lexLoopBlockTokens"), [](Lexer* lexer, bool is_value) -> bool {
                return lexer->lexLoopBlockTokens(is_value);
            }},
            {lexer_func("lexParameterList"), [](Lexer* lexer, bool optionalTypes, bool defValues, bool lexSelfParam, bool variadicParam) -> void {
                return lexer->lexParameterList(optionalTypes, defValues, lexSelfParam, variadicParam);
            }},
            {lexer_func("lexFunctionSignatureTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexFunctionSignatureTokens();
            }},
            {lexer_func("lexGenericParametersList"), [](Lexer* lexer) -> bool {
                return lexer->lexGenericParametersList();
            }},
            {lexer_func("lexAfterFuncKeyword"), [](Lexer* lexer, bool allow_extensions) -> bool {
                return lexer->lexAfterFuncKeyword(allow_extensions);
            }},
            {lexer_func("lexFunctionStructureTokens"), [](Lexer* lexer, unsigned start, bool allow_declaration, bool allow_extensions) -> bool {
                return lexer->lexFunctionStructureTokens(start, allow_declaration, allow_extensions);
            }},
            {lexer_func("lexInterfaceBlockTokens"), [](Lexer* lexer) -> void {
                return lexer->lexInterfaceBlockTokens();
            }},
            {lexer_func("lexInterfaceStructureTokens"), [](Lexer* lexer, unsigned start) -> bool {
                return lexer->lexInterfaceStructureTokens(start);
            }},
            {lexer_func("lexNamespaceTokens"), [](Lexer* lexer, unsigned start) -> bool {
                return lexer->lexNamespaceTokens(start);
            }},
            {lexer_func("lexStructMemberTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexStructMemberTokens();
            }},
            {lexer_func("lexStructBlockTokens"), [](Lexer* lexer) -> void {
                return lexer->lexStructBlockTokens();
            }},
            {lexer_func("lexStructStructureTokens"), [](Lexer* lexer, unsigned start, bool unnamed, bool direct_init) -> bool {
                return lexer->lexStructStructureTokens(start, unnamed, direct_init);
            }},
            {lexer_func("lexVariantMemberTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexVariantMemberTokens();
            }},
            {lexer_func("lexVariantBlockTokens"), [](Lexer* lexer) -> void {
                return lexer->lexVariantBlockTokens();
            }},
            {lexer_func("lexVariantStructureTokens"), [](Lexer* lexer, unsigned start) -> bool {
                return lexer->lexVariantStructureTokens();
            }},
            {lexer_func("lexUnionBlockTokens"), [](Lexer* lexer) -> void {
                return lexer->lexUnionBlockTokens();
            }},
            {lexer_func("lexUnionStructureTokens"), [](Lexer* lexer, unsigned start, bool unnamed, bool direct_init) -> bool {
                return lexer->lexUnionStructureTokens(start, unnamed, direct_init);
            }},
            {lexer_func("lexImplBlockTokens"), [](Lexer* lexer) -> void {
                return lexer->lexImplBlockTokens();
            }},
            {lexer_func("lexImplTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexImplTokens();
            }},
            {lexer_func("lexEnumBlockTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexEnumBlockTokens();
            }},
            {lexer_func("lexEnumStructureTokens"), [](Lexer* lexer, unsigned start) -> bool {
                return lexer->lexEnumStructureTokens(start);
            }},
            {lexer_func("readWhitespace"), [](Lexer* lexer) -> bool {
                return lexer->readWhitespace();
            }},
            {lexer_func("lexWhitespaceToken"), [](Lexer* lexer) -> bool {
                return lexer->lexWhitespaceToken();
            }},
            {lexer_func("lexStringToken"), [](Lexer* lexer) -> bool {
                return lexer->lexStringToken();
            }},
            {lexer_func("lexCharToken"), [](Lexer* lexer) -> bool {
                return lexer->lexCharToken();
            }},
            {lexer_func("lexAnnotationMacro"), [](Lexer* lexer) -> bool {
                return lexer->lexAnnotationMacro();
            }},
            {lexer_func("lexNull"), [](Lexer* lexer) -> bool {
                return lexer->lexNull();
            }},
            {lexer_func("lexBoolToken"), [](Lexer* lexer) -> bool {
                return lexer->lexBoolToken();
            }},
            {lexer_func("lexUnsignedIntAsNumberToken"), [](Lexer* lexer) -> bool {
                return lexer->lexUnsignedIntAsNumberToken();
            }},
            {lexer_func("lexNumberToken"), [](Lexer* lexer) -> bool {
                return lexer->lexNumberToken();
            }},
            {lexer_func("lexStructValueTokens"), [](Lexer* lexer, unsigned back_start) -> bool {
                return lexer->lexStructValueTokens(back_start);
            }},
            {lexer_func("lexValueToken"), [](Lexer* lexer) -> bool {
                return lexer->lexValueToken();
            }},
            {lexer_func("lexSwitchCaseValue"), [](Lexer* lexer) -> bool {
                return lexer->lexSwitchCaseValue();
            }},
            {lexer_func("lexAccessChainValueToken"), [](Lexer* lexer) -> bool {
                return lexer->lexAccessChainValueToken();
            }},
            {lexer_func("lexArrayInit"), [](Lexer* lexer) -> bool {
                return lexer->lexArrayInit();
            }},
            {lexer_func("lexAccessChainOrValue"), [](Lexer* lexer, bool lexStruct) -> bool {
                return lexer->lexAccessChainOrValue(lexStruct);
            }},
            {lexer_func("lexValueNode"), [](Lexer* lexer) -> bool {
                return lexer->lexValueNode();
            }},
            {lexer_func("lexIdentifierList"), [](Lexer* lexer) -> void {
                return lexer->lexIdentifierList();
            }},
            {lexer_func("lexLambdaAfterParamsList"), [](Lexer* lexer, unsigned int start) -> void {
                return lexer->lexLambdaAfterParamsList(start);
            }},
            {lexer_func("lexLambdaValue"), [](Lexer* lexer) -> bool {
                return lexer->lexLambdaValue();
            }},
            {lexer_func("lexRemainingExpression"), [](Lexer* lexer, unsigned start) -> bool {
                return lexer->lexRemainingExpression(start);
            }},
            {lexer_func("lexLambdaOrExprAfterLParen"), [](Lexer* lexer) -> bool {
                return lexer->lexLambdaOrExprAfterLParen();
            }},
            {lexer_func("lexParenExpressionAfterLParen"), [](Lexer* lexer) -> void {
                return lexer->lexParenExpressionAfterLParen();
            }},
            {lexer_func("lexParenExpression"), [](Lexer* lexer) -> bool {
                return lexer->lexParenExpression();
            }},
            {lexer_func("lexExpressionTokens"), [](Lexer* lexer, bool lexStruct, bool lambda) -> bool {
                return lexer->lexExpressionTokens(lexStruct, lambda);
            }},
            {lexer_func("lexSwitchStatementBlock"), [](Lexer* lexer, bool is_value, bool lex_value_node) -> bool {
                return lexer->lexSwitchStatementBlock(is_value, lex_value_node);
            }},
            {lexer_func("lexTryCatchTokens"), [](Lexer* lexer) -> bool {
                return lexer->lexTryCatchTokens();
            }},
            {lexer_func("lexUsingStatement"), [](Lexer* lexer) -> bool {
                return lexer->lexUsingStatement();
            }},
    };
}