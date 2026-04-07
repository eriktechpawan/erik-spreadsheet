#include <QCoreApplication>
#include <QJsonObject>
#include <iostream>

#include "data/model/transformation_pipeline.h"
#include "data/model/transformation_step.h"
#include "data/types/formula_config.h"

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
        std::cerr << "FAIL: " << #a << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; \
    } else { tests_passed++; } \
} while(0)

void test_empty_pipeline()
{
    TransformationPipeline pipeline;
    ASSERT_TRUE(pipeline.isEmpty());
    ASSERT_EQ(pipeline.stepCount(), 0);
}

void test_add_steps()
{
    TransformationPipeline pipeline;
    pipeline.addStep(TransformationStep::makeSource(
        QStringLiteral("/data/sales.csv"), QStringLiteral("sales")));
    ASSERT_EQ(pipeline.stepCount(), 1);
    ASSERT_TRUE(!pipeline.isEmpty());
    ASSERT_TRUE(pipeline.steps[0].type == TransformationType::Source);
}

void test_remove_last_step()
{
    TransformationPipeline pipeline;
    pipeline.addStep(TransformationStep::makeSource(
        QStringLiteral("/data/sales.csv"), QStringLiteral("sales")));
    pipeline.addStep(TransformationStep::makeHideColumns({QStringLiteral("col1")}));
    ASSERT_EQ(pipeline.stepCount(), 2);

    pipeline.removeLastStep();
    ASSERT_EQ(pipeline.stepCount(), 1);
}

void test_clear_pipeline()
{
    TransformationPipeline pipeline;
    pipeline.addStep(TransformationStep::makeSource(
        QStringLiteral("/data/sales.csv"), QStringLiteral("sales")));
    pipeline.clear();
    ASSERT_TRUE(pipeline.isEmpty());
}

void test_pipeline_serialization()
{
    TransformationPipeline pipeline;
    pipeline.addStep(TransformationStep::makeSource(
        QStringLiteral("/data/sales.csv"), QStringLiteral("sales")));

    CalculatedColumnDef calcDef;
    calcDef.columnName = QStringLiteral("total");
    calcDef.formulaText = QStringLiteral("[price] * [qty]");
    pipeline.addStep(TransformationStep::makeCalculatedColumn(calcDef));
    pipeline.addStep(TransformationStep::makeHideColumns(
        {QStringLiteral("internal_col")}));

    QJsonObject json = pipeline.toJson();
    TransformationPipeline restored = TransformationPipeline::fromJson(json);

    ASSERT_EQ(restored.stepCount(), 3);
    ASSERT_TRUE(restored.steps[0].type == TransformationType::Source);
    ASSERT_TRUE(restored.steps[1].type == TransformationType::CalculatedColumn);
    ASSERT_TRUE(restored.steps[2].type == TransformationType::HideColumns);
}

void test_step_source_factory()
{
    auto step = TransformationStep::makeSource(
        QStringLiteral("/data/orders.csv"), QStringLiteral("orders"));
    ASSERT_TRUE(step.type == TransformationType::Source);
    ASSERT_TRUE(step.config.contains(QStringLiteral("filePath")));
    ASSERT_TRUE(step.config.contains(QStringLiteral("tableName")));
    ASSERT_TRUE(!step.description.isEmpty());
}

void test_step_hide_columns_factory()
{
    auto step = TransformationStep::makeHideColumns(
        {QStringLiteral("col1"), QStringLiteral("col2")});
    ASSERT_TRUE(step.type == TransformationType::HideColumns);
    ASSERT_TRUE(step.description.contains(QStringLiteral("2")));
}

void test_step_calculated_column_factory()
{
    CalculatedColumnDef def;
    def.columnName = QStringLiteral("profit");
    def.formulaText = QStringLiteral("[revenue] - [cost]");
    auto step = TransformationStep::makeCalculatedColumn(def);
    ASSERT_TRUE(step.type == TransformationType::CalculatedColumn);
    ASSERT_TRUE(step.description.contains(QStringLiteral("profit")));
}

