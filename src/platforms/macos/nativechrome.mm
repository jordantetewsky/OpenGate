// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <objc/runtime.h>
#include <notify.h>
#include <cmath>
#include <QWidget>
#include <QApplication>
#include <QCursor>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include "nativechrome.h"
@interface OpenGateHeaderControl : NSButton {
    std::function<void()> callback;
}
- (void)setOpenCallback:(std::function<void()>)value;
- (void)openPanels:(id)sender;
@end
@implementation OpenGateHeaderControl
- (void)setOpenCallback:(std::function<void()>)value { callback=std::move(value); }
- (void)openPanels:(id)sender { (void)sender; if (callback) callback(); }
@end
@interface UnboundPinLight : NSButton {
    std::function<void(bool)> callback;
}
- (void)setChangeCallback:(std::function<void(bool)>)value;
- (void)pinClicked:(id)sender;
@end
@implementation UnboundPinLight
- (void)setChangeCallback:(std::function<void(bool)>)value { callback = std::move(value); }
- (void)pinClicked:(id)sender { (void)sender; auto current = callback; if (current) current(self.state == NSControlStateValueOn); [self setNeedsDisplay:YES]; }
- (void)drawRect:(NSRect)rect {
    (void)rect;
    const NSRect circle = NSInsetRect(self.bounds, 0.5, 0.5);
    [(self.state == NSControlStateValueOn ? [NSColor colorWithRed:0.35 green:0.65 blue:0.78 alpha:1] : NSColor.systemGrayColor) setFill];
    [[NSBezierPath bezierPathWithOvalInRect:circle] fill];
    NSAffineTransform *scale = [NSAffineTransform transform]; [scale scaleXBy:self.bounds.size.width / 14.0 yBy:self.bounds.size.height / 14.0];
    NSBezierPath *pin = [NSBezierPath bezierPath];
    [pin moveToPoint:NSMakePoint(5, 10)]; [pin lineToPoint:NSMakePoint(9, 10)]; [pin lineToPoint:NSMakePoint(8.4, 8)];
    [pin lineToPoint:NSMakePoint(10, 5.8)]; [pin lineToPoint:NSMakePoint(4, 5.8)]; [pin lineToPoint:NSMakePoint(5.6, 8)]; [pin closePath];
    [pin moveToPoint:NSMakePoint(7, 5.8)]; [pin lineToPoint:NSMakePoint(7, 2.5)]; [pin transformUsingAffineTransform:scale];
    [NSColor.blackColor setStroke]; pin.lineWidth = 0.85; [pin stroke];
}
@end

