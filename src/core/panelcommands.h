// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QSaveFile>
#include <QShortcut>
#include <QTimer>
#include <functional>
#include "diagnostics.h"
#include "panelcommandcontext.h"
#include "commandbindingconflicts.h"
#include "settingsprofile.h"
namespace Unbound {
// Unbound-owned bindings. This is not registration in Resolve's command database.
class PanelCommands : public QObject {
public:
    struct Command { QString id,label; std::function<bool()> available; std::function<void()> invoke; QString scope={}; };
    explicit PanelCommands(QObject *owner) : QObject(owner),path(qEnvironmentVariable("UNBOUND_SHORTCUTS_FILE",
        settingsProfilePath("Shortcuts.json"))) {
        QFile f(path); if (f.open(QIODevice::ReadOnly)) bindings=QJsonDocument::fromJson(f.readAll()).object();
        refresh.setSingleShot(true);
        connect(&refresh,&QTimer::timeout,this,[this] { conflicts=bindingErrors(bindings); });
        refresh.start(0); qApp->installEventFilter(this);
    }
    ~PanelCommands() override { if (qApp) qApp->removeEventFilter(this); }
    void add(Command value) { commands.append(std::move(value)); }
    void defaultBinding(const QString &id,const QString &sequence) {
        if (!bindings.contains(id)) bindings[id]=sequence;
        refresh.start(0);
    }
    bool invoke(const QString &id,const QString &origin) {
        auto active=QApplication::activeWindow();
        const auto inputBlock=PanelCommandContext::blocked();
        const QString blocked=!inputBlock.isEmpty() ? inputBlock :
            !context || !context() ? "outside-workspace" : QString();
        const bool eligible=blocked.isEmpty();
        for (const auto &command:commands) if (command.id==id) {
            const bool scoped=origin=="opengate-command-menu" || shortcutEligible(command.id);
            const bool accepted=eligible && scoped && command.available && command.available();
            logEvent("command.invoke",{{"command",id},{"origin",origin},{"accepted",accepted},
                {"reason",accepted ? "accepted" : !blocked.isEmpty() ? blocked : !scoped ? "panel-context" : "command-unavailable"},
                {"active_window",active ? active->objectName() : QString()}});
            if (accepted && command.invoke) command.invoke();
            return accepted;
        }
        logEvent("command.invoke.unknown",{{"command",id},{"origin",origin}}); return false;
    }
    const QList<Command> &entries() const { return commands; }
    QJsonObject assigned() const { return bindings; }
    QString validate(const QJsonObject &proposed) const {
        const auto errors=bindingErrors(proposed);
        for (const auto &command : commands) if (errors.contains(command.id)) return errors.value(command.id);
        return {};
    }
    QString save(const QJsonObject &proposed) {
        auto error=validate(proposed); if (!error.isEmpty()) return error;
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) return "Cannot create shortcut settings directory.";
        QSaveFile f(path); const auto bytes=QJsonDocument(proposed).toJson();
        if (!f.open(QIODevice::WriteOnly) || f.write(bytes)!=bytes.size() || !f.commit()) return "Cannot save shortcut settings.";
        bindings=proposed; conflicts.clear(); logEvent("command.bindings.saved",{{"count",bindings.size()}}); return {};
    }
    QString commandScope(const QString &id) const {
        for (const auto &command:commands) if (command.id==id) return command.scope;
        return {};
    }
    PanelCommandContext::Target focusedTarget() const {
        QStringList ids;
        for (const auto &command:commands) {
            if (command.id.startsWith("panel.toggle.")) ids.append(command.id.mid(13));
            if (!command.scope.isEmpty() && command.scope!="*") ids.append(command.scope);
        }
        return PanelCommandContext::capture(ids);
    }
    bool shortcutEligible(const QString &id) const {
        return PanelCommandContext::blocked().isEmpty() && PanelCommandContext::allows(commandScope(id),focusedTarget());
    }
    struct NativeMatch { QString command,error; };
    // Optional read-only adapter. Assignment remains owned by Resolve.
    std::function<NativeMatch(const QKeySequence &)> nativeMatch;
    std::function<bool()> legacyBacktickAllowed;
    std::function<bool()> context;
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if (event->type()==QEvent::ActionAdded || event->type()==QEvent::ActionChanged || event->type()==QEvent::ActionRemoved) {
            refresh.start(0); return false;
        }
        if (event->type()!=QEvent::ShortcutOverride && event->type()!=QEvent::KeyPress) return false;
        if (!PanelCommandContext::blocked().isEmpty() || !context || !context()) return false;
        auto key=static_cast<QKeyEvent *>(event);
        const QKeySequence pressed(key->key() | int(key->modifiers() & ~Qt::KeypadModifier));
        if (nativeMatch) {
            const auto match=nativeMatch(pressed);
            if (!match.error.isEmpty()) {
                logEvent("command.preset.blocked",{{"key",pressed.toString(QKeySequence::PortableText)},{"reason",match.error}});
                return false;
            }
            if (!match.command.isEmpty()) {
                if (!shortcutEligible(match.command)) return false;
                // Claim ShortcutOverride before Qt's shortcut map; consume the
                // matching KeyPress once. No time-window deduplication.
                event->accept();
                if(event->type()==QEvent::KeyPress && !key->isAutoRepeat()) {
                    const auto id=match.command; const auto target=focusedTarget();
                    QTimer::singleShot(0,this,[this,id,pressed,target] {
                        // Preset or focus can change between observation and dispatch.
                        const auto current=nativeMatch ? nativeMatch(pressed):NativeMatch{};
                        if(current.error.isEmpty() && current.command==id &&
                           (commandScope(id).isEmpty() || focusedTarget()==target)) invoke(id,"resolve-preset-input");
                    });
                }
                return true;
            }
        }
        if(textEditor(QApplication::focusWidget()))return false;
        for (const auto &command : commands) {
            if(nativeMatch && (command.id!="panel.expand.hovered" || (legacyBacktickAllowed && !legacyBacktickAllowed())))continue;
            const auto configured=QKeySequence::fromString(bindings.value(command.id).toString(),QKeySequence::PortableText);
            if (configured.isEmpty() || configured!=pressed || !shortcutEligible(command.id) || !command.available || !command.available()) continue;
            // Recheck native conflicts after loading a different Resolve keyboard preset.
            if (refresh.isActive()) { refresh.stop(); conflicts=bindingErrors(bindings); }
            if (conflicts.contains(command.id)) { logEvent("command.binding.conflict",{{"command",command.id},{"key",pressed.toString()},{"reason",conflicts.value(command.id)}}); return false; }
            event->accept();
            if (event->type()==QEvent::KeyPress && !key->isAutoRepeat()) {
                const auto id=command.id; const auto available=command.available; const auto invoke=command.invoke; const auto target=focusedTarget();
                QTimer::singleShot(0,this,[id,available,invoke,target,this] {
                    const bool ready=PanelCommandContext::blocked().isEmpty() && context && context() && available() &&
                        shortcutEligible(id) && (commandScope(id).isEmpty() || focusedTarget()==target);
                    logEvent("command.invoke",{{"command",id},{"origin","unbound-shortcut"},{"accepted",ready}});
                    if (ready && invoke) {
                        QElapsedTimer timing; timing.start(); invoke();
                        logEvent("command.dispatched",{{"command",id},{"dispatch_ms",timing.nsecsElapsed()/1000000.0},
                            {"completion","panel-transition-logged-separately"}});
                    }
                });
            }
            return true;
        }
        return false;
    }
private:
    QList<Command> commands;
    QJsonObject bindings;
    QString path;
    QHash<QString,QString> conflicts;
    QTimer refresh;
    QHash<QString,QString> bindingErrors(const QJsonObject &proposed) const {
        QList<CommandBindingConflicts::Binding> scopes;
        for (const auto &command:commands) scopes.append({command.id,command.scope});
        return CommandBindingConflicts::evaluate(proposed,scopes);
    }
};
}
