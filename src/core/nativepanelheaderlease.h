// SPDX-License-Identifier: MIT
#pragma once
#include <QList>
#include <QPointer>
#include <QWidget>
namespace Unbound {
// Suppress only redundant presentation while a managed header owns the name
// and placement control. Native options and panel functions stay untouched.
class NativePanelHeaderLease {
public:
 ~NativePanelHeaderLease(){restore();}
 void acquire(QWidget *surface,const QString &id) {
  restore();if(!surface)return;
  for(auto child:surface->findChildren<QWidget *>()) {
   const auto name=child->objectName();
   const bool mixerTitle=id=="frameEditAudioMeter"&&name=="lblMixerMeter"&&
    child->parentWidget()&&child->parentWidget()->objectName()=="frameMixerMeterTitle";
   if(name=="unboundTimelineHeader"||name=="unboundNativeUndock_"+id||
      name.startsWith("unboundNativeKeyboard_")||mixerTitle) {
    hidden.append({child,child->isHidden()});child->hide();
   }
  }
 }
 void restore() {
  const auto prior=hidden;hidden.clear();
  for(const auto &entry:prior)if(entry.widget)entry.widget->setVisible(!entry.hidden);
 }
private:
 struct Entry {QPointer<QWidget> widget;bool hidden;};QList<Entry> hidden;
};
}
