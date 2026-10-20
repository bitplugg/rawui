#include "md3/color.hpp"

#include <cmath>

namespace rawui {
namespace {

// D65 white point.
constexpr float kXn = 0.95047f;
constexpr float kYn = 1.00000f;
constexpr float kZn = 1.08883f;

constexpr float kEps = 216.0f / 24389.0f;
constexpr float kKappa = 24389.0f / 27.0f;

inline float srgbToLinear(float c) {
    return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

inline float linearToSrgb(float c) {
    return (c <= 0.0031308f) ? (c * 12.92f) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
}

inline float labF(float t) {
    return (t > kEps) ? std::cbrt(t) : ((kKappa * t + 16.0f) / 116.0f);
}

inline float labFInv(float t) {
    const float t3 = t * t * t;
    return (t3 > kEps) ? t3 : ((116.0f * t - 16.0f) / kKappa);
}

// ---------------------------------------------------------------------------
// Linear RGB (0..1) for a Lab colour.  No clamping - the caller checks gamut.
void labToLinearRgb(const Lab& lab, float& r, float& g, float& b) {
    const float fy = (lab.l + 16.0f) / 116.0f;
    const float fx = fy + lab.a / 500.0f;
    const float fz = fy - lab.b / 200.0f;

    const float x = kXn * labFInv(fx);
    const float y = kYn * labFInv(fy);
    const float z = kZn * labFInv(fz);

    r = 3.2404542f * x - 1.5371385f * y - 0.4985314f * z;
    g = -0.9692660f * x + 1.8760108f * y + 0.0415560f * z;
    b = 0.0556434f * x - 0.2040259f * y + 1.0572252f * z;
}

inline bool inGamut(float r, float g, float b) {
    constexpr float tol = 1e-4f;
    return r >= -tol && r <= 1.0f + tol && g >= -tol && g <= 1.0f + tol && b >= -tol &&
           b <= 1.0f + tol;
}

inline uint8_t toByte(float linear) {
    const float s = linearToSrgb(clampf(linear, 0.0f, 1.0f));
    return static_cast<uint8_t>(clampf(s * 255.0f + 0.5f, 0.0f, 255.0f));
}

// One tonal step: target lightness, hue from the source, chroma scaled to fit.
Color toneAt(float lightness, float hue, float chromaWanted) {
    LCh lch;
    lch.l = lightness;
    lch.h = hue;
    lch.c = clampf(chromaWanted, 0.0f, maxChromaInGamut(lightness, hue));
    return labToRgb(lchToLab(lch));
}

}  // namespace

// ---------------------------------------------------------------------------
Lab rgbToLab(const Color& c) {
    const float r = srgbToLinear(static_cast<float>(c.r) / 255.0f);
    const float g = srgbToLinear(static_cast<float>(c.g) / 255.0f);
    const float b = srgbToLinear(static_cast<float>(c.b) / 255.0f);

    const float x = 0.4124564f * r + 0.3575761f * g + 0.1804375f * b;
    const float y = 0.2126729f * r + 0.7151522f * g + 0.0721750f * b;
    const float z = 0.0193339f * r + 0.1191920f * g + 0.9503041f * b;

    const float fx = labF(x / kXn);
    const float fy = labF(y / kYn);
    const float fz = labF(z / kZn);

    Lab out;
    out.l = 116.0f * fy - 16.0f;
    out.a = 500.0f * (fx - fy);
    out.b = 200.0f * (fy - fz);
    return out;
}

Color labToRgb(const Lab& lab) {
    float r, g, b;
    labToLinearRgb(lab, r, g, b);
    return Color(toByte(r), toByte(g), toByte(b), 255);
}

LCh labToLCh(const Lab& lab) {
    LCh out;
    out.l = lab.l;
    out.c = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    out.h = std::atan2(lab.b, lab.a) * 180.0f / kPi;
    if (out.h < 0.0f) out.h += 360.0f;
    return out;
}

Lab lchToLab(const LCh& lch) {
    const float rad = lch.h * kPi / 180.0f;
    Lab out;
    out.l = lch.l;
    out.a = lch.c * std::cos(rad);
    out.b = lch.c * std::sin(rad);
    return out;
}

float maxChromaInGamut(float lightness, float hueDegrees) {
    // Binary search the chroma axis; 24 iterations is far more than enough for
    // byte-quantised output.
    float lo = 0.0f;
    float hi = 160.0f;

    for (int i = 0; i < 24; ++i) {
        const float mid = 0.5f * (lo + hi);
        LCh lch;
        lch.l = lightness;
        lch.h = hueDegrees;
        lch.c = mid;

        float r, g, b;
        labToLinearRgb(lchToLab(lch), r, g, b);
        if (inGamut(r, g, b)) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;
}

Color mixLab(const Color& a, const Color& b, float t) {
    const Lab la = rgbToLab(a);
    const Lab lb = rgbToLab(b);
    Lab out;
    out.l = lerpf(la.l, lb.l, t);
    out.a = lerpf(la.a, lb.a, t);
    out.b = lerpf(la.b, lb.b, t);
    Color c = labToRgb(out);
    c.a = static_cast<uint8_t>(lerpf(a.a, b.a, t) + 0.5f);
    return c;
}

// ---------------------------------------------------------------------------
Scheme makeScheme(const Color& seed, bool dark, const SchemeOptions& options) {
    const LCh seedLch = labToLCh(rgbToLab(seed));

    // Chroma budgets.  A neutral grey seed has almost no chroma, so lift the
    // floor a little to keep the UI from looking completely colourless.
    const float hue = seedLch.h;
    const float baseChroma = maxf(seedLch.c, 8.0f);

    const float cPrimary = options.vibrant ? baseChroma * 1.15f : baseChroma;
    const float cSecondary = baseChroma * 0.42f;
    const float cTertiary = baseChroma * 0.75f;
    const float cNeutral = 3.0f;
    const float cNeutralVariant = 8.0f;

    // Expressive schemes rotate the tertiary hue away from the seed.
    const float tertiaryHue = std::fmod(hue + 60.0f, 360.0f);

    const float k = 1.0f + options.contrast * 0.35f;

    // Contrast is first of all a *lightness* lever.  Scaling chroma alone does
    // almost nothing: at most tonal steps the seed chroma already sits at the
    // sRGB gamut boundary, so toneAt() clamps it straight back.  Pushing the
    // tones away from mid grey moves foreground and background apart, which is
    // what the Material contrast tokens actually do.  At contrast 0 this is an
    // exact no-op, so the default palette is untouched.
    const float cs = options.contrast;
    const float spread = 15.0f;
    const auto K = [&](float t) {
        float dir;
        if (t > 50.0f) {
            dir = 1.0f;
        } else if (t < 50.0f) {
            dir = -1.0f;
        } else {
            dir = dark ? 1.0f : -1.0f;  // ambiguous midpoint: follow the surface
        }
        return clampf(t + dir * cs * spread, 0.0f, 100.0f);
    };

    const auto P = [&](float t) { return toneAt(K(t), hue, cPrimary * k); };
    const auto S = [&](float t) { return toneAt(K(t), hue, cSecondary * k); };
    const auto T = [&](float t) { return toneAt(K(t), tertiaryHue, cTertiary * k); };
    const auto N = [&](float t) { return toneAt(K(t), hue, cNeutral); };
    const auto NV = [&](float t) { return toneAt(K(t), hue, cNeutralVariant); };
    // Error is always the Material red family, re-tuned for the theme's contrast.
    const auto E = [&](float t) { return toneAt(K(t), 27.0f, 60.0f * k); };
    // Status colours used fixed tones and ignored contrast entirely; they form
    // container/on-container pairs, so they take part in the same push.
    const auto ST = [&](float t, float h, float c) { return toneAt(K(t), h, c * k); };

    Scheme s;

    if (!dark) {
        s.primary = P(40);
        s.onPrimary = P(100);
        s.primaryContainer = P(90);
        s.onPrimaryContainer = P(10);
        s.primaryFixed = P(90);
        s.primaryFixedDim = P(80);
        s.onPrimaryFixed = P(10);
        s.onPrimaryFixedVariant = P(30);

        s.secondary = S(40);
        s.onSecondary = S(100);
        s.secondaryContainer = S(90);
        s.onSecondaryContainer = S(10);

        s.tertiary = T(40);
        s.onTertiary = T(100);
        s.tertiaryContainer = T(90);
        s.onTertiaryContainer = T(10);

        s.error = E(40);
        s.onError = E(100);
        s.errorContainer = E(90);
        s.onErrorContainer = E(10);

        s.surface = N(98);
        s.onSurface = N(10);
        s.surfaceVariant = NV(90);
        s.onSurfaceVariant = NV(30);
        s.surfaceDim = N(87);
        s.surfaceBright = N(98);
        s.surfaceContainerLowest = N(100);
        s.surfaceContainerLow = N(96);
        s.surfaceContainer = N(94);
        s.surfaceContainerHigh = N(92);
        s.surfaceContainerHighest = N(90);
        s.surfaceTint = P(40);

        s.outline = NV(50);
        s.outlineVariant = NV(80);

        s.inverseSurface = N(20);
        s.inverseOnSurface = N(95);
        s.inversePrimary = P(80);

        s.background = N(98);
        s.onBackground = N(10);

        s.success = ST(45.0f, 140.0f, 45.0f);
        s.onSuccess = ST(100.0f, 140.0f, 40.0f);
        s.successContainer = ST(90.0f, 140.0f, 30.0f);
        s.onSuccessContainer = ST(20.0f, 140.0f, 35.0f);

        s.warning = ST(45.0f, 80.0f, 55.0f);
        s.onWarning = ST(100.0f, 80.0f, 50.0f);
        s.warningContainer = ST(90.0f, 80.0f, 40.0f);
        s.onWarningContainer = ST(20.0f, 80.0f, 45.0f);
    } else {
        s.primary = P(80);
        s.onPrimary = P(20);
        s.primaryContainer = P(30);
        s.onPrimaryContainer = P(90);
        s.primaryFixed = P(90);
        s.primaryFixedDim = P(80);
        s.onPrimaryFixed = P(10);
        s.onPrimaryFixedVariant = P(30);

        s.secondary = S(80);
        s.onSecondary = S(20);
        s.secondaryContainer = S(30);
        s.onSecondaryContainer = S(90);

        s.tertiary = T(80);
        s.onTertiary = T(20);
        s.tertiaryContainer = T(30);
        s.onTertiaryContainer = T(90);

        s.error = E(80);
        s.onError = E(20);
        s.errorContainer = E(30);
        s.onErrorContainer = E(90);

        s.surface = N(6);
        s.onSurface = N(90);
        s.surfaceVariant = NV(30);
        s.onSurfaceVariant = NV(80);
        s.surfaceDim = N(6);
        s.surfaceBright = N(24);
        s.surfaceContainerLowest = N(4);
        s.surfaceContainerLow = N(10);
        s.surfaceContainer = N(12);
        s.surfaceContainerHigh = N(17);
        s.surfaceContainerHighest = N(22);
        s.surfaceTint = P(80);

        s.outline = NV(60);
        s.outlineVariant = NV(30);

        s.inverseSurface = N(90);
        s.inverseOnSurface = N(20);
        s.inversePrimary = P(40);

        s.background = N(6);
        s.onBackground = N(90);

        s.success = ST(80.0f, 140.0f, 40.0f);
        s.onSuccess = ST(20.0f, 140.0f, 30.0f);
        s.successContainer = ST(30.0f, 140.0f, 35.0f);
        s.onSuccessContainer = ST(90.0f, 140.0f, 25.0f);

        s.warning = ST(80.0f, 80.0f, 50.0f);
        s.onWarning = ST(20.0f, 80.0f, 40.0f);
        s.warningContainer = ST(30.0f, 80.0f, 45.0f);
        s.onWarningContainer = ST(90.0f, 80.0f, 35.0f);
    }

    s.scrim = Color(0, 0, 0, 255);
    s.shadow = Color(0, 0, 0, 255);
    return s;
}

// ---------------------------------------------------------------------------
namespace {
Scheme mixSchemeImpl(const Scheme& a, const Scheme& b, float t) {
    Scheme o;
    const auto M = [t](const Color& x, const Color& y) { return mixLab(x, y, t); };
#define RAWUI_MIX(field) o.field = M(a.field, b.field)
    RAWUI_MIX(primary);
    RAWUI_MIX(onPrimary);
    RAWUI_MIX(primaryContainer);
    RAWUI_MIX(onPrimaryContainer);
    RAWUI_MIX(primaryFixed);
    RAWUI_MIX(primaryFixedDim);
    RAWUI_MIX(onPrimaryFixed);
    RAWUI_MIX(onPrimaryFixedVariant);
    RAWUI_MIX(secondary);
    RAWUI_MIX(onSecondary);
    RAWUI_MIX(secondaryContainer);
    RAWUI_MIX(onSecondaryContainer);
    RAWUI_MIX(tertiary);
    RAWUI_MIX(onTertiary);
    RAWUI_MIX(tertiaryContainer);
    RAWUI_MIX(onTertiaryContainer);
    RAWUI_MIX(error);
    RAWUI_MIX(onError);
    RAWUI_MIX(errorContainer);
    RAWUI_MIX(onErrorContainer);
    RAWUI_MIX(surface);
    RAWUI_MIX(onSurface);
    RAWUI_MIX(surfaceVariant);
    RAWUI_MIX(onSurfaceVariant);
    RAWUI_MIX(surfaceDim);
    RAWUI_MIX(surfaceBright);
    RAWUI_MIX(surfaceContainerLowest);
    RAWUI_MIX(surfaceContainerLow);
    RAWUI_MIX(surfaceContainer);
    RAWUI_MIX(surfaceContainerHigh);
    RAWUI_MIX(surfaceContainerHighest);
    RAWUI_MIX(surfaceTint);
    RAWUI_MIX(outline);
    RAWUI_MIX(outlineVariant);
    RAWUI_MIX(inverseSurface);
    RAWUI_MIX(inverseOnSurface);
    RAWUI_MIX(inversePrimary);
    RAWUI_MIX(scrim);
    RAWUI_MIX(shadow);
    RAWUI_MIX(background);
    RAWUI_MIX(onBackground);
    RAWUI_MIX(success);
    RAWUI_MIX(onSuccess);
    RAWUI_MIX(successContainer);
    RAWUI_MIX(onSuccessContainer);
    RAWUI_MIX(warning);
    RAWUI_MIX(onWarning);
    RAWUI_MIX(warningContainer);
    RAWUI_MIX(onWarningContainer);
#undef RAWUI_MIX
    return o;
}
}  // namespace

Scheme mixScheme(const Scheme& a, const Scheme& b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return mixSchemeImpl(a, b, t);
}

// ---------------------------------------------------------------------------
const SeedPreset kSeedPresets[] = {
    {"Amethyst", 0x6750A4},
    {"Sky",      0x0B57D0},
    {"Mint",     0x146C2E},
    {"Sunset",   0xB3261E},
    {"Mango",    0xB26A00},
    {"Orchid",   0x7D5260},
    {"Cyber",    0x00639B},
    {"Lime",     0x4C6A00},
    {"Graphite", 0x5A5A66},
    {"Rose",     0xC2185B},
};
const int kSeedPresetCount = static_cast<int>(sizeof(kSeedPresets) / sizeof(kSeedPresets[0]));

}  // namespace rawui
