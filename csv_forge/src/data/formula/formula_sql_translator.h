#pragma once

#include <QString>

#include "data/formula/formula_parser.h"

namespace csvforge {

/// Translates a formula AST into a DuckDB-compatible SQL expression.
class FormulaSQLTranslator {
public:
    FormulaSQLTranslator();

    /// Translate an AST to a SQL expression string.
    QString translate(const ASTNodePtr& node) const;

    /// Convenience: parse + translate a formula string.
    /// Returns empty string on parse error.
    QString translateFormula(const QString& formula) const;

private:
    QString translateNode(const ASTNodePtr& node) const;
    QString translateFunctionCall(const ASTNodePtr& node) const;
    QString quoteColumn(const QString& name) const;
};

} // namespace csvforge
