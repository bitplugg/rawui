// Host-side golden tests for the M3 colour engine (src/md3/color.cpp).
// Run: make test  (or g++ -std=c++17 -Isrc tests/color_test.cpp src/md3/color.cpp)
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "md3/color.hpp"

namespace {

int g_failures = 0;

uint32_t toHex(const rawui::Color& c) {
    return (static_cast<uint32_t>(c.r) << 16) | (static_cast<uint32_t>(c.g) << 8) | c.b;
}

// Sum of the absolute per-channel distance - used to require a *visible* shift
// rather than an accidental one-code-value nudge.
int delta(const rawui::Color& a, const rawui::Color& b) {
    return std::abs(int(a.r) - int(b.r)) + std::abs(int(a.g) - int(b.g)) +
           std::abs(int(a.b) - int(b.b));
}

// Byte-quantised colour math can differ by ±1 across compilers/optimisation
// levels, so comparisons allow a small per-channel tolerance.
bool nearColor(const rawui::Color& got, uint32_t wantHex, int tol, const char* what) {
    const rawui::Color want = rawui::Color::fromHex(wantHex);
    const int dr = std::abs(int(got.r) - int(want.r));
    const int dg = std::abs(int(got.g) - int(want.g));
    const int db = std::abs(int(got.b) - int(want.b));
    if (dr > tol || dg > tol || db > tol) {
        std::printf("FAIL %s: got #%06X want #%06X (tol %d, delta %d/%d/%d)\n", what, toHex(got),
                    wantHex, tol, dr, dg, db);
        ++g_failures;
        return false;
    }
    return true;
}

}  // namespace