namespace Unbound {
class NativeCommandWatch : public QObject {
public:
    NativeCommandWatch(QObject *owner,std::function<void()> ready) : QObject(owner) {
        QPointer<NativeCommandWatch> alive(this);
        const auto status=notify_register_dispatch("com.cinewaltz.opengate.command-request",&token,dispatch_get_main_queue(),^(int) {
            if (alive && ready) ready();
        });
        setProperty("openGateNativeWakeReady",status==NOTIFY_STATUS_OK);
    }
    ~NativeCommandWatch() override { if (token>=0) notify_cancel(token); }
private:
    int token=-1;
};
QObject *watchNativeCommandRequests(QObject *owner,std::function<void()> ready) { return new NativeCommandWatch(owner,std::move(ready)); }
#ifdef UNBOUND_NATIVE_TESTS
void postNativeCommandRequestForTest() { notify_post("com.cinewaltz.opengate.command-request"); }
#endif

double nativeInputAgeMilliseconds(quint64 timestamp)
{
    if (!timestamp) return -1;
    const double age = NSProcessInfo.processInfo.systemUptime * 1000.0 - double(timestamp);
    return age >= 0 && age < 60000 ? age : -1;
}
double nativeCurrentDragAgeMilliseconds()
{
    NSEvent *event = NSApp.currentEvent;
    if (!event || event.type != NSEventTypeLeftMouseDragged) return -1;
    return nativeInputAgeMilliseconds(quint64(event.timestamp * 1000.0));
}
static char pinKey, titleKey, expansionLevelKey, headerMenuKey;
class PointerReleaseWatch : public QObject
{
public:
    PointerReleaseWatch(QObject *owner, std::function<void(QPoint, bool)> callback) : QObject(owner)
    {
        QPointer<PointerReleaseWatch> alive(this);
        monitor = [[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseUp handler:^NSEvent *(NSEvent *event) {
            if (alive) {
                const bool option = event.modifierFlags & NSEventModifierFlagOption;
                const QPoint position = QCursor::pos();
                QTimer::singleShot(0, alive, [alive, callback, option, position] { if (alive) callback(position, option); });
            }
            return event;
        }] retain];
    }
    ~PointerReleaseWatch() override { if (monitor) { [NSEvent removeMonitor:monitor]; [monitor release]; } }
private:
    id monitor = nil;
};
QObject *watchNativePointerRelease(QObject *owner, std::function<void(QPoint, bool)> released)
{ return new PointerReleaseWatch(owner, std::move(released)); }
#ifdef UNBOUND_NATIVE_TESTS
void postNativeMoveStartForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    [NSNotificationCenter.defaultCenter postNotificationName:NSWindowWillMoveNotification object:window];
}
void postNativePointerReleaseForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp location:NSMakePoint(20, 20) modifierFlags:0
        timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
    [NSApp postEvent:event atStart:NO];
}
#endif
class NativeMoveWatch : public QObject
{
public:
    NativeMoveWatch(QWidget *owner, std::function<void()> begin, std::function<void(QPoint)> moved,
        std::function<void(QPoint, bool)> ended) : QObject(owner)
    {
        NSWindow *window = reinterpret_cast<NSView *>(owner->winId()).window;
        QPointer<NativeMoveWatch> alive(this);
        beginObserver = [[NSNotificationCenter.defaultCenter addObserverForName:NSWindowWillMoveNotification object:window queue:nil usingBlock:^(NSNotification *) {
            if (!alive) return;
            begin();
            if (alive->timer) { [alive->timer invalidate]; [alive->timer release]; }
            alive->timer = [[NSTimer timerWithTimeInterval:0.03 repeats:YES block:^(NSTimer *timer) {
                if (!alive || !(NSEvent.pressedMouseButtons & 1)) {
                    [timer invalidate];
                    if (alive) ended(QCursor::pos(), bool(NSEvent.modifierFlags & NSEventModifierFlagOption));
                }
            }] retain];
            [NSRunLoop.mainRunLoop addTimer:alive->timer forMode:NSRunLoopCommonModes];
        }] retain];
        moveObserver = [[NSNotificationCenter.defaultCenter addObserverForName:NSWindowDidMoveNotification object:window queue:nil usingBlock:^(NSNotification *) {
            if (alive) moved(QCursor::pos());
        }] retain];
    }
    ~NativeMoveWatch() override
    {
        [NSNotificationCenter.defaultCenter removeObserver:beginObserver]; [beginObserver release];
        [NSNotificationCenter.defaultCenter removeObserver:moveObserver]; [moveObserver release];
        if (timer) { [timer invalidate]; [timer release]; }
    }
private:
    id beginObserver = nil, moveObserver = nil;
    NSTimer *timer = nil;
};
QObject *watchNativeWindowMove(QWidget *window, std::function<void()> begin,
    std::function<void(QPoint)> moved, std::function<void(QPoint, bool)> ended)
{ return new NativeMoveWatch(window, std::move(begin), std::move(moved), std::move(ended)); }

