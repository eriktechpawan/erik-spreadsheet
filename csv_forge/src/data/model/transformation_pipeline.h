#pragma once

#include <QJsonObject>
#include <QString>
#include <vector>

#include "data/model/transformation_step.h"

namespace csvforge {

/// Ordered pipeline of transformation steps applied to produce a dataset.
struct TransformationPipeline {
    std::vector<TransformationStep> steps;

    /// Generate a human-readable summary of the pipeline.
    QString toSummaryText() const;

    /// Generate the combined SQL query (or description) for display.
    QString toSQLPreview() const;

    /// Append a step.
    void addStep(const TransformationStep& step);

    /// Remove last step.
    void removeLastStep();

    /// Clear all steps.
    void clear();

    bool isEmpty() const;
    int stepCount() const;

    // Serialization
    QJsonObject toJson() const;
    static TransformationPipeline fromJson(const QJsonObject& obj);
};

} // namespace csvforge
