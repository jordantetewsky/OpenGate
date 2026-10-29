// SPDX-License-Identifier: MIT
#pragma once
namespace Unbound {
// Thread-safe, idempotent bootstrap; does not construct a QApplication.
bool startController();
}
