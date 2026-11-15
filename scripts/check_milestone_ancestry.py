#!/usr/bin/env python3
"""Validate declared milestone predecessors against the checked-out Git history."""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path

MILESTONE_PATTERN = re.compile(r"\b(M3b|M3c|M4|M5|M6)\b", re.IGNORECASE)
SHA_PATTERN = re.compile(r"^[0-9a-f]{40}$")


def fail(message: str) -> int:
    print(f"milestone ancestry: ERROR: {message}", file=sys.stderr)
    return 1


def main() -> int:
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if not event_path:
        return fail("GITHUB_EVENT_PATH is not set")

    try:
        event = json.loads(Path(event_path).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return fail(f"cannot read GitHub event payload: {exc}")

    pull_request = event.get("pull_request")
    if not isinstance(pull_request, dict):
        print("milestone ancestry: no pull_request payload; skipped")
        return 0

    title = str(pull_request.get("title", ""))
    match = MILESTONE_PATTERN.search(title)
    if not match:
        print("milestone ancestry: PR is not a gated future milestone; skipped")
        return 0

    milestone = match.group(1)
    milestone = "M" + milestone[1:].lower() if milestone.lower().startswith("m") else milestone
    if milestone == "M3b":
        milestone = "M3b"
    elif milestone == "M3c":
        milestone = "M3c"
    else:
        milestone = milestone.upper()

    ledger_path = Path("docs/milestones.json")
    try:
        ledger = json.loads(ledger_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return fail(f"cannot read {ledger_path}: {exc}")

    entry = ledger.get("future_milestones", {}).get(milestone)
    if not isinstance(entry, dict):
        return fail(f"{milestone} has no future_milestones ledger entry")

    required = entry.get("required_predecessor")
    if not isinstance(required, str) or not SHA_PATTERN.fullmatch(required):
        return fail(
            f"{milestone} predecessor SHA is not recorded; "
            "the predecessor milestone must be merged and recorded before this PR opens"
        )

    body = str(pull_request.get("body") or "")
    declaration = re.search(r"(?im)^Milestone-Ancestors:\s*(.+)$", body)
    if not declaration:
        return fail("PR body must contain 'Milestone-Ancestors: <full SHA>'")

    declared = declaration.group(1).strip().split()
    if required not in declared:
        return fail(f"PR declaration omits required predecessor SHA {required}")

    result = subprocess.run(
        ["git", "merge-base", "--is-ancestor", required, "HEAD"],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode == 1:
        return fail(f"required predecessor {required} is not an ancestor of this PR")
    if result.returncode != 0:
        return fail(f"git merge-base failed: {result.stderr.strip()}")

    print(f"milestone ancestry: {milestone} descends from required predecessor {required}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
