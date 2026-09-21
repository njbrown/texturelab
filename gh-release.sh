#!/usr/bin/env bash
#
# Kicks off an official release build (.github/workflows/release.yml), which
# ends in a DRAFT GitHub Release you review and publish yourself.
#
#   ./gh-release.sh                 # tag taken from the version in CMakeLists.txt
#   ./gh-release.sh v0.4.0-beta     # explicit tag
#   ./gh-release.sh --watch         # follow the run until it finishes
#
# If the tag doesn't exist on the remote yet it offers to create and push it,
# which is what starts the workflow. If it already exists, the run is dispatched
# against that tag instead (for re-running a failed release).
set -euo pipefail

if [ -f .env ]; then source .env; fi

# Force gh to use the keyring login (which has repo scope). gh prefers the
# GH_TOKEN / GITHUB_TOKEN env vars over keyring auth, and the token exported
# from the shell only has read:packages scope, which 403s on workflow dispatch.
unset GH_TOKEN GITHUB_TOKEN

REPO="njbrown/texturelab"
WORKFLOW="release.yml"

WATCH=false
TAG=""
for arg in "$@"; do
    case "$arg" in
        --watch) WATCH=true ;;
        *) TAG="$arg" ;;
    esac
done

# Default tag = the version the app will report, which is what release.yml's
# preflight job insists the tag matches.
if [ -z "$TAG" ]; then
    NUM=$(grep -oP 'project\(texturelab VERSION \K[0-9.]+' src/texturelab/CMakeLists.txt)
    SUFFIX=$(grep -oP 'set\(TEXTURELAB_VERSION_SUFFIX "\K[^"]*' src/texturelab/CMakeLists.txt)
    TAG="v${NUM}${SUFFIX}"
fi

echo "Release tag: $TAG"

if git ls-remote --exit-code --tags origin "refs/tags/$TAG" >/dev/null 2>&1; then
    echo "Tag already on origin — dispatching a re-run against it."
    gh workflow run "$WORKFLOW" --repo "$REPO" --ref "$TAG" -f tag="$TAG"
else
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    SHA=$(git rev-parse --short HEAD)
    echo
    echo "Tag $TAG does not exist on origin. It would be created at $BRANCH ($SHA)"
    echo "and pushed, which starts the release build."
    read -r -p "Create and push $TAG? [y/N] " answer
    [[ "$answer" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 1; }

    # Annotated tag: release.yml checks out the tag, and --verify-tag on the
    # draft release requires it to exist on the remote.
    git tag -a "$TAG" -m "TextureLab $TAG"
    git push origin "$TAG"
fi

echo "Waiting for run to start..."
sleep 5

RUN_ID=$(gh run list --repo "$REPO" --workflow "$WORKFLOW" --limit 1 --json databaseId -q '.[0].databaseId')
RUN_URL="https://github.com/$REPO/actions/runs/$RUN_ID"
echo "Run ID: $RUN_ID"
echo "$RUN_URL"

if [ "$WATCH" = true ]; then
    # Non-zero exit here means the release build failed; the draft is not created.
    gh run watch "$RUN_ID" --repo "$REPO" --exit-status || {
        echo "Release build failed — see $RUN_URL"
        exit 1
    }
    echo
    echo "Draft release:"
    gh release view "$TAG" --repo "$REPO" --json url,isDraft,assets \
        --jq '"\(.url)\ndraft: \(.isDraft)\nassets:\n" + (.assets | map("  " + .name) | join("\n"))'
    echo
    echo "Review it, then publish with:"
    echo "  gh release edit $TAG --repo $REPO --draft=false"
fi
