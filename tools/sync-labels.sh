#!/usr/bin/env bash
# ---------------------------------------------------------------------------
#  RAW UI - sync repository labels.
#
#  Labels are needed by the bots (.github/labeler.yml, release-drafter and the
#  stale workflow) but GitHub has no declarative label store, so this script is
#  the source of truth.  Running it twice is a no-op.
#
#  Usage:  tools/sync-labels.sh [owner/repo]     # defaults to the current repo
# ---------------------------------------------------------------------------
set -euo pipefail

REPO="${1:-$(gh repo view --json nameWithOwner -q .nameWithOwner)}"

# name|colour|description
LABELS='
app|6A737D|C++ console application (src/**)
site|0E8A16|Static Pages site (docs/**)
ci|1D76DB|GitHub Actions workflows (.github/**)
build|BFD4F2|Build system (Makefile, tools/**)
tests|F9D0C4|Unit tests (tests/**)
documentation|0075CA|Docs, README and changelog
feature|A2EEEF|New capability or behaviour
fix|D73A4A|Something that did not work
performance|FBCA04|Faster, leanmer or lower latency
chore|FEF2C0|Maintenance, tooling, housekeeping
bug|D73A4A|Confirmed defect
bugfix|D73A4A|Pull request that fixes a defect
enhancement|A2EEEF|Improvement to existing behaviour
dependencies|0366D6|Dependency updates (Dependabot)
major|B60205|Release that bumps the major version
minor|0E8A16|Release that bumps the minor version
patch|C2E0C6|Release that bumps the patch version
skip-changelog|EDEDED|Leave this pull request out of the changelog
stale|FFFFFF|Automatically marked as inactive
pinned|8B4513|Exempt from the stale bot
security|B60205|Security relevant
work-in-progress|FBCA04|Not ready for review
'

echo "==> syncing labels into ${REPO}"

while IFS='|' read -r name color desc; do
    [ -n "$name" ] || continue
    if gh api "repos/${REPO}/labels/${name}" >/dev/null 2>&1; then
        gh api -X PATCH "repos/${REPO}/labels/${name}" \
            -f color="$color" -f description="$desc" >/dev/null
        echo "    updated ${name}"
    else
        gh api -X POST "repos/${REPO}/labels" \
            -f name="$name" -f color="$color" -f description="$desc" >/dev/null
        echo "    created ${name}"
    fi
done <<< "$LABELS"

echo "==> done"
