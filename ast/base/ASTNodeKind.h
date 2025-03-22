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
    SymResNode,
    ForLoopStmt,
    IfStmt,
    TryStmt,
    ValueNode,
    ValueWrapper,

    EnumDecl,
    EnumMember,
    FunctionDecl,
    MultiFunctionNode,
    ImplDecl,
    InterfaceDecl,
    InitBlock,
    StructDecl,
    StructMember,
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
    PackageDef,

};