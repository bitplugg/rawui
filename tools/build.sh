#!/usr/bin/env bash
# ---------------------------------------------------------------------------
#  Build RAW UI. Works both inside the build container (tools/Dockerfile) and
#  on a host that has clang/lld plus OO_PS4_TOOLCHAIN exported.
#
#  Usage:  tools/build.sh [extra make targets/flags]
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ -z "${OO_PS4_TOOLCHAIN:-}" ]]; then
    cat >&2 <<'EOF'
error: OO_PS4_TOOLCHAIN is not set.

  * Easiest:  tools/docker-build.sh
  * Or locally:
      curl -L -o tc.tar.gz \
        https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/download/v0.5.4/toolchain-llvm-18.tar.gz
      tar -xzf tc.tar.gz
      export OO_PS4_TOOLCHAIN="$PWD/OpenOrbis/PS4Toolchain"
EOF
    exit 1
fi

echo "==> toolchain : $OO_PS4_TOOLCHAIN"
echo "==> compiler  : $(command -v clang) ($(clang --version | head -1))"

bash tools/fetch-assets.sh

JOBS="$( (command -v nproc >/dev/null && nproc) || echo 4 )"
make -j"$JOBS" "$@"

echo
echo "==> artefacts:"
ls -1 dist/*.pkg 2>/dev/null || true