void setNativeExpansionPriority(QWidget *widget, bool expanded)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    if (!window) return;
    auto prior = (NSNumber *)objc_getAssociatedObject(window, &expansionLevelKey);
    if (expanded) {
        if (!prior) objc_setAssociatedObject(window, &expansionLevelKey, @(window.level), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        NSInteger highest = NSFloatingWindowLevel;
        for (auto candidate : QApplication::topLevelWidgets()) {
            if (candidate == widget || !candidate->isVisible() || candidate->isMinimized()) continue;
            if (!candidate->objectName().startsWith("unboundFloating_") && candidate->objectName() != "unboundPanelTest") continue;
            NSWindow *other = reinterpret_cast<NSView *>(candidate->winId()).window;
            highest = MAX(highest, other.level);
        }
        window.level = highest + 1;
    } else if (prior) {
        window.level = prior.integerValue;
        objc_setAssociatedObject(window, &expansionLevelKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}
int nativeWorkspaceTopInset(QWidget *page)
{
    if(!page)return 0;
    auto main=page->window();
    NSView *view=reinterpret_cast<NSView *>(main->winId());
    NSWindow *window=view.window;
    int top=0;
    if(window && (window.styleMask&NSWindowStyleMaskFullSizeContentView)) {
        const NSRect safe=[view convertRect:window.contentLayoutRect fromView:nil];
        const double distance=view.isFlipped?NSMinY(safe)-NSMinY(view.bounds):NSMaxY(view.bounds)-NSMaxY(safe);
        top=qMax(0,int(std::ceil(distance)));
    }
    // Resolve can draw its project strip inside Qt's client rectangle even
    // when AppKit reports no full-size title-bar inset. Preserve that real row.
    for(auto strip:main->findChildren<QWidget *>("UiProjectsMainBarImp",Qt::FindDirectChildrenOnly))
        if(strip->isVisible())top=qMax(top,strip->mapTo(main,QPoint(0,strip->height())).y());
    return qMax(0,top-page->mapTo(main,QPoint()).y());
}
void applyNativeChrome(QWidget *widget, bool compact)
{
    NSView *view = reinterpret_cast<NSView *>(widget->winId());
    NSWindow *window = view.window;
    if (!window) return;
    // Qt tool ownership preserves the main window's shortcut scope; native level
    // stays normal so the explicit hierarchy and optional control pin decide Z.
    if (widget->windowType() == Qt::Tool) {
        if (window.level != NSNormalWindowLevel) window.level = NSNormalWindowLevel;
        if (window.hidesOnDeactivate) window.hidesOnDeactivate = NO;
    }
    const auto visibility = compact ? NSWindowTitleHidden : NSWindowTitleVisible;
    if (window.titleVisibility != visibility) window.titleVisibility = visibility;
    if (window.titlebarAppearsTransparent != compact) window.titlebarAppearsTransparent = compact;
    const auto mask = compact ? window.styleMask | NSWindowStyleMaskFullSizeContentView :
        window.styleMask & ~NSWindowStyleMaskFullSizeContentView;
    // Request a native style change only when the current mask differs.
    if (window.styleMask != mask) window.styleMask = mask;
}
bool setNativePin(QWidget *widget, bool pinned, const std::function<void(bool)> &changed)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    NSButton *green = [window standardWindowButton:NSWindowZoomButton];
    NSButton *yellow = [window standardWindowButton:NSWindowMiniaturizeButton];
    auto button = (UnboundPinLight *)objc_getAssociatedObject(window, &pinKey);
    if (!green || !yellow || !green.superview || green.isHidden) { if (button) button.hidden = YES; return false; }
    if (!button) {
        button = [[UnboundPinLight alloc] initWithFrame:green.frame];
        button.buttonType = NSButtonTypeToggle; button.bordered = NO; button.title = @"";
        button.target = button; button.action = @selector(pinClicked:);
        button.toolTip = @"Keep above the Resolve workspace";
        [button setAccessibilityLabel:@"Pin above the Resolve workspace"];
        objc_setAssociatedObject(window, &pinKey, button, OBJC_ASSOCIATION_RETAIN_NONATOMIC); [button release];
    }
    if (button.superview != green.superview) { [button removeFromSuperview]; [green.superview addSubview:button]; }
    NSRect frame = green.frame;
    frame.origin.x += green.frame.origin.x - yellow.frame.origin.x;
    if (!NSEqualRects(button.frame,frame)) button.frame = frame;
    if (button.hidden) button.hidden = NO;
    const auto state = pinned ? NSControlStateValueOn : NSControlStateValueOff;
    const bool stateChanged = button.state != state;
    if (stateChanged) button.state = state;
    // Reserve actual traffic-light space instead of relying on a centered native title.
    auto title = (NSTextField *)objc_getAssociatedObject(window, &titleKey);
    if (!title) {
        title = [[NSTextField alloc] initWithFrame:NSZeroRect];
        title.editable = NO; title.selectable = NO; title.bezeled = NO; title.drawsBackground = NO;
        title.font = [NSFont boldSystemFontOfSize:12]; title.textColor = NSColor.labelColor;
        title.lineBreakMode = NSLineBreakByTruncatingTail;
        objc_setAssociatedObject(window, &titleKey, title, OBJC_ASSOCIATION_RETAIN_NONATOMIC); [title release];
    }
    if (title.superview != green.superview) { [title removeFromSuperview]; [green.superview addSubview:title]; }
    const bool strip=widget->property("unboundToolbarStrip").toBool();
    if (title.hidden!=strip) title.hidden=strip;
    if (!strip) {
        if (window.titleVisibility != NSWindowTitleHidden) window.titleVisibility = NSWindowTitleHidden;
        const CGFloat left = NSMaxX(frame) + 12;
        auto header=(OpenGateHeaderControl *)objc_getAssociatedObject(window,&headerMenuKey);
        const CGFloat right=header && !header.hidden ? NSMinX(header.frame)-8 : NSWidth(green.superview.bounds)-12;
        const auto titleFrame = NSMakeRect(left, NSMidY(frame)-9, MAX(0, right-left), 18);
        if (!NSEqualRects(title.frame,titleFrame)) title.frame = titleFrame;
        NSString *text = window.title ?: @"";
        if (![title.stringValue isEqualToString:text]) title.stringValue = text;
        if (![title.toolTip isEqualToString:text]) title.toolTip = text;
    }
    [button setChangeCallback:changed];
    if (stateChanged) [button setNeedsDisplay:YES];
    return true;
}
bool setNativeHeaderMenu(QWidget *widget,bool enabled,const std::function<void()> &opened)
{
    if (!widget || !widget->testAttribute(Qt::WA_WState_Created)) return false;
    NSWindow *window=reinterpret_cast<NSView *>(widget->winId()).window;
    if (!window) return false;
    auto button=(OpenGateHeaderControl *)objc_getAssociatedObject(window,&headerMenuKey);
    NSButton *green=[window standardWindowButton:NSWindowZoomButton];
    if (!enabled || !green || !green.superview || green.hidden) {
        if (button) { button.hidden=YES; [button setOpenCallback:{}]; } return false;
    }
    if (!button) {
        button=[[OpenGateHeaderControl alloc] initWithFrame:NSZeroRect];button.title=@"Panels ▾";
        button.bezelStyle=NSBezelStyleRounded;button.font=[NSFont systemFontOfSize:11];
        button.target=button;button.action=@selector(openPanels:);button.toolTip=@"Panel doors and floating windows";
        [button setAccessibilityLabel:@"OpenGate Panels"];
        objc_setAssociatedObject(window,&headerMenuKey,button,OBJC_ASSOCIATION_RETAIN_NONATOMIC);[button release];
    }
    if (button.superview!=green.superview) { [button removeFromSuperview];[green.superview addSubview:button]; }
    const NSRect bounds=green.superview.bounds;
    const CGFloat height=MIN(18,MAX(0,NSHeight(bounds)-4));
    const CGFloat y=MAX(NSMinY(bounds)+2,MIN(NSMidY(green.frame)-height/2,NSMaxY(bounds)-height-2));
    const CGFloat x=MAX(NSMaxX(green.frame)+48,NSMaxX(bounds)-80);
    const CGFloat width=MIN(70,MAX(0,NSMaxX(bounds)-x-2));
    button.frame=NSMakeRect(x,y,width,height);
    button.hidden=NO;[button setOpenCallback:opened];
    auto title=(NSTextField *)objc_getAssociatedObject(window,&titleKey);
    if (title && !title.hidden) { auto frame=title.frame;frame.size.width=MAX(0,NSMinX(button.frame)-8-frame.origin.x);title.frame=frame; }
    return true;
}
#ifdef UNBOUND_NATIVE_TESTS
QString nativeTitleToolTipForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    auto title = (NSTextField *)objc_getAssociatedObject(window, &titleKey);
    return title ? QString::fromUtf8(title.toolTip.UTF8String) : QString();
}
QString nativeTitleTextForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    auto title = (NSTextField *)objc_getAssociatedObject(window, &titleKey);
    return title ? QString::fromUtf8(title.stringValue.UTF8String) : QString();
}
QRect nativeHeaderMenuFrameForTest(QWidget *widget) {
    NSWindow *window=reinterpret_cast<NSView *>(widget->winId()).window;
    auto button=(OpenGateHeaderControl *)objc_getAssociatedObject(window,&headerMenuKey);
    if (!button || button.hidden) return {};
    auto frame=button.frame;return QRect(frame.origin.x,frame.origin.y,frame.size.width,frame.size.height);
}
void clickNativeHeaderMenuForTest(QWidget *widget) {
    NSWindow *window=reinterpret_cast<NSView *>(widget->winId()).window;
    auto button=(OpenGateHeaderControl *)objc_getAssociatedObject(window,&headerMenuKey);
    if (button) [button performClick:nil];
}
QRect nativeTitleFrameForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    auto title = (NSTextField *)objc_getAssociatedObject(window, &titleKey);
    if (!title || title.hidden) return {};
    const auto frame = title.frame;
    return QRect(qRound(frame.origin.x), qRound(frame.origin.y), qRound(frame.size.width), qRound(frame.size.height));
}
void activateNativeWindowForTest(QWidget *widget) { [NSApp activateIgnoringOtherApps:YES]; [reinterpret_cast<NSView *>(widget->winId()).window makeKeyAndOrderFront:nil]; }
qint64 nativeWindowLevelForTest(QWidget *widget) { return reinterpret_cast<NSView *>(widget->winId()).window.level; }
qint64 nativeFrontmostProcessForTest() { return NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier; }
bool saveNativeTrafficForTest(QWidget *widget, const QString &path)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    auto pin = (UnboundPinLight *)objc_getAssociatedObject(window, &pinKey);
    NSButton *red = [window standardWindowButton:NSWindowCloseButton];
    if (!pin || !red || pin.superview != red.superview) return false;
    NSRect bounds = NSInsetRect(NSUnionRect(red.frame, pin.frame), -4, -4);
    NSBitmapImageRep *rep = [red.superview bitmapImageRepForCachingDisplayInRect:bounds];
    [red.superview cacheDisplayInRect:bounds toBitmapImageRep:rep];
    NSData *data = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    return [data writeToFile:[NSString stringWithUTF8String:path.toUtf8().constData()] atomically:YES];
}
QList<QRect> nativeTrafficFramesForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    auto pin = (UnboundPinLight *)objc_getAssociatedObject(window, &pinKey);
    QList<QRect> frames;
    if (!pin || ![window standardWindowButton:NSWindowCloseButton] || ![window standardWindowButton:NSWindowMiniaturizeButton] || ![window standardWindowButton:NSWindowZoomButton]) return {};
    for (NSButton *button in @[[window standardWindowButton:NSWindowCloseButton], [window standardWindowButton:NSWindowMiniaturizeButton], [window standardWindowButton:NSWindowZoomButton], pin]) {
        if (!button || button.hidden) return {};
        NSRect r = button.frame; frames.append(QRect(qRound(r.origin.x), qRound(r.origin.y), qRound(r.size.width), qRound(r.size.height)));
    }
    return frames;
}
void clickNativePinForTest(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    auto pin = (UnboundPinLight *)objc_getAssociatedObject(window, &pinKey); [pin performClick:nil];
}
#endif
int nativeChromeFlags(QWidget *widget)
{
    NSView *view = reinterpret_cast<NSView *>(widget->winId());
    NSWindow *window = view.window;
    if (!window) return 0;
    return ((window.styleMask & NSWindowStyleMaskTitled) ? 1 : 0) |
        ((window.styleMask & NSWindowStyleMaskResizable) ? 2 : 0) |
        ((window.styleMask & NSWindowStyleMaskFullSizeContentView) ? 4 : 0);
}
void orderNativeWindows(const QList<QWidget *> &bottomToTop)
{
    NSWindow *anchor = NSApp.keyWindow;
    for (auto widget : bottomToTop) if (widget && nativeWindowNumber(widget) == anchor.windowNumber) anchor = nil;
    NSWindow *previous = nil;
    for (QWidget *widget : bottomToTop) {
        if (!widget || !widget->isVisible() || widget->isMinimized()) continue;
        NSView *view = reinterpret_cast<NSView *>(widget->winId());
        NSWindow *window = view.window;
        if (!window) continue;
        if (!previous && anchor && anchor.isVisible && anchor.level == window.level)
            [window orderWindow:NSWindowBelow relativeTo:anchor.windowNumber];
        else [window orderWindow:NSWindowAbove relativeTo:(previous && previous.level == window.level ? previous.windowNumber : 0)];
        previous = window;
    }
}
// Own one clipping NSView per group cell. Resolve's original widgets, GPU
// surfaces and native restoration slots retain their own rendering attributes.
bool setNativeGroupSurfaceClipping(QWidget *widget)
{
    if (!widget || !widget->objectName().startsWith("unboundGroupCell_")) return false;
    widget->setAttribute(Qt::WA_DontCreateNativeAncestors);
    widget->setAttribute(Qt::WA_NativeWindow);
    NSView *view = reinterpret_cast<NSView *>(widget->winId());
    if (!view || ![view respondsToSelector:@selector(setClipsToBounds:)]) return false;
    if (!view.clipsToBounds) view.clipsToBounds = YES;
    return view.clipsToBounds;
}

