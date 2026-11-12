// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#include "../../platform/nativekeyboardmenuroute.h"
#include "../../core/diagnostics.h"
#include <QPointer>
#include <QTimer>

// Lease this menu item's target/action, not Resolve's QAction connections or
// the application's dispatch method. Native validation and fallback survive.
@interface OpenGateKeyboardMenuTarget : NSObject <NSMenuItemValidation> {
@public
    NSMenuItem *item;
    id previousTarget;
    SEL previousAction;
    std::function<bool()> openPanel;
}
- (void)invoke:(id)sender;
@end
@implementation OpenGateKeyboardMenuTarget
- (void)invoke:(id)sender {
    const bool opened=openPanel && openPanel();
    Unbound::logEvent("keyboard.native-menu.invoked",{{"panel_opened",opened},{"responder_chain",previousTarget==nil}});
    if(opened)return;
    if(previousAction)[NSApp sendAction:previousAction to:previousTarget from:sender];
}
- (BOOL)validateMenuItem:(NSMenuItem *)sender {
    // Qt deliberately leaves the target nil and resolves it through AppKit's
    // responder chain. Resolve the ORIGINAL selector, never our invoke: bridge.
    id validator=previousTarget ?: [NSApp targetForAction:previousAction to:nil from:sender];
    if([validator respondsToSelector:@selector(validateMenuItem:)])
        return [validator validateMenuItem:sender];
    if([validator respondsToSelector:@selector(validateUserInterfaceItem:)])
        return [validator validateUserInterfaceItem:sender];
    return validator && sender.enabled;
}
- (void)dealloc {
    [item release];[previousTarget release];[super dealloc];
}
@end

namespace Unbound {
class MacKeyboardMenuRoute final : public NativeKeyboardMenuRoute {
public:
    MacKeyboardMenuRoute(QObject *owner,std::function<bool()> open)
        : NativeKeyboardMenuRoute(owner),open(std::move(open)) {
        QPointer<MacKeyboardMenuRoute> alive(this);
        observer=[[NSNotificationCenter defaultCenter]
            addObserverForName:NSMenuDidBeginTrackingNotification object:nil queue:nil
            usingBlock:^(NSNotification *) {if(alive)alive->refresh();}];
        QTimer::singleShot(0,this,[this]{refresh();});
    }
    ~MacKeyboardMenuRoute() override {
        stopObserving();
        if(target) {
            if(target->item.target==target && target->item.action==@selector(invoke:)) {
                target->item.target=target->previousTarget;
                target->item.action=target->previousAction;
            }
            [target release];
        }
    }
    void abandonForQuit() override {
        stopped=true;stopObserving();open={};
        // Do not mutate AppKit's menu ownership while Resolve tears down.
        // Keep the tiny native fallback bridge alive until process exit.
        if(target){target->openPanel={};target=nil;}
    }
private:
    static void find(NSMenu *menu,NSMutableArray *matches) {
        for(NSMenuItem *item in menu.itemArray) {
            if([item.title isEqualToString:@"Keyboard Customization…"] ||
               [item.title isEqualToString:@"Keyboard Customization..."] ||
               [item.title isEqualToString:@"Keyboard Customization"])[matches addObject:item];
            if(item.submenu)find(item.submenu,matches);
        }
    }
    void refresh() {
        if(stopped)return;
        NSMutableArray *matches=[NSMutableArray array];find(NSApp.mainMenu,matches);
        if(matches.count!=1)return;
        NSMenuItem *item=matches.firstObject;
        if(target && target->item==item && item.target==target && item.action==@selector(invoke:))return;
        if(target) {
            if(target->item.target==target){target->item.target=target->previousTarget;target->item.action=target->previousAction;}
            [target release];target=nil;
        }
        if(!item.action)return;
        target=[[OpenGateKeyboardMenuTarget alloc] init];
        target->item=[item retain];target->previousTarget=[item.target retain];target->previousAction=item.action;
        QPointer<MacKeyboardMenuRoute> alive(this);
        target->openPanel=[alive]{return alive && !alive->stopped && alive->open && alive->open();};
        item.target=target;item.action=@selector(invoke:);
        logEvent("keyboard.native-menu.leased",{{"responder_chain",target->previousTarget==nil}});
    }
    void stopObserving() {
        if(observer){[[NSNotificationCenter defaultCenter] removeObserver:observer];observer=nil;}
    }
    std::function<bool()> open;
    OpenGateKeyboardMenuTarget *target=nil;
    id observer=nil;
    bool stopped=false;
};
NativeKeyboardMenuRoute *createNativeKeyboardMenuRoute(QObject *owner,std::function<bool()> open) {
    return new MacKeyboardMenuRoute(owner,std::move(open));
}
}
