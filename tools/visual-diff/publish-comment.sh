#!/usr/bin/env bash
# Publishes a visual diff report as the pull request's visual diff comment: pushes the thumbnails to the
# visual-diff-assets branch (under pr-<number>/, replacing the previous run's) and creates or edits the one
# comment that starts with the <!-- vt-visual-diff --> marker.
#
#   publish-comment.sh <report folder>
#
# Runs in the Visual diff comment workflow, from the default branch. The report folder comes from the
# pull request's run, so everything in it is treated as untrusted data.
#
# Environment: GH_TOKEN, REPO (owner/name), RUN_ID and RUN_URL of the Object pools run, HEAD_SHA of that run.
set -euo pipefail

report_dir="$1"
script_dir="$(cd "$(dirname "$0")" && pwd)"
marker='<!-- vt-visual-diff -->'

pr="$(jq -r '.pr' "$report_dir/pull-request.json")"
if ! [[ "$pr" =~ ^[0-9]+$ ]]; then
  echo "pull-request.json does not hold a pull request number"
  exit 1
fi
# Only comment on the pull request this run was for
pr_head="$(gh api "repos/$REPO/pulls/$pr" --jq '.head.sha')"
if [ "$pr_head" != "$HEAD_SHA" ]; then
  echo "Pull request #$pr is now at $pr_head, not $HEAD_SHA; a newer run will comment."
  exit 0
fi

artifact_url="$(gh api "repos/$REPO/actions/runs/$RUN_ID/artifacts" \
  --jq ".artifacts[] | select(.name == \"visual-diff-report\") | \"https://github.com/$REPO/actions/runs/$RUN_ID/artifacts/\\(.id)\"" | head -n 1)"

assets_url=""
if [ -f "$report_dir/report.json" ] && [ -d "$report_dir/thumbnails" ]; then
  if find "$report_dir/thumbnails" ! -type f ! -type d | grep -q . ||
    find "$report_dir/thumbnails" -type f | grep -Ev '/thumbnails/[0-9a-f]{8}/[A-Za-z0-9_.-]+\.png$' | grep -q .; then
    echo "The report holds thumbnails with unexpected names"
    exit 1
  fi

  assets="$(mktemp -d)"
  remote="https://x-access-token:${GH_TOKEN}@github.com/${REPO}.git"
  git -C "$assets" init -q
  git -C "$assets" config user.name 'github-actions[bot]'
  git -C "$assets" config user.email '41898282+github-actions[bot]@users.noreply.github.com'

  for attempt in 1 2 3; do
    if git -C "$assets" fetch -q --depth 1 "$remote" visual-diff-assets 2>/dev/null; then
      git -C "$assets" checkout -q -B visual-diff-assets FETCH_HEAD
    else
      git -C "$assets" checkout -q --orphan visual-diff-assets
      git -C "$assets" rm -rfq --ignore-unmatch .
    fi
    rm -rf "$assets/pr-$pr"
    mkdir -p "$assets/pr-$pr"
    cp -R "$report_dir/thumbnails" "$assets/pr-$pr/"
    git -C "$assets" add -A
    git -C "$assets" commit -q -m "Visual diff thumbnails for #$pr, run $RUN_ID" || true
    # Another pull request's run may have pushed in between; then start again from its commit
    if git -C "$assets" push -q "$remote" HEAD:visual-diff-assets; then
      assets_url="https://raw.githubusercontent.com/$REPO/$(git -C "$assets" rev-parse HEAD)/pr-$pr"
      break
    fi
    echo "Push attempt $attempt failed"
  done
fi

if [ -f "$report_dir/report.json" ]; then
  body="$(node "$script_dir/comment.mjs" --report "$report_dir/report.json" --assets-url "$assets_url" --artifact-url "$artifact_url")"
else
  body="$(printf '%s\n\n## Visual diff: could not run\n\nThe comparison stopped before writing a report; see the [run log](%s).\n' "$marker" "$RUN_URL")"
fi

comment_id="$(gh api --paginate "repos/$REPO/issues/$pr/comments" \
  --jq ".[] | select(.user.login == \"github-actions[bot]\" and (.body | startswith(\"$marker\"))) | .id" | head -n 1)"
if [ -n "$comment_id" ]; then
  gh api -X PATCH "repos/$REPO/issues/comments/$comment_id" -f body="$body" > /dev/null
  echo "Updated comment $comment_id on #$pr"
else
  gh api -X POST "repos/$REPO/issues/$pr/comments" -f body="$body" > /dev/null
  echo "Commented on #$pr"
fi
