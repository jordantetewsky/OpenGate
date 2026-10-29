// SPDX-License-Identifier: MIT
#include "windowplacement.h"
#include <QWidget>
#include <QGuiApplication>
#include <QJsonDocument>
#include <qt_windows.h>
namespace Unbound { namespace Platform {
bool restoreWindowPlacement(QWidget *window, const QJsonObject &document, const QJsonArray &screens)
{
    if (document.value("format").toString()==QLatin1String("unbound-main-placement-1") && QGuiApplication::platformName()==QLatin1String("windows")
        && document.value("screens").toArray()==screens) {
        const auto rect=document.value("normal").toArray();
        if (rect.size()==4 && rect[2].toInt()>rect[0].toInt() && rect[3].toInt()>rect[1].toInt()) {
            WINDOWPLACEMENT placement{}; placement.length=sizeof(placement);
            const auto handle=reinterpret_cast<HWND>(window->winId());
            if (GetWindowPlacement(handle,&placement)) {
                placement.flags=0;
                placement.showCmd=document.value("maximized").toBool() ? SW_SHOWMAXIMIZED : SW_SHOWNOACTIVATE;
                placement.rcNormalPosition=RECT{rect[0].toInt(),rect[1].toInt(),rect[2].toInt(),rect[3].toInt()};
                return SetWindowPlacement(handle,&placement);
            }
        }
    }
    return false;
}
QByteArray saveWindowPlacement(QWidget *window, const QJsonArray &screens)
{
    const auto qt=window->saveGeometry();
    if (QGuiApplication::platformName()==QLatin1String("windows")) {
        WINDOWPLACEMENT placement{}; placement.length=sizeof(placement);
        if (GetWindowPlacement(reinterpret_cast<HWND>(window->winId()),&placement)) {
            // Resolve's custom frame can disagree with Qt's cached margins.
            // Persist and restore the same native workspace coordinates;
            // never reconstruct this rectangle from QWidget frame margins.
            const auto &r=placement.rcNormalPosition;
            return QJsonDocument(QJsonObject{{"format","unbound-main-placement-1"},
                {"qt",QString::fromLatin1(qt.toBase64())},{"screens",screens},
                {"normal",QJsonArray{r.left,r.top,r.right,r.bottom}},
                {"maximized",IsZoomed(reinterpret_cast<HWND>(window->winId()))!=FALSE}}).toJson(QJsonDocument::Compact);
        }
    }
    return qt;
}
bool windowPlacementEligible(QWidget *window)
{
    if (QGuiApplication::platformName()==QLatin1String("windows")
        && !(GetWindowLongPtr(reinterpret_cast<HWND>(window->winId()),GWL_STYLE)&WS_CAPTION)) return false;
    return true;
}
} }