void normalizeNativePanel(QWidget *widget)
{
    if (!widget || widget->windowType() != Qt::Tool) return;
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    if (window.hidesOnDeactivate) window.hidesOnDeactivate = NO;
    if (window.level != NSNormalWindowLevel) window.level = NSNormalWindowLevel;
}
int nativeVisibilityFlags(QWidget *widget)
{
    NSWindow *window = reinterpret_cast<NSView *>(widget->winId()).window;
    return (window.isVisible ? 1 : 0) | (window.hidesOnDeactivate ? 2 : 0) | (window.isMiniaturized ? 4 : 0);
}
bool setNativeBackgroundHidden(QWidget *main, const QList<QWidget *> &panels, bool hidden)
{
    if (!main) return false;
    NSWindow *background = reinterpret_cast<NSView *>(main->winId()).window;
    if (hidden) {
        if (background.isVisible) [background orderOut:nil];
        for (auto panel : panels) if (panel && panel->isVisible() && !panel->isMinimized()) {
            NSWindow *window = reinterpret_cast<NSView *>(panel->winId()).window;
            if (!window.isVisible) [window orderFront:nil];
        }
        for (auto panel : panels) if (panel && panel->isVisible() && !panel->isMinimized() && !(nativeVisibilityFlags(panel) & 1)) {
            [background orderFront:nil]; return false;
        }
    } else [background orderFront:nil];
    main->setProperty("unboundBackgroundHidden", hidden); return true;
}
qint64 nativeKeyWindowNumber() { return NSApp.keyWindow ? NSApp.keyWindow.windowNumber : 0; }
qint64 nativeWindowNumber(QWidget *widget)
{
    NSView *view = reinterpret_cast<NSView *>(widget->winId());
    return view.window ? view.window.windowNumber : 0;
}
QJsonObject nativeWindowGeometryState(QWidget *widget)
{
    if (!widget || !widget->testAttribute(Qt::WA_WState_Created)) return {};
    NSWindow *window=reinterpret_cast<NSView *>(widget->winId()).window;
    if (!window) return {};
    const NSRect frame=window.frame, content=[window contentRectForFrameRect:frame];
    return {{"native_id",qint64(window.windowNumber)},{"native_frame_x",frame.origin.x},{"native_frame_y",frame.origin.y},
        {"native_content_x",content.origin.x},{"native_content_y",content.origin.y},
        {"native_content_width",content.size.width},{"native_content_height",content.size.height},
        {"backing_scale",window.backingScaleFactor},{"native_primary_height",NSScreen.screens.firstObject.frame.size.height}};
}
QList<qint64> nativeOrderedWindowNumbers()
{
    QList<qint64> result;
    // NSApp.orderedWindows can omit owned Qt tool windows. Use the actual
    // on-screen stack, including child windows, without capturing window content.
    CFArrayRef info = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    for (NSDictionary *entry in (NSArray *)info) {
        if ([entry[(id)kCGWindowOwnerPID] intValue] == NSProcessInfo.processInfo.processIdentifier)
            result.append([entry[(id)kCGWindowNumber] longLongValue]);
    }
    if (info) CFRelease(info);
    return result;
}
}
