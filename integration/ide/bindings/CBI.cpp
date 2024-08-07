// Copyright (c) Qinetik 2024.

#include "SourceProviderCBI.h"
#include "LexerCBI.h"
#include "lexer/Lexer.h"
#include "BuildContextCBI.h"
#include "stream/SourceProvider.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/PathUtils.h"
#include "utils/ProcessUtils.h"
#include "compiler/InvokeUtils.h"

chem::string* init_chem_string(chem::string* str) {
    str->storage.constant.data = nullptr;
    str->storage.constant.length = 0;
    str->state = '0';
    return str;
}

void bind_source_provider_cbi(SourceProviderCBI* cbi, SourceProvider* provider) {
    cbi->instance = provider;
}

void bind_lexer_cbi(LexerCBI* cbi, SourceProviderCBI* provider_cbi, Lexer* lexer) {
    cbi->instance = lexer;
    cbi->provider = provider_cbi;
    bind_source_provider_cbi(provider_cbi, &lexer->provider);
}

void prep_lexer_cbi(LexerCBI* cbi, SourceProviderCBI* provider) {
    cbi->provider = provider;
    prep_source_provider_cbi(provider);
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
    cbi->lexIfExpression = [](LexerCBI* cbi){
        return cbi->instance->lexIfExpression();
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
        return cbi->instance->lexIfExprAndBlock();
    };
    cbi->lexIfBlockTokens = [](LexerCBI* cbi){
        return cbi->instance->lexIfBlockTokens();
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
    cbi->collectStructAsLexer = [](LexerCBI* cbi, unsigned int start, unsigned int end){
        return cbi->instance->collect_cbi_node(start, end);
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

void prep_source_provider_cbi(SourceProviderCBI* cbi) {
    cbi->currentPosition = [](struct SourceProviderCBI* cbi){
        return cbi->instance->currentPosition();
    };
    cbi->readCharacter = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readCharacter();
    };
    cbi->eof = [](struct SourceProviderCBI* cbi){
        return cbi->instance->eof();
    };
    cbi->peek = [](struct SourceProviderCBI* cbi){
        return cbi->instance->peek();
    };
    cbi->peek_at = [](struct SourceProviderCBI* cbi, int ahead){
        return cbi->instance->peek(ahead);
    };
    cbi->readUntil = [](struct chem::string* str, struct SourceProviderCBI* cbi, char stop){
        cbi->instance->readUntil(str, stop);
    };
    cbi->increment = [](struct SourceProviderCBI* cbi, char* text, bool peek){
        return cbi->instance->increment(text, peek);
    };
    cbi->increment_char = [](struct SourceProviderCBI* cbi, char c){
        return cbi->instance->increment(c);
    };
    cbi->getLineNumber = [](struct SourceProviderCBI* cbi){
        return cbi->instance->getLineNumber();
    };
    cbi->getLineCharNumber = [](struct SourceProviderCBI* cbi){
        return cbi->instance->getLineCharNumber();
    };
    cbi->readEscaping = [](struct SourceProviderCBI* cbi, chem::string* value, char stopAt){
        cbi->instance->readEscaping(value, stopAt);
    };
    cbi->readAnything = [](chem::string* str, struct SourceProviderCBI* cbi, char until){
        cbi->instance->readAnything(init_chem_string(str), until);
    };
    cbi->readAlpha = [](chem::string* str, struct SourceProviderCBI* cbi){
        cbi->instance->readAlpha(init_chem_string(str));
    };
    cbi->readUnsignedInt = [](chem::string* str, struct SourceProviderCBI* cbi){
        cbi->instance->readUnsignedInt(init_chem_string(str));
    };
    cbi->readNumber = [](struct chem::string* str, struct SourceProviderCBI* cbi){
        cbi->instance->readNumber(init_chem_string(str));
    };
    cbi->readAlphaNum = [](chem::string* str, struct SourceProviderCBI* cbi){
        cbi->instance->readAlphaNum(init_chem_string(str));
    };
    cbi->readIdentifier = [](chem::string* str, struct SourceProviderCBI* cbi){
        cbi->instance->readIdentifier(init_chem_string(str));
    };
    cbi->readAnnotationIdentifier = [](chem::string* str, struct SourceProviderCBI* cbi){
        cbi->instance->readAnnotationIdentifier(init_chem_string(str));
    };
    cbi->readWhitespaces = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readWhitespaces();
    };
    cbi->hasNewLine = [](struct SourceProviderCBI* cbi){
        return cbi->instance->hasNewLine();
    };
    cbi->readNewLineChars = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readNewLineChars();
    };
    cbi->readWhitespacesAndNewLines = [](struct SourceProviderCBI* cbi){
        return cbi->instance->readWhitespacesAndNewLines();
    };
}

