// SPDX-License-Identifier: MIT
#pragma once
#include "panelcommands.h"
#include "panelsession.h"
namespace Unbound {
// Registers one scale command set; the existing session owns scale and saving.
class FocusedPanelScaleCommands {
public:
    using Lookup=std::function<ManagedPanelSession *(const QString &)>;
    static void addTo(PanelCommands *commands,Lookup lookup) {
        const QPair<const char *,double> operations[]={{"Increase",0.1},{"Decrease",-0.1},{"Reset",0.0}};
        for (const auto &operation:operations) {
            const QString name=QString::fromLatin1(operation.first);
            commands->add({"panel.scale."+name.toLower(),"Panel Content Scale / "+name,
                [commands,lookup] {
                    auto session=lookup(commands->focusedTarget().panel);
                    return session && session->active();
                },[commands,lookup,delta=operation.second] {
                    auto session=lookup(commands->focusedTarget().panel);
                    if (!session || !session->active()) return;
                    const auto factor=delta==0.0 ? 1.0 : qBound(0.7,qRound((session->interfaceScale()+delta)*10.)/10.,3.0);
                    session->setInterfaceScale(factor);
                },"*"});
        }
    }
};
}
