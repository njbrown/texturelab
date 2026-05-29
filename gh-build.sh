#!/usr/bin/env bash
set -euo pipefail

REPO="njbrown/texturelab"
WORKFLOW="build.yml"
BRANCH="${1:-$(git rev-parse --abbrev-ref HEAD)}"

echo "Triggering build for branch: $BRANCH"
gh workflow run "$WORKFLOW" --repo "$REPO" --ref "$BRANCH"

echo "Waiting for run to start..."
sleep 3

RUN_ID=$(gh run list --repo "$REPO" --workflow "$WORKFLOW" --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')

echo "Run ID: $RUN_ID"
echo "https://github.com/$REPO/actions/runs/$RUN_ID"

if [[ "${2:-}" == "--watch" || "${1:-}" == "--watch" ]]; then
    gh run watch "$RUN_ID" --repo "$REPO"
fi
