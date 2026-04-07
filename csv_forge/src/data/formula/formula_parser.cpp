#include "data/formula/formula_parser.h"

#include <QSet>

namespace csvforge {

// ===== ASTNode factory =====

ASTNodePtr ASTNode::makeNumber(double val)
{
    auto n = std::make_shared<ASTNode>();
    n->type = ASTNodeType::NumberLiteral;
    n->numericValue = val;
    return n;
}

ASTNodePtr ASTNode::makeString(const QString& val)
{
    auto n = std::make_shared<ASTNode>();
    n->type = ASTNodeType::StringLiteral;
    n->stringValue = val;
    return n;
}

ASTNodePtr ASTNode::makeColumnRef(const QString& name)
{
    auto n = std::make_shared<ASTNode>();
    n->type = ASTNodeType::ColumnRef;
    n->stringValue = name;
    return n;
}

ASTNodePtr ASTNode::makeFunction(const QString& name, std::vector<ASTNodePtr> args)
{
    auto n = std::make_shared<ASTNode>();
    n->type = ASTNodeType::FunctionCall;
    n->stringValue = name.toLower();
    n->children = std::move(args);
    return n;
}

ASTNodePtr ASTNode::makeBinaryOp(const QString& op, ASTNodePtr left, ASTNodePtr right)
{
    auto n = std::make_shared<ASTNode>();
    n->type = ASTNodeType::BinaryOp;
    n->op = op;
    n->children.push_back(std::move(left));
    n->children.push_back(std::move(right));
    return n;
}

ASTNodePtr ASTNode::makeUnaryOp(const QString& op, ASTNodePtr operand)
{
    auto n = std::make_shared<ASTNode>();
    n->type = ASTNodeType::UnaryOp;
    n->op = op;
    n->children.push_back(std::move(operand));
    return n;
}

// ===== Tokenizer =====

std::vector<Token> tokenize(const QString& formula)
{
    std::vector<Token> tokens;
    const int len = formula.size();
    int i = 0;

    while (i < len) {
        const QChar ch = formula[i];

        // Skip whitespace
        if (ch.isSpace()) { ++i; continue; }

        Token tok;
        tok.position = i;

        // Column reference: [column_name]
        if (ch == QLatin1Char('[')) {
            const int start = i + 1;
            const int end = formula.indexOf(QLatin1Char(']'), start);
            if (end < 0) {
                tok.type = TokenType::Unknown;
                tok.text = formula.mid(i);
                tokens.push_back(tok);
                break;
            }
            tok.type = TokenType::ColumnRef;
            tok.text = formula.mid(start, end - start);
            tokens.push_back(tok);
            i = end + 1;
            continue;
        }

        // String literal: 'text'
        if (ch == QLatin1Char('\'')) {
            const int start = i + 1;
            int end = start;
            while (end < len) {
                if (formula[end] == QLatin1Char('\'')) {
                    // Check for escaped quotes ''
                    if (end + 1 < len && formula[end + 1] == QLatin1Char('\'')) {
                        end += 2;
                        continue;
                    }
                    break;
                }
                ++end;
            }
            tok.type = TokenType::StringLiteral;
            tok.text = formula.mid(start, end - start).replace(QLatin1String("''"), QLatin1String("'"));
            tokens.push_back(tok);
            i = (end < len) ? end + 1 : end;
            continue;
        }

        // Number
        if (ch.isDigit() || (ch == QLatin1Char('.') && i + 1 < len && formula[i + 1].isDigit())) {
            int start = i;
            while (i < len && (formula[i].isDigit() || formula[i] == QLatin1Char('.'))) ++i;
            tok.type = TokenType::Number;
            tok.text = formula.mid(start, i - start);
            tokens.push_back(tok);
            continue;
        }

        // Identifier (function name or keyword)
        if (ch.isLetter() || ch == QLatin1Char('_')) {
            int start = i;
            while (i < len && (formula[i].isLetterOrNumber() || formula[i] == QLatin1Char('_'))) ++i;
            tok.type = TokenType::Identifier;
            tok.text = formula.mid(start, i - start);
            tokens.push_back(tok);
            continue;
        }

        // Operators and punctuation
        if (ch == QLatin1Char('+'))  { tok.type = TokenType::Plus; tok.text = QStringLiteral("+"); }
        else if (ch == QLatin1Char('-'))  { tok.type = TokenType::Minus; tok.text = QStringLiteral("-"); }
        else if (ch == QLatin1Char('*'))  { tok.type = TokenType::Star; tok.text = QStringLiteral("*"); }
        else if (ch == QLatin1Char('/'))  { tok.type = TokenType::Slash; tok.text = QStringLiteral("/"); }
        else if (ch == QLatin1Char('%'))  { tok.type = TokenType::Percent; tok.text = QStringLiteral("%"); }
        else if (ch == QLatin1Char('('))  { tok.type = TokenType::LParen; tok.text = QStringLiteral("("); }
        else if (ch == QLatin1Char(')'))  { tok.type = TokenType::RParen; tok.text = QStringLiteral(")"); }
        else if (ch == QLatin1Char(','))  { tok.type = TokenType::Comma; tok.text = QStringLiteral(","); }
        else if (ch == QLatin1Char('='))  { tok.type = TokenType::Equal; tok.text = QStringLiteral("="); }
        else if (ch == QLatin1Char('>')) {
            if (i + 1 < len && formula[i + 1] == QLatin1Char('=')) {
                tok.type = TokenType::GreaterEqual; tok.text = QStringLiteral(">="); ++i;
            } else {
                tok.type = TokenType::GreaterThan; tok.text = QStringLiteral(">");
            }
        }
        else if (ch == QLatin1Char('<')) {
            if (i + 1 < len && formula[i + 1] == QLatin1Char('=')) {
                tok.type = TokenType::LessEqual; tok.text = QStringLiteral("<="); ++i;
            } else if (i + 1 < len && formula[i + 1] == QLatin1Char('>')) {
                tok.type = TokenType::NotEqual; tok.text = QStringLiteral("<>"); ++i;
            } else {
                tok.type = TokenType::LessThan; tok.text = QStringLiteral("<");
            }
        }
        else if (ch == QLatin1Char('!') && i + 1 < len && formula[i + 1] == QLatin1Char('=')) {
            tok.type = TokenType::NotEqual; tok.text = QStringLiteral("!="); ++i;
        }
        else {
            tok.type = TokenType::Unknown;
            tok.text = QString(ch);
        }

        tokens.push_back(tok);
        ++i;
    }

    Token endTok;
    endTok.type = TokenType::End;
    endTok.position = len;
    tokens.push_back(endTok);

    return tokens;
}

// ===== Recursive-descent parser =====

namespace {

class Parser {
public:
    Parser(const std::vector<Token>& tokens, std::vector<FormulaError>& errors)
        : m_tokens(tokens), m_errors(errors), m_pos(0) {}

