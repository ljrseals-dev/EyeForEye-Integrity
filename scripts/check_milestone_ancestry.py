#!/usr/bin/env python3
"""Validate declared milestone predecessors against the checked-out Git history."""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path

# Match complete milestone tokens. Unknown IDs are rejected against the ledger,
# rather than silently bypassing the gate. The optional suffix is part of the
# token, so M5b cannot be truncated to M5.
MILESTONE_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_])"
    r"(?:milestone[\s_-]+(?P<spelled>M?\d+[a-z]?)|(?P<short>M\d+[a-z]?))"
    r"(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
BODY_MILESTONE_PATTERN = re.compile(r"(?im)^Milestone:\s*(.*?)\s*$")
ANCESTOR_DECLARATION_PATTERN = re.compile(
    r"(?im)^Milestone-Ancestors:\s*(.*?)\s*$"
)
SHA_PATTERN = re.compile(r"^[0-9a-f]{40}$")


def normalize_milestone(value: str) -> str:
    """Normalize M5B, m5b, 5b, and 5B to the ledger key M5b."""
    token = value.upper()
    if token.startswith("M"):
        token = token[1:]
    suffix_match = re.fullmatch(r"(\d+)([A-Z]?)", token)
    if suffix_match is None:
        raise ValueError(f"invalid milestone identifier: {value!r}")
    number, suffix = suffix_match.groups()
    return f"M{number}{suffix.lower()}"


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
    if not isinstance(event, dict):
        return fail("GitHub event payload must be a JSON object")

    pull_request = event.get("pull_request")
    if not isinstance(pull_request, dict):
        print(
            "milestone ancestry: push/non-PR event; skipped "
            "(ancestry enforcement is pull-request-only)"
        )
        return 0

    title = pull_request.get("title")
    if not isinstance(title, str) or not title.strip():
        return fail("pull_request payload is missing a valid title")
    title_matches = list(MILESTONE_PATTERN.finditer(title))
    if not title_matches:
        print("milestone ancestry: PR title is not a gated future milestone; skipped")
        return 0

    try:
        title_milestones = {
            normalize_milestone(match.group("spelled") or match.group("short"))
            for match in title_matches
        }
    except ValueError as exc:
        return fail(str(exc))
    if len(title_milestones) != 1:
        return fail(
            "PR title names multiple milestone identifiers: "
            + ", ".join(sorted(title_milestones))
        )
    milestone = title_milestones.pop()

    ledger_path = Path("docs/milestones.json")
    try:
        ledger = json.loads(ledger_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return fail(f"cannot read {ledger_path}: {exc}")
    if not isinstance(ledger, dict):
        return fail(f"{ledger_path} must contain a JSON object")

    future_milestones = ledger.get("future_milestones")
    if not isinstance(future_milestones, dict):
        return fail("future_milestones must be an object in docs/milestones.json")

    entry = future_milestones.get(milestone)
    if not isinstance(entry, dict):
        known = ", ".join(sorted(future_milestones)) or "(none)"
        return fail(
            f"title identifies {milestone}, but docs/milestones.json has no "
            f"matching future_milestones entry (known: {known})"
        )

    body = str(pull_request.get("body") or "")
    body_matches = list(BODY_MILESTONE_PATTERN.finditer(body))
    if len(body_matches) > 1:
        return fail("PR body must contain at most one 'Milestone:' declaration")
    if body_matches:
        try:
            body_milestone = normalize_milestone(body_matches[0].group(1).strip())
        except ValueError:
            return fail("PR body 'Milestone:' declaration must be a valid ID such as M5b")
        if body_milestone != milestone:
            return fail(
                f"PR title identifies {milestone}, but PR body declares {body_milestone}"
            )

    required = entry.get("required_predecessor")
    if not isinstance(required, str) or not SHA_PATTERN.fullmatch(required):
        return fail(
            f"{milestone} predecessor SHA is not recorded; "
            "the predecessor milestone must be merged and recorded before this PR opens"
        )

    declarations = list(ANCESTOR_DECLARATION_PATTERN.finditer(body))
    if len(declarations) != 1:
        return fail(
            "PR body must contain exactly one 'Milestone-Ancestors: <full SHA>' declaration"
        )

    declared = declarations[0].group(1).split()
    if not declared or any(not SHA_PATTERN.fullmatch(value) for value in declared):
        return fail("Milestone-Ancestors must contain one or more full 40-character SHAs")
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

    print(
        f"milestone ancestry: {milestone} descends from required predecessor {required}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
