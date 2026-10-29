#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "panelcontrols.h"
#include "controlservice.h"


static void captureWidgets(int capture)
{
    if (!qApp || QThread::currentThread() != qApp->thread()) return;
    QJsonArray widgets;
    for (QWidget *widget : QApplication::allWidgets()) {
        const QString name = widget->objectName();
        if (name.isEmpty() && !widget->isWindow()) continue;
        const QRect geometry = widget->geometry();
        widgets.append(QJsonObject{
            {"name", name}, {"class", widget->metaObject()->className()},
            {"parent", widget->parentWidget() ? widget->parentWidget()->objectName() : QString()},
            {"window", widget->isWindow()}, {"visible", widget->isVisible()},
            {"x", geometry.x()}, {"y", geometry.y()},
            {"width", geometry.width()}, {"height", geometry.height()}});
    }
    const QString directory = QDir::homePath() + QStringLiteral("/Library/Logs/OpenGate");
    if (!QDir().mkpath(directory)) return;
    QSaveFile file(directory + QStringLiteral("/widgets-%1.json").arg(capture));
    if (!file.open(QIODevice::WriteOnly)) return;
    const QByteArray data = QJsonDocument(QJsonObject{
        {"qt", qVersion()}, {"main_thread", true}, {"widgets", widgets}}).toJson();
    if (file.write(data) == data.size()) file.commit();
}

extern "C" __attribute__((visibility("default"))) void unboundInspectWidgets()
{
    // Read only, bounded snapshots let Resolve finish creating its lazy panels.
    if (qstrcmp(qVersion(), "5.15.2") != 0 || !qApp ||
        QThread::currentThread() != qApp->thread()) return;
    QTimer::singleShot(2000, qApp, [] { captureWidgets(1); });
    QTimer::singleShot(10000, qApp, [] { captureWidgets(2); });
    QTimer::singleShot(30000, qApp, [] { captureWidgets(3); });
    QTimer::singleShot(2000, qApp, [] {
        if (qApp->findChild<QObject *>(QStringLiteral("unboundControlService"))) return;
        auto diagnostics = new Unbound::WindowDiagnostics(qApp);
        qApp->installEventFilter(diagnostics);
        new Unbound::ControlService(qApp);
    });
}
