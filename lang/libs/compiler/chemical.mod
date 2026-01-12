module compiler

source "src"

import std

interface SourceProvider
interface BatchAllocator
interface AnnotationController
interface Lexer
interface Parser
interface ASTBuilder
interface PtrVec
interface SymbolResolver
interface ASTDiagnoser