#include "ui/ui.hpp"

#include <cmath>

#include "gfx/icons.hpp"

namespace rawui {
namespace ui {
namespace {

Rect scaleRect(const Rect& r, float s) {
    if (s == 1.0f) return r;
    const float w = r.w * s;
    const float h = r.h * s;
    return Rect(r.cx() - w * 0.5f, r.cy() - h * 0.5f, w, h);
}

inline float iconGlyphSize(const Typescale& t) { return t.titleLarge.size + 6.0f; }

}  // namespace

// ------------------------------------------------------------------- text --
float textWidth(Ctx& ctx, const std::string& text, const TextStyle& style) {
    Font& f = ctx.textFont(style);
    const float base = f.measure(text);
    if (style.letterSpacing == 0.0f || text.empty()) return base;

    const size_t count = utf8Decode(text).size();
    if (count < 2) return base;
    return base + style.letterSpacing * static_cast<float>(count - 1);
}

void drawText(Ctx& ctx, const std::string& text, const TextStyle& style, const Rect& box,
              const Color& color, Align align, VAlign valign) {
    if (text.empty()) return;

    Font& f = ctx.textFont(style);
    const float w = textWidth(ctx, text, style);

    float x = box.x;
    switch (align) {
        case Align::Center:
            x = box.x + (box.w - w) * 0.5f;
            break;
        case Align::Right:
            x = box.right() - w;
            break;
        case Align::Left:
            break;
    }

    const float asc = f.ascent();
    const float desc = f.descent();

    float baseline = box.cy() + (asc + desc) * 0.5f;
    switch (valign) {
        case VAlign::Top:
            baseline = box.y + asc;
            break;
        case VAlign::Bottom:
            baseline = box.bottom() + desc;
            break;
        case VAlign::Middle:
            break;
    }

    f.draw(*ctx.canvas, text, x, baseline, color, style.letterSpacing);
}

int drawParagraph(Ctx& ctx, const std::string& text, const TextStyle& style, const Rect& box,
                  const Color& color, float lineHeight) {
    Font& f = ctx.textFont(style);
    const float lh = (lineHeight > 0.0f) ? lineHeight : style.lineHeight;
    return f.drawWrapped(*ctx.canvas, text, box, color, lh, style.letterSpacing);
}

// ------------------------------------------------------------------ icons --
void drawIcon(Ctx& ctx, uint32_t codepoint, float size, const Vec2& center, const Color& color,
              bool filled) {
    Font& f = ctx.iconFont(size, filled);
    const Rect box(center.x - size * 0.5f, center.y - size * 0.5f, size, size);
    f.drawGlyphCentered(*ctx.canvas, codepoint, box, color);
}

void drawIconIn(Ctx& ctx, uint32_t codepoint, float size, const Rect& box, const Color& color,
                bool filled, Align align) {
    float cx = box.cx();
    if (align == Align::Left) cx = box.x + size * 0.5f;
    else if (align == Align::Right) cx = box.right() - size * 0.5f;
    drawIcon(ctx, codepoint, size, Vec2(cx, box.cy()), color, filled);
}

// ----------------------------------------------------------------- shapes --
void paintSurface(Ctx& ctx, const Rect& rect, float radius, int elevation, const Color& fill,
                  const Color& outline) {
    if (rect.empty()) return;

    const Scheme& c = ctx.colors();

    if (elevation > 0) {
        const Elevation e = elevationLevel(elevation);
        if (e.shadowAlpha > 0.0f) {
            const Color shadow(0, 0, 0, static_cast<uint8_t>(clampf(e.shadowAlpha, 0.0f, 1.0f) *
                                                              255.0f));
            ctx.canvas->shadowRoundRect(rect, radius, e.shadowBlur, shadow, 0.0f, e.shadowDy);
        }
    }

    if (fill.a > 0) {
        ctx.canvas->fillRoundRect(rect, radius, fill);
    }

    // In a dark theme the elevation is also expressed as a primary tint.
    if (ctx.theme->dark && elevation > 0) {
        const Elevation e = elevationLevel(elevation);
        if (e.tintAlpha > 0.0f) {
            ctx.canvas->fillRoundRect(rect, radius, c.surfaceTint.withAlphaF(e.tintAlpha));
        }
    }

    if (outline.a > 0) {
        ctx.canvas->strokeRoundRect(rect, radius, 1.0f, outline);
    }
}

void paintSquircle(Ctx& ctx, const Rect& rect, float n, int elevation, const Color& fill) {
    if (rect.empty()) return;

    const Scheme& c = ctx.colors();

    if (elevation > 0) {
        const Elevation e = elevationLevel(elevation);
        const Color shadow(0, 0, 0, static_cast<uint8_t>(clampf(e.shadowAlpha, 0.0f, 1.0f) * 255.0f));
        ctx.canvas->shadowSquircle(rect, n, e.shadowBlur, shadow, 0.0f, e.shadowDy);
    }

    if (fill.a > 0) ctx.canvas->fillSquircle(rect, n, fill);

    if (ctx.theme->dark && elevation > 0) {
        const Elevation e = elevationLevel(elevation);
        if (e.tintAlpha > 0.0f) {
            ctx.canvas->fillSquircle(rect, n, c.surfaceTint.withAlphaF(e.tintAlpha));
        }
    }
}

void stateOverlay(Ctx& ctx, const Rect& rect, float radius, const Color& color, float amount) {
    if (amount <= 0.001f) return;
    ctx.canvas->fillRoundRect(rect, radius, color.withAlphaF(clampf(amount, 0.0f, 1.0f)));
}

void focusRing(Ctx& ctx, float thickness) {
    if (!ctx.focus) return;
    const Rect r = ctx.focus->animatedRect();
    if (r.empty()) return;

    const float alpha = ctx.focus->highlightAlpha();
    const float radius = ctx.focus->animatedRadius();
    ctx.canvas->strokeRoundRect(r.outset(thickness * 0.5f + 1.0f), radius + thickness * 0.5f + 1.0f,
                                thickness, ctx.colors().primary.withAlphaF(0.95f * alpha));
}

float focusProximity(Ctx& ctx, const Rect& rect, float falloff) {
    if (!ctx.focus) return 0.0f;
    const Rect a = ctx.focus->animatedRect();
    if (a.empty()) return 0.0f;

    const float dx = std::fabs(a.cx() - rect.cx());
    const float dy = std::fabs(a.cy() - rect.cy());
    const float d = maxf(dx, dy);
    return clampf(1.0f - d / falloff, 0.0f, 1.0f);
}

// ------------------------------------------------------------------- misc --
void divider(Ctx& ctx, const Rect& rect, const Color& color) {
    ctx.canvas->fillRect(rect, color);
}

void progressBar(Ctx& ctx, const Rect& rect, float value, const Color& track,
                 const Color& indicator, bool indeterminate) {
    const float radius = rect.h * 0.5f;
    ctx.canvas->fillRoundRect(rect, radius, track);

    if (indeterminate) {
        // A wide highlight that sweeps back and forth.
        const float period = 1.6f;
        const float phase = std::fmod(ctx.time, period) / period;
        const float tri = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
        const float eased = tri * tri * (3.0f - 2.0f * tri);

        const float w = rect.w * 0.32f;
        const float x = rect.x + (rect.w - w) * eased;
        ctx.canvas->fillRoundRect(Rect(x, rect.y, w, rect.h), radius, indicator);
        return;
    }

    const float w = maxf(0.0f, rect.w * clampf(value, 0.0f, 1.0f));
    if (w > 0.5f) {
        ctx.canvas->fillRoundRect(Rect(rect.x, rect.y, maxf(w, rect.h), rect.h), radius, indicator);
    }
}

void progressRing(Ctx& ctx, const Vec2& center, float radius, float thickness, float value,
                  const Color& track, const Color& indicator, bool indeterminate) {
    ctx.canvas->strokeArc(center.x, center.y, radius, thickness, 0.0f, 360.0f, track);

    if (indeterminate) {
        const float start = std::fmod(ctx.time * 280.0f, 360.0f);
        ctx.canvas->strokeArc(center.x, center.y, radius, thickness, start, 100.0f, indicator);
        ctx.canvas->strokeArc(center.x, center.y, radius, thickness, start + 180.0f, 70.0f,
                              indicator);
        return;
    }

    const float sweep = clampf(value, 0.0f, 1.0f) * 360.0f;
    if (sweep > 0.1f) {
        ctx.canvas->strokeArc(center.x, center.y, radius, thickness, -90.0f, sweep, indicator);
    }
}

void switchControl(Ctx& ctx, const Rect& rect, float value, bool enabled) {
    const Scheme& c = ctx.colors();
    const float radius = rect.h * 0.5f;

    const Color offTrack = ctx.theme->dark ? c.surfaceContainerHighest : c.surfaceVariant;
    const Color track = lerpColor(offTrack, c.primary, value);
    ctx.canvas->fillRoundRect(rect, radius, track);

    if (value < 0.5f) {
        ctx.canvas->strokeRoundRect(rect, radius, 2.0f, c.outline);
    }

    const float thumbR = rect.h * 0.5f - 6.0f;
    const float travel = rect.w - 2.0f * (rect.h * 0.5f);
    const float cx = rect.x + rect.h * 0.5f + travel * clampf(value, 0.0f, 1.0f);
    const Color thumb = value > 0.5f ? c.onPrimary : (ctx.theme->dark ? c.outline : c.onSurfaceVariant);
    ctx.canvas->fillCircle(cx, rect.cy(), maxf(thumbR, 2.0f), enabled ? thumb : c.onSurface.withAlphaF(state::kDisabled));
}

void sliderControl(Ctx& ctx, const Rect& rect, float value, bool enabled, bool focused) {
    const Scheme& c = ctx.colors();
    const float cy = rect.cy();
    const float trackH = 6.0f;

    const float thumbR = focused ? 13.0f : 11.0f;
    const float usable = rect.w - thumbR * 2.0f;
    const float thumbX = rect.x + thumbR + usable * clampf(value, 0.0f, 1.0f);

    const Rect track(rect.x + thumbR, cy - trackH * 0.5f, usable, trackH);
    ctx.canvas->fillRoundRect(track, trackH * 0.5f, c.surfaceContainerHighest);

    const Rect active(track.x, track.y, maxf(thumbX - track.x, 0.0f), track.h);
    if (active.w > 0.5f) {
        ctx.canvas->fillRoundRect(active, trackH * 0.5f, enabled ? c.primary : c.onSurface.withAlphaF(state::kDisabled));
    }

    if (focused) {
        ctx.canvas->fillCircle(thumbX, cy, thumbR + 8.0f, c.primary.withAlphaF(0.16f));
    }
    ctx.canvas->fillCircle(thumbX, cy, thumbR, enabled ? c.primary : c.onSurface.withAlphaF(state::kDisabled));
}

void scrollIndicator(Ctx& ctx, const Rect& track, float offset, float contentHeight,
                     float viewHeight) {
    if (contentHeight <= viewHeight + 1.0f) return;

    const float ratio = clampf(viewHeight / contentHeight, 0.05f, 1.0f);
    const float thumbH = maxf(track.h * ratio, 40.0f);
    const float maxOffset = maxf(1.0f, contentHeight - viewHeight);
    const float t = clampf(offset / maxOffset, 0.0f, 1.0f);
    const float y = track.y + (track.h - thumbH) * t;

    ctx.canvas->fillRoundRect(Rect(track.x, y, track.w, thumbH), track.w * 0.5f,
                              ctx.colors().outline.withAlphaF(0.6f));
}

// ------------------------------------------------------------- components --
void button(Ctx& ctx, const Rect& rect, const std::string& label, uint32_t leadingIcon,
            ButtonStyle style, float focusAmount, bool pressed, bool enabled) {
    const Scheme& c = ctx.colors();

    Color bg(0, 0, 0, 0);
    Color fg = c.primary;
    Color outline(0, 0, 0, 0);
    int elevation = 0;

    switch (style) {
        case ButtonStyle::Filled:
            bg = c.primary;
            fg = c.onPrimary;
            break;
        case ButtonStyle::Tonal:
            bg = c.secondaryContainer;
            fg = c.onSecondaryContainer;
            break;
        case ButtonStyle::Outlined:
            outline = c.outline;
            break;
        case ButtonStyle::Text:
            break;
        case ButtonStyle::Elevated:
            bg = c.surfaceContainerLow;
            elevation = 1;
            break;
    }

    if (!enabled) {
        bg = bg.a > 0 ? c.onSurface.withAlphaF(state::kDisabledContainer) : bg;
        fg = c.onSurface.withAlphaF(state::kDisabled);
        outline = c.onSurface.withAlphaF(state::kDisabledContainer);
    }

    const float scale = 1.0f + 0.02f * focusAmount;
    const Rect r = scaleRect(rect, scale);
    const float radius = r.h * 0.5f;

    paintSurface(ctx, r, radius, elevation, bg, outline);

    if (focusAmount > 0.001f && enabled) {
        stateOverlay(ctx, r, radius, fg, state::kHover * focusAmount);
    }
    if (pressed && enabled) {
        stateOverlay(ctx, r, radius, fg, state::kPressed);
    }

    // Content: optional icon + label, centred as a group.
    const float iconSize = iconGlyphSize(ctx.type());
    const float gap = 8.0f * ctx.displayScale;
    const float textW = textWidth(ctx, label, ctx.type().labelLarge);

    const float totalW = (leadingIcon != 0 ? iconSize + gap : 0.0f) + textW;
    float x = r.x + (r.w - totalW) * 0.5f;

    if (leadingIcon != 0) {
        drawIcon(ctx, leadingIcon, iconSize, Vec2(x + iconSize * 0.5f, r.cy()), fg, false);
        x += iconSize + gap;
    }

    if (!label.empty()) {
        drawText(ctx, label, ctx.type().labelLarge,
                 Rect(x, r.y, textW + 2.0f, r.h), fg, Align::Left, VAlign::Middle);
    }
}

void iconButton(Ctx& ctx, const Rect& rect, uint32_t icon, ButtonStyle style, float focusAmount,
                bool pressed, bool selected, bool filledIcon, bool enabled) {
    const Scheme& c = ctx.colors();

    Color bg(0, 0, 0, 0);
    Color fg = c.onSurfaceVariant;
    Color outline(0, 0, 0, 0);
    int elevation = 0;

    switch (style) {
        case ButtonStyle::Filled:
            bg = selected ? c.primary : c.surfaceContainerHighest;
            fg = selected ? c.onPrimary : c.primary;
            break;
        case ButtonStyle::Tonal:
            bg = selected ? c.secondaryContainer : c.surfaceContainerHighest;
            fg = selected ? c.onSecondaryContainer : c.onSurfaceVariant;
            break;
        case ButtonStyle::Outlined:
            outline = c.outline;
            fg = c.onSurfaceVariant;
            break;
        case ButtonStyle::Elevated:
            bg = c.surfaceContainerLow;
            elevation = 1;
            fg = c.primary;
            break;
        case ButtonStyle::Text:
            fg = c.onSurfaceVariant;
            break;
    }

    if (!enabled) {
        fg = c.onSurface.withAlphaF(state::kDisabled);
        bg = bg.a > 0 ? c.onSurface.withAlphaF(state::kDisabledContainer) : bg;
    }

    const float scale = 1.0f + 0.03f * focusAmount;
    const Rect r = scaleRect(rect, scale);
    const float radius = shape::kFull;

    paintSurface(ctx, r, radius, elevation, bg, outline);

    if (focusAmount > 0.001f && enabled) {
        stateOverlay(ctx, r, radius, bg.a > 0 ? fg : c.onSurfaceVariant, state::kHover * focusAmount);
    }
    if (pressed && enabled) {
        stateOverlay(ctx, r, radius, bg.a > 0 ? fg : c.onSurfaceVariant, state::kPressed);
    }

    const float size = minf(r.w, r.h) * 0.52f;
    drawIcon(ctx, icon, size, Vec2(r.cx(), r.cy()), fg, filledIcon || selected);
}

void fab(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& label, float focusAmount,
         bool pressed) {
    const Scheme& c = ctx.colors();
    const float scale = 1.0f + 0.04f * focusAmount;
    const Rect r = scaleRect(rect, scale);
    const float n = 4.0f;  // squircle

    paintSquircle(ctx, r, n, 3, c.primaryContainer);

    if (focusAmount > 0.001f) {
        stateOverlay(ctx, r, r.h * 0.5f, c.onPrimaryContainer, state::kHover * focusAmount);
    }
    if (pressed) {
        stateOverlay(ctx, r, r.h * 0.5f, c.onPrimaryContainer, state::kPressed);
    }

    const float iconSize = minf(r.h * 0.5f, 32.0f);
    const float textW = label.empty() ? 0.0f : textWidth(ctx, label, ctx.type().titleMedium);
    const float gap = label.empty() ? 0.0f : 12.0f * ctx.displayScale;
    const float totalW = iconSize + gap + textW;
    float x = r.x + (r.w - totalW) * 0.5f;

    drawIcon(ctx, icon, iconSize, Vec2(x + iconSize * 0.5f, r.cy()), c.onPrimaryContainer, false);
    x += iconSize + gap;

    if (!label.empty()) {
        drawText(ctx, label, ctx.type().titleMedium, Rect(x, r.y, textW + 2.0f, r.h),
                 c.onPrimaryContainer, Align::Left, VAlign::Middle);
    }
}

void chip(Ctx& ctx, const Rect& rect, const std::string& label, uint32_t leadingIcon, bool selected,
          float focusAmount, bool filledIcon) {
    const Scheme& c = ctx.colors();

    const Color bg = selected ? c.secondaryContainer : Color(0, 0, 0, 0);
    const Color fg = selected ? c.onSecondaryContainer : c.onSurfaceVariant;
    const float radius = shape::kSmall;
    const float scale = 1.0f + 0.02f * focusAmount;
    const Rect r = scaleRect(rect, scale);

    paintSurface(ctx, r, radius, 0, bg, selected ? Color(0, 0, 0, 0) : c.outline);

    if (focusAmount > 0.001f) {
        stateOverlay(ctx, r, radius, fg, state::kHover * focusAmount);
    }

    const float iconSize = ctx.type().labelLarge.size + 4.0f;
    const float gap = 6.0f * ctx.displayScale;
    const float textW = textWidth(ctx, label, ctx.type().labelLarge);
    const float totalW = (leadingIcon != 0 ? iconSize + gap : 0.0f) + textW;
    float x = r.x + (r.w - totalW) * 0.5f;

    if (leadingIcon != 0) {
        drawIcon(ctx, leadingIcon, iconSize, Vec2(x + iconSize * 0.5f, r.cy()), fg, filledIcon);
        x += iconSize + gap;
    }
    drawText(ctx, label, ctx.type().labelLarge, Rect(x, r.y, textW + 2.0f, r.h), fg, Align::Left,
             VAlign::Middle);
}

void listItem(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& title,
              const std::string& supporting, const std::string& trailing, bool selected,
              float focusAmount, bool filledIcon, const Color* iconColor) {
    const Scheme& c = ctx.colors();
    const float radius = shape::kMedium;

    const Color bg = selected ? c.secondaryContainer : Color(0, 0, 0, 0);
    const Color titleColor = selected ? c.onSecondaryContainer : c.onSurface;
    const Color subColor = selected ? c.onSecondaryContainer.withAlphaF(0.75f) : c.onSurfaceVariant;

    if (bg.a > 0) ctx.canvas->fillRoundRect(rect, radius, bg);
    if (focusAmount > 0.001f) {
        stateOverlay(ctx, rect, radius, selected ? c.onSecondaryContainer : c.onSurface,
                     state::kHover * focusAmount);
    }

    const float pad = 16.0f * ctx.displayScale;
    const float iconSize = iconGlyphSize(ctx.type()) + 4.0f;
    float textX = rect.x + pad;

    if (icon != 0) {
        const Color ic = iconColor ? *iconColor
                                   : (selected ? c.onSecondaryContainer : c.onSurfaceVariant);
        drawIcon(ctx, icon, iconSize, Vec2(rect.x + pad + iconSize * 0.5f, rect.cy()), ic,
                 filledIcon || selected);
        textX = rect.x + pad + iconSize + 14.0f * ctx.displayScale;
    }

    float trailingW = 0.0f;
    if (!trailing.empty()) {
        trailingW = textWidth(ctx, trailing, ctx.type().labelMedium) + pad;
    }

    const float textW = maxf(10.0f, rect.right() - pad - trailingW - textX);

    if (supporting.empty()) {
        drawText(ctx, title, ctx.type().bodyLarge, Rect(textX, rect.y, textW, rect.h), titleColor,
                 Align::Left, VAlign::Middle);
    } else {
        const float titleH = rect.h * 0.5f;
        drawText(ctx, title, ctx.type().bodyLarge, Rect(textX, rect.y + rect.h * 0.12f, textW, titleH),
                 titleColor, Align::Left, VAlign::Middle);
        drawText(ctx, supporting, ctx.type().bodyMedium,
                 Rect(textX, rect.y + rect.h * 0.5f, textW, titleH * 0.95f), subColor, Align::Left,
                 VAlign::Middle);
    }

    if (!trailing.empty()) {
        drawText(ctx, trailing, ctx.type().labelMedium,
                 Rect(rect.right() - pad - trailingW, rect.y, trailingW, rect.h), subColor,
                 Align::Right, VAlign::Middle);
    }
}

void navRailItem(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& label, bool selected,
                 float focusAmount) {
    const Scheme& c = ctx.colors();
    const float radius = shape::kFull;

    if (selected) {
        ctx.canvas->fillRoundRect(rect, radius, c.secondaryContainer);
    }
    if (focusAmount > 0.001f) {
        stateOverlay(ctx, rect, radius, selected ? c.onSecondaryContainer : c.onSurface,
                     state::kHover * focusAmount);
    }

    const float pad = 20.0f * ctx.displayScale;
    const float iconSize = iconGlyphSize(ctx.type()) + 6.0f;
    const Color fg = selected ? c.onSecondaryContainer : c.onSurfaceVariant;

    drawIcon(ctx, icon, iconSize, Vec2(rect.x + pad + iconSize * 0.5f, rect.cy()), fg, selected);

    const float textX = rect.x + pad + iconSize + 16.0f * ctx.displayScale;
    const TextStyle& style = selected ? ctx.type().titleMedium : ctx.type().bodyLarge;
    const Color textColor = selected ? c.onSecondaryContainer : c.onSurfaceVariant;
    drawText(ctx, label, style, Rect(textX, rect.y, rect.right() - pad - textX, rect.h), textColor,
             Align::Left, VAlign::Middle);
}

void segmented(Ctx& ctx, const Rect& rect, const std::vector<std::string>& labels,
               int selectedIndex) {
    if (labels.empty()) return;

    const Scheme& c = ctx.colors();
    const float radius = rect.h * 0.5f;

    ctx.canvas->fillRoundRect(rect, radius, Color(0, 0, 0, 0));
    ctx.canvas->strokeRoundRect(rect, radius, 1.0f, c.outline);

    const float segW = rect.w / static_cast<float>(labels.size());

    for (size_t i = 0; i < labels.size(); ++i) {
        const Rect seg(rect.x + segW * static_cast<float>(i), rect.y, segW, rect.h);
        const bool selected = (static_cast<int>(i) == selectedIndex);

        if (selected) {
            ctx.canvas->fillRoundRect(seg.inset(1.0f),
                                      maxf(0.0f, radius - 1.0f), c.secondaryContainer);
        }

        const Color fg = selected ? c.onSecondaryContainer : c.onSurfaceVariant;
        drawText(ctx, labels[i], ctx.type().labelLarge, seg, fg, Align::Center, VAlign::Middle);

        if (i > 0) {
            ctx.canvas->fillRect(Rect(seg.x, seg.y + rect.h * 0.2f, 1.0f, rect.h * 0.6f),
                                 c.outlineVariant);
        }
    }
}

void statusChip(Ctx& ctx, const Rect& rect, uint32_t icon, const std::string& text,
                const Color& color, const Color& container) {
    const float radius = rect.h * 0.5f;
    ctx.canvas->fillRoundRect(rect, radius, container);

    const float pad = 10.0f * ctx.displayScale;
    const float iconSize = ctx.type().labelMedium.size + 4.0f;
    float x = rect.x + pad;

    if (icon != 0) {
        drawIcon(ctx, icon, iconSize, Vec2(x + iconSize * 0.5f, rect.cy()), color, true);
        x += iconSize + 6.0f * ctx.displayScale;
    }

    drawText(ctx, text, ctx.type().labelMedium,
             Rect(x, rect.y, rect.right() - pad - x, rect.h), color, Align::Left, VAlign::Middle);
}

// --------------------------------------------------------------- pad glyph --
void padGlyph(Ctx& ctx, PadGlyph glyph, const Vec2& center, float radius) {
    const Scheme& c = ctx.colors();
    const Color fg = c.onSurfaceVariant;

    switch (glyph) {
        case PadGlyph::Cross: {
            const float s = radius * 0.62f;
            const float t = maxf(2.0f, radius * 0.28f);
            ctx.canvas->drawLine(center.x - s, center.y - s, center.x + s, center.y + s, t, fg);
            ctx.canvas->drawLine(center.x + s, center.y - s, center.x - s, center.y + s, t, fg);
            break;
        }
        case PadGlyph::Circle:
            ctx.canvas->strokeArc(center.x, center.y, radius * 0.66f, maxf(2.0f, radius * 0.26f),
                                  0.0f, 360.0f, fg);
            break;
        case PadGlyph::Square: {
            const float s = radius * 0.62f;
            ctx.canvas->strokeRoundRect(Rect(center.x - s, center.y - s, s * 2.0f, s * 2.0f), 2.0f,
                                        maxf(2.0f, radius * 0.24f), fg);
            break;
        }
        case PadGlyph::Triangle: {
            const float s = radius * 0.72f;
            const float t = maxf(2.0f, radius * 0.24f);
            ctx.canvas->drawLine(center.x, center.y - s, center.x + s, center.y + s * 0.7f, t, fg);
            ctx.canvas->drawLine(center.x + s, center.y + s * 0.7f, center.x - s, center.y + s * 0.7f,
                                 t, fg);
            ctx.canvas->drawLine(center.x - s, center.y + s * 0.7f, center.x, center.y - s, t, fg);
            break;
        }
        case PadGlyph::L1:
        case PadGlyph::R1:
        case PadGlyph::Options: {
            const char* label = (glyph == PadGlyph::L1) ? "L1"
                                : (glyph == PadGlyph::R1) ? "R1"
                                                          : "OPT";
            const Rect box(center.x - radius, center.y - radius * 0.72f, radius * 2.0f,
                           radius * 1.44f);
            ctx.canvas->strokeRoundRect(box, 6.0f, 2.0f, fg);
            drawText(ctx, label, ctx.type().labelSmall, box, fg, Align::Center, VAlign::Middle);
            break;
        }
        case PadGlyph::Dpad: {
            const float s = radius * 0.5f;
            const float t = maxf(3.0f, radius * 0.3f);
            ctx.canvas->drawLine(center.x - s, center.y, center.x + s, center.y, t, fg);
            ctx.canvas->drawLine(center.x, center.y - s, center.x, center.y + s, t, fg);
            break;
        }
    }
}

void hintBar(Ctx& ctx, const Rect& rect, const std::vector<Hint>& hints) {
    if (hints.empty()) return;

    const float glyphR = rect.h * 0.32f;
    const float gapAfterGlyph = 8.0f * ctx.displayScale;
    const float gapBetween = 26.0f * ctx.displayScale;

    // Measure first so the group can be centred.
    float total = 0.0f;
    std::vector<float> widths(hints.size(), 0.0f);
    for (size_t i = 0; i < hints.size(); ++i) {
        widths[i] = textWidth(ctx, hints[i].label, ctx.type().labelMedium);
        total += glyphR * 2.0f + gapAfterGlyph + widths[i];
        if (i + 1 < hints.size()) total += gapBetween;
    }

    float x = rect.cx() - total * 0.5f;
    for (size_t i = 0; i < hints.size(); ++i) {
        padGlyph(ctx, hints[i].glyph, Vec2(x + glyphR, rect.cy()), glyphR);
        x += glyphR * 2.0f + gapAfterGlyph;
        drawText(ctx, hints[i].label, ctx.type().labelMedium,
                 Rect(x, rect.y, widths[i] + 2.0f, rect.h), ctx.colors().onSurfaceVariant,
                 Align::Left, VAlign::Middle);
        x += widths[i] + gapBetween;
    }
}

void fieldReadout(Ctx& ctx, const Rect& rect, const std::string& label, const std::string& value,
                  bool focused) {
    const Scheme& c = ctx.colors();
    const float radius = shape::kExtraSmall;

    ctx.canvas->fillRoundRect(rect, radius, c.surfaceContainerHighest);
    ctx.canvas->strokeRoundRect(rect, radius, focused ? 2.0f : 1.0f,
                                focused ? c.primary : c.outline);

    const float pad = 14.0f * ctx.displayScale;
    const TextStyle& lbl = ctx.type().labelSmall;
    const TextStyle& val = ctx.type().bodyLarge;

    drawText(ctx, label, lbl, Rect(rect.x + pad, rect.y + rect.h * 0.10f, rect.w - pad * 2.0f,
                                   rect.h * 0.34f),
             c.onSurfaceVariant, Align::Left, VAlign::Middle);
    drawText(ctx, value, val, Rect(rect.x + pad, rect.y + rect.h * 0.42f, rect.w - pad * 2.0f,
                                   rect.h * 0.46f),
             c.onSurface, Align::Left, VAlign::Middle);
}

}  // namespace ui
}  // namespace rawui
