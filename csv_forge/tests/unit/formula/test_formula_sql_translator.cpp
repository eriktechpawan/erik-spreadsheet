#include <QCoreApplication>
#include <iostream>
#include <string>

#include "data/formula/formula_engine.h"
#include "data/formula/formula_parser.h"
#include "data/formula/formula_sql_translator.h"
#include "data/types/column_schema.h"

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
        std::cerr << "FAIL: " << #a << " == '" << (a).toStdString() << "' != '" << (b).toStdString() << "' at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; \
    } else { tests_passed++; } \
} while(0)

FormulaSQLTranslator translator;

void test_translate_simple_arithmetic()
{
    auto sql = translator.translateFormula(QStringLiteral("[price] * [qty]"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("\"price\"")));
    ASSERT_TRUE(sql.contains(QStringLiteral("\"qty\"")));
    ASSERT_TRUE(sql.contains(QStringLiteral("*")));
}

void test_translate_function_upper()
{
    auto sql = translator.translateFormula(QStringLiteral("upper([name])"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_EQ(sql, QStringLiteral("upper(\"name\")"));
}

void test_translate_concat()
{
    auto sql = translator.translateFormula(QStringLiteral("concat([first], ' ', [last])"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("concat(")));
    ASSERT_TRUE(sql.contains(QStringLiteral("' '")));
}

void test_translate_if_to_case()
{
    auto sql = translator.translateFormula(QStringLiteral("if([amount] > 1000, 'High', 'Normal')"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("CASE WHEN")));
    ASSERT_TRUE(sql.contains(QStringLiteral("THEN")));
    ASSERT_TRUE(sql.contains(QStringLiteral("ELSE")));
    ASSERT_TRUE(sql.contains(QStringLiteral("END")));
}

void test_translate_coalesce()
{
    auto sql = translator.translateFormula(QStringLiteral("coalesce([discount], 0)"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("coalesce(")));
}

void test_translate_ifnull_to_coalesce()
{
    auto sql = translator.translateFormula(QStringLiteral("ifnull([val], 0)"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("coalesce(")));
}

void test_translate_cast_int()
{
    auto sql = translator.translateFormula(QStringLiteral("cast_int([text_col])"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("CAST(")));
    ASSERT_TRUE(sql.contains(QStringLiteral("INTEGER")));
}

void test_translate_cast_double()
{
    auto sql = translator.translateFormula(QStringLiteral("cast_double([text_col])"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("DOUBLE")));
}

void test_translate_case_when()
{
    auto sql = translator.translateFormula(
        QStringLiteral("case_when([score] >= 90, 'A', [score] >= 80, 'B', 'C')"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("CASE")));
    ASSERT_TRUE(sql.contains(QStringLiteral("WHEN")));
    ASSERT_TRUE(sql.contains(QStringLiteral("ELSE")));
    ASSERT_TRUE(sql.contains(QStringLiteral("END")));
}

void test_translate_date_functions()
{
    auto sql = translator.translateFormula(QStringLiteral("year([order_date])"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("year(")));
}

void test_translate_complex_expression()
{
    auto sql = translator.translateFormula(
        QStringLiteral("([subtotal] - coalesce([discount], 0)) * 1.1"));
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("coalesce(")));
    ASSERT_TRUE(sql.contains(QStringLiteral("1.1")));
}

void test_translate_invalid_formula()
{
    auto sql = translator.translateFormula(QStringLiteral(""));
    ASSERT_TRUE(sql.isEmpty());
}

void test_translate_string_literal()
{
    auto sql = translator.translateFormula(QStringLiteral("'hello'"));
    ASSERT_EQ(sql, QStringLiteral("'hello'"));
}

void test_translate_number_literal()
{
    auto sql = translator.translateFormula(QStringLiteral("42.5"));
    ASSERT_EQ(sql, QStringLiteral("42.5"));
}

void test_engine_validate()
{
    FormulaEngine engine;
    ColumnSchemaList schema;
    schema.push_back({0, QStringLiteral("price"), ColumnType::Double, {}, true, 0, 120, false});
    schema.push_back({1, QStringLiteral("qty"), ColumnType::Integer, {}, true, 1, 120, false});
    engine.setSchema(schema);

    std::vector<FormulaError> errors;
    bool valid = engine.validate(QStringLiteral("[price] * [qty]"), errors);
    ASSERT_TRUE(valid);
    ASSERT_TRUE(errors.empty());
}

void test_engine_validate_unknown_column()
{
    FormulaEngine engine;
    ColumnSchemaList schema;
    schema.push_back({0, QStringLiteral("price"), ColumnType::Double, {}, true, 0, 120, false});
    engine.setSchema(schema);

    std::vector<FormulaError> errors;
    bool valid = engine.validate(QStringLiteral("[price] * [nonexistent]"), errors);
    ASSERT_TRUE(!valid);
    ASSERT_TRUE(!errors.empty());
}

void test_engine_validate_unknown_function()
{
    FormulaEngine engine;
    ColumnSchemaList schema;
    engine.setSchema(schema);

    std::vector<FormulaError> errors;
    bool valid = engine.validate(QStringLiteral("badvfunc(1)"), errors);
    ASSERT_TRUE(!valid);
}

void test_engine_validate_wrong_arg_count()
{
    FormulaEngine engine;
    ColumnSchemaList schema;
    engine.setSchema(schema);

    std::vector<FormulaError> errors;
    bool valid = engine.validate(QStringLiteral("abs(1, 2)"), errors);
    ASSERT_TRUE(!valid);
}

void test_engine_infer_type_numeric()
{
    FormulaEngine engine;
    ColumnSchemaList schema;
    schema.push_back({0, QStringLiteral("price"), ColumnType::Double, {}, true, 0, 120, false});
    schema.push_back({1, QStringLiteral("qty"), ColumnType::Integer, {}, true, 1, 120, false});
    engine.setSchema(schema);

    std::vector<FormulaError> errors;
    auto ast = engine.parseAndValidate(QStringLiteral("[price] * [qty]"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(engine.inferType(ast) == FormulaOutputType::Numeric);
}

void test_engine_infer_type_text()
{
    FormulaEngine engine;
    ColumnSchemaList schema;
    schema.push_back({0, QStringLiteral("name"), ColumnType::Text, {}, true, 0, 120, false});
    engine.setSchema(schema);

    std::vector<FormulaError> errors;
    auto ast = engine.parseAndValidate(QStringLiteral("upper([name])"), errors);
    ASSERT_TRUE(ast != nullptr);
    ASSERT_TRUE(engine.inferType(ast) == FormulaOutputType::Text);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_translate_simple_arithmetic();
    test_translate_function_upper();
    test_translate_concat();
    test_translate_if_to_case();
    test_translate_coalesce();
    test_translate_ifnull_to_coalesce();
    test_translate_cast_int();
    test_translate_cast_double();
    test_translate_case_when();
    test_translate_date_functions();
    test_translate_complex_expression();
    test_translate_invalid_formula();
    test_translate_string_literal();
    test_translate_number_literal();
    test_engine_validate();
    test_engine_validate_unknown_column();
    test_engine_validate_unknown_function();
    test_engine_validate_wrong_arg_count();
    test_engine_infer_type_numeric();
    test_engine_infer_type_text();

    std::cout << "\n=== Formula SQL Translator & Engine Tests ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
