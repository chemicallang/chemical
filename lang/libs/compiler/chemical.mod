module compiler

source "src"

import std

interface SourceProvider
interface BatchAllocator
interface SerialStrAllocator
interface Lexer
interface Parser
interface ASTBuilder
interface PtrVec