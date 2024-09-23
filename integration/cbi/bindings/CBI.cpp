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

constexpr std::string lexer_func(const std::string& name) {
    return "Lexer" + name;
};

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

void lexer_symbol_map(std::unordered_map<std::string, void*>& sym_map) {
    // TODO this
    sym_map = {
            {lexer_func("storeVariable"), [](Lexer* lexer) -> void {
                // TODO this
            }},
            {lexer_func("storeIdentifier"), [](Lexer* lexer) -> void {
                // TODO this
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
            {sp_func("increment"), [](SourceProvider* provider, chem::string* text, bool peek) -> void {
                // TODO we will do this when increment supports string_view
            }},
            {sp_func("increment_char"), [](SourceProvider* provider, char c) -> bool {
                return provider->increment(c);
            }},
            {sp_func("getLineNumber"), [](SourceProvider* provider, char c) -> unsigned int {
                return provider->getLineNumber();
            }},
            {sp_func("getLineCharNumber"), [](SourceProvider* provider, char c) -> unsigned int {
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

void bind_lexer_cbi(LexerCBI* cbi, Lexer* lexer) {
    cbi->instance = lexer;
}

void prep_lexer_cbi(LexerCBI* cbi) {
    cbi->storeVariable = [](LexerCBI* cbi, char* variable){
        return cbi->instance->storeVariable(variable);
    };
    cbi->storeIdentifier = [](LexerCBI* cbi, char* id){
        return cbi->instance->storeIdentifier(id);
    };
    cbi->lexVariableToken = [](LexerCBI* cbi){
        return cbi->instance->lexVariableToken();
    };
    cbi->lexIdentifierToken = [](LexerCBI* cbi){
        return cbi->instance->lexIdentifierToken();
    };
    cbi->lexAccessChainAfterId = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainAfterId(lexStruct);
    };
    cbi->lexAccessChainRecursive = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainRecursive(lexStruct);
    };
    cbi->lexAccessChain = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChain(lexStruct);
    };
    cbi->lexAccessChainOrAddrOf = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainOrAddrOf(lexStruct);
    };
    cbi->lexVarInitializationTokens = [](LexerCBI* cbi, bool allowDecls, bool requiredType){
        return cbi->instance->lexVarInitializationTokens(allowDecls, requiredType);
    };
    cbi->lexAssignmentTokens = [](LexerCBI* cbi){
        return cbi->instance->lexAssignmentTokens();
    };
    cbi->lexLanguageOperatorToken = [](LexerCBI* cbi){
        return cbi->instance->lexLanguageOperatorToken();
    };
    cbi->lexAssignmentOperatorToken = [](LexerCBI* cbi){
        return cbi->instance->lexAssignmentOperatorToken();
    };
    cbi->lexLambdaTypeTokens = [](LexerCBI* cbi, unsigned int start){
        return cbi->instance->lexLambdaTypeTokens(start);
    };
    cbi->lexTypeTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTypeTokens();
    };
    cbi->lexTopLevelStatementTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTopLevelStatementTokens();
    };
    cbi->lexNestedLevelStatementTokens = [](LexerCBI* cbi){
        return cbi->instance->lexNestedLevelStatementTokens();
    };
    cbi->lexStatementTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStatementTokens();
    };
    cbi->lexOperatorToken = [](LexerCBI* cbi, char op){
        return cbi->instance->lexOperatorToken(op);
    };
    cbi->lexOperatorTokenStr = [](LexerCBI* cbi, char* str){
        return cbi->instance->lexOperatorToken(str);
    };
    cbi->lexOperationToken = [](LexerCBI* cbi, char token, Operation op){
        return cbi->instance->lexOperationToken(token, op);
    };
    cbi->lexStrOperationToken = [](LexerCBI* cbi, char* token, Operation op){
        return cbi->instance->lexOperatorToken(token, op);
    };
    cbi->lexKeywordToken = [](LexerCBI* cbi, char* keyword){
        return cbi->instance->lexKeywordToken(keyword);
    };
    cbi->lexTopLevelMultipleStatementsTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTopLevelMultipleStatementsTokens();
    };
    cbi->lexTopLevelMultipleImportStatements = [](LexerCBI* cbi){
        return cbi->instance->lexTopLevelMultipleImportStatements();
    };
    cbi->lexNestedLevelMultipleStatementsTokens = [](LexerCBI* cbi){
        return cbi->instance->lexNestedLevelMultipleStatementsTokens();
    };
    cbi->lexMultipleStatementsTokens = [](LexerCBI* cbi){
        return cbi->instance->lexMultipleStatementsTokens();
    };
    cbi->lexSingleLineCommentTokens = [](LexerCBI* cbi){
        return cbi->instance->lexSingleLineCommentTokens();
    };
    cbi->lexMultiLineCommentTokens = [](LexerCBI* cbi){
        return cbi->instance->lexMultiLineCommentTokens();
    };
    cbi->lexBraceBlock = [](LexerCBI* cbi, char* forThing){
        return cbi->instance->lexBraceBlock(forThing);
    };
    cbi->lexImportIdentifierList = [](LexerCBI* cbi){
        return cbi->instance->lexImportIdentifierList();
    };
    cbi->lexImportStatement = [](LexerCBI* cbi){
        return cbi->instance->lexImportStatement();
    };
    cbi->lexReturnStatement = [](LexerCBI* cbi){
        return cbi->instance->lexReturnStatement();
    };
    cbi->lexBreakStatement = [](LexerCBI* cbi){
        return cbi->instance->lexBreakStatement();
    };
    cbi->lexTypealiasStatement = [](LexerCBI* cbi){
        return cbi->instance->lexTypealiasStatement();
    };
    cbi->lexContinueStatement = [](LexerCBI* cbi){
        return cbi->instance->lexContinueStatement();
    };
    cbi->lexIfExprAndBlock = [](LexerCBI* cbi){
        return cbi->instance->lexIfExprAndBlock(false, false, false);
    };
    cbi->lexIfBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexIfBlockTokens(false, false, false);
    };
    cbi->lexDoWhileBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexDoWhileBlockTokens();
    };
    cbi->lexWhileBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexWhileBlockTokens();
    };
    cbi->lexForBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexForBlockTokens();
    };
    cbi->lexParameterList = [](LexerCBI* cbi, bool optionalTypes, bool defValues){
        return cbi->instance->lexParameterList(optionalTypes, defValues);
    };
    cbi->lexFunctionSignatureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexFunctionSignatureTokens();
    };
    cbi->lexAfterFuncKeyword = [](LexerCBI* cbi){
        return cbi->instance->lexAfterFuncKeyword();
    };
    cbi->lexFunctionStructureTokens = [](LexerCBI* cbi, bool allowDecls){
        return cbi->instance->lexFunctionStructureTokens(allowDecls);
    };
    cbi->lexInterfaceBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexInterfaceBlockTokens();
    };
    cbi->lexInterfaceStructureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexInterfaceStructureTokens();
    };
    cbi->lexStructMemberTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructMemberTokens();
    };
    cbi->lexStructBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructBlockTokens();
    };
    cbi->lexStructStructureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexStructStructureTokens();
    };
    cbi->lexImplBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexImplBlockTokens();
    };
    cbi->lexImplTokens = [](LexerCBI* cbi){
        return cbi->instance->lexImplTokens();
    };
    cbi->lexEnumBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexEnumBlockTokens();
    };
    cbi->lexEnumStructureTokens = [](LexerCBI* cbi){
        return cbi->instance->lexEnumStructureTokens();
    };
    cbi->lexWhitespaceToken = [](LexerCBI* cbi){
        return cbi->instance->lexWhitespaceToken();
    };
    cbi->lexWhitespaceAndNewLines = [](LexerCBI* cbi){
        return cbi->instance->lexWhitespaceAndNewLines();
    };
    cbi->lexStringToken = [](LexerCBI* cbi){
        return cbi->instance->lexStringToken();
    };
    cbi->lexCharToken = [](LexerCBI* cbi){
        return cbi->instance->lexCharToken();
    };
    cbi->lexAnnotationMacro = [](LexerCBI* cbi){
        return cbi->instance->lexAnnotationMacro();
    };
    cbi->lexNull = [](LexerCBI* cbi){
        return cbi->instance->lexNull();
    };
    cbi->lexBoolToken = [](LexerCBI* cbi){
        return cbi->instance->lexBoolToken();
    };
    cbi->lexUnsignedIntAsNumberToken = [](LexerCBI* cbi){
        return cbi->instance->lexUnsignedIntAsNumberToken();
    };
    cbi->lexNumberToken = [](LexerCBI* cbi){
        return cbi->instance->lexNumberToken();
    };
    cbi->lexStructValueTokens = [](LexerCBI* cbi){
        // TODO back_start not taken into account
        return cbi->instance->lexStructValueTokens(1);
    };
    cbi->lexValueToken = [](LexerCBI* cbi){
        return cbi->instance->lexValueToken();
    };
    cbi->lexAccessChainValueToken = [](LexerCBI* cbi){
        return cbi->instance->lexAccessChainValueToken();
    };
    cbi->lexArrayInit = [](LexerCBI* cbi){
        return cbi->instance->lexArrayInit();
    };
    cbi->lexAccessChainOrValue = [](LexerCBI* cbi, bool lexStruct){
        return cbi->instance->lexAccessChainOrValue(lexStruct);
    };
    cbi->lexIdentifierList = [](LexerCBI* cbi){
        return cbi->instance->lexIdentifierList();
    };
    cbi->lexLambdaAfterParamsList = [](LexerCBI* cbi, unsigned int start){
        return cbi->instance->lexLambdaAfterParamsList(start);
    };
    cbi->lexLambdaValue = [](LexerCBI* cbi){
        return cbi->instance->lexLambdaValue();
    };
    cbi->lexRemainingExpression = [](LexerCBI* cbi, unsigned int start){
        return cbi->instance->lexRemainingExpression(start);
    };
    cbi->lexLambdaAfterLParen = [](LexerCBI* cbi){
        return cbi->instance->lexLambdaOrExprAfterLParen();
    };
    cbi->lexParenExpressionAfterLParen = [](LexerCBI* cbi){
        return cbi->instance->lexParenExpressionAfterLParen();
    };
    cbi->lexParenExpression = [](LexerCBI* cbi){
        return cbi->instance->lexParenExpression();
    };
    cbi->lexExpressionTokens = [](LexerCBI* cbi, bool lexStruct, bool lambda){
        return cbi->instance->lexExpressionTokens(lexStruct, lambda);
    };
    cbi->lexSwitchStatementBlock = [](LexerCBI* cbi){
        return cbi->instance->lexSwitchStatementBlock();
    };
    cbi->lexTryCatchTokens = [](LexerCBI* cbi){
        return cbi->instance->lexTryCatchTokens();
    };
}