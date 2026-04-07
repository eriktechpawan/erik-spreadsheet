#include <QCoreApplication>
#include <QJsonObject>
#include <iostream>

#include "data/types/lookup_config.h"

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

void test_lookup_config_valid()
{
    LookupConfig cfg;
    cfg.sourceTable = QStringLiteral("orders");
    cfg.sourceKeyColumn = QStringLiteral("customer_id");
    cfg.targetTable = QStringLiteral("customers");
    cfg.targetKeyColumn = QStringLiteral("id");
    cfg.returnColumns << QStringLiteral("name") << QStringLiteral("email");
    ASSERT_TRUE(cfg.isValid());
}

void test_lookup_config_valid_with_file()
{
    LookupConfig cfg;
    cfg.sourceTable = QStringLiteral("orders");
    cfg.sourceKeyColumn = QStringLiteral("customer_id");
    cfg.targetFile = QStringLiteral("/path/to/customers.csv");
    cfg.targetKeyColumn = QStringLiteral("id");
    ASSERT_TRUE(cfg.isValid());
}

void test_lookup_config_invalid_no_source()
{
    LookupConfig cfg;
    cfg.sourceKeyColumn = QStringLiteral("customer_id");
    cfg.targetTable = QStringLiteral("customers");
    cfg.targetKeyColumn = QStringLiteral("id");
    ASSERT_TRUE(!cfg.isValid());
}

void test_lookup_config_invalid_no_target()
{
    LookupConfig cfg;
    cfg.sourceTable = QStringLiteral("orders");
    cfg.sourceKeyColumn = QStringLiteral("customer_id");
    cfg.targetKeyColumn = QStringLiteral("id");
    // Neither targetFile nor targetTable
    ASSERT_TRUE(!cfg.isValid());
}

void test_lookup_config_serialization()
{
    LookupConfig cfg;
    cfg.sourceTable = QStringLiteral("orders");
    cfg.sourceKeyColumn = QStringLiteral("customer_id");
    cfg.targetFile = QStringLiteral("/path/to/customers.csv");
    cfg.targetTable = QStringLiteral("customers");
    cfg.targetKeyColumn = QStringLiteral("id");
    cfg.returnColumns << QStringLiteral("name") << QStringLiteral("email");
    cfg.resultTableName = QStringLiteral("lookup_result");
    cfg.trimWhitespace = true;
    cfg.caseInsensitive = true;
    cfg.createNewTab = false;

    cfg.returnColumnsDetailed.push_back({QStringLiteral("name"), QStringLiteral("customer_name")});
    cfg.returnColumnsDetailed.push_back({QStringLiteral("email"), {}});

    QJsonObject json = cfg.toJson();
    LookupConfig restored = LookupConfig::fromJson(json);

    ASSERT_EQ(restored.sourceTable, cfg.sourceTable);
    ASSERT_EQ(restored.sourceKeyColumn, cfg.sourceKeyColumn);
    ASSERT_EQ(restored.targetFile, cfg.targetFile);
    ASSERT_EQ(restored.targetTable, cfg.targetTable);
    ASSERT_EQ(restored.targetKeyColumn, cfg.targetKeyColumn);
    ASSERT_EQ(restored.returnColumns.size(), 2);
    ASSERT_EQ(restored.returnColumns[0], QStringLiteral("name"));
    ASSERT_EQ(restored.resultTableName, QStringLiteral("lookup_result"));
    ASSERT_TRUE(restored.trimWhitespace);
    ASSERT_TRUE(restored.caseInsensitive);
    ASSERT_TRUE(!restored.createNewTab);
    ASSERT_EQ(static_cast<int>(restored.returnColumnsDetailed.size()), 2);
    ASSERT_EQ(restored.returnColumnsDetailed[0].sourceColumn, QStringLiteral("name"));
    ASSERT_EQ(restored.returnColumnsDetailed[0].outputAlias, QStringLiteral("customer_name"));
}

void test_lookup_return_column()
{
    LookupReturnColumn rc;
    rc.sourceColumn = QStringLiteral("name");
    rc.outputAlias = QStringLiteral("customer_name");
    ASSERT_EQ(rc.sourceColumn, QStringLiteral("name"));
    ASSERT_EQ(rc.outputAlias, QStringLiteral("customer_name"));
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_lookup_config_valid();
    test_lookup_config_valid_with_file();
    test_lookup_config_invalid_no_source();
    test_lookup_config_invalid_no_target();
    test_lookup_config_serialization();
    test_lookup_return_column();

    std::cout << "\n=== Lookup Config Tests ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
