// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <cstdint>

/**
 * @brief Enum class representing kind of nodes
 */
enum class ASTNodeKind : uint8_t {

    AssignmentStmt,
    BreakStmt,
    ContinueStmt,
    UnreachableStmt,
    DeleteStmt,
    DeallocStmt,
    ImportStmt,
    ReturnStmt,
    SwitchStmt,
    ThrowStmt,
    TypealiasStmt,
    AliasStmt,
    UsingStmt,
    VarInitStmt,
    LoopBlock,
    ProvideStmt,
    ComptimeBlock,
    WhileLoopStmt,
    DoWhileLoopStmt,
    ForLoopStmt,
    IfStmt,
    TryStmt,
    ValueNode,
    ValueWrapper,
    AccessChainNode,
    IncDecNode,
    PatternMatchExprNode,
    PlacementNewNode,

    EnumDecl,
    EnumMember,
    FunctionDecl,
    MultiFunctionNode,
    ImplDecl,
    InterfaceDecl,
    StructDecl,
    StructMember,
    StructMemberInitializer,
    NamespaceDecl,
    UnionDecl,
    VariantDecl,
    VariantMember,
    UnnamedStruct,
    UnnamedUnion,

    Scope,
    UnsafeBlock,

    FunctionParam,
    GenericTypeParam,
    VariantMemberParam,
    CapturedVariable,
    VariantCaseVariable,
    PatternMatchId,

    StructType,
    UnionType,

    GenericFuncDecl,
    GenericStructDecl,
    GenericVariantDecl,
    GenericUnionDecl,
    GenericInterfaceDecl,
    GenericImplDecl,
    GenericTypeDecl,

    FileScope,
    ModuleScope,

    EmbeddedNode,

    UnresolvedDecl,

};