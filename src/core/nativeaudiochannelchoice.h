// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPointer>
#include <QRegularExpression>
namespace Unbound {
// Interprets one freshly obtained native submenu. Owns no selection or mapping.
// A choice is not proof of clip identity, native dispatch, or undo support.
struct NativeAudioChannelChoice {
    QPointer<QAction> action;
    int current=0,target=0;
    QString reason;
    static NativeAudioChannelChoice inspect(QMenu *menu,int direction,int expectedCurrent=0,int plannedTarget=0,int sourceCount=0) {
        auto reject=[](const QString &reason){return NativeAudioChannelChoice{nullptr,0,0,reason};};
        if(direction!=1 && direction!=-1)return reject("invalid-direction");
        if(!menu || menu->property("unboundOwnMenu").toBool())return reject("native-menu-required");
        QString title=menu->title();title.remove('&');
        if(title.trimmed()!="Audio Channel")return reject("unsupported-menu");
        const auto actions=menu->actions();
        if(actions.isEmpty() || actions.size()>64)return reject("unsupported-channel-count");
        const QRegularExpression label(QStringLiteral("^([1-9][0-9]*)\\s+-\\s+.+$"));
        int checked=-1;
        for(int i=0;i<actions.size();++i) {
            auto action=actions[i];const auto match=label.match(action->text().trimmed());
            if(action->isSeparator() || action->menu() || !action->isEnabled() || !action->isVisible() ||
               !action->isCheckable() || !match.hasMatch() || match.captured(1).toInt()!=i+1)
                return reject("unsupported-channel-actions");
            if(action->isChecked()) {
                if(checked!=-1)return reject("ambiguous-current-channel");
                checked=i;
            }
        }
        if(checked<0)return reject("unknown-current-channel");
        if(actions.size()==1)return reject("single-channel-source");
        if(expectedCurrent && (checked+1!=expectedCurrent || sourceCount!=actions.size()))
            return reject("native-plan-mapping-mismatch");
        if(plannedTarget && (plannedTarget<1 || plannedTarget>actions.size() || plannedTarget==checked+1))
            return reject("invalid-planned-channel");
        const int next=plannedTarget?plannedTarget-1:(checked+direction+actions.size())%actions.size();
        return {actions[next],checked+1,next+1,{}};
    }
};
}
