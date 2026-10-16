// ============================================================================
//  RAW UI - Material Design 3 colour.
//
//  Material You generates tonal palettes in HCT (CAM16 + L*).  Doing CAM16 on
//  the console is overkill, so the same idea is expressed in CIELAB/LCh: a seed
//  colour gives a hue and a chroma budget, and every tone is produced by asking
//  for a target lightness at the largest chroma that still fits in sRGB.
//  The result is perceptually even lightness and stays inside gamut.
// ============================================================================
#pragma once

#include <cstdint>

#include "core/math.hpp"

namespace rawui {

struct Lab {
    float l = 0.0f;
    float a = 0.0f;
    float b = 0.0f;
};

struct LCh {
    float l = 0.0f;
    float c = 0.0f;
    float h = 0.0f;  // degrees
};

Lab rgbToLab(const Color& c);
Color labToRgb(const Lab& lab);

LCh labToLCh(const Lab& lab);
Lab lchToLab(const LCh& lch);

// Largest chroma at (lightness, hue) that still converts to a legal sRGB value.
float maxChromaInGamut(float lightness, float hueDegrees);

// Material Design 3 colour roles.  Names follow the spec so it is easy to look
// a token up in the documentation.
struct Scheme {
    Color primary, onPrimary, primaryContainer, onPrimaryContainer;
    Color primaryFixed, primaryFixedDim, onPrimaryFixed, onPrimaryFixedVariant;

    Color secondary, onSecondary, secondaryContainer, onSecondaryContainer;
    Color tertiary, onTertiary, tertiaryContainer, onTertiaryContainer;

    Color error, onError, errorContainer, onErrorContainer;

    Color surface, onSurface, surfaceVariant, onSurfaceVariant;
    Color surfaceDim, surfaceBright;
    Color surfaceContainerLowest, surfaceContainerLow, surfaceContainer;
    Color surfaceContainerHigh, surfaceContainerHighest;
    Color surfaceTint;

    Color outline, outlineVariant;
    Color inverseSurface, inverseOnSurface, inversePrimary;
    Color scrim, shadow;
    Color background, onBackground;

    // Extras used by RAW UI for status readouts (not part of the spec).
    Color success, onSuccess, successContainer, onSuccessContainer;
    Color warning, onWarning, warningContainer, onWarningContainer;
};

struct SchemeOptions {
    float contrast = 0.0f;      // -1 .. 1, 0 = spec
    bool  vibrant = false;      // push the primary chroma harder
    bool  fromSeed = true;      // false -> use the built-in baseline palettes
};

// Generates a full scheme from a seed colour.
Scheme makeScheme(const Color& seed, bool dark, const SchemeOptions& options = {});

// A few hand-picked seeds for the theme picker in Settings.
struct SeedPreset {
    const char* name;
    uint32_t rgb;
};
extern const SeedPreset kSeedPresets[];
extern const int kSeedPresetCount;

// Blends `a` towards `b` in Lab space - used for smooth theme transitions.
Color mixLab(const Color& a, const Color& b, float t);

// Blends two complete schemes (animating a theme change).
Scheme mixScheme(const Scheme& a, const Scheme& b, float t);

}  // namespace rawui
