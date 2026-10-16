#!/usr/bin/env bash
# ---------------------------------------------------------------------------
#  Fetch the fonts RAW UI ships in its package.
#
#  They are downloaded rather than committed so the repository stays small and
#  the build stays reproducible: every file is pinned by URL and sha256.
#  All three fonts are licensed under the SIL Open Font License 1.1.
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FONTDIR="$ROOT/pkg/assets/fonts"
mkdir -p "$FONTDIR"

# name|url|sha256
FILES=(
"RobotoFlex.ttf|https://github.com/google/fonts/raw/main/ofl/robotoflex/RobotoFlex%5BGRAD%2CXOPQ%2CXTRA%2CYOPQ%2CYTAS%2CYTDE%2CYTFI%2CYTLC%2CYTUC%2Copsz%2Cslnt%2Cwdth%2Cwght%5D.ttf|9b523f7d82593df0107173849ebb8c817471a1df4b4fb2c3cbf40cfd810c8281"
"MaterialSymbolsRounded.ttf|https://github.com/google/material-design-icons/raw/master/variablefont/MaterialSymbolsRounded%5BFILL%2CGRAD%2Copsz%2Cwght%5D.ttf|32e4011709e055596eb8d2dd0f7cb547b47c1d40ac9ecb8fad54f7f7eac202e7"
"OFL-roboto.txt|https://github.com/google/fonts/raw/main/ofl/roboto/OFL.txt|061402327a96aadb0bfb694a960ed289ecd38d383e396243831ab81feb109c41"
)

if ! command -v sha256sum >/dev/null 2>&1; then
    echo "error: sha256sum is required" >&2
    exit 1
fi

for entry in "${FILES[@]}"; do
    IFS='|' read -r name url sum <<<"$entry"
    dest="$FONTDIR/$name"

    if [[ -f "$dest" ]] && echo "$sum  $dest" | sha256sum -c --status 2>/dev/null; then
        echo "    ok  $name"
        continue
    fi

    echo "==> fetch $name"
    curl -fsSL --retry 3 -o "$dest.part" "$url"
    mv "$dest.part" "$dest"

    if ! echo "$sum  $dest" | sha256sum -c --status; then
        echo "error: checksum mismatch for $name" >&2
        rm -f "$dest"
        exit 1
    fi
done

echo "==> assets ready in pkg/assets/fonts"
