#include "data/formula/formula_engine.h"

#include <QMap>
#include <QSet>

namespace csvforge {

FormulaEngine::FormulaEngine() = default;

void FormulaEngine::setSchema(const ColumnSchemaList& schema)
{
    m_schema = schema;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool FormulaEngine::validate(const QString& formula,
                             std::vector<FormulaError>& errors) const
{
    auto ast = parseAndValidate(formula, errors);
    return ast != nullptr && errors.empty();
}

ASTNodePtr FormulaEngine::parseAndValidate(const QString& formula,
                                           std::vector<FormulaError>& errors) const
{
    auto ast = parseFormula(formula, errors);
    if (!ast) return nullptr;

    validateNode(ast, errors);
    return errors.empty() ? ast : nullptr;
}

void FormulaEngine::validateNode(const ASTNodePtr& node,
                                 std::vector<FormulaError>& errors) const
{
    if (!node) return;

    switch (node->type) {
    case ASTNodeType::ColumnRef: {
        // Check column exists
        const QString& colName = node->stringValue;
        bool found = false;
        for (const auto& col : m_schema) {
            if (col.name == colName) { found = true; break; }
        }
        if (!found) {
            errors.push_back({-1,
                QStringLiteral("Unknown column: [%1]").arg(colName)});
        }
        break;
    }
    case ASTNodeType::FunctionCall: {
        // Check function is known
        const QString funcName = node->stringValue.toLower();
        static const QSet<QString> known = [] {
            QSet<QString> s;
            for (const auto& f : knownFunctions()) s.insert(f.toLower());
            return s;
        }();

        if (!known.contains(funcName)) {
            errors.push_back({-1,
                QStringLiteral("Unknown function: %1()").arg(funcName)});
        }

        // Argument count checks
        static const QMap<QString, QPair<int, int>> argCounts = {
            {QStringLiteral("abs"),        {1, 1}},
            {QStringLiteral("round"),      {1, 2}},
            {QStringLiteral("ceil"),       {1, 1}},
            {QStringLiteral("floor"),      {1, 1}},
            {QStringLiteral("sqrt"),       {1, 1}},
            {QStringLiteral("power"),      {2, 2}},
            {QStringLiteral("mod"),        {2, 2}},
            {QStringLiteral("log"),        {1, 1}},
            {QStringLiteral("concat"),     {1, 99}},
            {QStringLiteral("trim"),       {1, 1}},
            {QStringLiteral("upper"),      {1, 1}},
            {QStringLiteral("lower"),      {1, 1}},
            {QStringLiteral("length"),     {1, 1}},
            {QStringLiteral("substring"),  {2, 3}},
            {QStringLiteral("replace"),    {3, 3}},
            {QStringLiteral("left"),       {2, 2}},
            {QStringLiteral("right"),      {2, 2}},
            {QStringLiteral("lpad"),       {2, 3}},
            {QStringLiteral("rpad"),       {2, 3}},
            {QStringLiteral("coalesce"),   {1, 99}},
            {QStringLiteral("ifnull"),     {2, 2}},
            {QStringLiteral("nullif"),     {2, 2}},
            {QStringLiteral("if"),         {3, 3}},
            {QStringLiteral("case_when"),  {2, 99}},
            {QStringLiteral("year"),       {1, 1}},
            {QStringLiteral("month"),      {1, 1}},
            {QStringLiteral("day"),        {1, 1}},
            {QStringLiteral("date_diff"),  {3, 3}},
            {QStringLiteral("date_trunc"), {2, 2}},
            {QStringLiteral("current_date"),{0, 0}},
            {QStringLiteral("strftime"),   {2, 2}},
            {QStringLiteral("cast_int"),   {1, 1}},
            {QStringLiteral("cast_double"),{1, 1}},
            {QStringLiteral("cast_text"),  {1, 1}},
            {QStringLiteral("cast_date"),  {1, 1}},
        };

        auto it = argCounts.find(funcName);
        if (it != argCounts.end()) {
            const int argCount = static_cast<int>(node->children.size());
            if (argCount < it.value().first || argCount > it.value().second) {
                if (it.value().first == it.value().second) {
                    errors.push_back({-1,
                        QStringLiteral("%1() expects %2 argument(s), got %3")
                            .arg(funcName).arg(it.value().first).arg(argCount)});
                } else {
                    errors.push_back({-1,
                        QStringLiteral("%1() expects %2-%3 arguments, got %4")
                            .arg(funcName).arg(it.value().first)
                            .arg(it.value().second).arg(argCount)});
                }
            }
        }

        // Warn about divide by zero risk in '/' with literal 0
        break;
    }
    case ASTNodeType::BinaryOp: {
        // Warn about division by zero
        if (node->op == QLatin1String("/") && node->children.size() == 2) {
            const auto& divisor = node->children[1];
            if (divisor && divisor->type == ASTNodeType::NumberLiteral
                && divisor->numericValue == 0.0) {
                errors.push_back({-1, QStringLiteral("Division by zero")});
            }
        }
        break;
    }
    default:
        break;
    }

    // Recurse into children
    for (const auto& child : node->children) {
        validateNode(child, errors);
    }
}

// ---------------------------------------------------------------------------
// Type inference
// ---------------------------------------------------------------------------

FormulaOutputType FormulaEngine::inferType(const ASTNodePtr& node) const
{
    if (!node) return FormulaOutputType::Unknown;

    switch (node->type) {
    case ASTNodeType::NumberLiteral:
        return FormulaOutputType::Numeric;
    case ASTNodeType::StringLiteral:
        return FormulaOutputType::Text;
    case ASTNodeType::ColumnRef: {
        ColumnType ct = columnTypeByName(node->stringValue);
        switch (ct) {
        case ColumnType::Integer:
        case ColumnType::Double:
            return FormulaOutputType::Numeric;
        case ColumnType::Boolean:
            return FormulaOutputType::Boolean;
        case ColumnType::Date:
        case ColumnType::DateTime:
            return FormulaOutputType::Date;
        default:
            return FormulaOutputType::Text;
        }
    }
    case ASTNodeType::BinaryOp: {
        // Arithmetic ops return Numeric; comparison ops return Boolean
        if (node->op == QLatin1String("+") || node->op == QLatin1String("-") ||
            node->op == QLatin1String("*") || node->op == QLatin1String("/") ||
            node->op == QLatin1String("%")) {
            return FormulaOutputType::Numeric;
        }
        return FormulaOutputType::Boolean;
    }
    case ASTNodeType::UnaryOp:
        return FormulaOutputType::Numeric;
    case ASTNodeType::FunctionCall: {
        const QString fn = node->stringValue.toLower();
        // Text functions
        if (fn == QLatin1String("concat") || fn == QLatin1String("trim") ||
            fn == QLatin1String("upper") || fn == QLatin1String("lower") ||
            fn == QLatin1String("substring") || fn == QLatin1String("replace") ||
            fn == QLatin1String("left") || fn == QLatin1String("right") ||
            fn == QLatin1String("lpad") || fn == QLatin1String("rpad") ||
            fn == QLatin1String("cast_text") || fn == QLatin1String("strftime")) {
            return FormulaOutputType::Text;
        }
        // Numeric functions
        if (fn == QLatin1String("abs") || fn == QLatin1String("round") ||
            fn == QLatin1String("ceil") || fn == QLatin1String("floor") ||
            fn == QLatin1String("sqrt") || fn == QLatin1String("power") ||
            fn == QLatin1String("mod") || fn == QLatin1String("log") ||
            fn == QLatin1String("length") || fn == QLatin1String("cast_int") ||
            fn == QLatin1String("cast_double") || fn == QLatin1String("year") ||
            fn == QLatin1String("month") || fn == QLatin1String("day")) {
            return FormulaOutputType::Numeric;
        }
        // Date functions
        if (fn == QLatin1String("date_trunc") || fn == QLatin1String("cast_date") ||
            fn == QLatin1String("current_date")) {
            return FormulaOutputType::Date;
        }
        // Conditional — infer from first branch
        if (fn == QLatin1String("if") && node->children.size() >= 3) {
            return inferType(node->children[1]);
        }
        if (fn == QLatin1String("coalesce") || fn == QLatin1String("ifnull")) {
            if (!node->children.empty()) {
                return inferType(node->children[0]);
            }
        }
        return FormulaOutputType::Unknown;
    }
    default:
        return FormulaOutputType::Unknown;
    }
}

// ---------------------------------------------------------------------------
// Build definition
// ---------------------------------------------------------------------------

CalculatedColumnDef FormulaEngine::buildDefinition(
    const QString& columnName, const QString& formula,
    std::vector<FormulaError>& errors) const
{
    CalculatedColumnDef def;
    def.columnName = columnName;
    def.formulaText = formula;

    auto ast = parseAndValidate(formula, errors);
    if (ast) {
        def.referencedColumns = extractColumnRefs(ast);
        def.outputType = inferType(ast);
    }
    return def;
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

QStringList FormulaEngine::availableFunctions()
{
    return knownFunctions();
}

QString FormulaEngine::functionHelp(const QString& funcName)
{
    static const QMap<QString, QString> help = {
        {QStringLiteral("concat"),     QStringLiteral("concat(val1, val2, ...) — Concatenate text values")},
        {QStringLiteral("trim"),       QStringLiteral("trim(text) — Remove leading/trailing whitespace")},
        {QStringLiteral("upper"),      QStringLiteral("upper(text) — Convert to uppercase")},
        {QStringLiteral("lower"),      QStringLiteral("lower(text) — Convert to lowercase")},
        {QStringLiteral("length"),     QStringLiteral("length(text) — Number of characters")},
        {QStringLiteral("substring"),  QStringLiteral("substring(text, start [, length]) — Extract portion")},
        {QStringLiteral("replace"),    QStringLiteral("replace(text, search, replacement) — Replace occurrences")},
        {QStringLiteral("left"),       QStringLiteral("left(text, n) — First n characters")},
        {QStringLiteral("right"),      QStringLiteral("right(text, n) — Last n characters")},
        {QStringLiteral("abs"),        QStringLiteral("abs(number) — Absolute value")},
        {QStringLiteral("round"),      QStringLiteral("round(number [, decimals]) — Round")},
        {QStringLiteral("ceil"),       QStringLiteral("ceil(number) — Round up")},
        {QStringLiteral("floor"),      QStringLiteral("floor(number) — Round down")},
        {QStringLiteral("sqrt"),       QStringLiteral("sqrt(number) — Square root")},
        {QStringLiteral("power"),      QStringLiteral("power(base, exponent) — Raise to power")},
        {QStringLiteral("mod"),        QStringLiteral("mod(a, b) — Remainder of a/b")},
        {QStringLiteral("log"),        QStringLiteral("log(number) — Natural logarithm")},
        {QStringLiteral("coalesce"),   QStringLiteral("coalesce(val1, val2, ...) — First non-null")},
        {QStringLiteral("ifnull"),     QStringLiteral("ifnull(value, default) — Replace null")},
        {QStringLiteral("nullif"),     QStringLiteral("nullif(a, b) — Return null if a = b")},
        {QStringLiteral("if"),         QStringLiteral("if(condition, true_val, false_val) — Conditional")},
        {QStringLiteral("case_when"),  QStringLiteral("case_when(cond1, val1, cond2, val2, ..., default) — Multi-branch")},
        {QStringLiteral("year"),       QStringLiteral("year(date) — Extract year")},
        {QStringLiteral("month"),      QStringLiteral("month(date) — Extract month")},
        {QStringLiteral("day"),        QStringLiteral("day(date) — Extract day")},
        {QStringLiteral("date_diff"),  QStringLiteral("date_diff('part', start, end) — Difference between dates")},
        {QStringLiteral("date_trunc"), QStringLiteral("date_trunc('part', date) — Truncate to part")},
        {QStringLiteral("strftime"),   QStringLiteral("strftime(date, 'format') — Format date as text")},
        {QStringLiteral("cast_int"),   QStringLiteral("cast_int(value) — Convert to integer")},
        {QStringLiteral("cast_double"),QStringLiteral("cast_double(value) — Convert to decimal")},
        {QStringLiteral("cast_text"),  QStringLiteral("cast_text(value) — Convert to text")},
        {QStringLiteral("cast_date"),  QStringLiteral("cast_date(value) — Convert to date")},
    };
    return help.value(funcName.toLower(), QStringLiteral("No help available"));
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

ColumnType FormulaEngine::columnTypeByName(const QString& name) const
{
    for (const auto& col : m_schema) {
        if (col.name == name) return col.type;
    }
    return ColumnType::Unknown;
}

} // namespace csvforge
