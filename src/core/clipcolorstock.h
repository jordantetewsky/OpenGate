// SPDX-License-Identifier: MIT
#pragma once
#include <QPair>
#include <QString>
#include <QVector>
namespace Unbound {
// Sampled native reference swatches shared by comparison and candidate fallback
// policy. They do not redefine native swatches or establish runtime RGB parity.
inline const QVector<QPair<QString, QString>> &clipColorStockReferences() {
    static const QVector<QPair<QString, QString>> values{
        {"Orange", "#EB6E01"}, {"Apricot", "#FFA833"}, {"Yellow", "#D4AD1F"},
        {"Lime", "#9FC615"}, {"Olive", "#5F9921"}, {"Green", "#448F65"},
        {"Teal", "#019899"}, {"Navy", "#005278"}, {"Blue", "#4376A1"},
        {"Purple", "#9972A0"}, {"Violet", "#D0568D"}, {"Pink", "#E98CB5"},
        {"Tan", "#B9AF97"}, {"Beige", "#C4A07C"}, {"Brown", "#996601"},
        {"Chocolate", "#8C5A3F"}};
    return values;
}
} // namespace Unbound
