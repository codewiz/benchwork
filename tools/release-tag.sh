#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Sign a release tag at the head of main on the codewiz remote, with NOTES
# as the tag message, and push it. Pushing the tag starts the Release
# workflow, which builds the binaries and publishes the tag message as the
# release notes.
#
#   tools/release-tag.sh TAG NOTES [--confirm]
#
# NOTES is a subject paragraph, a blank line, then the rest of the notes,
# one line per paragraph: GitHub keeps the line breaks. It is used
# verbatim, so Markdown headings survive. The tag goes on main as the remote
# has it, which is what CI built, and only if the Build workflow passed on
# that commit. Without --confirm it only prints what it would tag.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -lt 2 ] || [ $# -gt 3 ] || { [ $# -eq 3 ] && [ "$3" != --confirm ]; }; then
    echo "Usage: $0 <tag> <notes> [--confirm]" >&2
    exit 1
fi

tag=$1 notes=$2 confirm=${3-}
remote=codewiz
branch=main

cd "$(dirname "$0")/.."

die() {
    echo "$0: $*" >&2
    exit 1
}

[[ $tag =~ ^v[0-9]+\.[0-9]+(\.[0-9]+)?$ ]] || die "$tag: not a vX.Y release tag"
[ -s "$notes" ] || die "$notes: missing or empty"
# The release body is what follows the first paragraph.
awk 'NF && s == 0 { s = 1 } !NF && s == 1 { s = 2 } NF && s == 2 { found = 1 }
     END { exit !found }' "$notes" ||
    die "$notes: no release notes after the subject and a blank line"

git fetch -q "$remote" "$branch"
sha=$(git rev-parse "$remote/$branch")
if git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then
    die "$tag already exists here; delete it to tag $sha"
fi
[ -z "$(git ls-remote --tags "$remote" "refs/tags/$tag")" ] ||
    die "$tag already exists on $remote"

# owner/name for gh, from git@github.com:owner/name.git or https://...
repo=$(git remote get-url "$remote" | sed -E 's#^.*github\.com[:/]##; s#\.git$##')
build=$(gh run list -R "$repo" -w Build -c "$sha" -L 1 --json conclusion --jq '.[0].conclusion // ""')
[ "$build" = success ] || die "the Build workflow on $sha is '${build:-not run}', not success"

echo "$tag at $remote/$branch $(git log -1 --format='%h %s' "$sha")"
echo "notes: $(head -1 "$notes" | cut -c1-72)..."

if [ "$confirm" != --confirm ]; then
    echo "Dry run: add --confirm (CONFIRM=yes with make) to sign $tag and push it."
    exit 0
fi

git tag -s -F "$notes" --cleanup=verbatim "$tag" "$sha"
git push -q "$remote" "refs/tags/$tag"
echo "pushed $tag: the Release workflow starts now"
