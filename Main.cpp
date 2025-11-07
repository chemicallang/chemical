
// Copyright (c) Chemical Language Foundation 2025.

#include <lsp/messages.h> // Generated message definitions
#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/io/stream.h>
#include <lsp/io/socket.h>
#include <iostream>
#include "server/WorkspaceManager.h"
#include "utils/FileUtils.h"
#include "utils/CmdUtils.h"
#include "utils/CmdUtils2.h"
#include "utils/Version.h"
#include "core/main/CompilerMain.h"

#include <thread>
#include <atomic>
#include <functional>
#include <utility>
#include "utils/JsonUtils.h"
#include "server/build/ChildProcessBuild.h"
#include "server/build/ContextSerialization.h"

#include <sstream>
#include <iostream>
#include <memory>
#include <thread>

#ifdef DEBUG
#define DEBUG_LOG_REQS
#endif

std::vector<std::string> getTokenTypes() {
    // the order here is insanely important
    // because these entries correspond to enum entries
    return {
        // these are standard token types, represented by SemanticTokenTypes enum (present in the lsp)
        "namespace","type","class","enum","interface","struct","typeparameter","parameter","variable","property","enummember","event","function","method","macro","keyword","modifier","comment","string","number","regexp","operator","decorator",
        // these are custom token types, represented by SemanticTokenScopes enum
        "MarkupItalic","MarkupItalicMarkdown","MarkupBold","MarkupBoldMarkdown","MarkupHeading","MarkupHeadingSetext","MarkupHeadingSetext1Markdown","MarkupHeadingSetext2Markdown","MarkupHeadingMarkdown","MarkupStrikethrough","MarkupInserted","MarkupInsertedDiff","MarkupInsertedGit_gutter","MarkupDeleted","MarkupDeletedDiff","MarkupDeletedGit_gutter","MarkupInlineRaw","MarkupInlineRawMarkdown","MarkupInlineRawStringMarkdown","MarkupChanged","MarkupChangedDiff","MarkupChangedGit_gutter","MarkupUnderline","MarkupUnderlineLinkMarkdown","MarkupUnderlineLinkImageMarkdown","MarkupList","MarkupListUnnumberedMarkdown","MarkupListNumberedMarkdown","MarkupListAsciidoc","MarkupQuote","MarkupQuoteMarkdown","MarkupOtherUrlAsciidoc","MarkupError","MarkupOutput","MarkupRaw","MarkupRawMonospaceAsciidoc","MarkupPrompt","MarkupTraceback","MarkupHeadingPunctuationDefinitionHeading","MarkupLinkAsciidoc","Entity","EntityNameFunction","EntityNameFunctionPreprocessor","EntityNameFunctionXi","EntityNameType","EntityNameTypeModule","EntityNameTypeNamespace","EntityNameTypeClass","EntityNameNamespace","EntityNameTag","EntityNameTagCss","EntityNameTagLess","EntityNameTagStyle","EntityNameTagScript","EntityNameScopeResolution","EntityNameScopeResolutionFunctionCall","EntityNameScopeResolutionFunctionDefinition","EntityNameClass","EntityNameClassIdentifierNamespaceType","EntityNameClassXi","EntityNameOperator","EntityNameOperatorCustomLiteral","EntityNameLabel","EntityNameLabelCs","EntityNameVariable","EntityNameVariableLocalCs","EntityNameVariableParameterCs","EntityNameVariableParameterPhp","EntityNameVariableFieldCs","EntityNameSection","EntityNameSectionMarkdown","EntityNameSelector","EntityNameFilenameFindInFiles","EntityNameException","EntityNameLifetimeRust","EntityNameGotoLabelPhp","EntityNamePackageGo","EntityOtherInheritedClass","EntityOtherAttributeName","EntityOtherAttributeNameClassCss","EntityOtherAttributeNameId","EntityOtherAttributeNameIdCss","EntityOtherAttributeNameParentSelectorCss","EntityOtherAttributeNameParentLess","EntityOtherAttributeNamePseudoElement","EntityOtherAttributeNamePseudoElementCss","EntityOtherAttributeNameScss","EntityOtherAttributeNameHtml","EntityOtherAttributeNamePseudoClass","EntityOtherAttribute","EntityOtherAliasPhp","EntityGlobalClojure","Support","SupportFunction","SupportFunctionGitRebase","SupportFunctionAnyMethod","SupportFunctionConstruct","SupportFunctionStdRust","SupportFunctionConsole","SupportClass","SupportClassMathBlockEnvironmentLatex","SupportType","SupportTypePropertyName","SupportTypePropertyNameJson","SupportTypePropertyNameCss","SupportTypePropertyNameToml","SupportTypePropertyNameTableToml","SupportTypePropertyNameArrayToml","SupportTypePropertyNameJsonPunctuation","SupportTypeVendoredPropertyName","SupportTypeVendoredPropertyNameCss","SupportTypeObjectHlsl","SupportTypeObjectModule","SupportTypeObjectConsole","SupportTypeObjectDom","SupportTypeObjectRwHlsl","SupportTypeException","SupportTypePosixReservedC","SupportTypePosixReservedCpp","SupportTypePython","SupportTypePrimitive","SupportTypePrimitiveTs","SupportTypePrimitiveTsx","SupportTypeBuiltinTs","SupportTypeBuiltinTsx","SupportTypeTypeFlowtype","SupportTypePreludeElm","SupportTypeTextureHlsl","SupportTypeSamplerHlsl","SupportTypeFxHlsl","SupportTypeSwift","SupportTypeVbAsp","SupportConstant","SupportConstantPropertyValue","SupportConstantPropertyValueScss","SupportConstantPropertyValueCss","SupportConstantFontName","SupportConstantColor","SupportConstantColorW3cStandardColorNameCss","SupportConstantColorW3cStandardColorNameScss","SupportConstantMediaType","SupportConstantMedia","SupportConstantHandlebars","SupportConstantMath","SupportConstantJson","SupportConstantDom","SupportConstantEdge","SupportConstantPropertyMath","SupportConstantCoreRust","SupportConstantCorePhp","SupportConstantExtPhp","SupportConstantStdPhp","SupportConstantParserTokenPhp","SupportConstantElm","SupportVariable","SupportVariableMagicPython","SupportVariableProperty","SupportVariablePropertyProcess","SupportVariablePropertyDom","SupportVariableObjectProcess","SupportVariableObjectNode","SupportVariableDom","SupportVariableSemanticHlsl","SupportOtherParenthesisRegexp","SupportOtherVariable","SupportOtherNamespaceUsePhp","SupportOtherNamespaceUseAsPhp","SupportOtherNamespacePhp","SupportOtherPhp","SupportModuleNode","SupportTokenDecoratorPython","Keyword","KeywordControl","KeywordControlAnchorRegexp","KeywordControlXi","KeywordOperator","KeywordOperatorNew","KeywordOperatorNewCpp","KeywordOperatorLogical","KeywordOperatorLogicalPython","KeywordOperatorLogicalPhp","KeywordOperatorQuantifierRegexp","KeywordOperatorExpression","KeywordOperatorExpressionInstanceof","KeywordOperatorExpressionImport","KeywordOperatorExpressionKeyof","KeywordOperatorExpressionDelete","KeywordOperatorExpressionIn","KeywordOperatorExpressionOf","KeywordOperatorExpressionTypeof","KeywordOperatorExpressionVoid","KeywordOperatorExpressionIs","KeywordOperatorCast","KeywordOperatorSizeof","KeywordOperatorSizeofC","KeywordOperatorSizeofCpp","KeywordOperatorNegationRegexp","KeywordOperatorOrRegexp","KeywordOperatorPlusExponent","KeywordOperatorMinusExponent","KeywordOperatorNoexcept","KeywordOperatorAlignof","KeywordOperatorTypeid","KeywordOperatorAlignas","KeywordOperatorInstanceof","KeywordOperatorInstanceofJava","KeywordOperatorWordlike","KeywordOperatorDelete","KeywordOperatorDeleteCpp","KeywordOperatorLess","KeywordOperatorClass","KeywordOperatorWord","KeywordOperatorBitwise","KeywordOperatorBitwiseShiftC","KeywordOperatorBitwiseShiftCpp","KeywordOperatorBitwisePhp","KeywordOperatorChannel","KeywordOperatorCss","KeywordOperatorScss","KeywordOperatorTernary","KeywordOperatorOptional","KeywordOperatorMiscRust","KeywordOperatorSigilRust","KeywordOperatorArithmetic","KeywordOperatorArithmeticPhp","KeywordOperatorArithmeticGo","KeywordOperatorArithmeticC","KeywordOperatorArithmeticCpp","KeywordOperatorComparison","KeywordOperatorComparisonC","KeywordOperatorComparisonCpp","KeywordOperatorComparisonPhp","KeywordOperatorDecrement","KeywordOperatorDecrementC","KeywordOperatorDecrementCpp","KeywordOperatorIncrement","KeywordOperatorIncrementC","KeywordOperatorIncrementCpp","KeywordOperatorRelational","KeywordOperatorAssignment","KeywordOperatorAssignmentC","KeywordOperatorAssignmentCpp","KeywordOperatorAssignmentCompound","KeywordOperatorAssignmentCompoundJs","KeywordOperatorAssignmentCompoundTs","KeywordOperatorAssignmentGo","KeywordOperatorC","KeywordOperatorCpp","KeywordOperatorErrorControlPhp","KeywordOperatorTypePhp","KeywordOperatorRegexpPhp","KeywordOperatorHeredocPhp","KeywordOperatorNowdocPhp","KeywordOperatorModule","KeywordOperatorAddressGo","KeywordOtherUnit","KeywordOtherUsing","KeywordOtherDirectiveUsing","KeywordOtherOperator","KeywordOtherSpecialMethod","KeywordOtherSpecialMethodRuby","KeywordOtherDMLSql","KeywordOtherImportant","KeywordOtherNew","KeywordOtherTypePhp","KeywordOtherArrayPhpdocPhp","KeywordOtherTemplateBegin","KeywordOtherTemplateEnd","KeywordOtherSubstitutionBegin","KeywordOtherSubstitutionEnd","MetaEmbedded","MetaEmbeddedAssembly","MetaDiff","MetaDiffHeader","MetaDiffHeaderFromFile","MetaDiffHeaderToFile","MetaDiffRange","MetaDiffIndex","MetaTemplateExpression","MetaObjectLiteralKey","MetaPreprocessor","MetaPreprocessorString","MetaPreprocessorNumeric","MetaPreprocessorAtRuleKeywordControlAtRule","MetaTypeCastExpr","MetaTypeNewExpr","MetaDefinitionVariableName","MetaDefinitionVariableNameJava","MetaDefinitionVariableNameGroovy","MetaDefinitionClassInheritedClassesGroovy","MetaStructureDictionaryKeyPython","MetaStructureDictionaryJsonStringQuotedDoubleJson","MetaTag","MetaTagSgmlDoctype","MetaTagSgmlDoctypeString","MetaTagSgmlDoctypeEntityNameTag","MetaTagSgmlDeclarationDoctype","MetaTagInlineSource","MetaTagOther","MetaTagBlockScript","MetaTagSgmlPunctuationDefinitionTagHtml","MetaSelector","MetaSelectorCssEntityNameTag","MetaSelectorCssEntityOtherAttributeNameId","MetaSelectorCssEntityOtherAttributeNameClass","MetaSeparator","MetaRequire","MetaLink","MetaFunctionCall","MetaFunctionCallObject","MetaFunctionCallObjectPhp","MetaFunctionCallGenericPython","MetaFunctionCallPhp","MetaFunctionCallStaticPhp","MetaResultLinePrefixContextLinePrefixSearch","MetaDoctype","MetaTagPunctuationDefinitionString","MetaTagStringSourcePunctuation","MetaBraceErbHtml","MetaBraceSquare","MetaTocListId","MetaSelectorEntity","MetaSelectorEntityOtherAttributeNameId","MetaPropertyGroupSupportConstantPropertyValue","MetaPropertyGroupSupportConstantPropertyValueCss","MetaPropertyValueSupportConstantPropertyValue","MetaPropertyValueSupportConstantPropertyValueCss","MetaPropertyValueSupportConstantNamedColorCss","MetaTagEntity","MetaTagEntityOtherAttributeName","MetaSelectorEntityPunctuation","MetaPropertyName","MetaPropertyValue","MetaPropertyValueConstant","MetaPropertyValueConstantOther","MetaJsxChildren","MetaJsxChildrenJs","MetaConstructorArgumentCss","MetaBlockLevel","MetaFunctionC","MetaFunctionCpp","MetaFunctionDecoratorPython","MetaFunctionDecoratorIdentifierPython","MetaMethodIdentifierJava","MetaMethodBodyJava","MetaMethodJava","MetaMethodGroovy","MetaMethodCallJava","MetaInterfacePhp","MetaOtherTypePhpdocPhp","MetaPropertyObject","MetaSymbolClojure","MetaArgumentsCoffee","MetaScopePrerequisitesMakefile","Comment","CommentBlockPreprocessor","CommentBlockDocumentation","CommentBlockDocumentationPunctuationDefinitionComment","CommentDocumentation","Storage","StorageType","StorageTypeCs","StorageTypeAnnotationJava","StorageTypeAnnotationGroovy","StorageTypeGenericJava","StorageTypeGenericCs","StorageTypeGenericGroovy","StorageTypeJava","StorageTypeObjectArrayJava","StorageTypeObjectArrayGroovy","StorageTypeModifierCs","StorageTypeVariableCs","StorageTypePrimitiveArrayJava","StorageTypePrimitiveArrayGroovy","StorageTypePrimitiveJava","StorageTypePrimitiveGroovy","StorageTypeTokenJava","StorageTypeGroovy","StorageTypeParametersGroovy","StorageTypeNumericGo","StorageTypeByteGo","StorageTypeBooleanGo","StorageTypeStringGo","StorageTypeUintptrGo","StorageTypeErrorGo","StorageTypeRuneGo","StorageTypeHaskell","StorageTypePhp","StorageModifier","StorageModifierImportJava","StorageModifierImportGroovy","StorageModifierPackageJava","StorageModifierLifetimeRust","String","StringRegexp","StringQuotedDoubleHtml","StringQuotedDoubleXml","StringQuotedDoubleHandlebars","StringQuotedDoubleHtmlSource","StringQuotedPug","StringQuotedSingleYaml","StringQuotedSingleXml","StringQuotedSingleHtml","StringQuotedSingleHandlebars","StringCommentBufferedBlockPug","StringInterpolatedPug","StringUnquotedPlainInYaml","StringUnquotedPlainOutYaml","StringUnquotedBlockYaml","StringUnquotedCdataXml","StringUnquotedHtml","StringUnquotedAsciidoc","StringOtherLink","StringOtherLinkTitleMarkdown","StringOtherLinkDescriptionMarkdown","StringTag","StringValue","Punctuation","PunctuationSeparatorNamespaceRuby","PunctuationSeparatorVariable","PunctuationSeparatorContinuation","PunctuationSeparatorPipeUnison","PunctuationSeparatorDelimiter","PunctuationSeparatorDelimiterUnison","PunctuationSeparatorDelimiterPhp","PunctuationSeparatorPeriodPython","PunctuationSeparatorPeriodJava","PunctuationSeparatorElementPython","PunctuationSeparatorKeyValue","PunctuationSeparatorListCommaCss","PunctuationSeparatorC","PunctuationSeparatorCpp","PunctuationSeparatorArgumentsPython","PunctuationSeparatorColonPhp","PunctuationSectionEmbedded","PunctuationSectionEmbeddedBegin","PunctuationSectionEmbeddedBeginPhp","PunctuationSectionEmbeddedBeginRuby","PunctuationSectionEmbeddedEnd","PunctuationSectionEmbeddedEndPhp","PunctuationSectionEmbeddedEndRuby","PunctuationSectionEmbeddedCoffee","PunctuationSectionEmbeddedSourceStringSourcePunctuationSectionEmbedded","PunctuationSectionBlockBeginBracketCurlyCpp","PunctuationSectionBlockBeginBracketCurlyC","PunctuationSectionBlockBeginJava","PunctuationSectionBlockEndBracketCurlyCpp","PunctuationSectionBlockEndBracketCurlyC","PunctuationSectionBlockEndJava","PunctuationSectionParensBeginBracketRoundC","PunctuationSectionParensEndBracketRoundC","PunctuationSectionParametersBeginBracketRoundC","PunctuationSectionParametersEndBracketRoundC","PunctuationSectionMethodBeginJava","PunctuationSectionMethodBeginBracketCurlyJava","PunctuationSectionMethodEndJava","PunctuationSectionMethodEndBracketCurlyJava","PunctuationSectionClassBeginJava","PunctuationSectionClassBeginBracketCurlyJava","PunctuationSectionClassEndJava","PunctuationSectionClassEndBracketCurlyJava","PunctuationSectionInnerClassBeginJava","PunctuationSectionInnerClassEndJava","PunctuationSectionArrayBeginPhp","PunctuationSectionArrayEndPhp","PunctuationSectionScopeBeginPhp","PunctuationSectionScopeEndPhp","PunctuationDefinitionListBeginMarkdown","PunctuationDefinitionListBeginUnison","PunctuationDefinitionListBeginPython","PunctuationDefinitionListEndUnison","PunctuationDefinitionListEndPython","PunctuationDefinitionListMarkdown","PunctuationDefinitionTemplateExpression","PunctuationDefinitionTemplateExpressionBegin","PunctuationDefinitionTemplateExpressionEnd","PunctuationDefinitionTag","PunctuationDefinitionTagHtml","PunctuationDefinitionTagBegin","PunctuationDefinitionTagBeginHtml","PunctuationDefinitionTagBeginJs","PunctuationDefinitionTagEnd","PunctuationDefinitionTagEndHtml","PunctuationDefinitionTagEndJs","PunctuationDefinitionTagJs","PunctuationDefinitionTagXi","PunctuationDefinitionGroupRegexp","PunctuationDefinitionGroupAssertionRegexp","PunctuationDefinitionCharacterClassRegexp","PunctuationDefinitionQuoteBeginMarkdown","PunctuationDefinitionComment","PunctuationDefinitionString","PunctuationDefinitionStringBeginHtml","PunctuationDefinitionStringBeginPhp","PunctuationDefinitionStringBeginMarkdown","PunctuationDefinitionStringEndHtml","PunctuationDefinitionStringEndHtmlSource","PunctuationDefinitionStringEndPhp","PunctuationDefinitionStringEndMarkdown","PunctuationDefinitionVariable","PunctuationDefinitionEntity","PunctuationDefinitionHeading","PunctuationDefinitionHeadingMarkdown","PunctuationDefinitionBold","PunctuationDefinitionBoldMarkdown","PunctuationDefinitionItalic","PunctuationDefinitionFromFileDiff","PunctuationDefinitionToFileDiff","PunctuationDefinitionMetadataMarkdown","PunctuationDefinitionSectionSwitchBlockEndBracketCurlyPhp","PunctuationDefinitionSectionSwitchBlockStartBracketCurlyPhp","PunctuationDefinitionSectionSwitchBlockBeginBracketCurlyPhp","PunctuationDefinitionParameters","PunctuationDefinitionParametersBeginBracketRoundPhp","PunctuationDefinitionParametersEndBracketRoundPhp","PunctuationDefinitionArray","PunctuationDefinitionArrayBeginBracketRoundPhp","PunctuationDefinitionArrayEndBracketRoundPhp","PunctuationDefinitionDelayedUnison","PunctuationDefinitionAbilityBeginUnison","PunctuationDefinitionAbilityEndUnison","PunctuationDefinitionHashUnison","PunctuationDefinitionMethodParametersBeginJava","PunctuationDefinitionMethodParametersEndJava","PunctuationDefinitionAnnotationJava","PunctuationDefinitionArgumentsBeginPython","PunctuationDefinitionArgumentsBeginBracketRoundPhp","PunctuationDefinitionArgumentsEndPython","PunctuationDefinitionArgumentsEndBracketRoundPhp","PunctuationDefinitionConstant","PunctuationDefinitionRawMarkdown","PunctuationDefinitionAsciidoc","PunctuationDefinitionStorageTypeBeginBracketRoundPhp","PunctuationDefinitionStorageTypeEndBracketRoundPhp","PunctuationDefinitionBeginBracketRoundPhp","PunctuationDefinitionBeginBracketCurlyPhp","PunctuationDefinitionEndBracketRoundPhp","PunctuationDefinitionEndBracketCurlyPhp","PunctuationDefinitionBlockSequenceItemYaml","PunctuationCharacterSetBeginRegexp","PunctuationCharacterSetEndRegexp","PunctuationOperatorAssignmentAsUnison","PunctuationParenthesisBeginPython","PunctuationParenthesisEndPython","PunctuationTerminatorStatementC","PunctuationTerminatorJava","PunctuationTerminatorExpressionPhp","PunctuationBracketAngleJava","PunctuationQuasiElement","SourceGroovyEmbedded","SourcePowershellVariableOtherMember","SourceCppKeywordOperatorNew","SourceCppKeywordOperatorDelete","SourceCssEntityOtherAttributeNameClass","SourceCssEntityOtherAttributeNamePseudoClass","SourceCssLessEntityOtherAttributeNameId","SourceCssEmbeddedPunctuationDefinitionTagHtml","SourceCssVariable","SourceCoffeeEmbedded","SourceJsEmbeddedPunctuationDefinitionTagHtml","SourceCssSupportTypePropertyName","SourcePhpEmbeddedLine","SourcePhpEmbeddedLineHtml","SourceJava","SourceJsonMetaStructureDictionaryJsonContainingStringQuotedJson","SourceJsonMetaStructureDictionaryJsonContainingStringQuotedJsonContainingPunctuationString","SourceJsonMetaStructureDictionaryJsonContainingValueJsonContainingStringQuotedJson","SourceJsonMetaStructureDictionaryJsonContainingValueJsonContainingStringQuotedJsonContainingPunctuation","SourceJsonMetaStructureDictionaryJsonContainingConstantLanguageJson","SourceJsonMetaStructureArrayJsonContainingValueJsonContainingStringQuotedJson","SourceJsonMetaStructureArrayJsonContainingValueJsonContainingStringQuotedJsonContainingPunctuation","SourceJsonMetaStructureArrayJsonContainingConstantLanguageJson","SourceIni","SourceMakefile","Variable","VariableLegacyBuiltinPython","VariableLanguage","VariableLanguageWildcardJava","VariableLanguageThis","VariableLanguageJs","VariableLanguageRuby","VariableLanguageRust","VariableOther","VariableOtherEnummember","VariableOtherConstant","VariableOtherConstantProperty","VariableOtherPhp","VariableOtherNormal","VariableOtherProperty","VariableOtherGenericTypeHaskell","VariableOtherReadwrite","VariableOtherReadwriteC","VariableOtherClassJs","VariableOtherClassTs","VariableOtherClassPhp","VariableOtherObject","VariableParameter","VariableParameterFunction","VariableParameterFunctionLanguageSpecialSelfPython","VariableParameterFunctionLanguageSpecialClsPython","VariableParameterFunctionLanguagePython","VariableParameterFunctionPython","VariableParameterFunctionJs","VariableParameterFunctionCoffee","VariableParameterFunctionLatex","VariableInterpolation","VariableJs","VariableC","VariableFunction","Constant","ConstantNumeric","ConstantNumericLineNumberFindInFilesMatch","ConstantNumericDecimalAsmX86_64","ConstantCharacter","ConstantCharacterEscape","ConstantCharacterCharacterClassRegexp","ConstantCharacterCharacterClassRegexpXi","ConstantCharacterSetRegexp","ConstantCharacterEntity","ConstantCharacterFormatPlaceholderOtherPython","ConstantCharacterXi","ConstantCharacterMathTex","ConstantLanguage","ConstantLanguageSymbolRuby","ConstantLanguageSymbolHashkeyRuby","ConstantLanguageSymbolElixir","ConstantLanguageSymbolDoubleQuotedElixir","ConstantOther","ConstantOtherColor","ConstantOtherColorRgbValue","ConstantOtherColorRgbValueXi","ConstantOtherRgbValue","ConstantOtherCharacterClassRegexp","ConstantOtherCharacterClassSetRegexp","ConstantOtherPlaceholder","ConstantOtherSymbol","ConstantOtherSymbolRuby","ConstantOtherOption","ConstantOtherPhp","ConstantOtherGeneralMathTex","ConstantRegexp","ConstantRegexpXi","ConstantShaGitRebase","ConstantKeywordClojure","Invalid","InvalidDeprecated","InvalidDeprecatedEntityOtherAttributeNameHtml","InvalidIllegal","InvalidIllegalBadAmpersandHtml","InvalidIllegalUnrecognizedTagHtml","InvalidIllegalNonNullTypehintedPhp","InvalidBroken","InvalidUnimplemented","InvalidXi","StringMetaImageInlineMarkdown","TokenInfoToken","TokenWarnToken","TokenErrorToken","TokenDebugToken","TokenVariableParameterJava","TokenPackage","TokenPackageKeyword","TokenStorage","TokenStorageTypeJava","Emphasis","Strong","None","DeclarationTag","DeclarationSgmlHtmlDeclarationDoctype","DeclarationSgmlHtmlDeclarationDoctypeEntity","DeclarationSgmlHtmlDeclarationDoctypeString","DeclarationXmlProcessing","DeclarationXmlProcessingEntity","DeclarationXmlProcessingString","DeclarationTagEntity","Header","StringSource","TextHtmlPhpSource","TextHtmlLaravelBladeSourcePhpEmbeddedLineHtmlEntityNameTagLaravelBlade","TextHtmlLaravelBladeSourcePhpEmbeddedLineHtmlSupportConstantLaravelBlade","TextVariable","TextBracketed","TextSourceTextMetaTagStringPunctuation","StringConstant","StringVariable","CommentMarkupLink","ImportStorageJava","ControlElements","TodoBold","TodoEmphasis","EmphasisMd","BeginningPunctuationDefinitionListMarkdown","BeginningPunctuationDefinitionListMarkdownXi","BeginningPunctuationDefinitionQuoteMarkdownXi","FunctionParameter","FunctionParameterRuby","FunctionParameterCs","FunctionBrace","RgbValue","InlineColorDecorationRgbValue","LessRgbValue","SelectorSass","BlockScopeEnd","BlockScopeBegin","AccentXi","WikiwordXi","LogInfo","LogWarning","LogError","TextHtmlDerivative"
    };
}

