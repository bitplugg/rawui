// ============================================================================
//  RAW UI - Material Design 3 component library.
//
//  Everything here is a painter: it takes a rectangle and a small amount of
//  state and draws.  Screens own the state, which keeps components free of
//  hidden animation caches and makes them easy to reuse across screens.
// ============================================================================
#pragma once

#include <string>
#include <vector>

#include "core/math.hpp"
#include "gfx/canvas.hpp"
#include "gfx/fontcache.hpp"
#include "md3/theme.hpp"
#include "ui/focus.hpp"

namespace rawui {
namespace ui {

enum class Align { Left, Center, Right };
enum class VAlign { Top, Middle, Bottom };

// Everything a component needs to draw itself.
struct Ctx {
    Canvas* canvas = nullptr;
    FontCache* fonts = nullptr;
    Theme* theme = nullptr;
    FocusManager* focus = nullptr;

    float time = 0.0f;
    float dt = 0.0f;
    float displayScale = 1.0f;
    int screenW = 1920;
    int screenH = 1080;

    Font& textFont(const TextStyle& style) { return fonts->text(style.size, style.weight); }
    Font& iconFont(float size, bool filled) { return fonts->icon(size, filled); }
    const Scheme& colors() const { return theme->scheme; }
    const Typescale& type() const { return theme->type; }
};

// ------------------------------------------------------------------- text --
float textWidth(Ctx& ctx, const std::string& text, const TextStyle& style);

void drawText(Ctx& ctx, const std::string& text, const TextStyle& style, const Rect& box,
              const Color& color, Align align = Align::Left,
              VAlign valign = VAlign::Middle);

// Draws wrapped text and returns the number of lines.
int drawParagraph(Ctx& ctx, const std::string& text, const TextStyle& style, const Rect& box,
                  const Color& color, float lineHeight = 0.0f);

// ------------------------------------------------------------------ icons --
void drawIcon(Ctx& ctx, uint32_t codepoint, float size, const Vec2& center, const Color& color,
              bool filled = false);

void drawIconIn(Ctx& ctx, uint32_t codepoint, float size, const Rect& box, const Color& color,
                bool filled = false, Align align = Align::Center);

// ----------------------------------------------------------------- shapes --
// Rounded rectangle with an elevation shadow and an optional outline.
void paintSurface(Ctx& ctx, const Rect& rect, float radius, int elevation, const Color& fill,
                  const Color& outline = Color(0, 0, 0, 0));

// Superellipse surface (the M3 Expressive "cookie" shape).
void paintSquircle(Ctx& ctx, const Rect& rect, float n, int elevation, const Color& fill);

// Translucent overlay used for hover/focus/press feedback.
void stateOverlay(Ctx& ctx, const Rect& rect, float radius, const Color& color, float amount);

// The sliding focus indicator.
void focusRing(Ctx& ctx, float thickness = 3.0f);

// How much the sliding highlight currently sits on `rect` (0..1).  Used for
// subtle scale/tint effects on nearby items.
float focusProximity(Ctx& ctx, const Rect& rect, float falloff = 90.0f);

// ------------------------------------------------------------------- misc --
void divider(Ctx& ctx, const Rect& rect, const Color& color);

void progressBar(Ctx& ctx, const Rect& rect, float value, const Color& track,
                 const Color& indicator, bool indeterminate);

void progressRing(Ctx& ctx, const Vec2& center, float radius, float thickness, float value,
                  const Color& track, const Color& indicator, bool indeterminate);

void switchControl(Ctx& ctx, const Rect& rect, float value, bool enabled);

void sliderControl(Ctx& ctx, const Rect& rect, float value, bool enabled, bool focused);

void scrollIndicator(Ctx& ctx, const Rect& track, float offset, float contentHeight,
                     float viewHeight);

// ------------------------------------------------------------- components --
enum class ButtonStyle { Filled, Tonal, Outlined, Text, Elevated };

void button(Ctx& ctx, const Rect& rect, const std::string& label, uint32_t leadingIcon,
            ButtonStyle style, float focusAmount, bool pressed, bool enabled = true);

void iconButton(Ctx& ctx, const Rect& rect, uint32_t icon, ButtonStyle style, float focusAmount,
                bool pressed, bool selected = false, bool filledIcon = false,
                bool enabled = true);

void fab(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& label,
         float focusAmount, bool pressed);

void chip(Ctx& ctx, const Rect& rect, const std::string& label, uint32_t leadingIcon,
          bool selected, float focusAmount, bool filledIcon = false);

// List row: leading icon, title, optional supporting text and trailing text.
void listItem(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& title,
              const std::string& supporting, const std::string& trailing, bool selected,
              float focusAmount, bool filledIcon = false, const Color* iconColor = nullptr);

// Navigation rail destination.
void navRailItem(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& label,
                 bool selected, float focusAmount);

// Segmented button row.  `selectedIndex` may be -1.
void segmented(Ctx& ctx, const Rect& rect, const std::vector<std::string>& labels,
               int selectedIndex);

// Small status pill (used in the header).
void statusChip(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& text,
                const Color& color, const Color& container);

// Controller button glyph: Cross, Circle, Square, Triangle, L1, R1, Options.
enum class PadGlyph { Cross, Circle, Square, Triangle, L1, R1, Options, Dpad };
void padGlyph(Ctx& ctx, PadGlyph glyph, const Vec2& center, float radius);

// A hint line such as  [X] Select   [O] Back.
struct Hint {
    PadGlyph glyph;
    std::string label;
};
void hintBar(Ctx& ctx, const Rect& rect, const std::vector<Hint>& hints);

// Text field look-alike (display only - there is no keyboard on a console UI).
void fieldReadout(Ctx& ctx, const Rect& rect, const std::string& label, const std::string& value,
                  bool focused);

}  // namespace ui
}  // namespace rawui
