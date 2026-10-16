#!/usr/bin/env python3
"""
Generates the package artwork for RAW UI:

    pkg/sce_sys/icon0.png   512x512    dashboard tile icon
    pkg/sce_sys/pic1.png    1920x1080  background picture behind the tile

Pure standard library (zlib + struct), no Pillow needed. Shapes are drawn from
signed distance fields so the edges are anti-aliased.
"""

import math
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "pkg", "sce_sys")

# Material Design 3 baseline-ish palette (seed #6750A4).
PRIMARY = (0x67, 0x50, 0xA4)
SECONDARY = (0x62, 0x5B, 0x71)
TERTIARY = (0x7D, 0x52, 0x60)
PRIMARY_CONTAINER = (0xE9, 0xDD, 0xFF)
ON_PRIMARY = (0xFF, 0xFF, 0xFF)


# --------------------------------------------------------------------------- #
#  tiny raster helpers
# --------------------------------------------------------------------------- #
def new_buffer(w, h, rgba=(0, 0, 0, 0)):
    return bytearray(bytes(rgba) * (w * h))


def blend_pixel(buf, w, x, y, color, coverage):
    """Source-over composite of `color` with `coverage` in 0..1."""
    if coverage <= 0.0:
        return
    if coverage > 1.0:
        coverage = 1.0
    i = (y * w + x) * 4
    sr, sg, sb = color[0], color[1], color[2]
    sa = color[3] / 255.0 * coverage
    da = buf[i + 3] / 255.0
    oa = sa + da * (1.0 - sa)
    if oa <= 0.0:
        return
    for c, s in enumerate((sr, sg, sb)):
        d = buf[i + c] / 255.0
        buf[i + c] = int(round(((s / 255.0 * sa) + d * da * (1.0 - sa)) / oa * 255.0))
    buf[i + 3] = int(round(oa * 255.0))


def sdf_rounded_rect(px, py, cx, cy, hw, hh, r):
    qx = abs(px - cx) - (hw - r)
    qy = abs(py - cy) - (hh - r)
    ax = qx if qx > 0.0 else 0.0
    ay = qy if qy > 0.0 else 0.0
    outside = math.sqrt(ax * ax + ay * ay)
    inside = min(max(qx, qy), 0.0)
    return outside + inside - r


def sdf_circle(px, py, cx, cy, r):
    return math.hypot(px - cx, py - cy) - r


def draw_sdf(buf, w, h, sdf, color, aa=1.0, inside_only=False):
    """Rasterise a shape given a signed distance function."""
    for y in range(h):
        py = y + 0.5
        for x in range(w):
            d = sdf(x + 0.5, py)
            cov = 0.5 - d / aa
            if cov <= 0.0 and not inside_only:
                continue
            blend_pixel(buf, w, x, y, color, cov)


def lerp(a, b, t):
    return a + (b - a) * t


def lerp_rgb(a, b, t):
    return (int(lerp(a[0], b[0], t)), int(lerp(a[1], b[1], t)), int(lerp(a[2], b[2], t)))


# --------------------------------------------------------------------------- #
#  artwork
# --------------------------------------------------------------------------- #
def draw_background(buf, w, h, radius):
    """Diagonal gradient squircle, clipped by the rounded-square mask."""
    for y in range(h):
        for x in range(w):
            d = sdf_rounded_rect(x + 0.5, y + 0.5, w / 2, h / 2, w / 2, h / 2, radius)
            cov = 0.5 - d
            if cov <= 0.0:
                continue
            # gradient from top-left (primary) to bottom-right (tertiary)
            t = (x / w * 0.45 + y / h * 0.55)
            base = lerp_rgb(PRIMARY, TERTIARY, t)
            # soft top-left highlight for a touch of MD3 "expressive" depth
            hl = max(0.0, 1.0 - math.hypot(x / w, y / h) * 1.9)
            base = lerp_rgb(base, (255, 255, 255), hl * 0.18)
            blend_pixel(buf, w, x, y, base + (255,), cov)


