// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "../../core/workspace-callers/timelinesceneswitchchecks.h"
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir logs;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", logs.path().toUtf8());
    int failures = 0;
    OpenGateTest::timelineSceneSwitchChecks([&](bool ok, const char *why) {
        if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", why); }
    });
    std::printf("timeline scene switch failures=%d\n", failures);
    return failures ? 1 : 0;
}
