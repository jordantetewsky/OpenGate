#!/usr/bin/env python3
"""Build the matching Qt metadata compiler for native-shaped fixture objects."""
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
qt = root / "build/macos/qtbase-source"
output = root / "build/macos/test-tools"
private = output / "include/QtCore/private"
private.mkdir(parents=True, exist_ok=True)
for source in [qt / "src/corelib/global/qconfig_p.h", qt / "src/corelib/qtcore-config_p.h"]:
    shutil.copy2(source, private / source.name)
arguments = ["xcrun", "clang++", "-std=c++17", "-O1", "-DQT_MOC", "-DQT_NO_COMPRESS", "-DQT_NO_FOREACH"]
for include in [output / "include", root / "build/macos/qt-generated", qt / "include", qt / "include/QtCore",
                qt / "include/QtCore/5.15.2", qt / "include/QtCore/5.15.2/QtCore",
                qt / "src/tools/moc", qt / "src/3rdparty/tinycbor/src"]:
    arguments += ["-I", str(include)]
arguments += [str(qt / "src/tools/moc" / source) for source in
              ["main.cpp", "moc.cpp", "preprocessor.cpp", "generator.cpp", "parser.cpp", "token.cpp", "collectjson.cpp"]]
frameworks = "/Applications/DaVinci Resolve/DaVinci Resolve.app/Contents/Frameworks"
arguments += ["-L", frameworks, "-lQt5Core.5", "-Wl,-rpath," + frameworks, "-o", str(output / "moc")]
subprocess.run(arguments, check=True)
subprocess.run([str(output / "moc"), "-v"], check=True)
