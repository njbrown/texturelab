#!/usr/bin/env bash
#
# Builds web/packages/runtime and publishes it to GitHub Packages
# (npm.pkg.github.com) as @njbrown/texturelab-runtime.
#
#   ./gh-publish-runtime.sh            # publish the version in package.json
#   ./gh-publish-runtime.sh --dry-run  # build + pack, show what would be published
#
# Bump "version" in web/packages/runtime/package.json first.
# Needs a gh login with write:packages:  gh auth refresh -s write:packages
set -euo pipefail

if [ -f .env ]; then source .env; fi
# Use the keyring login; the exported GH_TOKEN only has read:packages.
unset GH_TOKEN GITHUB_TOKEN

REGISTRY="https://npm.pkg.github.com"
PKG_DIR="web/packages/runtime"
DRY_RUN=false
[ "${1:-}" = "--dry-run" ] && DRY_RUN=true

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    echo "Refusing to publish: uncommitted changes to tracked files." >&2
    git status --short --untracked-files=no >&2
    exit 1
fi

NAME=$(node -p "require('./$PKG_DIR/package.json').name")
VERSION=$(node -p "require('./$PKG_DIR/package.json').version")
echo "Publishing $NAME@$VERSION from $(git rev-parse --short HEAD)"

TOKEN=$(gh auth token)

# The registry rejects republishing a version anyway, but failing here is
# faster and clearer.
if npm view "$NAME@$VERSION" version --registry="$REGISTRY" \
        --//npm.pkg.github.com/:_authToken="$TOKEN" >/dev/null 2>&1; then
    echo "$NAME@$VERSION is already published — bump the version." >&2
    exit 1
fi

yarn --cwd web install --frozen-lockfile
yarn --cwd web workspace "$NAME" typecheck
yarn --cwd web workspace "$NAME" build

cd "$PKG_DIR"
if $DRY_RUN; then
    npm pack --dry-run
else
    npm publish --registry="$REGISTRY" --//npm.pkg.github.com/:_authToken="$TOKEN"
    echo "Published $NAME@$VERSION"
fi
