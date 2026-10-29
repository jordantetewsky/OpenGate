// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QTemporaryDir>
#include <cstdio>
namespace OpenGateTest {
inline QString evidenceTemplate(const QString &name) {
    auto directory=qEnvironmentVariable("OPENGATE_FIXTURE_ROOT");
    if(directory.isEmpty())directory=QDir::tempPath();else QDir().mkpath(directory);
    return QDir(directory).filePath("opengate-"+name+"-XXXXXX");
}
inline void retainEvidence(QTemporaryDir &directory) {
    if(qEnvironmentVariableIsEmpty("OPENGATE_FIXTURE_ROOT"))return;
    directory.setAutoRemove(false);
    std::printf("Fixture evidence: %s\n",qPrintable(directory.path()));std::fflush(stdout);
}
}
