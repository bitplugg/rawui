#!/usr/bin/env bash
# ---------------------------------------------------------------------------
#  One-command local build: builds (or reuses) the build image, then compiles
#  the .pkg inside it. Requires Docker.
#
#  Usage:  tools/docker-build.sh [extra make targets/flags]
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${RAWUI_IMAGE:-rawui-build:latest}"

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "==> building image $IMAGE (one-off, downloads the PS4 toolchain)"
    docker build -t "$IMAGE" "$ROOT/tools"
fi

echo "==> compiling inside $IMAGE"
docker run --rm \
    -v "$ROOT:/workspace" \
    -u "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -w /workspace \
    "$IMAGE" \
    bash tools/build.sh "$@"