def draw_dashboard_motif(buf, w, h, alpha=0.96):
    """A 2x2 launcher grid: one wide tile plus small ones."""
    white = (255, 255, 255, int(255 * alpha))
    pad = w * 0.20
    gap = w * 0.055
    left = pad
    right = w - pad
    top = h * 0.22
    bottom = h - h * 0.20
    inner_w = right - left
    col_w = (inner_w - gap) / 2.0
    row_h = (bottom - top - gap) / 2.0
    r = w * 0.055

    tiles = [
        (left, top, col_w, row_h, white),
        (left + col_w + gap, top, col_w, row_h, (255, 255, 255, int(255 * alpha * 0.62))),
        (left, top + row_h + gap, col_w, row_h, (255, 255, 255, int(255 * alpha * 0.62))),
        (left + col_w + gap, top + row_h + gap, col_w, row_h, white),
    ]
    for (tx, ty, tw, th, color) in tiles:
        cx, cy = tx + tw / 2.0, ty + th / 2.0
        draw_sdf(buf, w, h,
                 lambda px, py, cx=cx, cy=cy, hw=tw / 2.0, hh=th / 2.0:
                 sdf_rounded_rect(px, py, cx, cy, hw, hh, r),
                 color)


def write_png(path, w, h, buf):
    raw = bytearray()
    stride = w * 4
    for y in range(h):
        raw.append(0)  # filter type 0
        raw += buf[y * stride:(y + 1) * stride]

    def chunk(tag, data):
        out = struct.pack(">I", len(data)) + tag + data
        return out + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", header)
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    with open(path, "wb") as fh:
        fh.write(png)
    print("wrote %s (%dx%d, %d KiB)" % (os.path.relpath(path, ROOT), w, h, len(png) // 1024))


def make_icon():
    size = 512
    buf = new_buffer(size, size)
    draw_background(buf, size, size, radius=size * 0.235)
    draw_dashboard_motif(buf, size, size, alpha=0.97)
    write_png(os.path.join(OUT, "icon0.png"), size, size, buf)


def make_pic1():
    w, h = 1920, 1080
    buf = new_buffer(w, h)
    # vertical gradient background: deep surface -> primary tinted
    top = (0x14, 0x11, 0x1B)
    bottom = (0x2B, 0x22, 0x45)
    for y in range(h):
        row = lerp_rgb(top, bottom, y / h)
        base = bytes(row + (255,)) * w
        buf[y * w * 4:(y + 1) * w * 4] = base

    # large soft blurred blobs (cheap: radial falloff, additive-ish blend)
    blobs = [(0.22, 0.30, 0.42, PRIMARY, 0.55),
             (0.78, 0.22, 0.30, TERTIARY, 0.40),
             (0.62, 0.82, 0.36, (0x4F, 0x37, 0x8B), 0.45)]
    for (bx, by, br, color, strength) in blobs:
        cx, cy, rad = bx * w, by * h, br * w
        for y in range(h):
            py = y + 0.5
            for x in range(w):
                d = math.hypot(x + 0.5 - cx, py - cy)
                if d >= rad:
                    continue
                f = (1.0 - d / rad) ** 2.4
                blend_pixel(buf, w, x, y, color + (255,), f * strength)

    # decorative squircle outline (MD3 expressive shape language)
    cx, cy = w * 0.5, h * 0.5
    hw, hh, r = w * 0.30, h * 0.40, w * 0.10
    draw_sdf(buf, w, h,
             lambda px, py: abs(sdf_rounded_rect(px, py, cx, cy, hw, hh, r)) - 3.0,
             PRIMARY_CONTAINER + (255,), aa=1.6)
    write_png(os.path.join(OUT, "pic1.png"), w, h, buf)


def main():
    os.makedirs(OUT, exist_ok=True)
    make_icon()
    make_pic1()


if __name__ == "__main__":
    main()
