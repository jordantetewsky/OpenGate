"""Show the resident OpenGate controls from Resolve's Workflow Integrations menu."""
import datetime
import json
import os
import pathlib
import uuid

directory = pathlib.Path.home() / "Library/Logs/OpenGate"
directory.mkdir(parents=True, exist_ok=True)
token = "workflow-" + str(uuid.uuid4())
temporary = directory / (token + ".request")
temporary.write_text(token)
os.replace(temporary, directory / "show-controls.request")
with (directory / "launches.jsonl").open("a") as log:
    log.write(json.dumps({"component": "OpenGate", "vendor": "CineWaltz", "event": "workflow.launch", "time": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                          "pid": os.getpid(), "request": token}) + "\n")
