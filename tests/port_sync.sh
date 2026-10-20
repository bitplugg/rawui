#!/usr/bin/env bash
# ============================================================================
#  RAW UI - colour engine port sync.
#
#  docs/themes.html carries a JavaScript copy of src/md3/color.cpp so the web
#  theme generator previews exactly what the console renders.  The two drift
#  silently (someone edits one side), so this script generates schemes with
#  both and diffs them.
#
#  Usage:  tests/port_sync.sh          (or `make test-port`)
# ============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CXX="${CXX:-g++}"
TMP="${TMPDIR:-/tmp}/rawui-port-sync.$$"

cleanup() { rm -f "$TMP" "$TMP.cpp" "$TMP.js"; }
trap cleanup EXIT

command -v node >/dev/null 2>&1 || {
    echo "port-sync: node is not available, skipping" >&2
    exit 0
}

"$CXX" -std=c++17 -O2 -I"$ROOT/src" -o "$TMP" "$ROOT/tests/scheme_dump.cpp" \
    "$ROOT/src/md3/color.cpp"
"$TMP" > "$TMP.cpp"
node "$ROOT/tests/scheme_dump.js" > "$TMP.js"

python3 - "$TMP.cpp" "$TMP.js" <<'PY'
import sys

cpp_path, js_path = sys.argv[1], sys.argv[2]
cpp = [l for l in open(cpp_path).read().splitlines() if l.strip()]
js = [l for l in open(js_path).read().splitlines() if l.strip()]

if len(cpp) != len(js):
    sys.exit(f"port-sync: line count differs (C++ {len(cpp)}, JS {len(js)})")

# C++ uses float, JS uses double, so a channel may legitimately round to a
# neighbouring code value; anything larger means the implementations disagree.
TOL = 1
worst = 0
mismatches = []

for n, (a, b) in enumerate(zip(cpp, js), start=1):
    ka, kb = a.split("|"), b.split("|")
    if ka[:4] != kb[:4]:
        sys.exit(f"port-sync: header mismatch on line {n}: {ka[:4]} vs {kb[:4]}")
    if len(ka) != len(kb):
        sys.exit(f"port-sync: field count differs on line {n}: {len(ka)} vs {len(kb)}")
    for role, x, y in zip(range(len(ka) - 4), ka[4:], kb[4:]):
        for i in range(0, 6, 2):
            d = abs(int(x[i:i + 2], 16) - int(y[i:i + 2], 16))
            worst = max(worst, d)
            if d > TOL:
                mismatches.append(f"line {n} field #{role}: C++ {x} vs JS {y} (delta {d})")

if mismatches:
    print(f"port-sync: {len(mismatches)} channel(s) out of tolerance:")
    for m in mismatches[:20]:
        print("  " + m)
    sys.exit(1)

print(f"port-sync: OK - {len(cpp)} schemes x {len(cpp[0].split('|')) - 4} roles, "
      f"worst channel delta {worst} (tolerance {TOL})")
PY
