#!/usr/bin/env python3
"""Check observation coverage, never infer loading success from missing signals."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("events", type=Path)
parser.add_argument("--session", required=True)
args = parser.parse_args()
records = []
malformed = 0
for line in args.events.read_text().splitlines():
    try:
        record = json.loads(line)
    except json.JSONDecodeError:
        malformed += 1
        continue
    if record.get("session") == args.session:
        records.append(record)
required = {
    "mediaPoolClipView": {"PlaceClipOnSourceViewer"},
    "widgetEditSourcePlayer": {"DragEntered", "DropTriggered", "CurrentItemsChanged", "CurrentTitleChanged"},
}
connected = {name: set() for name in required}
handlers = set()
for record in records:
    if record.get("event") == "viewer.native-signals":
        name = record.get("player", {}).get("name")
        if name in connected:
            connected[name] = set()
            for signal in record.get("signals", []):
                if signal.get("connected"):
                    connected[name].add(signal.get("signal", "").split("(")[0])
    if record.get("event") == "viewer.native-methods":
        if record.get("object", {}).get("class") == "UiConformerPanelImp":
            handlers = set()
            for method in record.get("methods", []):
                if "method_type" in method:
                    handlers.add(method["name"])
missing = {name: sorted(names-connected[name]) for name, names in required.items() if names-connected[name]}
missing_handlers = sorted({"OnMpPlaceOnSourceViewer", "OnClipDropped"}-handlers)
ready = bool(records) and not missing and not missing_handlers and not malformed
print(json.dumps({
    "session": args.session,
    "coverage_gate_passed": ready,
    "records": len(records),
    "malformed_lines": malformed,
    "missing_signal_connections": missing,
    "missing_handler_metadata": missing_handlers,
    "handler_execution_observed": False,
    "loading_success_verified": False,
    "interpretation": "Coverage only. Missing coverage cannot establish a missing load request. Handler metadata cannot establish handler execution.",
}, indent=2))
raise SystemExit(0 if ready else 2)