    ASTNodePtr parse()
    {
        auto node = parseExpression();
        if (peek().type != TokenType::End) {
            addError(QStringLiteral("Unexpected token '%1'").arg(peek().text));
        }
        return node;
    }

private:
    const std::vector<Token>& m_tokens;
    std::vector<FormulaError>& m_errors;
    size_t m_pos;

    const Token& peek() const { return m_tokens[m_pos]; }
    const Token& advance() { return m_tokens[m_pos++]; }

    bool match(TokenType type) {
        if (peek().type == type) { advance(); return true; }
        return false;
    }

    void addError(const QString& msg)
    {
        FormulaError err;
        err.position = (m_pos < m_tokens.size()) ? m_tokens[m_pos].position : -1;
        err.message = msg;
        m_errors.push_back(err);
    }

    // Expression → Comparison (( '=' | '!=' | '<>' ) Comparison)*
    ASTNodePtr parseExpression()
    {
        auto node = parseComparison();
        while (peek().type == TokenType::Equal ||
               peek().type == TokenType::NotEqual) {
            const QString op = advance().text;
            auto right = parseComparison();
            node = ASTNode::makeBinaryOp(op, std::move(node), std::move(right));
        }
        return node;
    }

    // Comparison → Addition (( '>' | '<' | '>=' | '<=' ) Addition)*
    ASTNodePtr parseComparison()
    {
        auto node = parseAddition();
        while (peek().type == TokenType::GreaterThan ||
               peek().type == TokenType::LessThan ||
               peek().type == TokenType::GreaterEqual ||
               peek().type == TokenType::LessEqual) {
            const QString op = advance().text;
            auto right = parseAddition();
            node = ASTNode::makeBinaryOp(op, std::move(node), std::move(right));
        }
        return node;
    }

    // Addition → Multiplication (( '+' | '-' ) Multiplication)*
    ASTNodePtr parseAddition()
    {
        auto node = parseMultiplication();
        while (peek().type == TokenType::Plus ||
               peek().type == TokenType::Minus) {
            const QString op = advance().text;
            auto right = parseMultiplication();
            node = ASTNode::makeBinaryOp(op, std::move(node), std::move(right));
        }
        return node;
    }

