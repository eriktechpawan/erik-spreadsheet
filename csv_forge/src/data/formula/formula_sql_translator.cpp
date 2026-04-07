#include "data/formula/formula_sql_translator.h"
#include "utils/string_utils.h"

namespace csvforge {

FormulaSQLTranslator::FormulaSQLTranslator() = default;

QString FormulaSQLTranslator::translate(const ASTNodePtr& node) const
{
    if (!node) return {};
    return translateNode(node);
}

QString FormulaSQLTranslator::translateFormula(const QString& formula) const
{
    std::vector<FormulaError> errors;
    auto ast = parseFormula(formula, errors);
    if (!ast || !errors.empty()) return {};
    return translateNode(ast);
}

// ---------------------------------------------------------------------------
// Core translation
// ---------------------------------------------------------------------------

QString FormulaSQLTranslator::translateNode(const ASTNodePtr& node) const
{
    if (!node) return QStringLiteral("NULL");

    switch (node->type) {
    case ASTNodeType::NumberLiteral:
        return QString::number(node->numericValue, 'g', 15);

    case ASTNodeType::StringLiteral:
        return QStringLiteral("'%1'").arg(
            QString(node->stringValue).replace(QLatin1Char('\''), QLatin1String("''")));

    case ASTNodeType::ColumnRef:
        return quoteColumn(node->stringValue);

    case ASTNodeType::BinaryOp: {
        if (node->children.size() != 2) return QStringLiteral("NULL");
        const QString left = translateNode(node->children[0]);
        const QString right = translateNode(node->children[1]);

        // Map user operators to SQL
        QString sqlOp = node->op;
        if (sqlOp == QLatin1String("=")) sqlOp = QStringLiteral("=");
        if (sqlOp == QLatin1String("!=") || sqlOp == QLatin1String("<>"))
            sqlOp = QStringLiteral("<>");

        return QStringLiteral("(%1 %2 %3)").arg(left, sqlOp, right);
    }

    case ASTNodeType::UnaryOp: {
        if (node->children.empty()) return QStringLiteral("NULL");
        return QStringLiteral("(%1%2)").arg(node->op, translateNode(node->children[0]));
    }

    case ASTNodeType::FunctionCall:
        return translateFunctionCall(node);

    default:
        return QStringLiteral("NULL");
    }
}

// ---------------------------------------------------------------------------
// Function translation
// ---------------------------------------------------------------------------

QString FormulaSQLTranslator::translateFunctionCall(const ASTNodePtr& node) const
{
    const QString fn = node->stringValue.toLower();
    QStringList args;
    args.reserve(static_cast<int>(node->children.size()));
    for (const auto& child : node->children) {
        args << translateNode(child);
    }

    // Direct DuckDB functions (same name)
    static const QStringList directFuncs = {
        QStringLiteral("abs"), QStringLiteral("round"), QStringLiteral("ceil"),
        QStringLiteral("floor"), QStringLiteral("sqrt"), QStringLiteral("power"),
        QStringLiteral("log"), QStringLiteral("trim"), QStringLiteral("upper"),
        QStringLiteral("lower"), QStringLiteral("length"), QStringLiteral("replace"),
        QStringLiteral("coalesce"), QStringLiteral("nullif"),
        QStringLiteral("year"), QStringLiteral("month"), QStringLiteral("day"),
        QStringLiteral("date_trunc"), QStringLiteral("strftime"),
    };

    if (directFuncs.contains(fn)) {
        return QStringLiteral("%1(%2)").arg(fn, args.join(QStringLiteral(", ")));
    }

    // concat → DuckDB concat()
    if (fn == QLatin1String("concat")) {
        return QStringLiteral("concat(%1)").arg(args.join(QStringLiteral(", ")));
    }

    // substring → DuckDB substring(str, start, length)
    if (fn == QLatin1String("substring")) {
        return QStringLiteral("substring(%1)").arg(args.join(QStringLiteral(", ")));
    }

    // left / right
    if (fn == QLatin1String("left") && args.size() == 2) {
        return QStringLiteral("left(%1, %2)").arg(args[0], args[1]);
    }
    if (fn == QLatin1String("right") && args.size() == 2) {
        return QStringLiteral("right(%1, %2)").arg(args[0], args[1]);
    }

    // lpad / rpad
    if (fn == QLatin1String("lpad")) {
        return QStringLiteral("lpad(%1)").arg(args.join(QStringLiteral(", ")));
    }
    if (fn == QLatin1String("rpad")) {
        return QStringLiteral("rpad(%1)").arg(args.join(QStringLiteral(", ")));
    }

    // mod → DuckDB mod()
    if (fn == QLatin1String("mod") && args.size() == 2) {
        return QStringLiteral("(%1 %% %2)").arg(args[0], args[1]);
    }

    // ifnull → coalesce
    if (fn == QLatin1String("ifnull") && args.size() == 2) {
        return QStringLiteral("coalesce(%1, %2)").arg(args[0], args[1]);
    }

    // if(cond, true_val, false_val) → CASE WHEN cond THEN true_val ELSE false_val END
    if (fn == QLatin1String("if") && args.size() == 3) {
        return QStringLiteral("CASE WHEN %1 THEN %2 ELSE %3 END")
            .arg(args[0], args[1], args[2]);
    }

    // case_when(cond1, val1, cond2, val2, ..., default)
    // → CASE WHEN cond1 THEN val1 WHEN cond2 THEN val2 ... ELSE default END
    if (fn == QLatin1String("case_when") && args.size() >= 2) {
        QString sql = QStringLiteral("CASE");
        int i = 0;
        while (i + 1 < args.size()) {
            // Last single arg is the ELSE default
            if (i + 2 == args.size() && (args.size() % 2 != 0)) {
                break;
            }
            sql += QStringLiteral(" WHEN %1 THEN %2").arg(args[i], args[i + 1]);
            i += 2;
        }
        // Remaining single arg is ELSE
        if (i < args.size()) {
            sql += QStringLiteral(" ELSE %1").arg(args[i]);
        }
        sql += QStringLiteral(" END");
        return sql;
    }

    // date_diff('part', start, end) → date_diff('part', start, end) (DuckDB native)
    if (fn == QLatin1String("date_diff") && args.size() == 3) {
        return QStringLiteral("date_diff(%1, %2, %3)").arg(args[0], args[1], args[2]);
    }

    // current_date
    if (fn == QLatin1String("current_date")) {
        return QStringLiteral("current_date");
    }

    // Cast functions
    if (fn == QLatin1String("cast_int") && args.size() == 1) {
        return QStringLiteral("CAST(%1 AS INTEGER)").arg(args[0]);
    }
    if (fn == QLatin1String("cast_double") && args.size() == 1) {
        return QStringLiteral("CAST(%1 AS DOUBLE)").arg(args[0]);
    }
    if (fn == QLatin1String("cast_text") && args.size() == 1) {
        return QStringLiteral("CAST(%1 AS VARCHAR)").arg(args[0]);
    }
    if (fn == QLatin1String("cast_date") && args.size() == 1) {
        return QStringLiteral("CAST(%1 AS DATE)").arg(args[0]);
    }

    // Fallback: pass through as-is
    return QStringLiteral("%1(%2)").arg(fn, args.join(QStringLiteral(", ")));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString FormulaSQLTranslator::quoteColumn(const QString& name) const
{
    return quoteName(name);
}

} // namespace csvforge
