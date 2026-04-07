#pragma once

#include <QString>
#include <QStringList>
#include <vector>

#include "data/formula/formula_parser.h"
#include "data/types/column_schema.h"
#include "data/types/formula_config.h"

namespace csvforge {

/// Validates formulas against a column schema and infers output types.
class FormulaEngine {
public:
    FormulaEngine();

    /// Set the available columns for validation.
    void setSchema(const ColumnSchemaList& schema);

    /// Validate a formula. Returns true if valid.
    bool validate(const QString& formula, std::vector<FormulaError>& errors) const;

    /// Parse, validate, and return the AST.
    ASTNodePtr parseAndValidate(const QString& formula,
                                std::vector<FormulaError>& errors) const;

    /// Infer the output type of an AST node.
    FormulaOutputType inferType(const ASTNodePtr& node) const;

    /// Build a CalculatedColumnDef from a formula string.
    CalculatedColumnDef buildDefinition(const QString& columnName,
                                        const QString& formula,
                                        std::vector<FormulaError>& errors) const;

    /// Return the list of known function names.
    static QStringList availableFunctions();

    /// Return a brief description of a function.
    static QString functionHelp(const QString& funcName);

private:
    ColumnSchemaList m_schema;

    void validateNode(const ASTNodePtr& node,
                      std::vector<FormulaError>& errors) const;
    ColumnType columnTypeByName(const QString& name) const;
};

} // namespace csvforge
