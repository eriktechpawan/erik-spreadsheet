#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <vector>

namespace csvforge {

/// Output type inferred for a calculated column.
enum class FormulaOutputType {
    Text,
    Numeric,
    Boolean,
    Date,
    Unknown
};

/// Definition of a single calculated column.
struct CalculatedColumnDef {
    QString columnName;         ///< Output column name
    QString formulaText;        ///< Raw user formula, e.g. "[price] * [qty]"
    FormulaOutputType outputType = FormulaOutputType::Unknown;
    QStringList referencedColumns; ///< Columns used in the formula

    QJsonObject toJson() const;
    static CalculatedColumnDef fromJson(const QJsonObject& obj);
    bool isValid() const;
};

/// Container for all calculated columns in a dataset.
struct FormulaConfig {
    std::vector<CalculatedColumnDef> columns;

    QJsonObject toJson() const;
    static FormulaConfig fromJson(const QJsonObject& obj);
    bool isEmpty() const;
};

/// Convert output type to/from string.
QString formulaOutputTypeToString(FormulaOutputType t);
FormulaOutputType formulaOutputTypeFromString(const QString& s);

} // namespace csvforge
