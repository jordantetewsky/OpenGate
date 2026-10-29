// SPDX-License-Identifier: MIT
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
class QWidget;
namespace Unbound { namespace Platform {
// Native restore returns false when the common Qt geometry fallback is needed.
bool restoreWindowPlacement(QWidget *window, const QJsonObject &document, const QJsonArray &screens);
QByteArray saveWindowPlacement(QWidget *window, const QJsonArray &screens);
bool windowPlacementEligible(QWidget *window);
} }
