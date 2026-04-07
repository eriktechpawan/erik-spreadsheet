#include <QCoreApplication>
#include <QJsonObject>
#include <iostream>

#include "data/types/pivot_config.h"

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

void test_pivot_config_valid()
{
    PivotConfig cfg;
    cfg.sourceTable = QStringLiteral("sales");
    cfg.columnFields << QStringLiteral("region");
    cfg.valueFields.push_back({QStringLiteral("revenue"), PivotAggregation::Sum, {}});
    ASSERT_TRUE(cfg.isValid());
}

void test_pivot_config_invalid_no_source()
{
    PivotConfig cfg;
    cfg.columnFields << QStringLiteral("region");
    cfg.valueFields.push_back({QStringLiteral("revenue"), PivotAggregation::Sum, {}});
    ASSERT_TRUE(!cfg.isValid());
}

void test_pivot_config_invalid_no_column_fields()
{
    PivotConfig cfg;
    cfg.sourceTable = QStringLiteral("sales");
    cfg.valueFields.push_back({QStringLiteral("revenue"), PivotAggregation::Sum, {}});
    ASSERT_TRUE(!cfg.isValid());
}

void test_pivot_config_serialization()
{
    PivotConfig cfg;
    cfg.sourceTable = QStringLiteral("sales");
    cfg.rowFields << QStringLiteral("category") << QStringLiteral("product");
    cfg.columnFields << QStringLiteral("region");
    cfg.valueFields.push_back({QStringLiteral("revenue"), PivotAggregation::Sum, QStringLiteral("Total Revenue")});
    cfg.valueFields.push_back({QStringLiteral("orders"), PivotAggregation::Count, {}});
    cfg.includeGrandTotals = true;
    cfg.sortByValue = true;
    cfg.previewLimit = 50;

    QJsonObject json = cfg.toJson();
    PivotConfig restored = PivotConfig::fromJson(json);

    ASSERT_EQ(restored.sourceTable, cfg.sourceTable);
    ASSERT_EQ(restored.rowFields.size(), 2);
    ASSERT_EQ(restored.rowFields[0], QStringLiteral("category"));
    ASSERT_EQ(restored.columnFields.size(), 1);
    ASSERT_EQ(static_cast<int>(restored.valueFields.size()), 2);
    ASSERT_EQ(restored.valueFields[0].columnName, QStringLiteral("revenue"));
    ASSERT_TRUE(restored.valueFields[0].aggregation == PivotAggregation::Sum);
    ASSERT_EQ(restored.valueFields[0].alias, QStringLiteral("Total Revenue"));
    ASSERT_TRUE(restored.valueFields[1].aggregation == PivotAggregation::Count);
    ASSERT_TRUE(restored.includeGrandTotals);
    ASSERT_TRUE(restored.sortByValue);
    ASSERT_EQ(restored.previewLimit, 50);
}

void test_pivot_value_field_to_sql()
{
    PivotValueField vf;
    vf.columnName = QStringLiteral("revenue");
    vf.aggregation = PivotAggregation::Sum;
    QString sql = vf.toSQL();
    ASSERT_TRUE(sql.contains(QStringLiteral("SUM")));
    ASSERT_TRUE(sql.contains(QStringLiteral("revenue")));
}

void test_pivot_value_field_display_name_with_alias()
{
    PivotValueField vf;
    vf.columnName = QStringLiteral("revenue");
    vf.aggregation = PivotAggregation::Sum;
    vf.alias = QStringLiteral("Total");
    ASSERT_EQ(vf.displayName(), QStringLiteral("Total"));
}

void test_pivot_value_field_display_name_without_alias()
{
    PivotValueField vf;
    vf.columnName = QStringLiteral("revenue");
    vf.aggregation = PivotAggregation::Average;
    ASSERT_TRUE(vf.displayName().contains(QStringLiteral("AVG")));
}

void test_aggregation_string_conversion()
{
    ASSERT_EQ(pivotAggregationToString(PivotAggregation::Sum), QStringLiteral("SUM"));
    ASSERT_EQ(pivotAggregationToString(PivotAggregation::CountDistinct), QStringLiteral("COUNT_DISTINCT"));
    ASSERT_TRUE(pivotAggregationFromString(QStringLiteral("AVG")) == PivotAggregation::Average);
    ASSERT_TRUE(pivotAggregationFromString(QStringLiteral("COUNT_DISTINCT")) == PivotAggregation::CountDistinct);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_pivot_config_valid();
    test_pivot_config_invalid_no_source();
    test_pivot_config_invalid_no_column_fields();
    test_pivot_config_serialization();
    test_pivot_value_field_to_sql();
    test_pivot_value_field_display_name_with_alias();
    test_pivot_value_field_display_name_without_alias();
    test_aggregation_string_conversion();

    std::cout << "\n=== Pivot Config Tests ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
