// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "../../core/workspace-callers/timelinefollowupchecks.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir files;qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;OpenGateTest::timelineFollowupChecks([&](bool good,const char *why){if(!good){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}});
    std::printf("timeline followup failures=%d\n",failures);return failures?1:0;
}
