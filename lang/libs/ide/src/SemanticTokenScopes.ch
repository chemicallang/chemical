public enum SemanticTokenScopes {
    MarkupItalic = 23,
    MarkupItalicMarkdown,
    MarkupBold,
    MarkupBoldMarkdown,
    MarkupHeading,
    MarkupHeadingSetext,
    MarkupHeadingSetext1Markdown,
    MarkupHeadingSetext2Markdown,
    MarkupHeadingMarkdown,
    MarkupStrikethrough,
    MarkupInserted,
    MarkupInsertedDiff,
    MarkupInsertedGit_gutter,
    MarkupDeleted,
    MarkupDeletedDiff,
    MarkupDeletedGit_gutter,
    MarkupInlineRaw,
    MarkupInlineRawMarkdown,
    MarkupInlineRawStringMarkdown,
    MarkupChanged,
    MarkupChangedDiff,
    MarkupChangedGit_gutter,
    MarkupUnderline,
    MarkupUnderlineLinkMarkdown,
    MarkupUnderlineLinkImageMarkdown,
    MarkupList,
    MarkupListUnnumberedMarkdown,
    MarkupListNumberedMarkdown,
    MarkupListAsciidoc,
    MarkupQuote,
    MarkupQuoteMarkdown,
    MarkupOtherUrlAsciidoc,
    MarkupError,
    MarkupOutput,
    MarkupRaw,
    MarkupRawMonospaceAsciidoc,
    MarkupPrompt,
    MarkupTraceback,
    MarkupHeadingPunctuationDefinitionHeading,
    MarkupLinkAsciidoc,
    Entity,
    EntityNameFunction,
    EntityNameFunctionPreprocessor,
    EntityNameFunctionXi,
    EntityNameType,
    EntityNameTypeModule,
    EntityNameTypeNamespace,
    EntityNameTypeClass,
    EntityNameNamespace,
    EntityNameTag,
    EntityNameTagCss,
    EntityNameTagLess,
    EntityNameTagStyle,
    EntityNameTagScript,
    EntityNameScopeResolution,
    EntityNameScopeResolutionFunctionCall,
    EntityNameScopeResolutionFunctionDefinition,
    EntityNameClass,
    EntityNameClassIdentifierNamespaceType,
    EntityNameClassXi,
    EntityNameOperator,
    EntityNameOperatorCustomLiteral,
    EntityNameLabel,
    EntityNameLabelCs,
    EntityNameVariable,
    EntityNameVariableLocalCs,
    EntityNameVariableParameterCs,
    EntityNameVariableParameterPhp,
    EntityNameVariableFieldCs,
    EntityNameSection,
    EntityNameSectionMarkdown,
    EntityNameSelector,
    EntityNameFilenameFindInFiles,
    EntityNameException,
    EntityNameLifetimeRust,
    EntityNameGotoLabelPhp,
    EntityNamePackageGo,
    EntityOtherInheritedClass,
    EntityOtherAttributeName,
    EntityOtherAttributeNameClassCss,
    EntityOtherAttributeNameId,
    EntityOtherAttributeNameIdCss,
    EntityOtherAttributeNameParentSelectorCss,
    EntityOtherAttributeNameParentLess,
    EntityOtherAttributeNamePseudoElement,
    EntityOtherAttributeNamePseudoElementCss,
    EntityOtherAttributeNameScss,
    EntityOtherAttributeNameHtml,
    EntityOtherAttributeNamePseudoClass,
    EntityOtherAttribute,
    EntityOtherAliasPhp,
    EntityGlobalClojure,
    Support,
    SupportFunction,
    SupportFunctionGitRebase,
    SupportFunctionAnyMethod,
    SupportFunctionConstruct,
    SupportFunctionStdRust,
    SupportFunctionConsole,
    SupportClass,
    SupportClassMathBlockEnvironmentLatex,
    SupportType,
    SupportTypePropertyName,
    SupportTypePropertyNameJson,
    SupportTypePropertyNameCss,
    SupportTypePropertyNameToml,
    SupportTypePropertyNameTableToml,
    SupportTypePropertyNameArrayToml,
    SupportTypePropertyNameJsonPunctuation,
    SupportTypeVendoredPropertyName,
    SupportTypeVendoredPropertyNameCss,
    SupportTypeObjectHlsl,
    SupportTypeObjectModule,
    SupportTypeObjectConsole,
    SupportTypeObjectDom,
    SupportTypeObjectRwHlsl,
    SupportTypeException,
    SupportTypePosixReservedC,
    SupportTypePosixReservedCpp,
    SupportTypePython,
    SupportTypePrimitive,
    SupportTypePrimitiveTs,
    SupportTypePrimitiveTsx,
    SupportTypeBuiltinTs,
    SupportTypeBuiltinTsx,
    SupportTypeTypeFlowtype,
    SupportTypePreludeElm,
    SupportTypeTextureHlsl,
    SupportTypeSamplerHlsl,
    SupportTypeFxHlsl,
    SupportTypeSwift,
    SupportTypeVbAsp,
    SupportConstant,
    SupportConstantPropertyValue,
    SupportConstantPropertyValueScss,
    SupportConstantPropertyValueCss,
    SupportConstantFontName,
    SupportConstantColor,
    SupportConstantColorW3cStandardColorNameCss,
    SupportConstantColorW3cStandardColorNameScss,
    SupportConstantMediaType,
    SupportConstantMedia,
    SupportConstantHandlebars,
    SupportConstantMath,
    SupportConstantJson,
    SupportConstantDom,
    SupportConstantEdge,
    SupportConstantPropertyMath,
    SupportConstantCoreRust,
    SupportConstantCorePhp,
    SupportConstantExtPhp,
    SupportConstantStdPhp,
    SupportConstantParserTokenPhp,
    SupportConstantElm,
    SupportVariable,
    SupportVariableMagicPython,
    SupportVariableProperty,
    SupportVariablePropertyProcess,
    SupportVariablePropertyDom,
    SupportVariableObjectProcess,
    SupportVariableObjectNode,
    SupportVariableDom,
    SupportVariableSemanticHlsl,
    SupportOtherParenthesisRegexp,
    SupportOtherVariable,
    SupportOtherNamespaceUsePhp,
    SupportOtherNamespaceUseAsPhp,
    SupportOtherNamespacePhp,
    SupportOtherPhp,
    SupportModuleNode,
    SupportTokenDecoratorPython,
    Keyword,
    KeywordControl,
    KeywordControlAnchorRegexp,
    KeywordControlXi,
    KeywordOperator,
    KeywordOperatorNew,
    KeywordOperatorNewCpp,
    KeywordOperatorLogical,
    KeywordOperatorLogicalPython,
    KeywordOperatorLogicalPhp,
    KeywordOperatorQuantifierRegexp,
    KeywordOperatorExpression,
    KeywordOperatorExpressionInstanceof,
    KeywordOperatorExpressionImport,
    KeywordOperatorExpressionKeyof,
    KeywordOperatorExpressionDelete,
    KeywordOperatorExpressionIn,
    KeywordOperatorExpressionOf,
    KeywordOperatorExpressionTypeof,
    KeywordOperatorExpressionVoid,
    KeywordOperatorExpressionIs,
    KeywordOperatorCast,
    KeywordOperatorSizeof,
    KeywordOperatorSizeofC,
    KeywordOperatorSizeofCpp,
    KeywordOperatorNegationRegexp,
    KeywordOperatorOrRegexp,
    KeywordOperatorPlusExponent,
    KeywordOperatorMinusExponent,
    KeywordOperatorNoexcept,
    KeywordOperatorAlignof,
    KeywordOperatorTypeid,
    KeywordOperatorAlignas,
    KeywordOperatorInstanceof,
    KeywordOperatorInstanceofJava,
    KeywordOperatorWordlike,
    KeywordOperatorDelete,
    KeywordOperatorDeleteCpp,
    KeywordOperatorLess,
    KeywordOperatorClass,
    KeywordOperatorWord,
    KeywordOperatorBitwise,
    KeywordOperatorBitwiseShiftC,
    KeywordOperatorBitwiseShiftCpp,
    KeywordOperatorBitwisePhp,
    KeywordOperatorChannel,
    KeywordOperatorCss,
    KeywordOperatorScss,
    KeywordOperatorTernary,
    KeywordOperatorOptional,
    KeywordOperatorMiscRust,
    KeywordOperatorSigilRust,
    KeywordOperatorArithmetic,
    KeywordOperatorArithmeticPhp,
    KeywordOperatorArithmeticGo,
    KeywordOperatorArithmeticC,
    KeywordOperatorArithmeticCpp,
    KeywordOperatorComparison,
    KeywordOperatorComparisonC,
    KeywordOperatorComparisonCpp,
    KeywordOperatorComparisonPhp,
    KeywordOperatorDecrement,
    KeywordOperatorDecrementC,
    KeywordOperatorDecrementCpp,
    KeywordOperatorIncrement,
    KeywordOperatorIncrementC,
    KeywordOperatorIncrementCpp,
    KeywordOperatorRelational,
    KeywordOperatorAssignment,
    KeywordOperatorAssignmentC,
    KeywordOperatorAssignmentCpp,
    KeywordOperatorAssignmentCompound,
    KeywordOperatorAssignmentCompoundJs,
    KeywordOperatorAssignmentCompoundTs,
    KeywordOperatorAssignmentGo,
    KeywordOperatorC,
    KeywordOperatorCpp,
    KeywordOperatorErrorControlPhp,
    KeywordOperatorTypePhp,
    KeywordOperatorRegexpPhp,
    KeywordOperatorHeredocPhp,
    KeywordOperatorNowdocPhp,
    KeywordOperatorModule,
    KeywordOperatorAddressGo,
    KeywordOtherUnit,
    KeywordOtherUsing,
    KeywordOtherDirectiveUsing,
    KeywordOtherOperator,
    KeywordOtherSpecialMethod,
    KeywordOtherSpecialMethodRuby,
    KeywordOtherDMLSql,
    KeywordOtherImportant,
    KeywordOtherNew,
    KeywordOtherTypePhp,
    KeywordOtherArrayPhpdocPhp,
    KeywordOtherTemplateBegin,
    KeywordOtherTemplateEnd,
    KeywordOtherSubstitutionBegin,
    KeywordOtherSubstitutionEnd,
    MetaEmbedded,
    MetaEmbeddedAssembly,
    MetaDiff,
    MetaDiffHeader,
    MetaDiffHeaderFromFile,
    MetaDiffHeaderToFile,
    MetaDiffRange,
    MetaDiffIndex,
    MetaTemplateExpression,
    MetaObjectLiteralKey,
    MetaPreprocessor,
    MetaPreprocessorString,
    MetaPreprocessorNumeric,
    MetaPreprocessorAtRuleKeywordControlAtRule,
    MetaTypeCastExpr,
    MetaTypeNewExpr,
    MetaDefinitionVariableName,
    MetaDefinitionVariableNameJava,
    MetaDefinitionVariableNameGroovy,
    MetaDefinitionClassInheritedClassesGroovy,
    MetaStructureDictionaryKeyPython,
    MetaStructureDictionaryJsonStringQuotedDoubleJson,
    MetaTag,
    MetaTagSgmlDoctype,
    MetaTagSgmlDoctypeString,
    MetaTagSgmlDoctypeEntityNameTag,
    MetaTagSgmlDeclarationDoctype,
    MetaTagInlineSource,
    MetaTagOther,
    MetaTagBlockScript,
    MetaTagSgmlPunctuationDefinitionTagHtml,
    MetaSelector,
    MetaSelectorCssEntityNameTag,
    MetaSelectorCssEntityOtherAttributeNameId,
    MetaSelectorCssEntityOtherAttributeNameClass,
    MetaSeparator,
    MetaRequire,
    MetaLink,
    MetaFunctionCall,
    MetaFunctionCallObject,
    MetaFunctionCallObjectPhp,
    MetaFunctionCallGenericPython,
    MetaFunctionCallPhp,
    MetaFunctionCallStaticPhp,
    MetaResultLinePrefixContextLinePrefixSearch,
    MetaDoctype,
    MetaTagPunctuationDefinitionString,
    MetaTagStringSourcePunctuation,
    MetaBraceErbHtml,
    MetaBraceSquare,
    MetaTocListId,
    MetaSelectorEntity,
    MetaSelectorEntityOtherAttributeNameId,
    MetaPropertyGroupSupportConstantPropertyValue,
    MetaPropertyGroupSupportConstantPropertyValueCss,
    MetaPropertyValueSupportConstantPropertyValue,
    MetaPropertyValueSupportConstantPropertyValueCss,
    MetaPropertyValueSupportConstantNamedColorCss,
    MetaTagEntity,
    MetaTagEntityOtherAttributeName,
    MetaSelectorEntityPunctuation,
    MetaPropertyName,
    MetaPropertyValue,
    MetaPropertyValueConstant,
    MetaPropertyValueConstantOther,
    MetaJsxChildren,
    MetaJsxChildrenJs,
    MetaConstructorArgumentCss,
    MetaBlockLevel,
    MetaFunctionC,
    MetaFunctionCpp,
    MetaFunctionDecoratorPython,
    MetaFunctionDecoratorIdentifierPython,
    MetaMethodIdentifierJava,
    MetaMethodBodyJava,
    MetaMethodJava,
    MetaMethodGroovy,
    MetaMethodCallJava,
    MetaInterfacePhp,
    MetaOtherTypePhpdocPhp,
    MetaPropertyObject,
    MetaSymbolClojure,
    MetaArgumentsCoffee,
    MetaScopePrerequisitesMakefile,
    Comment,
    CommentBlockPreprocessor,
    CommentBlockDocumentation,
    CommentBlockDocumentationPunctuationDefinitionComment,
    CommentDocumentation,
    Storage,
    StorageType,
    StorageTypeCs,
    StorageTypeAnnotationJava,
    StorageTypeAnnotationGroovy,
    StorageTypeGenericJava,
    StorageTypeGenericCs,
    StorageTypeGenericGroovy,
    StorageTypeJava,
    StorageTypeObjectArrayJava,
    StorageTypeObjectArrayGroovy,
    StorageTypeModifierCs,
    StorageTypeVariableCs,
    StorageTypePrimitiveArrayJava,
    StorageTypePrimitiveArrayGroovy,
    StorageTypePrimitiveJava,
    StorageTypePrimitiveGroovy,
    StorageTypeTokenJava,
    StorageTypeGroovy,
    StorageTypeParametersGroovy,
    StorageTypeNumericGo,
    StorageTypeByteGo,
    StorageTypeBooleanGo,
    StorageTypeStringGo,
    StorageTypeUintptrGo,
    StorageTypeErrorGo,
    StorageTypeRuneGo,
    StorageTypeHaskell,
    StorageTypePhp,
    StorageModifier,
    StorageModifierImportJava,
    StorageModifierImportGroovy,
    StorageModifierPackageJava,
    StorageModifierLifetimeRust,
    String,
    StringRegexp,
    StringQuotedDoubleHtml,
    StringQuotedDoubleXml,
    StringQuotedDoubleHandlebars,
    StringQuotedDoubleHtmlSource,
    StringQuotedPug,
    StringQuotedSingleYaml,
    StringQuotedSingleXml,
    StringQuotedSingleHtml,
    StringQuotedSingleHandlebars,
    StringCommentBufferedBlockPug,
    StringInterpolatedPug,
    StringUnquotedPlainInYaml,
    StringUnquotedPlainOutYaml,
    StringUnquotedBlockYaml,
    StringUnquotedCdataXml,
    StringUnquotedHtml,
    StringUnquotedAsciidoc,
    StringOtherLink,
    StringOtherLinkTitleMarkdown,
    StringOtherLinkDescriptionMarkdown,
    StringTag,
    StringValue,
    Punctuation,
    PunctuationSeparatorNamespaceRuby,
    PunctuationSeparatorVariable,
    PunctuationSeparatorContinuation,
    PunctuationSeparatorPipeUnison,
    PunctuationSeparatorDelimiter,
    PunctuationSeparatorDelimiterUnison,
    PunctuationSeparatorDelimiterPhp,
    PunctuationSeparatorPeriodPython,
    PunctuationSeparatorPeriodJava,
    PunctuationSeparatorElementPython,
    PunctuationSeparatorKeyValue,
    PunctuationSeparatorListCommaCss,
    PunctuationSeparatorC,
    PunctuationSeparatorCpp,
    PunctuationSeparatorArgumentsPython,
    PunctuationSeparatorColonPhp,
    PunctuationSectionEmbedded,
    PunctuationSectionEmbeddedBegin,
    PunctuationSectionEmbeddedBeginPhp,
    PunctuationSectionEmbeddedBeginRuby,
    PunctuationSectionEmbeddedEnd,
    PunctuationSectionEmbeddedEndPhp,
    PunctuationSectionEmbeddedEndRuby,
    PunctuationSectionEmbeddedCoffee,
    PunctuationSectionEmbeddedSourceStringSourcePunctuationSectionEmbedded,
    PunctuationSectionBlockBeginBracketCurlyCpp,
    PunctuationSectionBlockBeginBracketCurlyC,
    PunctuationSectionBlockBeginJava,
    PunctuationSectionBlockEndBracketCurlyCpp,
    PunctuationSectionBlockEndBracketCurlyC,
    PunctuationSectionBlockEndJava,
    PunctuationSectionParensBeginBracketRoundC,
    PunctuationSectionParensEndBracketRoundC,
    PunctuationSectionParametersBeginBracketRoundC,
    PunctuationSectionParametersEndBracketRoundC,
    PunctuationSectionMethodBeginJava,
    PunctuationSectionMethodBeginBracketCurlyJava,
    PunctuationSectionMethodEndJava,
    PunctuationSectionMethodEndBracketCurlyJava,
    PunctuationSectionClassBeginJava,
    PunctuationSectionClassBeginBracketCurlyJava,
    PunctuationSectionClassEndJava,
    PunctuationSectionClassEndBracketCurlyJava,
    PunctuationSectionInnerClassBeginJava,
    PunctuationSectionInnerClassEndJava,
    PunctuationSectionArrayBeginPhp,
    PunctuationSectionArrayEndPhp,
    PunctuationSectionScopeBeginPhp,
    PunctuationSectionScopeEndPhp,
    PunctuationDefinitionListBeginMarkdown,
    PunctuationDefinitionListBeginUnison,
    PunctuationDefinitionListBeginPython,
    PunctuationDefinitionListEndUnison,
    PunctuationDefinitionListEndPython,
    PunctuationDefinitionListMarkdown,
    PunctuationDefinitionTemplateExpression,
    PunctuationDefinitionTemplateExpressionBegin,
    PunctuationDefinitionTemplateExpressionEnd,
    PunctuationDefinitionTag,
    PunctuationDefinitionTagHtml,
    PunctuationDefinitionTagBegin,
    PunctuationDefinitionTagBeginHtml,
    PunctuationDefinitionTagBeginJs,
    PunctuationDefinitionTagEnd,
    PunctuationDefinitionTagEndHtml,
    PunctuationDefinitionTagEndJs,
    PunctuationDefinitionTagJs,
    PunctuationDefinitionTagXi,
    PunctuationDefinitionGroupRegexp,
    PunctuationDefinitionGroupAssertionRegexp,
    PunctuationDefinitionCharacterClassRegexp,
    PunctuationDefinitionQuoteBeginMarkdown,
    PunctuationDefinitionComment,
    PunctuationDefinitionString,
    PunctuationDefinitionStringBeginHtml,
    PunctuationDefinitionStringBeginPhp,
    PunctuationDefinitionStringBeginMarkdown,
    PunctuationDefinitionStringEndHtml,
    PunctuationDefinitionStringEndHtmlSource,
    PunctuationDefinitionStringEndPhp,
    PunctuationDefinitionStringEndMarkdown,
    PunctuationDefinitionVariable,
    PunctuationDefinitionEntity,
    PunctuationDefinitionHeading,
    PunctuationDefinitionHeadingMarkdown,
    PunctuationDefinitionBold,
    PunctuationDefinitionBoldMarkdown,
    PunctuationDefinitionItalic,
    PunctuationDefinitionFromFileDiff,
    PunctuationDefinitionToFileDiff,
    PunctuationDefinitionMetadataMarkdown,
    PunctuationDefinitionSectionSwitchBlockEndBracketCurlyPhp,
    PunctuationDefinitionSectionSwitchBlockStartBracketCurlyPhp,
    PunctuationDefinitionSectionSwitchBlockBeginBracketCurlyPhp,
    PunctuationDefinitionParameters,
    PunctuationDefinitionParametersBeginBracketRoundPhp,
    PunctuationDefinitionParametersEndBracketRoundPhp,
    PunctuationDefinitionArray,
    PunctuationDefinitionArrayBeginBracketRoundPhp,
    PunctuationDefinitionArrayEndBracketRoundPhp,
    PunctuationDefinitionDelayedUnison,
    PunctuationDefinitionAbilityBeginUnison,
    PunctuationDefinitionAbilityEndUnison,
    PunctuationDefinitionHashUnison,
    PunctuationDefinitionMethodParametersBeginJava,
    PunctuationDefinitionMethodParametersEndJava,
    PunctuationDefinitionAnnotationJava,
    PunctuationDefinitionArgumentsBeginPython,
    PunctuationDefinitionArgumentsBeginBracketRoundPhp,
    PunctuationDefinitionArgumentsEndPython,
    PunctuationDefinitionArgumentsEndBracketRoundPhp,
    PunctuationDefinitionConstant,
    PunctuationDefinitionRawMarkdown,
    PunctuationDefinitionAsciidoc,
    PunctuationDefinitionStorageTypeBeginBracketRoundPhp,
    PunctuationDefinitionStorageTypeEndBracketRoundPhp,
    PunctuationDefinitionBeginBracketRoundPhp,
    PunctuationDefinitionBeginBracketCurlyPhp,
    PunctuationDefinitionEndBracketRoundPhp,
    PunctuationDefinitionEndBracketCurlyPhp,
    PunctuationDefinitionBlockSequenceItemYaml,
    PunctuationCharacterSetBeginRegexp,
    PunctuationCharacterSetEndRegexp,
    PunctuationOperatorAssignmentAsUnison,
    PunctuationParenthesisBeginPython,
    PunctuationParenthesisEndPython,
    PunctuationTerminatorStatementC,
    PunctuationTerminatorJava,
    PunctuationTerminatorExpressionPhp,
    PunctuationBracketAngleJava,
    PunctuationQuasiElement,
    SourceGroovyEmbedded,
    SourcePowershellVariableOtherMember,
    SourceCppKeywordOperatorNew,
    SourceCppKeywordOperatorDelete,
    SourceCssEntityOtherAttributeNameClass,
    SourceCssEntityOtherAttributeNamePseudoClass,
    SourceCssLessEntityOtherAttributeNameId,
    SourceCssEmbeddedPunctuationDefinitionTagHtml,
    SourceCssVariable,
    SourceCoffeeEmbedded,
    SourceJsEmbeddedPunctuationDefinitionTagHtml,
    SourceCssSupportTypePropertyName,
    SourcePhpEmbeddedLine,
    SourcePhpEmbeddedLineHtml,
    SourceJava,
    SourceJsonMetaStructureDictionaryJsonContainingStringQuotedJson,
    SourceJsonMetaStructureDictionaryJsonContainingStringQuotedJsonContainingPunctuationString,
    SourceJsonMetaStructureDictionaryJsonContainingValueJsonContainingStringQuotedJson,
    SourceJsonMetaStructureDictionaryJsonContainingValueJsonContainingStringQuotedJsonContainingPunctuation,
    SourceJsonMetaStructureDictionaryJsonContainingConstantLanguageJson,
    SourceJsonMetaStructureArrayJsonContainingValueJsonContainingStringQuotedJson,
    SourceJsonMetaStructureArrayJsonContainingValueJsonContainingStringQuotedJsonContainingPunctuation,
    SourceJsonMetaStructureArrayJsonContainingConstantLanguageJson,
    SourceIni,
    SourceMakefile,
    Variable,
    VariableLegacyBuiltinPython,
    VariableLanguage,
    VariableLanguageWildcardJava,
    VariableLanguageThis,
    VariableLanguageJs,
    VariableLanguageRuby,
    VariableLanguageRust,
    VariableOther,
    VariableOtherEnummember,
    VariableOtherConstant,
    VariableOtherConstantProperty,
    VariableOtherPhp,
    VariableOtherNormal,
    VariableOtherProperty,
    VariableOtherGenericTypeHaskell,
    VariableOtherReadwrite,
    VariableOtherReadwriteC,
    VariableOtherClassJs,
    VariableOtherClassTs,
    VariableOtherClassPhp,
    VariableOtherObject,
    VariableParameter,
    VariableParameterFunction,
    VariableParameterFunctionLanguageSpecialSelfPython,
    VariableParameterFunctionLanguageSpecialClsPython,
    VariableParameterFunctionLanguagePython,
    VariableParameterFunctionPython,
    VariableParameterFunctionJs,
    VariableParameterFunctionCoffee,
    VariableParameterFunctionLatex,
    VariableInterpolation,
    VariableJs,
    VariableC,
    VariableFunction,
    Constant,
    ConstantNumeric,
    ConstantNumericLineNumberFindInFilesMatch,
    ConstantNumericDecimalAsmX86_64,
    ConstantCharacter,
    ConstantCharacterEscape,
    ConstantCharacterCharacterClassRegexp,
    ConstantCharacterCharacterClassRegexpXi,
    ConstantCharacterSetRegexp,
    ConstantCharacterEntity,
    ConstantCharacterFormatPlaceholderOtherPython,
    ConstantCharacterXi,
    ConstantCharacterMathTex,
    ConstantLanguage,
    ConstantLanguageSymbolRuby,
    ConstantLanguageSymbolHashkeyRuby,
    ConstantLanguageSymbolElixir,
    ConstantLanguageSymbolDoubleQuotedElixir,
    ConstantOther,
    ConstantOtherColor,
    ConstantOtherColorRgbValue,
    ConstantOtherColorRgbValueXi,
    ConstantOtherRgbValue,
    ConstantOtherCharacterClassRegexp,
    ConstantOtherCharacterClassSetRegexp,
    ConstantOtherPlaceholder,
    ConstantOtherSymbol,
    ConstantOtherSymbolRuby,
    ConstantOtherOption,
    ConstantOtherPhp,
    ConstantOtherGeneralMathTex,
    ConstantRegexp,
    ConstantRegexpXi,
    ConstantShaGitRebase,
    ConstantKeywordClojure,
    Invalid,
    InvalidDeprecated,
    InvalidDeprecatedEntityOtherAttributeNameHtml,
    InvalidIllegal,
    InvalidIllegalBadAmpersandHtml,
    InvalidIllegalUnrecognizedTagHtml,
    InvalidIllegalNonNullTypehintedPhp,
    InvalidBroken,
    InvalidUnimplemented,
    InvalidXi,
    StringMetaImageInlineMarkdown,
    TokenInfoToken,
    TokenWarnToken,
    TokenErrorToken,
    TokenDebugToken,
    TokenVariableParameterJava,
    TokenPackage,
    TokenPackageKeyword,
    TokenStorage,
    TokenStorageTypeJava,
    Emphasis,
    Strong,
    None,
    DeclarationTag,
    DeclarationSgmlHtmlDeclarationDoctype,
    DeclarationSgmlHtmlDeclarationDoctypeEntity,
    DeclarationSgmlHtmlDeclarationDoctypeString,
    DeclarationXmlProcessing,
    DeclarationXmlProcessingEntity,
    DeclarationXmlProcessingString,
    DeclarationTagEntity,
    Header,
    StringSource,
    TextHtmlPhpSource,
    TextHtmlLaravelBladeSourcePhpEmbeddedLineHtmlEntityNameTagLaravelBlade,
    TextHtmlLaravelBladeSourcePhpEmbeddedLineHtmlSupportConstantLaravelBlade,
    TextVariable,
    TextBracketed,
    TextSourceTextMetaTagStringPunctuation,
    StringConstant,
    StringVariable,
    CommentMarkupLink,
    ImportStorageJava,
    ControlElements,
    TodoBold,
    TodoEmphasis,
    EmphasisMd,
    BeginningPunctuationDefinitionListMarkdown,
    BeginningPunctuationDefinitionListMarkdownXi,
    BeginningPunctuationDefinitionQuoteMarkdownXi,
    FunctionParameter,
    FunctionParameterRuby,
    FunctionParameterCs,
    FunctionBrace,
    RgbValue,
    InlineColorDecorationRgbValue,
    LessRgbValue,
    SelectorSass,
    BlockScopeEnd,
    BlockScopeBegin,
    AccentXi,
    WikiwordXi,
    LogInfo,
    LogWarning,
    LogError,
    TextHtmlDerivative
};