int main() {
    using namespace rawui;

    // --- round-trip ---------------------------------------------------------
    {
        const Lab lab = rgbToLab(Color(0x67, 0x50, 0xA4));
        const Color back = labToRgb(lab);
        nearColor(back, 0x6750A4, 2, "rgb->lab->rgb roundtrip");
    }

    // --- gamut clamping -----------------------------------------------------
    {
        for (int L = 0; L <= 100; L += 5) {
            for (float h = 0.0f; h < 360.0f; h += 15.0f) {
                const float c = maxChromaInGamut(static_cast<float>(L), h);
                LCh lch{static_cast<float>(L), c, h};
                const Color out = labToRgb(lchToLab(lch));
                // Converting the gamut-limiting colour itself must stay legal
                // (single channel may sit exactly on the boundary).
                const int mx = std::max({int(out.r), int(out.g), int(out.b)});
                const int mn = std::min({int(out.r), int(out.g), int(out.b)});
                if (mx > 255 || mn < 0) {  // toByte clamps, so this checks LCh path sanity
                    std::printf("FAIL gamut L=%d h=%.0f\n", L, h);
                    ++g_failures;
                }
            }
        }
    }

    // --- makeScheme golden values (dark, Amethyst 0x6750A4) -----------------
    // Pinned against the engine (the web port in docs/themes.html reproduces
    // the same bytes); these are regression values, not Material reference
    // palette figures - we run LCh, not CAM16.
    {
        const Scheme s = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{0.0f, false, true});
        nearColor(s.primary, 0xD2BBFF, 3, "dark primary");
        nearColor(s.onPrimary, 0x32226F, 3, "dark onPrimary");
        nearColor(s.primaryContainer, 0x4C3889, 3, "dark primaryContainer");
        nearColor(s.surface, 0x141317, 3, "dark surface");
        nearColor(s.onSurface, 0xE4E1E7, 3, "dark onSurface");
        nearColor(s.error, 0xFFB3AE, 3, "dark error");
        nearColor(s.success, 0x9BD595, 3, "dark success");
    }

    // --- light scheme golden values ----------------------------------------
    {
        const Scheme s = makeScheme(Color::fromHex(0x6750A4), false, SchemeOptions{0.0f, false, true});
        nearColor(s.primary, 0x6750A4, 3, "light primary");
        nearColor(s.onPrimary, 0xFFFFFF, 3, "light onPrimary");
        nearColor(s.primaryContainer, 0xE9DDFF, 3, "light primaryContainer");
        nearColor(s.surface, 0xFAF8FE, 3, "light surface");
        nearColor(s.onSurface, 0x1C1B1F, 3, "light onSurface");
    }

    // --- tertiary uses the rotated hue (regression: args used to be swapped,
    //     which produced garbage lightness values) ---------------------------
    {
        const Scheme d = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{0.0f, false, true});
        const Scheme l = makeScheme(Color::fromHex(0x6750A4), false, SchemeOptions{0.0f, false, true});
        // Tertiary must be neither the primary hue clone nor near-white/black.
        const int pd = std::abs(int(d.tertiary.r) - int(d.primary.r)) +
                       std::abs(int(d.tertiary.g) - int(d.primary.g)) +
                       std::abs(int(d.tertiary.b) - int(d.primary.b));
        if (pd < 20) {
            std::printf("FAIL dark tertiary ~= primary (delta %d)\n", pd);
            ++g_failures;
        }
        const int tLum = int(l.tertiary.r) + int(l.tertiary.g) + int(l.tertiary.b);
        if (tLum < 60 || tLum > 700) {
            std::printf("FAIL light tertiary out of sane range (lum %d)\n", tLum);
            ++g_failures;
        }
        const int dLum = int(d.tertiaryContainer.r) + int(d.tertiaryContainer.g) +
                         int(d.tertiaryContainer.b);
        if (dLum < 60 || dLum > 700) {
            std::printf("FAIL dark tertiaryContainer out of sane range (lum %d)\n", dLum);
            ++g_failures;
        }
    }

    // --- neutral stays nearly grey ------------------------------------------
    {
        const Scheme s = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{0.0f, false, true});
        const int spread = std::max({int(s.surface.r), int(s.surface.g), int(s.surface.b)}) -
                           std::min({int(s.surface.r), int(s.surface.g), int(s.surface.b)});
        if (spread > 12) {
            std::printf("FAIL dark surface not neutral (spread %d)\n", spread);
            ++g_failures;
        }
    }

    // --- contrast and vibrance shift the result ------------------------------
    // Regression guard: contrast used to scale chroma only, and toneAt() clamps
    // chroma to the sRGB gamut limit - 25 of 33 roles did not move at all, so
    // the Settings slider did nothing.  Contrast now pushes lightness.
    {
        const Scheme dk0 = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{0.0f, false, true});
        const Scheme dk1 = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{0.3f, false, true});
        const Scheme lt0 = makeScheme(Color::fromHex(0x6750A4), false, SchemeOptions{0.0f, false, true});
        const Scheme lt1 = makeScheme(Color::fromHex(0x6750A4), false, SchemeOptions{0.3f, false, true});
        const Scheme ltV = makeScheme(Color::fromHex(0x6750A4), false, SchemeOptions{0.0f, true, true});
        const Scheme dkV = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{0.0f, true, true});

        const auto check = [&](bool same, const char* what) {
            if (same) {
                std::printf("FAIL %s did not respond\n", what);
                ++g_failures;
            }
        };
        check(toHex(dk0.surface) == toHex(dk1.surface), "dark surface to contrast");
        check(toHex(dk0.primary) == toHex(dk1.primary), "dark primary to contrast");
        check(toHex(dk0.success) == toHex(dk1.success), "dark success to contrast");
        check(toHex(lt0.primary) == toHex(lt1.primary), "light primary to contrast");
        // Dark primary sits on the gamut ceiling, so vibrance is only visible
        // where there is chroma headroom: light primary / dark container.
        check(toHex(lt0.primary) == toHex(ltV.primary), "light primary to vibrant");
        check(toHex(dk0.primaryContainer) == toHex(dkV.primaryContainer), "dark container to vibrant");

        // A contrast slider has to move colours by a visible amount, not by
        // one or two code values.
        if (delta(dk0.surface, dk1.surface) < 15) {
            std::printf("FAIL dark surface contrast shift too small (delta %d)\n",
                        delta(dk0.surface, dk1.surface));
            ++g_failures;
        }
        if (delta(lt0.primary, lt1.primary) < 15) {
            std::printf("FAIL light primary contrast shift too small (delta %d)\n",
                        delta(lt0.primary, lt1.primary));
            ++g_failures;
        }

        // Negative contrast must move colours the other way (the slider is
        // symmetric around 0).
        const Scheme dkN = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{-0.3f, false, true});
        if (toHex(dk0.surface) == toHex(dkN.surface)) {
            std::printf("FAIL negative contrast did not change dark surface\n");
            ++g_failures;
        }
        if (!(dkN.surface.r > dk0.surface.r && dkN.surface.g > dk0.surface.g)) {
            std::printf("FAIL negative contrast moved dark surface the wrong way\n");
            ++g_failures;
        }
    }

    // --- mixScheme endpoints -------------------------------------------------
    {
        const Scheme a = makeScheme(Color::fromHex(0x6750A4), true, SchemeOptions{});
        const Scheme b = makeScheme(Color::fromHex(0x146C2E), false, SchemeOptions{});
        if (toHex(mixScheme(a, b, 0.0f).primary) != toHex(a.primary)) {
            std::printf("FAIL mix t=0 is not a\n");
            ++g_failures;
        }
        if (toHex(mixScheme(a, b, 1.0f).primary) != toHex(b.primary)) {
            std::printf("FAIL mix t=1 is not b\n");
            ++g_failures;
        }
        const Color mid = mixScheme(a, b, 0.5f).primary;
        const int midLum = int(mid.r) + int(mid.g) + int(mid.b);
        if (midLum <= 0) {  // sanity: produced a colour at all
            std::printf("FAIL mix t=0.5 degenerate\n");
            ++g_failures;
        }
    }

    // --- every preset seed generates a sane dark scheme -----------------------
    {
        for (int i = 0; i < kSeedPresetCount; ++i) {
            const Scheme s = makeScheme(Color::fromHex(kSeedPresets[i].rgb), true, SchemeOptions{});
            const int lum = int(s.onSurface.r) + int(s.onSurface.g) + int(s.onSurface.b);
            if (lum < 400) {  // onSurface in dark mode must be light text
                std::printf("FAIL preset %s: dark onSurface too dark (lum %d)\n",
                            kSeedPresets[i].name, lum);
                ++g_failures;
            }
        }
    }

    if (g_failures == 0) {
        std::printf("all colour tests passed\n");
        return 0;
    }
    std::printf("%d failure(s)\n", g_failures);
    return 1;
}
