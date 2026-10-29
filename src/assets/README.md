# Application icon

The master is `unbound-symbol.svg`: a transparent 256×256 vector rendition
of the pane's dock/undock button, with uniform #848484 outlines and inner curves offset from the outer curves.
Small PNG sizes align straight edges to whole pixels and rasterize directly
from vectors, with antialiasing confined to the curves and fractional edges.
The symbol and its generated icons are covered by the root MIT license.

The Windows release build regenerates the icons automatically through
`infrastructure/windows/generate-icons.ps1`. The generated Windows resource
`src/platforms/windows/resources/unbound.ico` contains transparent frames at 16, 20, 24, 32, 40, 48, 64, 96,
128 and 256 pixels. It is embedded in the EXE. Individual PNGs
are included for inspection; they are not required alongside the executable.
