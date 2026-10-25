#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Upload a release to Aminet: benchwork.lha and benchwork.readme, as the
# tag's Release workflow built them, by anonymous FTP to the new/ directory,
# from where Aminet's moderators publish them as util/moni/benchwork.
#
#   tools/release-aminet.sh TAG [--confirm]
#
# The files come from the Release run's artifact (GitHub keeps it for 90
# days), into tmp/TAG/. The readme must name TAG's version, and the .lha must
# be the one attached to the GitHub release. Without --confirm it only
# prints what it would upload.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -lt 1 ] || [ $# -gt 2 ] || { [ $# -eq 2 ] && [ "$2" != --confirm ]; }; then
    echo "Usage: $0 <tag> [--confirm]" >&2
    exit 1
fi

tag=$1 confirm=${2-}
remote=codewiz
aminet=ftp://main.aminet.net/new/

cd "$(dirname "$0")/.."

die() {
    echo "$0: $*" >&2
    exit 1
}

[[ $tag =~ ^v[0-9]+\.[0-9]+(\.[0-9]+)?$ ]] || die "$tag: not a vX.Y release tag"
repo=$(git remote get-url "$remote" | sed -E 's#^.*github\.com[:/]##; s#\.git$##')

read -r run conclusion < <(gh run list -R "$repo" -w Release -b "$tag" -L 1 \
    --json databaseId,conclusion --jq '.[0] | "\(.databaseId) \(.conclusion)"') ||
    die "no Release run for $tag"
[ "$conclusion" = success ] || die "the Release run $run for $tag is '${conclusion:-not finished}'"

dir=tmp/$tag
rm -rf "$dir"
gh run download "$run" -R "$repo" -n benchwork -D "$dir" ||
    die "cannot download the artifact of run $run; it may have expired"

grep -q "^Version: *${tag#v}\$" "$dir/benchwork.readme" ||
    die "$dir/benchwork.readme does not say Version: ${tag#v}"
gh release download "$tag" -R "$repo" -p benchwork.lha -O "$dir/release.lha"
cmp -s "$dir/benchwork.lha" "$dir/release.lha" ||
    die "$dir/benchwork.lha differs from the one on the $tag release"
rm "$dir/release.lha"

ls -l "$dir/benchwork.lha" "$dir/benchwork.readme"

if [ "$confirm" != --confirm ]; then
    echo "Dry run: add --confirm (CONFIRM=yes with make) to upload them to $aminet."
    exit 0
fi

# The .lha first: Aminet pairs it with the readme of the same name.
curl -sS -T "$dir/benchwork.lha" "$aminet"
curl -sS -T "$dir/benchwork.readme" "$aminet"
echo "uploaded $tag to $aminet: it appears as util/moni/benchwork after moderation"