// 2) Per-client session: runs the LSP loop until error or disconnect
void run_session(
        std::atomic_bool& g_shutdown,
        lsp::io::SocketListener& listener,
        lsp::io::Socket socket,
        const char* executable_path
) {
  try {

    bool local_shutdown = false;
//    SocketStream stream();
    lsp::Connection connection(socket);
    lsp::MessageHandler handler(connection);
    WorkspaceManager manager(executable_path, handler);

    handler.add<lsp::requests::Initialize>([&manager](lsp::InitializeParams&& params){

#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::Initialize" << std::endl;
#endif

        // initializing the manager
        manager.initialize(params);

        std::ostringstream versionStr;
        versionStr << 'v' << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH;

        std::variant<lsp::TextDocumentSyncOptions, lsp::TextDocumentSyncKindEnum> textDocSync;
        textDocSync.emplace<lsp::TextDocumentSyncOptions>(
            true, lsp::TextDocumentSyncKind::Incremental, false, false, true
        );

        std::variant<lsp::SemanticTokensOptions, lsp::SemanticTokensRegistrationOptions> tokensProvider;
         tokensProvider.emplace<lsp::SemanticTokensOptions>(
             lsp::SemanticTokensOptions{
                 .workDoneProgress = false,
                 .legend = {
                     getTokenTypes(),
                    { "declaration","definition","readonly","static","deprecated","abstract","async","modification","documentation","defaultlibrary", }
                 },
                 .range = false,
                 .full = true
             }
         );

        std::variant<bool, lsp::FoldingRangeOptions, lsp::FoldingRangeRegistrationOptions> foldingOptions;
        foldingOptions.emplace<lsp::FoldingRangeOptions>(lsp::FoldingRangeOptions{
                .workDoneProgress = std::nullopt
        });

        std::variant<bool, lsp::DocumentSymbolOptions> symbolOptions;
        symbolOptions.emplace<lsp::DocumentSymbolOptions>(lsp::DocumentSymbolOptions{
                .workDoneProgress = std::nullopt,
                .label = std::nullopt
        });

        lsp::CompletionOptions completionOptions;
        std::vector<std::string> completionTriggers = { ".", "::" };
        completionOptions.triggerCharacters = std::move(completionTriggers);

        lsp::SignatureHelpOptions signatureOptions;
        std::vector<std::string> signatureTriggers = { "," };
        signatureOptions.triggerCharacters = std::move(signatureTriggers);

        return lsp::requests::Initialize::Result{
                .capabilities = {
                        .textDocumentSync = textDocSync,
                        .completionProvider = completionOptions,
                        .hoverProvider = true,
                        .signatureHelpProvider = signatureOptions,
                        .definitionProvider = true,
                        .documentSymbolProvider = symbolOptions,
                        .foldingRangeProvider = foldingOptions,
                        .semanticTokensProvider = tokensProvider,
                        .inlayHintProvider = true,
                },
                .serverInfo = lsp::InitializeResultServerInfo{
                        .name    = "Chemical Language Server",
                        .version = versionStr.str()
                }
        };

    });

    handler.add<lsp::requests::TextDocument_SemanticTokens_Full>([&manager](lsp::requests::TextDocument_SemanticTokens_Full::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_SemanticTokens_Full '" << path << '\'' << std::endl;
#endif
        try {
            auto tokens = manager.get_semantic_tokens_full(path);
            return lsp::requests::TextDocument_SemanticTokens_Full::Result(lsp::SemanticTokens{
                    .data = tokens
            });
        } catch(const std::exception& e) {
            throw std::runtime_error("UNCAUGHT_EXCEPTION");
        }
    });

    handler.add<lsp::requests::TextDocument_FoldingRange>([&manager](lsp::requests::TextDocument_FoldingRange::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_FoldingRange '" << path << '\'' << std::endl;
#endif
        return lsp::Nullable(manager.get_folding_range(path));
    });

    handler.add<lsp::requests::TextDocument_DocumentSymbol>([&manager](lsp::requests::TextDocument_DocumentSymbol::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_DocumentSymbol '" << path << '\'' << std::endl;
#endif
        return lsp::NullableVariant<std::vector<lsp::SymbolInformation>, std::vector<lsp::DocumentSymbol>>(manager.get_symbols(path));
    });

    handler.add<lsp::requests::TextDocument_Hover>([&manager](lsp::requests::TextDocument_Hover::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_Hover '" << path << '\'' << std::endl;
#endif
        auto hoverStr = manager.get_hover(path, Position { params.position.line, params.position.character });
        return lsp::NullOr<lsp::Hover>(lsp::Hover{ lsp::MarkupContent{lsp::MarkupKind::Markdown, std::move(hoverStr)} });
    });

    handler.add<lsp::requests::TextDocument_Definition>([&manager](lsp::requests::TextDocument_Definition::Params&& params) -> lsp::TextDocument_DefinitionResult {
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_Definition '" << path << '\'' << std::endl;
#endif
        auto& pos = params.position;
        return lsp::TextDocument_DefinitionResult(manager.get_definition(path, Position { pos.line, pos.character }));
    });

    handler.add<lsp::requests::TextDocument_Completion>([&manager](lsp::requests::TextDocument_Completion::Params&& params) -> lsp::TextDocument_CompletionResult {
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_Completion '" << path << '\'' << std::endl;
#endif
        auto& pos = params.position;
        return lsp::TextDocument_CompletionResult(manager.get_completion(path, Position { pos.line, pos.character }));
    });

    handler.add<lsp::requests::TextDocument_SignatureHelp>([&manager](lsp::requests::TextDocument_SignatureHelp::Params&& params) -> lsp::TextDocument_SignatureHelpResult {
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_SignatureHelp '" << path << '\'' << std::endl;
#endif
        auto& pos = params.position;
        return lsp::TextDocument_SignatureHelpResult(manager.get_signature_help(path, Position { pos.line, pos.character }));
    });

    handler.add<lsp::requests::TextDocument_InlayHint>([&manager](lsp::requests::TextDocument_InlayHint::Params&& params) -> lsp::TextDocument_InlayHintResult {
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::TextDocument_InlayHint '" << path << '\'' << std::endl;
#endif
        auto& start = params.range.start;
        auto& end = params.range.end;
        auto range = Range { Position { start.line, start.character }, Position { end.line, end.character } };
        return lsp::TextDocument_InlayHintResult(manager.get_hints(path, range));
    });

    handler.add<lsp::notifications::TextDocument_DidOpen>([&manager](lsp::notifications::TextDocument_DidOpen::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::notifications::TextDocument_DidOpen '" << path << '\'' << std::endl;
#endif
        manager.OnOpenedFile(path);
    });

    handler.add<lsp::notifications::TextDocument_DidChange>([&manager](lsp::notifications::TextDocument_DidChange::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::notifications::TextDocument_DidChange '" << path << '\'' << std::endl;
#endif
        manager.onChangedContents(path, params.contentChanges);
    });

    handler.add<lsp::notifications::TextDocument_DidSave>([&manager](lsp::notifications::TextDocument_DidSave::Params&& params){
        auto path = params.textDocument.uri.path();
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::notifications::TextDocument_DidSave '" << path << '\'' << std::endl;
#endif
        manager.onSave(path);
    });

    handler.add<lsp::notifications::Workspace_DidChangeWatchedFiles>([&manager](lsp::notifications::Workspace_DidChangeWatchedFiles::Params&& params){
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::notifications::Workspace_DidChangeWatchedFiles" << std::endl;
#endif
        for(auto& change : params.changes) {
            switch(change.type.index()) {
                case lsp::FileChangeType::Created:
                    manager.index_new_file(change.uri.path());
                    break;
                case lsp::FileChangeType::Deleted:
                    manager.de_index_deleted_file(change.uri.path());
                    break;
                case lsp::FileChangeType::Changed:
                default:
                    return;
            }
        }
    });

    handler.add<lsp::requests::Shutdown>([&listener, &local_shutdown, &g_shutdown]() -> std::nullptr_t {
#ifdef DEBUG_LOG_REQS
        std::cout << "[lsp] lsp::requests::Shutdown" << std::endl;
#endif
        // 1) mark both local and global shutdown
        local_shutdown = true;
        g_shutdown.store(true, std::memory_order_relaxed);
        return nullptr;
    });

    // ====================================

    // Main loop: will block inside processIncomingMessages()
    while (!local_shutdown) {
        handler.processIncomingMessages();
    }

    // Now the Shutdown response has gone out, we can close.
    std::cout << "[LSP] Shutting down socket." << std::endl;
    socket.close();

    // If this was *the* session that asked us to shut down,
    // we should also close the acceptor (to break out of main’s accept loop):
    if (g_shutdown.load(std::memory_order_relaxed)) {
        listener.shutdown();
    }
    std::cout << "[LSP] Session thread exiting." << std::endl;

  } catch (const lsp::RequestError& e) {
    // If you throw RequestError in a handler, lsp-framework
    // will automatically send an error response; we just log it here.
    std::cerr << "[LSP][RequestError] " << e.code() << ": " << e.what() << "\n";
  } catch (const std::exception& e) {
    std::cerr << "[LSP][Fatal] Unexpected exception: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "[LSP][Fatal] Unknown error\n";
  }
  // thread exits, socket closed on destruction
}

