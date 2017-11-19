#ifndef STYPES_HPP__
#define STYPES_HPP__

#include <cstdint>

#include <iosfwd>

enum class SType : std::uint8_t
{
    None,
    TranslationUnit,
    Declaration,
    FunctionDeclaration,
    FunctionDefinition,
    Comment,
    Directive,
    LineGlue,
    Macro,
    CompoundStatement,
    Separator,
    Punctuation,
    Statements,
    ExprStatement,
    IfStmt,
    IfCond,
    IfThen,
    IfElse,
    WhileStmt,
    DoWhileStmt,
    WhileCond,
    ForStmt,
    LabelStmt,
    ForHead,
    Expression,
    Declarator,
    Initializer,
    InitializerList,
    Specifiers,
    WithInitializer,
    WithoutInitializer,
    InitializerElement,
    SwitchStmt,
    GotoStmt,
    ContinueStmt,
    BreakStmt,
    ReturnValueStmt,
    ReturnNothingStmt,
    ArgumentList,
    Argument,
    ParameterList,
    Parameter,
    CallExpr,
    AssignmentExpr,
    ConditionExpr,
    ComparisonExpr,
    AdditiveExpr,
    PointerDecl,
    DirectDeclarator,
    TemporaryContainer,
    Bundle,
    BundleComma,
};

std::ostream & operator<<(std::ostream &os, SType stype);

#endif // STYPES_HPP__
