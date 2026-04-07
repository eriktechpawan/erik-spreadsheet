#include <QCoreApplication>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "data/formula/formula_parser.h"

using namespace csvforge;

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "FAIL: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; \
    } else { tests_passed++; } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAIL: " << #a << " != " << #b << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; \
    } else { tests_passed++; } \
} while(0)

void test_tokenize_simple_arithmetic()
{
    auto tokens = tokenize(QStringLiteral("[price] * [qty]"));
    // Expected: ColumnRef, Star, ColumnRef, End
    ASSERT_EQ(static_cast<int>(tokens.size()), 4);
    ASSERT_EQ(tokens[0].type, TokenType::ColumnRef);
    ASSERT_EQ(tokens[0].text, QStringLiteral("price"));
    ASSERT_EQ(tokens[1].type, TokenType::Star);
    ASSERT_EQ(tokens[2].type, TokenType::ColumnRef);
    ASSERT_EQ(tokens[2].text, QStringLiteral("qty"));
    ASSERT_EQ(tokens[3].type, TokenType::End);
}

void test_tokenize_function_call()
{
    auto tokens = tokenize(QStringLiteral("concat([first], ' ', [last])"));
    ASSERT_TRUE(tokens.size() >= 8); // Identifier, LParen, ColumnRef, Comma, String, Comma, ColumnRef, RParen, End
    ASSERT_EQ(tokens[0].type, TokenType::Identifier);
    ASSERT_EQ(tokens[0].text, QStringLiteral("concat"));
    ASSERT_EQ(tokens[1].type, TokenType::LParen);
    ASSERT_EQ(tokens[2].type, TokenType::ColumnRef);
    ASSERT_EQ(tokens[3].type, TokenType::Comma);
    ASSERT_EQ(tokens[4].type, TokenType::StringLiteral);
    ASSERT_EQ(tokens[4].text, QStringLiteral(" "));
}

void test_tokenize_comparison_operators()
{
    auto tokens = tokenize(QStringLiteral("[a] >= 100"));
    ASSERT_EQ(tokens[0].type, TokenType::ColumnRef);
    ASSERT_EQ(tokens[1].type, TokenType::GreaterEqual);
    ASSERT_EQ(tokens[2].type, TokenType::Number);
    ASSERT_EQ(tokens[2].text, QStringLiteral("100"));
}

void test_tokenize_not_equal()
{
    auto tokens1 = tokenize(QStringLiteral("[x] != 0"));
    ASSERT_EQ(tokens1[1].type, TokenType::NotEqual);

    auto tokens2 = tokenize(QStringLiteral("[x] <> 0"));
    ASSERT_EQ(tokens2[1].type, TokenType::NotEqual);
}

void test_tokenize_string_with_escaped_quotes()
{
    auto tokens = tokenize(QStringLiteral("'it''s'"));
    ASSERT_EQ(tokens[0].type, TokenType::StringLiteral);
    ASSERT_EQ(tokens[0].text, QStringLiteral("it's"));
}

void test_parse_simple_arithmetic()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("[price] * [qty]"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(errors.empty());
    ASSERT_EQ(ast->type, ASTNodeType::BinaryOp);
    ASSERT_EQ(ast->op, QStringLiteral("*"));
    ASSERT_EQ(ast->children.size(), 2u);
    ASSERT_EQ(ast->children[0]->type, ASTNodeType::ColumnRef);
    ASSERT_EQ(ast->children[0]->stringValue, QStringLiteral("price"));
    ASSERT_EQ(ast->children[1]->type, ASTNodeType::ColumnRef);
    ASSERT_EQ(ast->children[1]->stringValue, QStringLiteral("qty"));
}

