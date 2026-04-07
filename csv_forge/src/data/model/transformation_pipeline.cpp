#include "data/model/transformation_pipeline.h"
#include <QJsonArray>

namespace csvforge {

QString TransformationPipeline::toSummaryText() const
{
    if (steps.empty()) return QStringLiteral("(empty pipeline)");

    QStringList lines;
    int i = 1;
    for (const auto& step : steps) {
        lines << QStringLiteral("%1. [%2] %3")
                      .arg(i++)
                      .arg(transformationTypeToString(step.type), step.description);
    }
    return lines.join(QLatin1Char('\n'));
}

QString TransformationPipeline::toSQLPreview() const
{
    // Gather key info from steps to produce a readable SQL-like summary.
    QString source;
    QStringList filters;
    QStringList sortClauses;
    QStringList calcColumns;

    for (const auto& step : steps) {
        switch (step.type) {
        case TransformationType::Source:
            source = step.config[QStringLiteral("tableName")].toString();
            break;
        case TransformationType::Filter:
            filters << step.description;
            break;
        case TransformationType::Sort: {
            const QJsonArray arr = step.config[QStringLiteral("sorts")].toArray();
            for (const auto& v : arr) {
                const QJsonObject o = v.toObject();
                sortClauses << QStringLiteral("%1 %2")
                                   .arg(o[QStringLiteral("column")].toString(),
                                        o[QStringLiteral("ascending")].toBool()
                                            ? QStringLiteral("ASC")
                                            : QStringLiteral("DESC"));
            }
            break;
        }
        case TransformationType::CalculatedColumn:
            calcColumns << QStringLiteral("%1 AS %2")
                               .arg(step.config[QStringLiteral("formulaText")].toString(),
                                    step.config[QStringLiteral("columnName")].toString());
            break;
        case TransformationType::Pivot:
            return QStringLiteral("-- Pivot on %1\n%2")
                .arg(step.config[QStringLiteral("sourceTable")].toString(),
                     step.description);
        case TransformationType::Lookup:
            return QStringLiteral("-- Lookup join\n%1").arg(step.description);
        default:
            break;
        }
    }

    // Build a SELECT statement
    QString sql = QStringLiteral("SELECT *");
    if (!calcColumns.isEmpty()) {
        sql += QStringLiteral(", ") + calcColumns.join(QStringLiteral(", "));
    }
    if (!source.isEmpty()) {
        sql += QStringLiteral("\nFROM ") + source;
    }
    if (!filters.isEmpty()) {
        sql += QStringLiteral("\n-- ") + filters.join(QStringLiteral("; "));
    }
    if (!sortClauses.isEmpty()) {
        sql += QStringLiteral("\nORDER BY ") + sortClauses.join(QStringLiteral(", "));
    }

    return sql;
}

void TransformationPipeline::addStep(const TransformationStep& step)
{
    steps.push_back(step);
}

void TransformationPipeline::removeLastStep()
{
    if (!steps.empty()) steps.pop_back();
}

void TransformationPipeline::clear()
{
    steps.clear();
}

bool TransformationPipeline::isEmpty() const
{
    return steps.empty();
}

int TransformationPipeline::stepCount() const
{
    return static_cast<int>(steps.size());
}

QJsonObject TransformationPipeline::toJson() const
{
    QJsonObject obj;
    QJsonArray arr;
    for (const auto& step : steps) {
        arr.append(step.toJson());
    }
    obj[QStringLiteral("steps")] = arr;
    return obj;
}

TransformationPipeline TransformationPipeline::fromJson(const QJsonObject& obj)
{
    TransformationPipeline pipeline;
    const QJsonArray arr = obj[QStringLiteral("steps")].toArray();
    for (const auto& v : arr) {
        pipeline.steps.push_back(TransformationStep::fromJson(v.toObject()));
    }
    return pipeline;
}

} // namespace csvforge