    // Multiplication → Unary (( '*' | '/' | '%' ) Unary)*
    ASTNodePtr parseMultiplication()
    {
        auto node = parseUnary();
        while (peek().type == TokenType::Star ||
               peek().type == TokenType::Slash ||
               peek().type == TokenType::Percent) {
            const QString op = advance().text;
            auto right = parseUnary();
            node = ASTNode::makeBinaryOp(op, std::move(node), std::move(right));
        }
        return node;
    }

    // Unary → ('-' | '+') Unary | Primary
    ASTNodePtr parseUnary()
    {
        if (peek().type == TokenType::Minus) {
            const QString op = advance().text;
            auto operand = parseUnary();
            return ASTNode::makeUnaryOp(op, std::move(operand));
        }
        if (peek().type == TokenType::Plus) {
            advance(); // Skip unary +
            return parseUnary();
        }
        return parsePrimary();
    }

    // Primary → Number | String | ColumnRef | FunctionCall | '(' Expression ')'
    ASTNodePtr parsePrimary()
    {
        const Token& tok = peek();

        if (tok.type == TokenType::Number) {
            advance();
            return ASTNode::makeNumber(tok.text.toDouble());
        }

        if (tok.type == TokenType::StringLiteral) {
            advance();
            return ASTNode::makeString(tok.text);
        }

        if (tok.type == TokenType::ColumnRef) {
            advance();
            return ASTNode::makeColumnRef(tok.text);
        }

        if (tok.type == TokenType::Identifier) {
            const QString name = tok.text;
            advance();

            // Function call?
            if (peek().type == TokenType::LParen) {
                advance(); // consume '('
                std::vector<ASTNodePtr> args;
                if (peek().type != TokenType::RParen) {
                    args.push_back(parseExpression());
                    while (match(TokenType::Comma)) {
                        args.push_back(parseExpression());
                    }
                }
                if (!match(TokenType::RParen)) {
                    addError(QStringLiteral("Expected ')' after function arguments"));
                }
                return ASTNode::makeFunction(name, std::move(args));
            }

            // Bare identifier — treat as column ref
            return ASTNode::makeColumnRef(name);
        }

        if (tok.type == TokenType::LParen) {
            advance();
            auto expr = parseExpression();
            if (!match(TokenType::RParen)) {
                addError(QStringLiteral("Expected ')'"));
            }
            return expr;
        }

        addError(QStringLiteral("Unexpected token '%1'").arg(tok.text));
        advance(); // skip bad token
        return ASTNode::makeNumber(0);
    }
};

} // anonymous namespace

// ===== Public API =====

ASTNodePtr parseFormula(const QString& formula, std::vector<FormulaError>& errors)
{
    if (formula.trimmed().isEmpty()) {
        errors.push_back({0, QStringLiteral("Formula is empty")});
        return nullptr;
    }

    auto tokens = tokenize(formula);
    Parser parser(tokens, errors);
    auto ast = parser.parse();
    return errors.empty() ? ast : nullptr;
}

QStringList extractColumnRefs(const ASTNodePtr& node)
{
    QSet<QString> refs;

    std::function<void(const ASTNodePtr&)> walk = [&](const ASTNodePtr& n) {
        if (!n) return;
        if (n->type == ASTNodeType::ColumnRef) {
            refs.insert(n->stringValue);
        }
        for (const auto& child : n->children) {
            walk(child);
        }
    };
    walk(node);

    return QStringList(refs.begin(), refs.end());
}

QStringList knownFunctions()
{
    return {
        // Arithmetic
        QStringLiteral("abs"), QStringLiteral("round"), QStringLiteral("ceil"),
        QStringLiteral("floor"), QStringLiteral("power"), QStringLiteral("sqrt"),
        QStringLiteral("mod"), QStringLiteral("log"),
        // Text
        QStringLiteral("concat"), QStringLiteral("trim"), QStringLiteral("upper"),
        QStringLiteral("lower"), QStringLiteral("length"), QStringLiteral("substring"),
        QStringLiteral("replace"), QStringLiteral("left"), QStringLiteral("right"),
        QStringLiteral("lpad"), QStringLiteral("rpad"),
        // Null handling
        QStringLiteral("coalesce"), QStringLiteral("ifnull"),
        QStringLiteral("nullif"),
        // Conditional
        QStringLiteral("if"), QStringLiteral("case_when"),
        // Date
        QStringLiteral("year"), QStringLiteral("month"), QStringLiteral("day"),
        QStringLiteral("date_diff"), QStringLiteral("date_trunc"),
        QStringLiteral("current_date"), QStringLiteral("strftime"),
        // Type conversion
        QStringLiteral("cast_int"), QStringLiteral("cast_double"),
        QStringLiteral("cast_text"), QStringLiteral("cast_date"),
    };
}

} // namespace csvforge
