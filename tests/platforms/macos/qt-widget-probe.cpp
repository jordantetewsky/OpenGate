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
#ifdef OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY
#include "colorvisualtrial.h"
#endif


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

#ifdef OPENGATE_DEVELOPMENT_ENTRY
#include "developmententry.h"
#endif

extern "C" __attribute__((visibility("default"))) void unboundInspectWidgets()
{
    // Read only, bounded snapshots let Resolve finish creating its lazy panels.
    if (qstrcmp(qVersion(), "5.15.2") != 0 || !qApp ||
        QThread::currentThread() != qApp->thread()) return;
    QTimer::singleShot(2000, qApp, [] { captureWidgets(1); });
    QTimer::singleShot(10000, qApp, [] { captureWidgets(2); });
    QTimer::singleShot(30000, qApp, [] { captureWidgets(3); });
    const auto start=[] {
        if (qApp->findChild<QObject *>(QStringLiteral("unboundControlService"))) return;
#ifdef OPENGATE_INPUT_CUSTOMIZATION_PREVIEW
        qputenv("OPENGATE_EXPERIMENTAL_INPUT_CUSTOMIZATION", "1");
#endif
#ifdef OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY
        if (!Unbound::ColorVisualTrial::activate(QString::fromUtf8(OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY))) {
            Unbound::logEvent("color-trial.activation.failed",{{"reason","trial-profile-unavailable"}});
            return;
        }
        Unbound::logEvent("color-trial.activation.ready",{{"manager_enabled",true},{"paint_enabled",true},
            {"palette_file",qEnvironmentVariable("OPENGATE_COLOR_PALETTE_FILE")}});
#endif
        auto diagnostics = new Unbound::WindowDiagnostics(qApp);
        qApp->installEventFilter(diagnostics);
        new Unbound::ControlService(qApp);
    };
#ifdef OPENGATE_DEVELOPMENT_ENTRY
    if(qEnvironmentVariableIntValue("OPENGATE_DEVELOPMENT_SESSION")!=1)return;
    if(!OpenGateDevelopment::reserveEntry(qApp,2000,start))
        Unbound::logEvent("development.entry.rejected",{{"reason","service-already-owned"}});
#else
    QTimer::singleShot(2000,qApp,start);
#endif
}
