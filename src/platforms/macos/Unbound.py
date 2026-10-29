"""Compatibility entry point; new installations use OpenGate.py."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name("OpenGate.py")), run_name="__main__")
