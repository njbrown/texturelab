#!/usr/bin/env bash
# Triggers the GitHub Actions builds, which never run on their own.
#
#   scripts/ci.sh build [branch] [--crash-test] [--watch]
#       Nightly build of a pushed branch (default: the current one): all three
#       platforms, uploaded to the nightlies bucket and posted to Discord.
#       --crash-test also sends a real crash to Sentry to verify symbolication.
#
#   scripts/ci.sh release <tag> [--watch]
#       Release build of a pushed tag (v0.4.0 for stable, v0.4.0-beta or
#       v0.4.0-beta.2 for beta) into a draft GitHub Release, posted to Discord.
#
#   --watch follows the run in the terminal until it finishes.
#   --dry-run does every check and prints the dispatch instead of running it.
#
# CI builds what is on GitHub, not your working tree, so this refuses to start
# when the branch has commits that aren't pushed, or the tag isn't on origin.
# Needs the GitHub CLI (gh), logged in.

set -euo pipefail

usage() {
    sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

ROOT=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$ROOT"

command -v gh >/dev/null || { echo "error: needs the GitHub CLI (gh)" >&2; exit 1; }

COMMAND=${1:-}
[ -n "$COMMAND" ] || usage 2
shift

WATCH=false
DRY_RUN=false
CRASH_TEST=false
TARGET=""

while [ $# -gt 0 ]; do
    case "$1" in
        --watch) WATCH=true ;;
        --dry-run) DRY_RUN=true ;;
        --crash-test) CRASH_TEST=true ;;
        -h|--help) usage ;;
        -*) echo "unknown option: $1" >&2; usage 2 ;;
        *)
            [ -z "$TARGET" ] || { echo "unexpected argument: $1" >&2; usage 2; }
            TARGET=$1
            ;;
    esac
    shift
done

# Prints the newest run of a workflow started after $1 (ISO time), waiting a
# little for GitHub to register it; `gh workflow run` doesn't return the id.
find_run() {
    local workflow=$1 since=$2 id=""
    for _ in $(seq 1 15); do
        id=$(gh run list --workflow "$workflow" --event workflow_dispatch -L 5 \
                 --json databaseId,createdAt \
                 --jq "map(select(.createdAt >= \"$since\")) | .[0].databaseId // empty")
        [ -n "$id" ] && { echo "$id"; return 0; }
        sleep 2
    done
    return 1
}

dispatch() {
    local workflow=$1
    shift
    if $DRY_RUN; then
        echo "dry run: gh workflow run $workflow $*"
        return 0
    fi

    local since
    since=$(date -u +%Y-%m-%dT%H:%M:%SZ)

    gh workflow run "$workflow" "$@"

    local id
    if id=$(find_run "$workflow" "$since"); then
        echo "started: $(gh run view "$id" --json url -q .url)"
        if $WATCH; then
            gh run watch "$id" --exit-status
        fi
    else
        echo "started, but the run didn't show up yet; see: gh run list --workflow $workflow"
    fi
}

case "$COMMAND" in
    build)
        BRANCH=${TARGET:-$(git rev-parse --abbrev-ref HEAD)}
        [ "$BRANCH" != "HEAD" ] || { echo "error: detached HEAD; name a branch" >&2; exit 1; }

        git fetch -q origin "$BRANCH" 2>/dev/null || {
            echo "error: branch '$BRANCH' isn't on origin; push it first" >&2
            exit 1
        }
        if [ "$(git rev-parse --abbrev-ref HEAD)" = "$BRANCH" ]; then
            AHEAD=$(git rev-list --count "origin/$BRANCH..HEAD")
            if [ "$AHEAD" -gt 0 ]; then
                echo "error: $AHEAD commit(s) on $BRANCH aren't pushed; CI would build without them" >&2
                exit 1
            fi
        fi

        echo "building $BRANCH: $(scripts/build-id.sh --field identifier "origin/$BRANCH") (build $(scripts/build-id.sh --field build "origin/$BRANCH"))"
        dispatch build.yml --ref "$BRANCH" -f sentry_crash_test="$CRASH_TEST"
        ;;

    release)
        TAG=$TARGET
        [ -n "$TAG" ] || { echo "error: name the tag to release, e.g. v0.4.0-beta" >&2; exit 1; }
        $CRASH_TEST && { echo "error: --crash-test is for build, not release" >&2; exit 1; }

        # The commit the tag names on origin, peeled through an annotated tag.
        REFS=$(git ls-remote --tags origin)
        REMOTE=$(awk -v r="refs/tags/$TAG^{}" '$2 == r { print $1 }' <<<"$REFS")
        [ -n "$REMOTE" ] || REMOTE=$(awk -v r="refs/tags/$TAG" '$2 == r { print $1 }' <<<"$REFS")
        if [ -z "$REMOTE" ]; then
            echo "error: tag '$TAG' isn't on origin. Create and push it first:" >&2
            echo "  git tag $TAG && git push origin $TAG" >&2
            exit 1
        fi

        LOCAL=$(git rev-parse -q --verify "refs/tags/$TAG^{commit}" || true)
        if [ -z "$LOCAL" ]; then
            git fetch -q origin "refs/tags/$TAG:refs/tags/$TAG"
        elif [ "$LOCAL" != "$REMOTE" ]; then
            echo "error: local tag '$TAG' ($LOCAL) differs from origin's ($REMOTE)" >&2
            exit 1
        fi

        echo "releasing $TAG: $(scripts/build-id.sh --field identifier "$TAG") (build $(scripts/build-id.sh --field build "$TAG"))"

        # Dispatched from the default branch so the current workflow runs; it
        # checks out the tag itself.
        DEFAULT=$(gh repo view --json defaultBranchRef -q .defaultBranchRef.name)
        dispatch release.yml --ref "$DEFAULT" -f tag="$TAG"
        ;;

    -h|--help|help) usage ;;
    *) echo "unknown command: $COMMAND" >&2; usage 2 ;;
esac