void test_parse_operator_precedence()
{
    std::vector<FormulaError> errors;
    // Should parse as (a + (b * c))
    auto ast = parseFormula(QStringLiteral("[a] + [b] * [c]"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_EQ(ast->type, ASTNodeType::BinaryOp);
    ASSERT_EQ(ast->op, QStringLiteral("+"));
    ASSERT_EQ(ast->children[1]->type, ASTNodeType::BinaryOp);
    ASSERT_EQ(ast->children[1]->op, QStringLiteral("*"));
}

void test_parse_function_call()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("upper([name])"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(errors.empty());
    ASSERT_EQ(ast->type, ASTNodeType::FunctionCall);
    ASSERT_EQ(ast->stringValue, QStringLiteral("upper"));
    ASSERT_EQ(ast->children.size(), 1u);
    ASSERT_EQ(ast->children[0]->type, ASTNodeType::ColumnRef);
}

void test_parse_nested_function()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("concat(upper([first]), ' ', lower([last]))"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(errors.empty());
    ASSERT_EQ(ast->type, ASTNodeType::FunctionCall);
    ASSERT_EQ(ast->stringValue, QStringLiteral("concat"));
    ASSERT_EQ(ast->children.size(), 3u);
    ASSERT_EQ(ast->children[0]->type, ASTNodeType::FunctionCall);
    ASSERT_EQ(ast->children[1]->type, ASTNodeType::StringLiteral);
    ASSERT_EQ(ast->children[2]->type, ASTNodeType::FunctionCall);
}

void test_parse_if_conditional()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("if([amount] > 1000, 'High', 'Normal')"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(errors.empty());
    ASSERT_EQ(ast->type, ASTNodeType::FunctionCall);
    ASSERT_EQ(ast->stringValue, QStringLiteral("if"));
    ASSERT_EQ(ast->children.size(), 3u);
    // First child is comparison
    ASSERT_EQ(ast->children[0]->type, ASTNodeType::BinaryOp);
    ASSERT_EQ(ast->children[0]->op, QStringLiteral(">"));
}

void test_parse_unary_negative()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("-[price]"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(errors.empty());
    ASSERT_EQ(ast->type, ASTNodeType::UnaryOp);
    ASSERT_EQ(ast->op, QStringLiteral("-"));
}

void test_parse_parenthesized_expression()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("([a] + [b]) * [c]"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_EQ(ast->type, ASTNodeType::BinaryOp);
    ASSERT_EQ(ast->op, QStringLiteral("*"));
    ASSERT_EQ(ast->children[0]->type, ASTNodeType::BinaryOp);
    ASSERT_EQ(ast->children[0]->op, QStringLiteral("+"));
}

void test_parse_empty_formula()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral(""), errors);
    ASSERT_TRUE(ast == nullptr);
    ASSERT_TRUE(!errors.empty());
}

void test_extract_column_refs()
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(QStringLiteral("([price] * [qty]) - coalesce([discount], 0)"), errors);
    ASSERT_TRUE(ast != nullptr);
    auto refs = extractColumnRefs(ast);
    ASSERT_EQ(refs.size(), 3);
    ASSERT_TRUE(refs.contains(QStringLiteral("price")));
    ASSERT_TRUE(refs.contains(QStringLiteral("qty")));
    ASSERT_TRUE(refs.contains(QStringLiteral("discount")));
}

void test_known_functions()
{
    auto funcs = knownFunctions();
    ASSERT_TRUE(funcs.contains(QStringLiteral("concat")));
    ASSERT_TRUE(funcs.contains(QStringLiteral("if")));
    ASSERT_TRUE(funcs.contains(QStringLiteral("coalesce")));
    ASSERT_TRUE(funcs.contains(QStringLiteral("upper")));
    ASSERT_TRUE(funcs.contains(QStringLiteral("year")));
    ASSERT_TRUE(funcs.size() > 20);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_tokenize_simple_arithmetic();
    test_tokenize_function_call();
    test_tokenize_comparison_operators();
    test_tokenize_not_equal();
    test_tokenize_string_with_escaped_quotes();
    test_parse_simple_arithmetic();
    test_parse_operator_precedence();
    test_parse_function_call();
    test_parse_nested_function();
    test_parse_if_conditional();
    test_parse_unary_negative();
    test_parse_parenthesized_expression();
    test_parse_empty_formula();
    test_extract_column_refs();
    test_known_functions();

    std::cout << "\n=== Formula Parser Tests ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
