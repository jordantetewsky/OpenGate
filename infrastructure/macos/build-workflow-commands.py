#!/usr/bin/env python3
"""Compatibility entry for the accepted Mac workflow command generator."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parents[1]/'shared/build-workflow-commands.py'),run_name='__main__')
