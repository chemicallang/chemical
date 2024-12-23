// Copyright (c) Qinetik 2024.

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
    CommentStmt,
    ContinueStmt,
    UnreachableStmt,
    DeleteStmt,
    ImportStmt,
    ReturnStmt,
    SwitchStmt,
    ThrowStmt,
    TypealiasStmt,
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
    AccessChain,

    EnumDecl,
    EnumMember,
    FunctionDecl,
    ExtensionFunctionDecl,
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
    ExtensionFuncReceiver,
    GenericTypeParam,
    StructMemberInitializer,
    VariantMemberParam,
    CapturedVariable,
    VariantCaseVariable,
    Malformed


};