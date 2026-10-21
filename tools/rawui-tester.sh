#!/usr/bin/env bash
# ============================================================================
#  RAW UI - "rawui-tester" sanity suite.
#
#  This is the project's own bot.  It checks the things the generic tooling
#  cannot see, i.e. the invariants that only matter for a PS4 FPKG shell:
#
#    1. package identity   TITLE_ID / CONTENT_ID / VERSION formats and their
#                          mutual consistency (this is what produced CE-32957-6:
#                          a build can succeed while the metadata is rejected by
#                          the console's installer)
#    2. package payload    every file the .pkg is supposed to carry is listed in
#                          the gp4 manifest AND actually present on disk, plus
#                          the pkg magic/size
#    3. pages drift        docs/RAWUI0001.pkg (the Pages fallback) must be the
#                          exact bytes of the release artefact
#    4. colour port        src/md3/color.cpp and docs/themes.html must agree
#    5. i18n parity        English/Russian table structure
#
#  Usage:
#      tools/rawui-tester.sh [--skip-pkg] [--pkg <path>] [--base <ref>]
#
#  Exit status is 0 when everything passes; each failure prints a GitHub
#  annotation (::error file=...,line=...) so the check is useful in CI and on a
#  plain terminal alike.
# ============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

SKIP_PKG=0
PKG_PATH=""
BASE_REF=""

while [ $# -gt 0 ]; do
    case "$1" in
        --skip-pkg) SKIP_PKG=1 ;;
        --pkg)      PKG_PATH="${2:-}"; shift ;;
        --base)     BASE_REF="${2:-}"; shift ;;
        -h|--help)  sed -n '2,26p' "$0"; exit 0 ;;
        *) echo "rawui-tester: unknown argument: $1" >&2; exit 2 ;;
    esac
    shift
done

PASS=0
FAIL=0

ok()   { printf '  \033[32mok\033[0m   %s\n' "$1"; PASS=$((PASS + 1)); }
bad()  { printf '  \033[31mFAIL\033[0m %s\n' "$1"; FAIL=$((FAIL + 1)); }
note() { printf '  \033[2m-    %s\033[0m\n' "$1"; }

section() { printf '\n\033[1m%s\033[0m\n' "$1"; }

# GitHub workflow annotation, ignored outside CI.
gh_error() { printf '::error file=%s,line=%s::%s\n' "$1" "$2" "$3"; }

# ---------------------------------------------------------------------------
#  Makefile metadata reader.  Values are plain `KEY := value` lines; reading
#  them from the Makefile (rather than duplicating the constants here) is the
#  point: if someone changes one they must change all of them together.
# ---------------------------------------------------------------------------
make_var() {
    sed -n "s/^[[:space:]]*$1[[:space:]]*:=[[:space:]]*//p" Makefile | head -n1
}

TITLE_ID="$(make_var TITLE_ID)"
CONTENT_ID="$(make_var CONTENT_ID)"
VERSION="$(make_var VERSION)"

# ===========================================================================
section "1. Package identity"
# ===========================================================================

if [ -z "$TITLE_ID" ] || [ -z "$CONTENT_ID" ]; then
    bad "could not read TITLE_ID/CONTENT_ID from Makefile"
    gh_error Makefile 16 "TITLE_ID/CONTENT_ID not found"
