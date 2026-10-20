// Dumps generated schemes as stable text so the C++ engine can be diffed
// against the JavaScript port in docs/themes.html.
//
// Run directly, or via tests/port_sync.sh which does the comparison.
#include <cstdio>

#include "md3/color.hpp"

namespace {

// Field order is the contract with tests/scheme_dump.js - keep them in sync.
using Role = rawui::Color rawui::Scheme::*;

const Role kRoles[] = {
    &rawui::Scheme::primary,         &rawui::Scheme::onPrimary,
    &rawui::Scheme::primaryContainer, &rawui::Scheme::onPrimaryContainer,
    &rawui::Scheme::surface,         &rawui::Scheme::onSurface,
    &rawui::Scheme::error,           &rawui::Scheme::success,
    &rawui::Scheme::warning,         &rawui::Scheme::outline,
    &rawui::Scheme::tertiary,        &rawui::Scheme::secondary,
    &rawui::Scheme::surfaceVariant,  &rawui::Scheme::inversePrimary,
    &rawui::Scheme::surfaceContainer, &rawui::Scheme::onSurfaceVariant,
};
constexpr int kRoleCount = static_cast<int>(sizeof(kRoles) / sizeof(kRoles[0]));

const uint32_t kSeeds[] = {0x6750A4, 0x00497F, 0x006C4D, 0x7A3045, 0x7C5800,
                           0x4C3B6F, 0x5A5A66, 0x0B57D0, 0x00639B};
const float kContrasts[] = {-0.3f, 0.0f, 0.1f, 0.2f, 0.3f};

}  // namespace

int main() {
    using namespace rawui;

    for (uint32_t seed : kSeeds) {
        for (int dark = 0; dark <= 1; ++dark) {
            for (float contrast : kContrasts) {
                for (int vibrant = 0; vibrant <= 1; ++vibrant) {
                    const Scheme s = makeScheme(Color::fromHex(seed), dark != 0,
                                                SchemeOptions{contrast, vibrant != 0, true});

                    std::printf("%06X|%d|%.1f|%d", seed, dark, contrast, vibrant);
                    for (int i = 0; i < kRoleCount; ++i) {
                        const Color& c = s.*(kRoles[i]);
                        std::printf("|%02X%02X%02X", c.r, c.g, c.b);
                    }
                    std::printf("\n");
                }
            }
        }
    }
    return 0;
}
