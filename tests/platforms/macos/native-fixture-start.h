#pragma once
#include <QFile>
// These legacy fixtures explicitly exercise Native mode and often model only
// two panels. First-launch managed behavior is covered by opengate-first-open.
// Never overwrite a saved/restart scenario with a synthetic baseline.
inline void prepareNativeFixture() {
    const auto path=qEnvironmentVariable("UNBOUND_WORKSPACE_FILE");if(path.isEmpty())return;
    QFile file(path);if(file.exists())return;
    if(file.open(QIODevice::WriteOnly))file.write("{\"version\":1,\"page\":\"edit\",\"layout_mode\":\"native\"}");
}
