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
    DeleteStmt,
    ImportStmt,
    ReturnStmt,
    SwitchStmt,
    ThrowStmt,
    TypealiasStmt,
    UsingStmt,
    VarInitStmt,
    WhileLoopStmt,
    DoWhileLoopStmt,
    ForLoopStmt,
    IfStmt,
    TryStmt,
    AccessChain,

    EnumDecl,
    EnumMember,
    FunctionDecl,
    ExtensionFunctionDecl,
    MultiFunctionNode,
    ImplDecl,
    InterfaceDecl,
    StructDecl,
    StructMember,
    NamespaceDecl,
    UnionDecl,
    VariantDecl,
    VariantMember,
    UnnamedStruct,
    UnnamedUnion,

    Scope,

    FunctionParam,
    ExtensionFuncReceiver,
    GenericTypeParam,
    VariantMemberParam,
    CapturedVariable,
    VariantCaseVariable


};