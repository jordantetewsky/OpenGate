#include <QApplication>
#include <QAbstractButton>
#include <QLabel>
#include <QLayout>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <qt_windows.h>

static QJsonArray size(QSize s) { return {s.width(),s.height()}; }
static QJsonObject describe(QWidget *w)
{
    const auto r=w->geometry(); const auto policy=w->sizePolicy();
    QJsonObject o{{"name",w->objectName()},{"class",w->metaObject()->className()},
        {"visible",w->isVisible()},{"geometry",QJsonArray{r.x(),r.y(),r.width(),r.height()}},
        {"minimum",size(w->minimumSize())},{"maximum",size(w->maximumSize())},
        {"minimumHint",size(w->minimumSizeHint())},{"hint",size(w->sizeHint())},
        {"policy",QJsonArray{int(policy.horizontalPolicy()),int(policy.verticalPolicy())}}};
    if (auto b=qobject_cast<QAbstractButton *>(w)) o.insert("text",b->text());
    if (auto label=qobject_cast<QLabel *>(w)) o.insert("text",label->text());
    if (auto layout=w->layout()) o.insert("layout",QJsonObject{
        {"class",layout->metaObject()->className()},{"constraint",int(layout->sizeConstraint())},
        {"minimum",size(layout->minimumSize())},{"hint",size(layout->sizeHint())},{"count",layout->count()}});
    QJsonObject properties;
    for (int i=QWidget::staticMetaObject.propertyCount();i<w->metaObject()->propertyCount();++i) {
        auto p=w->metaObject()->property(i);
        if (p.isReadable()) properties.insert(p.name(),QJsonValue::fromVariant(p.read(w)));
    }
    o.insert("properties",properties);
    QJsonArray methods;
    for (int i=QWidget::staticMetaObject.methodCount();i<w->metaObject()->methodCount();++i) {
        const auto name=w->metaObject()->method(i).methodSignature();
        if (name.toLower().contains("elid") || name.toLower().contains("title") || name.toLower().contains("text"))
            methods.append(QString::fromLatin1(name));
    }
    o.insert("methods",methods);
    return o;
}
static void append(QWidget *w,int parent,QJsonArray &widgets)
{
    if (widgets.size()>=4000) return;
    const int index=widgets.size(); auto o=describe(w); o.insert("parent",parent); widgets.append(o);
    for (auto child:w->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly)) append(child,index,widgets);
}
extern "C" __declspec(dllexport) DWORD WINAPI UnboundStart(void *)
{
    auto app=qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app || QCoreApplication::closingDown()) return 0;
    return QMetaObject::invokeMethod(app,[] {
        QJsonArray widgets,ancestors;
        for (auto window:QApplication::topLevelWidgets()) {
            if (window->objectName()==QLatin1String("UiMainWindow")) {
                auto pane=window->findChild<QWidget *>(QStringLiteral("frameEditPageInspector"));
                if (pane) {
                    append(pane,-1,widgets);
                    for (auto parent=pane->parentWidget();parent;parent=parent->parentWidget()) ancestors.append(describe(parent));
                }
            }
            if (window->objectName()==QLatin1String("unboundFloating_frameEditPageInspector")) append(window,-1,widgets);
        }
        HMODULE module=nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&UnboundStart),&module);
        wchar_t path[32768]; const auto length=GetModuleFileNameW(module,path,32768);
        const auto directory=QString::fromWCharArray(path,int(length)).section('\\',0,-2);
        QFile output(QDir(directory).filePath(QStringLiteral("inspector-%1-%2.json").arg(GetCurrentProcessId())
            .arg(QDateTime::currentMSecsSinceEpoch())));
        if (output.open(QIODevice::WriteOnly)) output.write(QJsonDocument(QJsonObject{
            {"widgets",widgets},{"ancestors",ancestors},{"truncated",widgets.size()>=4000}}).toJson());
    },Qt::QueuedConnection) ? 1 : 0;
}