void prep_build_context_cbi(BuildContextCBI* cbi) {
    cbi->files_module = [](BuildContextCBI* self, chem::string* name, chem::string** path, unsigned int path_len, LabModule** dependencies, unsigned int dep_len) -> LabModule* {
        return self->instance->files_module(name, path, path_len, dependencies, dep_len);
    };
    cbi->chemical_files_module = [](BuildContextCBI* self, chem::string* name, chem::string** path, unsigned int path_len, LabModule** dependencies, unsigned int dep_len) -> LabModule* {
        return self->instance->chemical_files_module(name, path, path_len, dependencies, dep_len);
    };
    cbi->c_file_module = [](BuildContextCBI* self, chem::string* name, chem::string* path, LabModule** dependencies, unsigned int dep_len) -> LabModule* {
        return self->instance->c_file_module(name, path, dependencies, dep_len);
    };
    cbi->object_module = [](BuildContextCBI* self, chem::string* name, chem::string* path) -> LabModule* {
        return self->instance->obj_file_module(name, path);
    };
    cbi->translate_to_chemical = [](BuildContextCBI* self, chem::string* c_path, chem::string* output_path) -> LabJob* {
        return self->instance->translate_to_chemical(c_path, output_path);
    };
    cbi->translate_mod_to_c = [](BuildContextCBI* self, LabModule* module, chem::string* output_path) -> LabJob* {
        return self->instance->translate_to_c(module, output_path);
    };
    cbi->build_exe = [](BuildContextCBI* self, chem::string* name, LabModule** dependencies, unsigned int dep_len) -> LabJob* {
        return self->instance->build_exe(name, dependencies, dep_len);
    };
    cbi->build_dynamic_lib = [](BuildContextCBI* self, chem::string* name, LabModule** dependencies, unsigned int dep_len) -> LabJob* {
        return self->instance->build_dynamic_lib(name, dependencies, dep_len);
    };
    cbi->add_object = [](BuildContextCBI* self, LabJob* job, chem::string* path) {
        job->linkables.emplace_back(path->copy());
    };
    cbi->build_path = [](chem::string* str, BuildContextCBI* self) {
        return self->instance->build_path(init_chem_string(str));
    };
    cbi->has_arg = [](BuildContextCBI* self, chem::string* name) -> bool {
        return self->instance->has_arg(name);
    };
    cbi->get_arg = [](chem::string* str, BuildContextCBI* self, chem::string* name) {
        return self->instance->get_arg(init_chem_string(str), name);
    };
    cbi->remove_arg = [](BuildContextCBI* self, chem::string* name) {
        return self->instance->remove_arg(name);
    };
    cbi->launch_executable = [](BuildContextCBI* self, chem::string* path, bool same_window) {
        auto copied = path->to_std_string();
        copied = absolute_path(copied);
        if(same_window) {
            copied = '\"' + copied + '\"';
        }
        return launch_executable(copied.data(), same_window);
    };
    cbi->on_finished = [](BuildContextCBI* self, void(*lambda)(void*), void* data) {
        self->instance->on_finished = lambda;
        self->instance->on_finished_data = data;
    };
}

void bind_build_context_cbi(BuildContextCBI* cbi, LabBuildContext* context) {
    cbi->instance = context;
}