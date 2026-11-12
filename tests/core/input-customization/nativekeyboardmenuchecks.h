// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/platform/nativekeyboardmenuroute.h"
@interface OpenGateMenuFixtureTarget : NSObject <NSMenuItemValidation> {
@public
    int calls;
    BOOL available;
}
- (void)invoke:(id)sender;
@end
@implementation OpenGateMenuFixtureTarget
- (void)invoke:(id)sender {(void)sender;++calls;}
- (BOOL)validateMenuItem:(NSMenuItem *)item {(void)item;return available;}
@end
namespace OpenGateTest {
template<class Check,class Settle> void nativeKeyboardMenuChecks(Check check,Settle settle) {
    NSMenu *previous=[NSApp.mainMenu retain];
    NSMenu *root=[[NSMenu alloc] initWithTitle:@"Test"];
    NSMenuItem *app=[[NSMenuItem alloc] initWithTitle:@"Application" action:nil keyEquivalent:@""];
    NSMenu *menu=[[NSMenu alloc] initWithTitle:@"Application"];
    app.submenu=menu;[root addItem:app];NSApp.mainMenu=root;
    NSMenuItem *item=[[NSMenuItem alloc] initWithTitle:@"Keyboard Customization…" action:@selector(invoke:) keyEquivalent:@"k"];
    item.keyEquivalentModifierMask=NSEventModifierFlagCommand|NSEventModifierFlagOption;
    auto native=[[OpenGateMenuFixtureTarget alloc] init];native->available=YES;item.target=native;[menu addItem:item];
    QObject owner;int opened=0;bool enabled=true;
    auto route=Unbound::createNativeKeyboardMenuRoute(&owner,[&]{if(!enabled)return false;++opened;return true;});settle();
    check(item.target!=native,"AppKit application-menu row receives its own route without a Qt aboutToShow signal");
    [NSApp sendAction:item.action to:item.target from:item];
    check(opened==1 && native->calls==0,"native application-menu activation opens retained panel exactly once");
    check([item.keyEquivalent isEqualToString:@"k"] && item.keyEquivalentModifierMask==(NSEventModifierFlagCommand|NSEventModifierFlagOption),"AppKit route preserves assigned native key equivalent");
    enabled=false;[NSApp sendAction:item.action to:item.target from:item];
    check(opened==1 && native->calls==1,"unavailable panel retains original AppKit action fallback");
    native->available=NO;
    check(![(id<NSMenuItemValidation>)item.target validateMenuItem:item],"native menu enablement remains authoritative");
    delete route;
    check(item.target==native && item.action==@selector(invoke:),"normal teardown restores native menu target and action");
    NSApp.mainMenu=previous;
    [previous release];[item release];[native release];[menu release];[app release];[root release];
}
}
