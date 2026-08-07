#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
assets_dir=${RELEASE_ASSETS_DIR:-"$repo_root/release-assets"}

: "${GH_TOKEN:?GH_TOKEN must be set}"
: "${GITHUB_REPOSITORY:?GITHUB_REPOSITORY must be set}"
: "${GITHUB_SHA:?GITHUB_SHA must be set}"

version=$(sed -n 's/^project(global-menu-kde VERSION \([^ ]*\).*/\1/p' "$repo_root/CMakeLists.txt")
if [[ -z "$version" ]]; then
    echo "Could not determine project version from CMakeLists.txt" >&2
    exit 1
fi

tag="v${version}"
notes_template="$repo_root/packaging/release-notes.md.in"
notes_file=$(mktemp)
trap 'rm -f "$notes_file"' EXIT
sed "s/@VERSION@/${version}/g" "$notes_template" > "$notes_file"

shopt -s nullglob
assets=("$assets_dir"/*)
shopt -u nullglob
if (( ${#assets[@]} == 0 )); then
    echo "No release assets found in $assets_dir" >&2
    exit 1
fi

release_exists=false
asset_count=0
if gh release view "$tag" --repo "$GITHUB_REPOSITORY" >/dev/null 2>&1; then
    release_exists=true
    asset_count=$(gh release view "$tag" --repo "$GITHUB_REPOSITORY" --json assets --jq '.assets | length')
fi

resolve_tag_commit() {
    local object_type object_sha
    object_type=$(gh api "repos/${GITHUB_REPOSITORY}/git/ref/tags/${tag}" --jq '.object.type')
    object_sha=$(gh api "repos/${GITHUB_REPOSITORY}/git/ref/tags/${tag}" --jq '.object.sha')
    if [[ "$object_type" == "tag" ]]; then
        object_sha=$(gh api "repos/${GITHUB_REPOSITORY}/git/tags/${object_sha}" --jq '.object.sha')
    fi
    printf '%s\n' "$object_sha"
}

if [[ "$release_exists" == true ]]; then
    tag_commit=$(resolve_tag_commit)
    if [[ "$tag_commit" != "$GITHUB_SHA" ]]; then
        if (( asset_count == 0 )); then
            echo "Replacing empty release $tag because its tag points to $tag_commit instead of tested commit $GITHUB_SHA"
            gh release delete "$tag" --repo "$GITHUB_REPOSITORY" --cleanup-tag --yes
            release_exists=false
        else
            echo "Refusing to retarget non-empty release $tag from $tag_commit to $GITHUB_SHA. Bump the project version instead." >&2
            exit 1
        fi
    fi
fi

if [[ "$release_exists" == true ]]; then
    echo "Updating existing release $tag on tested commit $GITHUB_SHA"
    gh release edit "$tag" \
        --repo "$GITHUB_REPOSITORY" \
        --title "Global Menu KDE ${version}" \
        --notes-file "$notes_file"
    gh release upload "$tag" --repo "$GITHUB_REPOSITORY" --clobber "${assets[@]}"
else
    echo "Creating release $tag on tested commit $GITHUB_SHA"
    gh release create "$tag" \
        --repo "$GITHUB_REPOSITORY" \
        --target "$GITHUB_SHA" \
        --title "Global Menu KDE ${version}" \
        --notes-file "$notes_file" \
        "${assets[@]}"
fi

published_assets=$(gh release view "$tag" --repo "$GITHUB_REPOSITORY" --json assets --jq '.assets | length')
if (( published_assets == 0 )); then
    echo "Release $tag exists but has no published assets" >&2
    exit 1
fi

printf 'Published %s with %s assets.\n' "$tag" "$published_assets"
