"""Regression tests for the milestone ancestry PR gate."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "check_milestone_ancestry.py"
LEDGER_PATH = ROOT / "docs" / "milestones.json"
LEDGER = json.loads(LEDGER_PATH.read_text(encoding="utf-8"))


def git(cwd: Path, *args: str) -> str:
    return subprocess.run(
        [
            "git",
            "-c",
            "user.email=ci@example.invalid",
            "-c",
            "user.name=ci",
            *args,
        ],
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


def run_check(
    title: str = "M3b - test milestone",
    body: str = "",
    *,
    ancestor_ok: bool = True,
    record: tuple[str, ...] = ("M3b", "M5b"),
    ledger_data: dict | None = None,
    ledger_text: str | None = None,
    event_data: dict | None = None,
) -> tuple[int, str, str]:
    """Run the real script in a throwaway Git repository."""
    with tempfile.TemporaryDirectory() as tmp:
        repo = Path(tmp)
        git(repo, "init", "-q")
        (repo / "docs").mkdir()
        (repo / "base.txt").write_text("base", encoding="utf-8")
        git(repo, "add", "-A")
        git(repo, "commit", "-qm", "base")
        base = git(repo, "rev-parse", "HEAD")

        if ancestor_ok:
            required = base
        else:
            # A real commit on a separate root history makes merge-base return
            # 1, proving that the script reaches the ancestry check.
            git(repo, "checkout", "--orphan", "unrelated")
            git(repo, "rm", "-rf", ".")
            (repo / "unrelated.txt").write_text("unrelated", encoding="utf-8")
            git(repo, "add", "-A")
            git(repo, "commit", "-qm", "unrelated root")
            required = git(repo, "rev-parse", "HEAD")
            git(repo, "checkout", "--detach", base)

        ledger = json.loads(json.dumps(LEDGER if ledger_data is None else ledger_data))
        if ledger_text is None:
            future = ledger.get("future_milestones", {})
            for name in record:
                if name in future and isinstance(future[name], dict):
                    future[name]["required_predecessor"] = required
            ledger_text = json.dumps(ledger)
        (repo / "docs" / "milestones.json").write_text(
            ledger_text, encoding="utf-8"
        )

        event_path = repo / "event.json"
        if event_data is None:
            event_data = {
                "pull_request": {
                    "title": title,
                    "body": body.replace("{sha}", base).replace(
                        "{required}", required
                    ),
                }
            }
        event_path.write_text(json.dumps(event_data), encoding="utf-8")

        env = os.environ.copy()
        env["GITHUB_EVENT_PATH"] = str(event_path)
        done = subprocess.run(
            [sys.executable, str(SCRIPT)],
            cwd=repo,
            env=env,
            capture_output=True,
            text=True,
        )
        return done.returncode, done.stdout, done.stderr


@pytest.mark.parametrize(
    "title",
    [
        "M3b - pitch-cache prewarm",
        "m3b: lower case",
        "Milestone 3b",
        "Milestone-3b: alternate separator",
        "M5b - macOS auval",
    ],
)
def test_supported_title_forms_pass_with_valid_ancestor(title: str) -> None:
    milestone = "M5b" if "5b" in title.lower() else "M3b"
    body = f"Milestone: {milestone}\nMilestone-Ancestors: {{sha}}"
    code, out, err = run_check(title, body)
    assert code == 0, err
    assert "descends from" in out


def test_non_ancestor_reaches_and_fails_git_ancestry_check() -> None:
    code, out, err = run_check(
        "M3b - x",
        "Milestone: M3b\nMilestone-Ancestors: {required}",
        ancestor_ok=False,
    )
    assert code == 1
    assert "not an ancestor" in err
    assert "omits required predecessor" not in err


@pytest.mark.parametrize(
    ("title", "milestone"),
    [
        ("M5b - macOS auval", "M5b"),
        ("Milestone 3b", "M3b"),
        ("m3b", "M3b"),
    ],
)
def test_gated_titles_fail_without_ancestor_declaration(
    title: str, milestone: str
) -> None:
    code, out, err = run_check(title, f"Milestone: {milestone}")
    assert code == 1
    assert "Milestone-Ancestors" in err


def test_m5b_is_not_truncated_to_m5() -> None:
    code, out, err = run_check(
        "M5b - macOS auval",
        "Milestone: M5b\nMilestone-Ancestors: {sha}",
    )
    assert code == 0, err
    assert "M5b descends" in out


def test_m50_is_not_mistaken_for_m5() -> None:
    code, out, err = run_check(
        "M50 - unrelated work",
        "Milestone: M50\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "title identifies M50" in err
    assert "no matching future_milestones entry" in err
    assert "predecessor SHA is not recorded" not in err


def test_identifier_embedded_in_word_is_not_a_milestone_token() -> None:
    code, out, err = run_check("repairAM5 parser", "Milestone: none")
    assert code == 0
    assert "explicitly declares Milestone: none" in out


def test_multiple_distinct_milestones_in_title_are_rejected() -> None:
    code, out, err = run_check(
        "M3b integration with M5b",
        "Milestone: M3b\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "multiple milestone identifiers" in err


def test_repeated_same_milestone_in_title_is_not_ambiguous() -> None:
    code, out, err = run_check(
        "M3b: milestone M3b implementation",
        "Milestone: M3b\nMilestone-Ancestors: {sha}",
    )
    assert code == 0, err


def test_conflicting_title_and_body_milestone_are_rejected() -> None:
    code, out, err = run_check(
        "M5b - macOS auval",
        "Milestone: M4\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "PR title identifies M5b, but PR body declares M4" in err


def test_matching_title_and_body_milestone_pass() -> None:
    code, out, err = run_check(
        "M5b - macOS auval",
        "Milestone: m5B\nMilestone-Ancestors: {sha}",
    )
    assert code == 0, err


def test_duplicate_body_milestone_declarations_are_rejected() -> None:
    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b\nMilestone: M3b\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "exactly one 'Milestone:" in err


def test_malformed_body_milestone_declaration_is_rejected() -> None:
    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b extra\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "must be 'none' or a valid ID" in err


def test_missing_milestone_classification_fails_closed() -> None:
    code, out, err = run_check("Fix README typo", "")
    assert code == 1
    assert "exactly one 'Milestone:" in err


def test_milestone_title_cannot_be_downgraded_to_none() -> None:
    code, out, err = run_check("M3b - implementation", "Milestone: none")
    assert code == 1
    assert "title identifies M3b" in err


def test_body_milestone_cannot_be_hidden_in_generic_title() -> None:
    code, out, err = run_check(
        "Fix parsing",
        "Milestone: M3b\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "title does not identify a milestone" in err


def test_unrecorded_predecessor_fails_closed() -> None:
    code, out, err = run_check(
        "M4 - parsers",
        "Milestone: M4\nMilestone-Ancestors: {sha}",
        record=(),
    )
    assert code == 1
    assert "predecessor SHA is not recorded" in err


def test_missing_future_milestones_object_fails_closed() -> None:
    ledger = {"schema_version": 1}
    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b\nMilestone-Ancestors: {sha}",
        ledger_data=ledger,
    )
    assert code == 1
    assert "future_milestones must be an object" in err


def test_wrong_ledger_root_type_fails_closed() -> None:
    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b\nMilestone-Ancestors: {sha}",
        ledger_text="[]",
    )
    assert code == 1
    assert "must contain a JSON object" in err


def test_invalid_json_ledger_fails_closed() -> None:
    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b\nMilestone-Ancestors: {sha}",
        ledger_text="{not valid json",
    )
    assert code == 1
    assert "cannot read docs/milestones.json" in err


def test_multiple_or_malformed_ancestor_declarations_are_rejected() -> None:
    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b\nMilestone-Ancestors: {sha}\nMilestone-Ancestors: {sha}",
    )
    assert code == 1
    assert "exactly one" in err

    code, out, err = run_check(
        "M3b - test",
        "Milestone: M3b\nMilestone-Ancestors: not-a-full-sha",
    )
    assert code == 1
    assert "full 40-character SHAs" in err


@pytest.mark.parametrize(
    "title",
    ["Fix README typo", "Document the milestone ideas", "Bump dependency"],
)
def test_non_milestone_titles_are_skipped_only_with_explicit_none(
    title: str,
) -> None:
    code, out, err = run_check(title, "Milestone: none")
    assert code == 0
    assert "explicitly declares Milestone: none" in out


@pytest.mark.parametrize(
    ("title", "milestone"),
    [("Document M4b ideas", "M4b"), ("Bump M7", "M7")],
)
def test_unknown_milestone_ids_fail_closed_instead_of_skipping(
    title: str, milestone: str
) -> None:
    code, out, err = run_check(
        title, f"Milestone: {milestone}\nMilestone-Ancestors: {{sha}}"
    )
    assert code == 1
    assert "no matching future_milestones entry" in err


def test_push_event_is_explicitly_skipped() -> None:
    code, out, err = run_check(event_data={"ref": "refs/heads/topic"})
    assert code == 0
    assert "push/non-PR event; skipped" in out


def test_workflow_and_template_require_explicit_classification() -> None:
    workflow = (ROOT / ".github" / "workflows" / "ci.yml").read_text(
        encoding="utf-8"
    )
    assert "types: [opened, synchronize, reopened, edited]" in workflow
    assert "if: github.event_name == 'pull_request'" in workflow
    assert "github.event.pull_request.head.sha" in workflow
    assert "fetch-depth: 0" in workflow
    assert "run: python scripts/check_milestone_ancestry.py" in workflow
    template = (ROOT / ".github" / "pull_request_template.md").read_text(
        encoding="utf-8"
    )
    assert "Milestone: none" in template
