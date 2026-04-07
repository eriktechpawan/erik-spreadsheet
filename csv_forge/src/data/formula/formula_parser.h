#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include <variant>
#include <vector>

namespace csvforge {

// ===== Token types =====

enum class TokenType {
    Number,        // 123.45
    StringLiteral, // 'hello'
    ColumnRef,     // [column_name]
    Identifier,    // function name, keyword
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %
    LParen,        // (
    RParen,        // )
    Comma,         // ,
    GreaterThan,   // >
    LessThan,      // <
    GreaterEqual,  // >=
    LessEqual,     // <=
    Equal,         // =
    NotEqual,      // !=  or <>
    End,           // end of input
    Unknown
};

struct Token {
    TokenType type = TokenType::Unknown;
    QString text;
    int position = 0;
};

/// Tokenize a formula string into a list of tokens.
std::vector<Token> tokenize(const QString& formula);

// ===== AST node types =====

enum class ASTNodeType {
    NumberLiteral,
    StringLiteral,
    ColumnRef,
    FunctionCall,
    BinaryOp,
    UnaryOp,
    Conditional  // if(cond, true, false)
};

struct ASTNode;
using ASTNodePtr = std::shared_ptr<ASTNode>;

struct ASTNode {
    ASTNodeType type;

    // NumberLiteral
    double numericValue = 0.0;

    // StringLiteral / ColumnRef / identifier
    QString stringValue;

    // BinaryOp / UnaryOp
    QString op; // "+", "-", "*", "/", ">", "<", ">=", "<=", "=", "!="

    // FunctionCall: stringValue = function name
    std::vector<ASTNodePtr> children;  // arguments or operands

    // Factory helpers
    static ASTNodePtr makeNumber(double val);
    static ASTNodePtr makeString(const QString& val);
    static ASTNodePtr makeColumnRef(const QString& name);
    static ASTNodePtr makeFunction(const QString& name,
                                   std::vector<ASTNodePtr> args);
    static ASTNodePtr makeBinaryOp(const QString& op, ASTNodePtr left,
                                   ASTNodePtr right);
    static ASTNodePtr makeUnaryOp(const QString& op, ASTNodePtr operand);
};

// ===== Parser =====

/// Validation error from the formula parser.
struct FormulaError {
    int position = -1;
    QString message;
};

/// Parse a formula string into an AST.
/// Returns null on error; populate errors vector.
ASTNodePtr parseFormula(const QString& formula,
                        std::vector<FormulaError>& errors);

/// Extract all column references from an AST.
QStringList extractColumnRefs(const ASTNodePtr& node);

/// List of known built-in functions.
QStringList knownFunctions();

} // namespace csvforge
