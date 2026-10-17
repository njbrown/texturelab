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
# Runs created before this moment belong to an earlier attempt.
STARTED_AT=$(date -u +%Y-%m-%dT%H:%M:%SZ)

if git ls-remote --exit-code --tags origin "refs/tags/$TAG" >/dev/null 2>&1; then
    echo "Tag already on origin — dispatching a re-run against it."
    gh workflow run "$WORKFLOW" --repo "$REPO" --ref "$TAG" -f tag="$TAG"
else
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    SHA=$(git rev-parse --short HEAD)

    # The tag is cut from HEAD, so HEAD must be exactly what's on master.
    if [ "$BRANCH" != "master" ]; then
        echo "Refusing to tag: on '$BRANCH', not master." >&2
        exit 1
    fi
    if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
        echo "Refusing to tag: uncommitted changes to tracked files." >&2
        git status --short --untracked-files=no >&2
        exit 1
    fi
    git fetch --quiet origin master
    if [ "$(git rev-parse HEAD)" != "$(git rev-parse origin/master)" ]; then
        echo "Refusing to tag: HEAD ($SHA) is not origin/master. Push or pull first." >&2
        exit 1
    fi
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

# Find this attempt's run for this tag; "--limit 1" alone can pick up a
# previous run if the new one hasn't registered yet.
echo "Waiting for run to start..."
RUN_ID=""
for _ in $(seq 1 20); do
    sleep 3
    RUN_ID=$(gh run list --repo "$REPO" --workflow "$WORKFLOW" --branch "$TAG" \
        --limit 5 --json databaseId,createdAt \
        -q "[.[] | select(.createdAt >= \"$STARTED_AT\")][0].databaseId // empty")
    [ -n "$RUN_ID" ] && break
done
if [ -z "$RUN_ID" ]; then
    echo "No run found for $TAG yet — check https://github.com/$REPO/actions" >&2
    exit 1
fi
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
