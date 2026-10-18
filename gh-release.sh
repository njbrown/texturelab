#!/usr/bin/env bash
#
# Kicks off an official release build (.github/workflows/release.yml), which
# ends in two DRAFTS you review and publish yourself: a GitHub Release with the
# files, and a prefilled release on texturelab.io linking to them.
#
#   ./gh-release.sh                 # tag taken from the version in CMakeLists.txt
#   ./gh-release.sh v0.4.0-beta     # explicit tag
#   ./gh-release.sh --watch         # follow the run until it finishes
#
# If the tag doesn't exist on the remote yet it offers to create and push it.
# Either way the build is then started with scripts/ci.sh release — pushing a
# tag doesn't start anything on its own. Re-running for an existing tag is how
# a failed release is retried.
set -euo pipefail

if [ -f .env ]; then source .env; fi

# Force gh to use the keyring login (which has repo scope). gh prefers the
# GH_TOKEN / GITHUB_TOKEN env vars over keyring auth, and the token exported
# from the shell only has read:packages scope, which 403s on workflow dispatch.
unset GH_TOKEN GITHUB_TOKEN

REPO="njbrown/texturelab"

WATCH=false
TAG=""
for arg in "$@"; do
    case "$arg" in
        --watch) WATCH=true ;;
        *) TAG="$arg" ;;
    esac
done

# Default tag = the version and channel the app will report, which is what
# release.yml's preflight job insists the tag matches: v0.4.0 for stable,
# v0.4.0-beta for beta (pass v0.4.0-beta.2 etc. explicitly for later betas).
if [ -z "$TAG" ]; then
    NUM=$(scripts/build-id.sh --field version)
    CHANNEL=$(scripts/build-id.sh --field channel)
    if [ "$CHANNEL" = "stable" ]; then TAG="v${NUM}"; else TAG="v${NUM}-${CHANNEL}"; fi
fi

echo "Release tag: $TAG"

if git ls-remote --exit-code --tags origin "refs/tags/$TAG" >/dev/null 2>&1; then
    echo "Tag already on origin — re-running the release build for it."
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
    echo "as $(scripts/build-id.sh --field identifier) (build $(scripts/build-id.sh --field build)),"
    echo "pushed, and the release build started."
    read -r -p "Create and push $TAG? [y/N] " answer
    [[ "$answer" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 1; }

    # Annotated tag: release.yml checks out the tag, and --verify-tag on the
    # draft release requires it to exist on the remote.
    git tag -a "$TAG" -m "TextureLab $TAG"
    git push origin "$TAG"
fi

# Checks the tag on origin matches, starts the run and prints its URL.
if [ "$WATCH" = true ]; then
    scripts/ci.sh release "$TAG" --watch || {
        echo "Release build failed — no draft was created."
        exit 1
    }
else
    scripts/ci.sh release "$TAG"
fi

if [ "$WATCH" = true ]; then
    echo
    echo "Draft release:"
    gh release view "$TAG" --repo "$REPO" --json url,isDraft,assets \
        --jq '"\(.url)\ndraft: \(.isDraft)\nassets:\n" + (.assets | map("  " + .name) | join("\n"))'
    echo
    echo "Review both drafts, then publish them in this order:"
    echo "  1. GitHub (the site's download links point at these files):"
    echo "       gh release edit $TAG --repo $REPO --draft=false"
    echo "  2. texturelab.io, which offers it to the $(scripts/build-id.sh --field channel "$TAG") channel:"
    echo "       https://texturelab.io/ops/releases"
fi
