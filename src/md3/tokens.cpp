#include "md3/tokens.hpp"

namespace rawui {

Typescale defaultTypescale() {
    Typescale t;

    t.displayLarge = {57.0f, 64.0f, -0.25f, 400};
    t.displayMedium = {45.0f, 52.0f, 0.0f, 400};
    t.displaySmall = {36.0f, 44.0f, 0.0f, 400};

    t.headlineLarge = {32.0f, 40.0f, 0.0f, 400};
    t.headlineMedium = {28.0f, 36.0f, 0.0f, 400};
    t.headlineSmall = {24.0f, 32.0f, 0.0f, 400};

    t.titleLarge = {22.0f, 28.0f, 0.0f, 450};
    t.titleMedium = {16.0f, 24.0f, 0.15f, 500};
    t.titleSmall = {14.0f, 20.0f, 0.1f, 500};

    t.bodyLarge = {16.0f, 24.0f, 0.5f, 400};
    t.bodyMedium = {14.0f, 20.0f, 0.25f, 400};
    t.bodySmall = {12.0f, 16.0f, 0.4f, 400};

    t.labelLarge = {14.0f, 20.0f, 0.1f, 500};
    t.labelMedium = {12.0f, 16.0f, 0.5f, 500};
    t.labelSmall = {11.0f, 16.0f, 0.5f, 500};

    return t;
}

Typescale Typescale::scaled(float factor) const {
    const auto S = [factor](const TextStyle& in) {
        TextStyle o = in;
        o.size *= factor;
        o.lineHeight *= factor;
        o.letterSpacing *= factor;
        return o;
    };

    Typescale o;
    o.displayLarge = S(displayLarge);
    o.displayMedium = S(displayMedium);
    o.displaySmall = S(displaySmall);
    o.headlineLarge = S(headlineLarge);
    o.headlineMedium = S(headlineMedium);
    o.headlineSmall = S(headlineSmall);
    o.titleLarge = S(titleLarge);
    o.titleMedium = S(titleMedium);
    o.titleSmall = S(titleSmall);
    o.bodyLarge = S(bodyLarge);
    o.bodyMedium = S(bodyMedium);
    o.bodySmall = S(bodySmall);
    o.labelLarge = S(labelLarge);
    o.labelMedium = S(labelMedium);
    o.labelSmall = S(labelSmall);
    return o;
}

float displayScaleDefault() { return 1.30f; }

Elevation elevationLevel(int level) {
    switch (level) {
        case 0:
            return {0.0f, 0.0f, 0.0f, 0.0f};
        case 1:
            return {0.20f, 10.0f, 2.0f, 0.05f};
        case 2:
            return {0.22f, 14.0f, 3.0f, 0.08f};
        case 3:
            return {0.26f, 20.0f, 5.0f, 0.11f};
        case 4:
            return {0.28f, 26.0f, 7.0f, 0.12f};
        case 5:
            return {0.30f, 34.0f, 9.0f, 0.14f};
        default:
            return {0.0f, 0.0f, 0.0f, 0.0f};
    }
}

}  // namespace rawui