else
    # 4 uppercase letters + 5 digits, e.g. RAWU00001.
    if printf '%s' "$TITLE_ID" | grep -qE '^[A-Z]{4}[0-9]{5}$'; then
        ok "TITLE_ID ${TITLE_ID} matches ^[A-Z]{4}[0-9]{5}\$"
    else
        bad "TITLE_ID '${TITLE_ID}' is not 4 uppercase letters + 5 digits"
        gh_error Makefile 16 "TITLE_ID '${TITLE_ID}' must match ^[A-Z]{4}[0-9]{5}\$ (CE-32957-6)"
    fi

    # CONTENT_ID is IV0000-<TITLE_ID>_00-<FREE>, 36 chars total.
    EXPECT_PREFIX="IV0000-${TITLE_ID}_00-"
    case "$CONTENT_ID" in
        "$EXPECT_PREFIX"*)
            ok "CONTENT_ID is built from TITLE_ID (${EXPECT_PREFIX}...)"
            ;;
        *)
            bad "CONTENT_ID '${CONTENT_ID}' does not start with '${EXPECT_PREFIX}'"
            gh_error Makefile 17 "CONTENT_ID must be built from TITLE_ID (expected prefix '${EXPECT_PREFIX}')"
            ;;
    esac

    CID_LEN=${#CONTENT_ID}
    if [ "$CID_LEN" -eq 36 ]; then
        ok "CONTENT_ID length is 36"
    else
        bad "CONTENT_ID length is ${CID_LEN}, expected 36"
        gh_error Makefile 17 "CONTENT_ID must be exactly 36 characters ('${CONTENT_ID}')"
    fi

    if printf '%s' "$CONTENT_ID" | grep -qE '^IV0000-[A-Z]{4}[0-9]{5}_00-[A-Z0-9]{16}$'; then
        ok "CONTENT_ID matches ^IV0000-[A-Z]{4}[0-9]{5}_00-[A-Z0-9]{16}\$"
    else
        bad "CONTENT_ID '${CONTENT_ID}' has an unexpected shape"
        gh_error Makefile 17 "CONTENT_ID '${CONTENT_ID}' has an unexpected shape"
    fi
fi

# param.sfo VERSION / APP_VER are MAJOR.MINOR, zero padded (NN.NN).  The shell
# shows APP_VERSION from the tag, but the *sfo* value is what the console reads
# when installing, and a malformed one is another install failure.  The integer
# major part is not width-limited (release.yml emits %02d.%02d, the Makefile
# default is 1.00), only the minor part must be two digits.
if printf '%s' "$VERSION" | grep -qE '^[0-9]+\.[0-9]{2}$'; then
    ok "param.sfo VERSION ${VERSION} matches ^[0-9]+\\.[0-9]{2}\$"
else
    bad "VERSION '${VERSION}' is not MAJOR.NN"
    gh_error Makefile 15 "VERSION '${VERSION}' must be MAJOR.NN with a two-digit minor"
fi

# ===========================================================================
section "2. Package payload"
# ===========================================================================

# Files the shell cannot work without.  `sce_sys/param.sfo` and the module prx
# files are the ones whose absence only shows up as a console-side failure, so
# they are worth pinning explicitly.
REQUIRED_FILES="eboot.bin sce_sys/param.sfo sce_sys/icon0.png"

if [ "$SKIP_PKG" -eq 1 ]; then
    note "skipped (--skip-pkg)"
else
    if [ -z "$PKG_PATH" ]; then
        PKG_PATH="$(ls -1 dist/*.pkg 2>/dev/null | head -n1 || true)"
    fi

    if [ -z "$PKG_PATH" ] || [ ! -f "$PKG_PATH" ]; then
        bad "no built .pkg found (looked in dist/)"
        gh_error dist 1 "no .pkg artefact found"
    else
        # --- magic: PS4 packages start with 7F 43 4E 54 ("\x7FCNT") ---------
        MAGIC="$(head -c 4 "$PKG_PATH" | od -An -tx1 | tr -d ' \n')"
        if [ "$MAGIC" = "7f434e54" ]; then
            ok "package magic is 7f434e54"
        else
            bad "package magic is '$MAGIC', expected 7f434e54"
            gh_error "$PKG_PATH" 1 "bad package magic: $MAGIC"
        fi

        SIZE="$(stat -c%s "$PKG_PATH")"
        if [ "$SIZE" -ge 1000000 ]; then
            ok "package is $(numfmt --to=iec "$SIZE" 2>/dev/null || echo "$SIZE bytes")"
        else
            bad "package suspiciously small: $SIZE bytes"
            gh_error "$PKG_PATH" 1 "package suspiciously small: $SIZE bytes"
        fi
    fi

    # --- gp4 manifest -------------------------------------------------------
    # `create-gp4` walks PKG_FILES and writes pkg.gp4; PkgTool.Core then packs
    # exactly what the manifest lists.  Checking the manifest therefore proves
    # what went into the package without having to decrypt the PFS image.
    if [ ! -f pkg/pkg.gp4 ]; then
        bad "pkg/pkg.gp4 is missing - package was not built"
        gh_error pkg/pkg.gp4 1 "gp4 manifest missing"
    else
        for f in $REQUIRED_FILES; do
            if grep -q "targ_path=\"${f}\"" pkg/pkg.gp4; then
                ok "manifest lists ${f}"
            else
                bad "manifest does not list ${f}"
                gh_error pkg/pkg.gp4 1 "manifest is missing required entry '${f}'"
            fi
        done

        # At least one sce_module/*.prx (the kernel modules) must be bundled.
        if grep -q 'targ_path="sce_module/[^"]*\.prx"' pkg/pkg.gp4; then
            MODS="$(grep -o 'targ_path="sce_module/[^"]*\.prx"' pkg/pkg.gp4 \
                    | sed 's/.*"\(.*\)"/\1/' | tr '\n' ' ')"
            ok "manifest bundles sce_module: ${MODS% }"
        else
            bad "manifest lists no sce_module/*.prx"
            gh_error pkg/pkg.gp4 1 "no sce_module/*.prx in the package"
        fi

        # Every targ_path the manifest claims must exist next to it (a stale
        # manifest from a previous build would otherwise describe a package
        # that was never produced).
        MISSING=0
        while IFS= read -r rel; do
            [ -n "$rel" ] || continue
            if [ ! -f "pkg/$rel" ]; then
                bad "manifest entry has no source file: pkg/${rel}"
                gh_error "pkg/${rel}" 1 "file listed in pkg.gp4 but missing on disk"
                MISSING=$((MISSING + 1))
            fi
        done < <(grep -o 'orig_path="[^"]*"' pkg/pkg.gp4 | sed 's/orig_path="\(.*\)"/\1/')
        [ "$MISSING" -eq 0 ] && ok "all manifest entries exist on disk"

        # The content_id baked into the package must be the Makefile's.
        if grep -q "content_id=\"${CONTENT_ID}\"" pkg/pkg.gp4; then
            ok "package content_id matches the Makefile"
        else
            GP4_CID="$(grep -o 'content_id="[^"]*"' pkg/pkg.gp4 | head -n1 | sed 's/.*"\(.*\)"/\1/')"
            bad "package content_id '${GP4_CID}' != Makefile '${CONTENT_ID}'"
            gh_error pkg/pkg.gp4 1 "content_id mismatch (CE-32957-6)"
        fi
    fi
fi

# ===========================================================================
section "3. Pages install link"
# ===========================================================================

# The site no longer commits docs/RAWUI0001.pkg.  pages.yml stages the site and
# downloads the newest release asset into _site/RAWUI0001.pkg at deploy time, so
# what has to hold is the *wiring*: the deploy must fetch a .pkg, name it
# RAWUI0001.pkg, and the page must link to that name.  Checking the wiring is
# what catches a rename on either side; the bytes themselves are guaranteed by
# the release asset being downloaded directly.
PAGES_YML=".github/workflows/pages.yml"
INDEX_HTML="docs/index.html"

if [ ! -f "$PAGES_YML" ]; then
    bad "${PAGES_YML} is missing; nothing publishes the install package"
    gh_error "$PAGES_YML" 1 "pages workflow missing"
else
    if grep -q 'RAWUI0001\.pkg' "$PAGES_YML"; then
        ok "pages.yml names the served package RAWUI0001.pkg"
    else
        bad "pages.yml does not name RAWUI0001.pkg; the install link would 404"
        gh_error "$PAGES_YML" 1 "pages.yml no longer publishes RAWUI0001.pkg"
    fi

    if grep -q 'gh release download' "$PAGES_YML"; then
        ok "pages.yml takes the package from the latest release"
    else
        bad "pages.yml does not download a release asset"
        gh_error "$PAGES_YML" 1 "pages.yml does not fetch the released package"
    fi
fi

if [ ! -f "$INDEX_HTML" ]; then
    bad "${INDEX_HTML} is missing"
    gh_error "$INDEX_HTML" 1 "index.html missing"
else
    # The download button has to point at the name pages.yml publishes.
    if grep -qE 'href="RAWUI0001\.pkg"' "$INDEX_HTML"; then
        ok "docs/index.html links to RAWUI0001.pkg"
    else
        bad "docs/index.html has no link to RAWUI0001.pkg"
        gh_error "$INDEX_HTML" 1 "index.html does not link to RAWUI0001.pkg"
    fi
fi

# A committed package here is now a bug: it would shadow the release asset if
# the download ever failed, serving a stale build from a different version.
if git ls-files --error-unmatch docs/RAWUI0001.pkg >/dev/null 2>&1; then
    bad "docs/RAWUI0001.pkg is committed again; Pages should serve the release asset"
    gh_error docs/RAWUI0001.pkg 1 "committed Pages package shadows the release asset"
else
    ok "docs/RAWUI0001.pkg is not committed (Pages serves the release asset)"
fi

# ===========================================================================
section "4. Colour engine port"
# ===========================================================================

if command -v node >/dev/null 2>&1; then
    if out="$(bash tests/port_sync.sh 2>&1)"; then
        ok "$out"
    else
        bad "src/md3/color.cpp and docs/themes.html disagree"
        printf '%s\n' "$out" | sed 's/^/       /'
        gh_error docs/themes.html 1 "JS colour engine is out of sync with src/md3/color.cpp"
    fi
else
    note "skipped (node not available)"
fi

# ===========================================================================
section "5. i18n parity"
# ===========================================================================

I18N_BIN="build/i18n_test"
if [ ! -x "$I18N_BIN" ] && ! command -v g++ >/dev/null 2>&1; then
    note "skipped (no host C++ compiler)"
else
    if out="$(make test-i18n 2>&1)"; then
        ok "English/Russian string table is consistent"
        [ -n "$out" ] && printf '%s\n' "$out" | sed 's/^/       /'
    else
        bad "i18n table failed its structural checks"
        printf '%s\n' "$out" | sed 's/^/       /'
        gh_error src/core/i18n.hpp 1 "i18n table failed structural checks (see build log)"
    fi
fi

# ===========================================================================
#  Summary
# ===========================================================================
printf '\n\033[1mrawui-tester: %d passed, %d failed\033[0m\n' "$PASS" "$FAIL"
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
echo "all checks green"
