// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/platform/nativekeyboardmenuroute.h"
@interface OpenGateMenuFixtureTarget : NSResponder <NSMenuItemValidation> {
@public
    int calls;
    BOOL available;
}
- (void)invoke:(id)sender;
- (void)openGateFixtureKeyboard:(id)sender;
@end
@implementation OpenGateMenuFixtureTarget
- (void)invoke:(id)sender {(void)sender;++calls;}
- (void)openGateFixtureKeyboard:(id)sender {(void)sender;++calls;}
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

    // QCocoaMenuItem::resolveTargetAction intentionally installs a nil target.
    // AppKit resolves the original selector against the current responder chain.
    auto previousResponder=[NSApp.nextResponder retain];
    native.nextResponder=previousResponder;NSApp.nextResponder=native;
    item.target=nil;item.action=@selector(openGateFixtureKeyboard:);
    native->available=YES;enabled=true;
    route=Unbound::createNativeKeyboardMenuRoute(&owner,[&]{if(!enabled)return false;++opened;return true;});settle();
    check(item.target!=nil,"responder-chain menu item is leased despite its nil target");
    const int openedBefore=opened,nativeBefore=native->calls;
    [NSApp sendAction:item.action to:item.target from:item];
    check(opened==openedBefore+1 && native->calls==nativeBefore,"responder-chain application entry opens retained panel exactly once");
    item.target=nil;item.action=@selector(openGateFixtureKeyboard:);
    [[NSNotificationCenter defaultCenter] postNotificationName:NSMenuDidBeginTrackingNotification object:menu];
    check(item.target!=nil,"menu tracking reacquires responder-chain item after native target refresh");
    native->available=NO;
    check(![(id<NSMenuItemValidation>)item.target validateMenuItem:item],"responder-chain native validation still disables the entry");
    native->available=YES;
    check([(id<NSMenuItemValidation>)item.target validateMenuItem:item],"responder-chain native validation re-enables the entry");
    enabled=false;[NSApp sendAction:item.action to:item.target from:item];
    check(opened==openedBefore+1 && native->calls==nativeBefore+1,"unavailable panel falls back once through the original responder chain");
    delete route;
    check(item.target==nil && item.action==@selector(openGateFixtureKeyboard:),"teardown restores nil target and native responder selector");
    NSApp.nextResponder=previousResponder;native.nextResponder=nil;[previousResponder release];
    NSApp.mainMenu=previous;
    [previous release];[item release];[native release];[menu release];[app release];[root release];
}
}
