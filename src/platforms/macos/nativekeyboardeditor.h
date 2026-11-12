// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QDialog>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <array>
#include <cstring>
#include <dlfcn.h>
#include <memory>
#include <new>
#include "nativeimageverification.h"

namespace Unbound::NativeKeyboardEditor {
namespace Detail {
// Resolve 21.1.1 ARM64: static evidence is recorded in the native-input trace.
inline constexpr char imageHash[] =
    "801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9";
inline constexpr quint64 imageSize = 511507696;
inline constexpr std::uintptr_t constructor = 0x1052159a0ULL;
inline constexpr std::uintptr_t initialize = 0x1052189a4ULL;
inline constexpr std::uintptr_t vtableHeader = 0x11d4b1580ULL;
inline constexpr std::size_t objectSize = 0x108;
inline constexpr std::size_t tableBytes = 0x1f0;
using Table = std::array<std::uintptr_t, tableBytes / sizeof(std::uintptr_t)>;
struct CodeRange { std::uintptr_t address; std::size_t size; const char *hash; };
inline constexpr CodeRange codeRanges[] = {
    {constructor, 2084, "c7d71a456ed748233eb0c5c2c291701f99310f343348d400462be1611adaa0b1"},
    {initialize, 252, "28fcb23ee5a38c505f9ed59cb55fbeea3fbf33cfbcb2fb6e1d2017818dba1a74"},
    {0x1052187ecULL, 384, "24474ff3090be56edff1e4b35089a1e136a82db80cab39b6a43c8ece4afaae94"},
    {0x105218978ULL, 20, "7df7bfda5c7dea2da91cf89c5ca0a14ac2def55bac7493798c816b8dc187cadf"},
    {0x102af8464ULL, 16, "38ef3a8270ea3d7875a74bf8171ac1d4d7f8857b06bff77bbc9e8f9a3741e719"},
    {0x102af8474ULL, 476, "17a3fa73453fc4f0358ac45aa82d4f344974b47a2bb2cd9fd0a7e5d5a8e5212c"},
};
struct Slot { std::size_t offset; std::uintptr_t address; const char *symbol; };
inline constexpr Slot expectedSlots[] = {
    {0x0, 0x1051fe4fcULL, nullptr},
    {0x8, 0x1051fe518ULL, nullptr},
    {0x10, 0x1051fe574ULL, nullptr},
    {0x18, 0x10521896cULL, nullptr},
    {0x20, 0x105218978ULL, nullptr},
    {0x28, 0, "_ZN7QWidget5eventEP6QEvent"},
    {0x30, 0, "_ZN8BmdDavUI17UiFramelessDialog11eventFilterEP7QObjectP6QEvent"},
    {0x38, 0, "_ZN7QObject10timerEventEP11QTimerEvent"},
    {0x40, 0, "_ZN7QObject10childEventEP11QChildEvent"},
    {0x48, 0, "_ZN7QObject11customEventEP6QEvent"},
    {0x50, 0, "_ZN7QObject13connectNotifyERK11QMetaMethod"},
    {0x58, 0, "_ZN7QObject16disconnectNotifyERK11QMetaMethod"},
    {0x60, 0, "_ZNK7QWidget7devTypeEv"},
    {0x68, 0, "_ZN7QDialog10setVisibleEb"},
    {0x70, 0, "_ZNK7QDialog8sizeHintEv"},
    {0x78, 0, "_ZNK7QDialog15minimumSizeHintEv"},
    {0x80, 0, "_ZNK7QWidget14heightForWidthEi"},
    {0x88, 0, "_ZNK7QWidget17hasHeightForWidthEv"},
    {0x90, 0, "_ZNK7QWidget11paintEngineEv"},
    {0x98, 0, "_ZN8BmdDavUI17UiFramelessDialog15mousePressEventEP11QMouseEvent"},
    {0xa0, 0, "_ZN8BmdDavUI17UiFramelessDialog17mouseReleaseEventEP11QMouseEvent"},
    {0xa8, 0, "_ZN7QWidget21mouseDoubleClickEventEP11QMouseEvent"},
    {0xb0, 0, "_ZN8BmdDavUI17UiFramelessDialog14mouseMoveEventEP11QMouseEvent"},
    {0xb8, 0, "_ZN7QWidget10wheelEventEP11QWheelEvent"},
    {0xc0, 0, "_ZN7QDialog13keyPressEventEP9QKeyEvent"},
    {0xc8, 0, "_ZN7QWidget15keyReleaseEventEP9QKeyEvent"},
    {0xd0, 0, "_ZN7QWidget12focusInEventEP11QFocusEvent"},
    {0xd8, 0, "_ZN7QWidget13focusOutEventEP11QFocusEvent"},
    {0xe0, 0, "_ZN7QWidget10enterEventEP6QEvent"},
    {0xe8, 0, "_ZN7QWidget10leaveEventEP6QEvent"},
    {0xf0, 0, "_ZN7QWidget10paintEventEP11QPaintEvent"},
    {0xf8, 0, "_ZN7QWidget9moveEventEP10QMoveEvent"},
    {0x100, 0, "_ZN8BmdDavUI17UiFramelessDialog11resizeEventEP12QResizeEvent"},
    {0x108, 0, "_ZN7QDialog10closeEventEP11QCloseEvent"},
    {0x110, 0, "_ZN7QDialog16contextMenuEventEP17QContextMenuEvent"},
    {0x118, 0, "_ZN7QWidget11tabletEventEP12QTabletEvent"},
    {0x120, 0, "_ZN7QWidget11actionEventEP12QActionEvent"},
    {0x128, 0, "_ZN7QWidget14dragEnterEventEP15QDragEnterEvent"},
    {0x130, 0, "_ZN7QWidget13dragMoveEventEP14QDragMoveEvent"},
    {0x138, 0, "_ZN7QWidget14dragLeaveEventEP15QDragLeaveEvent"},
    {0x140, 0, "_ZN7QWidget9dropEventEP10QDropEvent"},
    {0x148, 0, "_ZN7QDialog9showEventEP10QShowEvent"},
    {0x150, 0, "_ZN7QWidget9hideEventEP10QHideEvent"},
    {0x158, 0, "_ZN7QWidget11nativeEventERK10QByteArrayPvPl"},
    {0x160, 0, "_ZN7QWidget11changeEventEP6QEvent"},
    {0x168, 0, "_ZNK7QWidget6metricEN12QPaintDevice17PaintDeviceMetricE"},
    {0x170, 0, "_ZNK7QWidget11initPainterEP8QPainter"},
    {0x178, 0, "_ZNK7QWidget10redirectedEP6QPoint"},
    {0x180, 0, "_ZNK7QWidget13sharedPainterEv"},
    {0x188, 0, "_ZN7QWidget16inputMethodEventEP17QInputMethodEvent"},
    {0x190, 0, "_ZNK7QWidget16inputMethodQueryEN2Qt16InputMethodQueryE"},
    {0x198, 0, "_ZN7QWidget18focusNextPrevChildEb"},
    {0x1a0, 0, "_ZN7QDialog4openEv"},
    {0x1a8, 0, "_ZN8BmdDavUI17UiFramelessDialog4execEv"},
    {0x1b0, 0, "_ZN7QDialog4doneEi"},
    {0x1b8, 0, "_ZN7QDialog6acceptEv"},
    {0x1c0, 0x105219878ULL, nullptr},
    {0x1c8, 0, "_ZNK8BmdDavUI17UiFramelessDialog16ConsumeDragEventEP7QWidget"},
    {0x1d0, 0, "_ZN8BmdDavUI17UiFramelessDialog15ClickOnTitleBarERK6QPoint"},
    {0x1d8, 0x102aa9dfcULL, nullptr},
};
inline const MacNativeImageVerification::Image &image() {
    static const auto verified = MacNativeImageVerification::verifyImage(imageHash, imageSize);
    return verified;
}
inline bool validate(Table &table, QString &error) {
    if (qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_INPUT_CUSTOMIZATION") != 1) {
        error = QStringLiteral("Native keyboard editor experiment is disabled.");
        return false;
    }
    if (!qApp || QThread::currentThread() != qApp->thread()) {
        error = QStringLiteral("Native keyboard editor requires Resolve's GUI thread.");
        return false;
    }
    const auto &verified = image();
    if (!verified.valid) {
        error = QString::fromLatin1(verified.status);
        return false;
    }
    using MacNativeImageVerification::copyMemory;
    const auto slide = verified.slide;
    for (const auto &range : codeRanges) {
        QByteArray code(int(range.size), Qt::Uninitialized);
        if (!copyMemory(slide + range.address, code.data(), range.size) ||
            QCryptographicHash::hash(code, QCryptographicHash::Sha256).toHex() != range.hash) {
            error = QStringLiteral("Native keyboard editor code does not match the verified build.");
            return false;
        }
    }
    constexpr char className[] = "24UiActionsEditorDialogImp";
    char loadedName[sizeof(className)]{};
    std::uintptr_t namePointer = 0;
    if (!copyMemory(slide + vtableHeader, table.data(), sizeof(table)) ||
        table[0] != 0 || table[1] != slide + 0x11d4b17c0ULL ||
        !copyMemory(slide + 0x11d4b17c8ULL, &namePointer, sizeof(namePointer)) ||
        namePointer != slide + 0x11785a2f6ULL ||
        !copyMemory(namePointer, loadedName, sizeof(loadedName)) ||
        std::memcmp(loadedName, className, sizeof(className)) != 0) {
        error = QStringLiteral("Native keyboard editor class layout does not match.");
        return false;
    }
    for (const auto &slot : expectedSlots) {
        const auto expected = slot.symbol ? std::uintptr_t(dlsym(RTLD_DEFAULT, slot.symbol))
                                          : slide + slot.address;
        if (!expected || table[2 + slot.offset / sizeof(std::uintptr_t)] != expected) {
            error = QStringLiteral("Native keyboard editor virtual methods do not match.");
            return false;
        }
    }
    return true;
}
inline int skipModalExecution(void *) noexcept { return 0; }
// Only this owned instance points at the temporary table, only during initialization.
class InitializationTable final {
public:
    InitializationTable(QDialog *dialog, Table table) : alive(dialog), replacement(table) {
        std::memcpy(&original, static_cast<const void *>(dialog), sizeof(original));
        replacement[2 + 0x1a8 / sizeof(std::uintptr_t)] =
            reinterpret_cast<std::uintptr_t>(&skipModalExecution);
        const auto address = reinterpret_cast<std::uintptr_t>(replacement.data() + 2);
        std::memcpy(static_cast<void *>(dialog), &address, sizeof(address));
    }
    ~InitializationTable() {
        if (alive) std::memcpy(static_cast<void *>(alive.data()), &original, sizeof(original));
    }
    InitializationTable(const InitializationTable &) = delete;
    InitializationTable &operator=(const InitializationTable &) = delete;
private:
    QPointer<QDialog> alive;
    Table replacement;
    std::uintptr_t original = 0;
};
// QPointer also handles a native initialization callback deleting its receiver.
struct OwnedDialog {
    QPointer<QDialog> dialog;
    ~OwnedDialog() { if (dialog) delete dialog.data(); }
};
inline void refreshAfterFinished(std::uintptr_t slide) {
    QPointer<QApplication> application = qApp;
    if (!application) return;
    QTimer::singleShot(0, application.data(), [application, slide] {
        if (application && !QCoreApplication::closingDown())
            reinterpret_cast<void (*)(QApplication *)>(slide + 0x102af8464ULL)(application.data());
    });
}
} // namespace Detail

inline bool available() {
    Detail::Table table{};
    QString error;
    return Detail::validate(table, error);
}
// Caller owns the returned native QDialog through ordinary Qt parent ownership.
// Reparent as Qt::Widget in the host; do not call the original stack-owned opener.
inline QDialog *create(QWidget *parent, QString &error) {
    error.clear();
    Detail::Table table{};
    if (!Detail::validate(table, error)) return nullptr;
    for(auto widget:QApplication::allWidgets()) {
        if(widget->inherits("UiActionsEditorDialogImp")) {
            error=QStringLiteral("Close the existing Keyboard Customization dialog before opening the panel.");
            return nullptr;
        }
    }
    const auto slide = Detail::image().slide;
    try {
        const auto freeUnconstructed = [](void *p) { ::operator delete(p); };
        std::unique_ptr<void, decltype(freeUnconstructed)> storage(
            ::operator new(Detail::objectSize), freeUnconstructed);
        reinterpret_cast<void (*)(void *, QWidget *)>(slide + Detail::constructor)(storage.get(), parent);
        Detail::OwnedDialog owner;
        owner.dialog = reinterpret_cast<QDialog *>(storage.release());
        auto *dialog = owner.dialog.data();
        std::uintptr_t primary = 0, secondary = 0;
        if (!MacNativeImageVerification::copyMemory(std::uintptr_t(dialog), &primary, sizeof(primary)) ||
            !MacNativeImageVerification::copyMemory(std::uintptr_t(dialog) + 0x10, &secondary, sizeof(secondary)) ||
            primary != slide + 0x11d4b1590ULL || secondary != slide + 0x11d4b1780ULL ||
            std::strcmp(dialog->metaObject()->className(), "UiActionsEditorDialogImp") != 0) {
            error = QStringLiteral("Constructed native keyboard editor failed its layout check.");
            return nullptr;
        }
        {
            Detail::InitializationTable initializationTable(dialog, table);
            reinterpret_cast<int (*)(QDialog *)>(slide + Detail::initialize)(dialog);
        }
        if (!owner.dialog) {
            error = QStringLiteral("Native keyboard editor closed during initialization.");
            return nullptr;
        }
        dialog->setAttribute(Qt::WA_DeleteOnClose, false);
        dialog->setWindowModality(Qt::NonModal);
        QObject::connect(dialog, &QDialog::finished, dialog, [slide](int) {
            Detail::refreshAfterFinished(slide);
        });
        owner.dialog.clear();
        return dialog;
    } catch (...) {
        error = QStringLiteral("Native keyboard editor initialization failed.");
        return nullptr;
    }
}
} // namespace Unbound::NativeKeyboardEditor
