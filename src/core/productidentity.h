// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
namespace Unbound {
inline QString productName() { return QStringLiteral("OpenGate"); }
inline QString productVendor() { return QStringLiteral("CineWaltz"); }
inline QString productCredit() {
    return QStringLiteral("OpenGate by CineWaltz\nBuilt on Unbound (MIT license).");
}
}
