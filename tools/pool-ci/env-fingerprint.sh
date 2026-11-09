#!/usr/bin/env bash
# Prints a hash of what, apart from the VT source, decides the pixels vt-render produces on Linux: the OS
# release, the compiler, and the libraries it links that take part in drawing. Renders are only compared,
# or taken from the cache, when both sides have the same fingerprint.
set -euo pipefail

{
  # shellcheck source=/dev/null
  . /etc/os-release && echo "os=${ID}-${VERSION_ID}"
  echo "arch=$(uname -m)"
  echo "compiler=$(c++ --version | head -n 1)"
  # Every installed package of these, whatever its t64 or soname suffix
  dpkg-query -W -f '${Package}=${Version}\n' 'libfreetype*' 'libfontconfig*' 'libc6' 'libstdc++6' 'libgcc-s1' 'libx11-6' 2>/dev/null \
    | grep -v '=$' | sort
} | sha256sum | cut -c1-16
