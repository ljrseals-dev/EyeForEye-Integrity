# Eyesore milestone ledger

This file records repository history and the required order for future milestone work.
A milestone is **complete** only when its implementation PR is merged, its merge commit
SHA is recorded below, and the required CI checks have passed on the merged result.
A planned milestone or open PR is not a completed milestone.

## Verified merged history

The merge commit SHAs below were retrieved from GitHub's pull-request records.
They are historical facts, not claims that later milestones have been implemented.

| PR | Scope | Merge commit SHA | Status |
|---:|---|---|---|
| #1 | Sanitized C++17 public baseline | `2803ef5b8bf1e3a44367592dfcbe0e4a1ab1dd9e` | Merged |
| #2 | M2 — C++17 DSP and sample foundation | `500fa220763cbd572737120c9ca9e90934b14e26` | Merged |
| #3 | M3 — phase-vocoder pitch shifting | `68709ea6a1131815aef3adca35223dc09e7cb807` | Merged |
| #4 | Phase-vocoder + local vocal F0/compressor integration | `deb8efa3fdb4194c854ad5188cec5ebc755eba93` | Merged |
| #5 | Engineering architecture, flowchart, diagnostics | `590cbe0d702419beb11c156b5c5219beb5c53880` | Merged |
| #6 | Float32 overflow rejection and F0 regression coverage | `a9fb39643ee939b488e121a8d7e193030b6db423` | Merged; current baseline |

PR #3 is an ancestor of the current baseline. PR #4 is integration work that was
merged from the M2 base; PRs #5 and #6 subsequently hardened and documented the
integrated repository. The M3b starting point is therefore the current verified
baseline at PR #6, not a reconstructed or guessed branch head.

## Required future sequence

| Stage | Required predecessor | Branch / integration rule | Status |
|---|---|---|---|
| M3b — pitch-cache prewarm | Current baseline / PR #6 | Feature branch from updated `main`; PR required | Not started |
| M3c — Audio Unit | M3b merge commit | Branch must descend from M3b merge commit | Not started |
| M4 — filename parsers | M3c merge commit | Branch must descend from M3c merge commit | Not started |
| M5 — AU milestone | M4 merge commit | Branch must descend from M4 merge commit | Not started |
| M5b — macOS AU validation | M5 merge commit | Separate post-M5 commit on `main`, only after the macOS `auval` gate passes | Not started |
| M6 | M5b commit | Branch must descend from the recorded M5b commit | Not started |

The authoritative machine-readable predecessor map and merge SHAs live in
`docs/milestones.json`. Update that file in the same PR that records a newly
merged milestone. Never fill a SHA with a branch head, PR head, planned SHA,
or a commit that has not actually entered `main`.

## PR ancestry declaration

Every future milestone PR must include this line in its description:

```text
Milestone-Ancestors: <full required predecessor merge SHA>
```

For milestones with more than one required predecessor, list every full SHA,
space-separated. CI checks both that the declaration contains the required SHA
and that the SHA is an ancestor of the checked-out PR merge ref using
`git merge-base --is-ancestor`.

## Enforcement boundary

The ancestry workflow prevents a milestone PR from passing CI when it omits or
does not descend from its recorded predecessor. It does not itself enable GitHub
branch protection. Repository administrators must separately configure a
`main` ruleset or branch protection rule requiring PRs and the named status
checks, blocking force pushes and branch deletion. Until that setting is
enabled and independently verified, it must be described as unenforced.