int main(int argc, char *argv[]) {

    if(argc == 0) {
        std::cerr << "[LSP] No executable path specified" << std::endl;
        return 1;
    }

    CmdOptions options;
    CmdOption cmd_data[] = {
            CmdOption("resources", "res", CmdOptionType::SingleValue),
            CmdOption("port", "port", CmdOptionType::SingleValue),
            CmdOption("version", "v", CmdOptionType::NoValue),
            CmdOption("build-lab", CmdOptionType::SingleValue),
            CmdOption("cc", CmdOptionType::SubCommand),
            CmdOption("shmName", CmdOptionType::SingleValue),
            CmdOption("evtChildDone", CmdOptionType::SingleValue),
            CmdOption("evtParentAck", CmdOptionType::SingleValue),
    };
    options.register_options(cmd_data, sizeof(cmd_data) / sizeof(CmdOption));
    options.parse_cmd_options(argc, argv, 1);

    if(options.has_value("version")) {
        std::cout << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH << std::endl;
        return 0;
    }

    auto& compileOpt = options.cmd_opt("cc");
    if(compileOpt.has_multi_value()) {
        std::vector<std::string> subc;
        subc.emplace_back(argv[0]);
        compileOpt.get_multi_value_vec(subc);
        auto& command_args = subc;
        char** pointers = convert_to_pointers(command_args);
        // invocation
        auto result = compiler_main(command_args.size(), pointers);
        free_pointers(pointers, command_args.size());
        return result;
    }

    auto build_lab = options.option_new("build-lab");
    if(build_lab.has_value()) {

        ModuleStorage storage;
        const auto context = WorkspaceManager::compile_lab(std::string(argv[0]), std::string(build_lab.value()), storage);

        auto shmName = options.option_new("shmName");
        if(!shmName.has_value() || shmName.value().empty()) {
            std::cerr << "[lsp] expected a 'shmName' command line argument" << std::endl;
            return 1;
        }

        auto childDone = options.option_new("evtChildDone");
        if(!childDone.has_value() || childDone.value().empty()) {
            std::cerr << "[lsp] expected a 'evtChildDone' command line argument" << std::endl;
            return 1;
        }

        auto parentAck = options.option_new("evtParentAck");
        if(!parentAck.has_value() || parentAck.value().empty()) {
            std::cerr << "[lsp] expected a 'evtParentAck' command line argument" << std::endl;
            return 1;
        }

        return report_context_to_parent(*context, std::string(shmName.value()), std::string(childDone.value()), std::string(parentAck.value()));

    }

    std::string user_agent = "websocket-server-async";
    std::string port = "5007";
    {
        auto port_opt = options.option_new("port");
        if(port_opt.has_value()) {
            port = port_opt.value();
        }
    }

    int port_int;
    try {
        port_int = std::atoi(port.data());
    } catch(...) {
        std::cerr << "wrong port " << port << std::endl;
        return 1;
    }

    std::cout << "[LSP] Listening on port " << port << std::endl;

    std::atomic<bool> g_shutdown{false};
    auto socketListener = lsp::io::SocketListener(port_int);

    try {

        while(socketListener.isReady()) {
            auto socket = socketListener.listen();

            if(!socket.isOpen()) {
                std::cout << "[LSP] Socket Not Open" << std::endl;
                break;
            }

            std::cout << "[LSP] Accepted connection"  << std::endl;

            // Detach a thread to serve this client
            std::thread(&run_session, std::ref(g_shutdown), std::ref(socketListener), std::move(socket), argv[0]).detach();

        }

    } catch (const std::exception& e) {
        std::cerr << "[LSP][Fatal] " << e.what() << "\n";
        return 1;
    }

    std::cout << "[LSP] Server shutting down." << std::endl;

    return 0;

}


