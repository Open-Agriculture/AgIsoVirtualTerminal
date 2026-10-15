#!/usr/bin/env bash
# Fails when a pull request moves the tests/pools submodule pin and also changes anything else, so that a
# pin bump always comes in a pull request of its own and its visual diff shows only what the new pools do.
#
#   check-pin-isolation.sh <base commit> <head commit>
set -euo pipefail

base="$1"
head="$2"

if [ -z "$(git ls-tree "$base" tests/pools)" ]; then
  echo "tests/pools does not exist on the base commit: this pull request adds the submodule."
  exit 0
fi

changed="$(git diff --name-only "$base...$head")"
if ! grep -qx 'tests/pools' <<<"$changed"; then
  echo "The tests/pools pin is unchanged."
  exit 0
fi

others="$(grep -vx -e 'tests/pools' -e '.gitmodules' <<<"$changed" || true)"
if [ -n "$others" ]; then
  echo "::error title=Submodule pin bump mixed with other changes::Move the tests/pools pin in a pull request of its own (see doc/object-pool-ci.md). Other changed files:"
  echo "$others"
  exit 1
fi
echo "This pull request only moves the tests/pools pin."
