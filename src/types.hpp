#ifndef TYPES_HPP__
#define TYPES_HPP__

#include <cstdint>

#include <iosfwd>

enum class Type : std::uint8_t
{
    Virtual,

    // FUNCTION
    Functions,

    // TYPENAME
    UserTypes,

    // ID
    Identifiers,

    // BREAK
    // CONTINUE
    // GOTO
    Jumps,

    // _ALIGNAS
    // EXTERN
    // STATIC
    // _THREAD_LOCAL
    // _ATOMIC
    // AUTO
    // REGISTER
    // INLINE
    // _NORETURN
    // CONST
    // VOLATILE
    // RESTRICT
    Specifiers,

    // VOID
    // CHAR
    // SHORT
    // INT
    // LONG
    // FLOAT
    // DOUBLE
    // SIGNED
    // UNSIGNED
    // _BOOL
    // _COMPLEX
    Types,

    // '('
    // '{'
    // '['
    LeftBrackets,

    // ')'
    // '}'
    // ']'
    RightBrackets,

    // LTE_OP
    // GTE_OP
    // EQ_OP
    // NE_OP
    // '<'
    // '>'
    Comparisons,

    // INC_OP
    // DEC_OP
    // LSH_OP
    // RSH_OP
    // '&'
    // '|'
    // '^'
    // '*'
    // '/'
    // '%'
    // '+'
    // '-'
    // '~'
    // '!'
    Operators,

    // AND_OP
    // OR_OP
    LogicalOperators,

    // '='
    // TIMESEQ_OP
    // DIVEQ_OP
    // MODEQ_OP
    // PLUSEQ_OP
    // MINUSEQ_OP
    // LSHIFTEQ_OP
    // RSHIFTEQ_OP
    // ANDEQ_OP
    // XOREQ_OP
    // OREQ_OP
    Assignments,

    // DIRECTIVE
    Directives,

    // SLCOMMENT
    // MLCOMMENT
    Comments,

    // SLIT
    StrConstants,
    // ICONST
    IntConstants,
    // FCONST
    FPConstants,
    // CHCONST
    CharConstants,

    // This is a separator, all types below are not interchangeable.
    NonInterchangeable,

    // DEFAULT
    // RETURN
    // SIZEOF
    // _ALIGNOF
    // _GENERIC
    // DOTS
    // _STATIC_ASSERT
    // IF
    // ELSE
    // SWITCH
    // WHILE
    // DO
    // FOR
    // CASE
    // TYPEDEF
    // STRUCT
    // UNION
    // ENUM
    Keywords,

    // '?'
    // ':'
    // ';'
    // '.'
    // ','
    // ARR_OP
    Other,
};

std::ostream & operator<<(std::ostream &os, Type type);

Type tokenToType(int token);

Type canonizeType(Type type);

#endif // TYPES_HPP__