void test_step_serialization()
{
    auto step = TransformationStep::makeSource(
        QStringLiteral("/data/test.csv"), QStringLiteral("test"));
    QJsonObject json = step.toJson();
    auto restored = TransformationStep::fromJson(json);
    ASSERT_TRUE(restored.type == TransformationType::Source);
    ASSERT_EQ(restored.config[QStringLiteral("filePath")].toString(),
              QStringLiteral("/data/test.csv"));
}

void test_type_string_conversion()
{
    ASSERT_EQ(transformationTypeToString(TransformationType::Source),
              QStringLiteral("Source"));
    ASSERT_EQ(transformationTypeToString(TransformationType::Pivot),
              QStringLiteral("Pivot"));
    ASSERT_EQ(transformationTypeToString(TransformationType::Lookup),
              QStringLiteral("Lookup"));
    ASSERT_EQ(transformationTypeToString(TransformationType::CalculatedColumn),
              QStringLiteral("CalculatedColumn"));

    ASSERT_TRUE(transformationTypeFromString(QStringLiteral("Filter"))
                == TransformationType::Filter);
    ASSERT_TRUE(transformationTypeFromString(QStringLiteral("Sort"))
                == TransformationType::Sort);
}

void test_pipeline_summary_text()
{
    TransformationPipeline pipeline;
    pipeline.addStep(TransformationStep::makeSource(
        QStringLiteral("test.csv"), QStringLiteral("test")));
    pipeline.addStep(TransformationStep::makeHideColumns({QStringLiteral("x")}));

    QString summary = pipeline.toSummaryText();
    ASSERT_TRUE(!summary.isEmpty());
    ASSERT_TRUE(summary.contains(QStringLiteral("#1")));
    ASSERT_TRUE(summary.contains(QStringLiteral("#2")));
}

void test_pipeline_sql_preview()
{
    TransformationPipeline pipeline;
    pipeline.addStep(TransformationStep::makeSource(
        QStringLiteral("test.csv"), QStringLiteral("test")));

    QString sql = pipeline.toSQLPreview();
    ASSERT_TRUE(!sql.isEmpty());
    ASSERT_TRUE(sql.contains(QStringLiteral("SELECT")));
    ASSERT_TRUE(sql.contains(QStringLiteral("test")));
}

void test_formula_config_serialization()
{
    FormulaConfig cfg;
    CalculatedColumnDef def;
    def.columnName = QStringLiteral("total");
    def.formulaText = QStringLiteral("[price] * [qty]");
    def.outputType = FormulaOutputType::Numeric;
    def.referencedColumns = {QStringLiteral("price"), QStringLiteral("qty")};
    cfg.columns.push_back(def);

    QJsonObject json = cfg.toJson();
    FormulaConfig restored = FormulaConfig::fromJson(json);

    ASSERT_EQ(static_cast<int>(restored.columns.size()), 1);
    ASSERT_EQ(restored.columns[0].columnName, QStringLiteral("total"));
    ASSERT_EQ(restored.columns[0].formulaText, QStringLiteral("[price] * [qty]"));
    ASSERT_TRUE(restored.columns[0].outputType == FormulaOutputType::Numeric);
    ASSERT_EQ(restored.columns[0].referencedColumns.size(), 2);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_empty_pipeline();
    test_add_steps();
    test_remove_last_step();
    test_clear_pipeline();
    test_pipeline_serialization();
    test_step_source_factory();
    test_step_hide_columns_factory();
    test_step_calculated_column_factory();
    test_step_serialization();
    test_type_string_conversion();
    test_pipeline_summary_text();
    test_pipeline_sql_preview();
    test_formula_config_serialization();

    std::cout << "\n=== Transformation Pipeline Tests ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
