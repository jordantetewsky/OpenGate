// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/clipcolorassignmentbackend.h"
#include "../../../src/core/nativecommandactivity.h"
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUndoCommand>
#include <QUndoStack>

namespace OpenGateTest {
// Test host document/history adapter. Production commands and display bindings
// are real; this backend models the external host boundary, not a second plugin
// assignment owner. These tests make no private Resolve API acceptance claim.
class AssignmentDocumentFixture : public Unbound::ClipColorAssignmentBackend {
  public:
    QUndoStack history;
    QHash<QString, Read> records;
    bool rejectNext = false;
    explicit AssignmentDocumentFixture(QString path) : path(std::move(path)) {
        QFile file(this->path);
        if (file.open(QIODevice::ReadOnly)) {
            const auto object = QJsonDocument::fromJson(file.readAll()).object();
            for (auto it = object.begin(); it != object.end(); ++it) {
                const auto value = it.value().toObject();
                records.insert(it.key(), {true, value["present"].toBool(),
                    value["bytes"].toString().toUtf8(), value["native"].toString()});
            }
        }
    }
    Read read(QGraphicsObject *clip) override {
        const auto id = clip ? clip->property("fixtureNativeId").toString() : QString();
        return id.isEmpty() ? Read{} : records.value(id, Read{true, false, {}, "ClipColorBlue"});
    }
    bool commit(const QVector<Edit> &edits, const QString &label) override {
        if (rejectNext) { rejectNext = false; return false; }
        auto before = records, after = records;
        for (const auto &edit : edits) {
            const auto actual = read(edit.clip);
            if (!actual.available || actual.present != edit.before.present || actual.bytes != edit.before.bytes ||
                actual.nativeColor != edit.before.nativeColor) return false;
            const auto id = edit.clip->property("fixtureNativeId").toString();
            after.insert(id, {true, edit.present, edit.bytes,
                             edit.nativeColor.isEmpty() ? QString() : "ClipColor"+edit.nativeColor});
        }
        class Change : public QUndoCommand {
          public:
            Change(AssignmentDocumentFixture &host, QHash<QString, Read> before,
                   QHash<QString, Read> after, const QString &label)
                : QUndoCommand(label), host(host), before(std::move(before)), after(std::move(after)) {}
            void undo() override { host.restore(before); }
            void redo() override { host.restore(after); }
          private:
            AssignmentDocumentFixture &host;
            QHash<QString, Read> before, after;
        };
        history.push(new Change(*this, before, after, label));
        return true;
    }
    void restore(const QHash<QString, Read> &next) {
        records = next;
        QJsonObject object;
        for (auto it = records.cbegin(); it != records.cend(); ++it)
            object.insert(it.key(), QJsonObject{{"present", it->present},
                {"bytes", QString::fromUtf8(it->bytes)}, {"native", it->nativeColor}});
        const auto bytes = QJsonDocument(object).toJson();
        QSaveFile file(path);
        const bool saved = file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
        Q_ASSERT(saved);
        Q_UNUSED(saved)
        Unbound::NativeCommandActivity::shared()->completed("fixture-host-history");
    }
  private:
    QString path;
};
} // namespace OpenGateTest
