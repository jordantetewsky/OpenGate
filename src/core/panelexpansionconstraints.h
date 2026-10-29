// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QWidget>
namespace Unbound {
// Lease only the native Mixer container's expansion cap. Track controls keep
// their native constraints; restore the exact cap when the transaction ends.
class PanelExpansionConstraints {
public:
 ~PanelExpansionConstraints(){restore();}
 void acquire(const QString &panel,QWidget *surface) {
  restore();if(panel!="frameEditAudioMeter"||!surface)return;
  content=surface->findChild<QWidget *>("frameEditAudioMixer");
  if(!content)return;
  maximumWidth=content->maximumWidth();content->setMaximumWidth(QWIDGETSIZE_MAX);
 }
 void restore() {
  if(content)content->setMaximumWidth(maximumWidth);
  content=nullptr;
 }
private:
 QPointer<QWidget> content;int maximumWidth=QWIDGETSIZE_MAX;
};
}
