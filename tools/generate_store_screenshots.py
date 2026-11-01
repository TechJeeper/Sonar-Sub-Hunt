#!/usr/bin/env python3
"""Render Rebble App Store screenshots at exact platform resolutions.

Mirrors Sonar Sub Hunt watch UI (no watch bezel — store requirement).
"""
from __future__ import annotations

import math
import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "store" / "screenshots"
RES = ROOT / "resources" / "images"

PLATFORMS = {
    "basalt": (144, 168, True),   # Pebble Time — color
    "diorite": (144, 168, False),  # Pebble 2 — B&W
    "flint": (144, 168, False),    # Pebble 2 Duo — B&W
    "emery": (200, 228, True),    # Pebble Time 2 — color
}

# Pebble 64-color approximations (matches ui.c)
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
LIGHT_GRAY = (170, 170, 170)
DUKE_BLUE = (0, 85, 170)
CYAN = (0, 255, 255)
BRIGHT_GREEN = (0, 255, 0)
RED = (255, 0, 0)
YELLOW = (255, 255, 0)


def font(size: int) -> ImageFont.ImageFont:
    candidates = [
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def font_bold(size: int) -> ImageFont.ImageFont:
    candidates = [
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return font(size)


def text_center(draw, text, y, w, fnt, fill):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text(((w - tw) // 2, y), text, font=fnt, fill=fill)


def paste_sprite(base: Image.Image, path: Path, center: tuple[int, int], color: bool):
    if not path.is_file():
        return
    spr = Image.open(path).convert("RGBA")
    if not color:
        # Flatten to white-on-transparent for B&W watches
        px = spr.load()
        for y in range(spr.height):
            for x in range(spr.width):
                r, g, b, a = px[x, y]
                if a < 32:
                    continue
                lum = (r + g + b) // 3
                if lum < 20:
                    px[x, y] = (0, 0, 0, 0)
                else:
                    px[x, y] = (255, 255, 255, a)
    x = center[0] - spr.width // 2
    y = center[1] - spr.height // 2
    base.paste(spr, (x, y), spr)


def radar_geometry(w: int, h: int):
    # Matches ui.c: radar GRect(0, 18, w, h-50), center offset +6
    radar_h = h - 50
    cx = w // 2
    cy = 18 + radar_h // 2 + 6
    radius = w // 2 - 6
    if radar_h // 2 - 18 < radius:
        radius = radar_h // 2 - 18
    return cx, cy, max(radius, 20)


def draw_radar(img: Image.Image, draw: ImageDraw.ImageDraw, w: int, h: int,
               color: bool, strength: int, bearing_deg: int, show_sub: bool):
    cx, cy, radius = radar_geometry(w, h)
    water = DUKE_BLUE if color else BLACK
    ring = CYAN if color else WHITE
    sweep = BRIGHT_GREEN if color else WHITE
    lock = RED if color else WHITE

    draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), fill=water)
    draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=ring)
    r2 = radius * 2 // 3
    r1 = radius // 3
    draw.ellipse((cx - r2, cy - r2, cx + r2, cy + r2), outline=ring)
    draw.ellipse((cx - r1, cy - r1, cx + r1, cy + r1), outline=ring)
    draw.line((cx - radius, cy, cx + radius, cy), fill=ring, width=1)
    draw.line((cx, cy - radius, cx, cy + radius), fill=ring, width=1)

    if strength > 0:
        sr = r1 + (radius * 2 // 3) * (100 - strength) // 100
        col = lock if strength >= 85 else sweep
        width = 3 if strength >= 70 else 1
        draw.ellipse((cx - sr, cy - sr, cx + sr, cy + sr), outline=col, width=width)

    if show_sub and strength > 0:
        reach = radius // 3 if strength >= 85 else radius - 8
        rad = math.radians(bearing_deg)
        nx = int(cx + math.sin(rad) * reach)
        ny = int(cy - math.cos(rad) * reach)
        paste_sprite(img, RES / "sub.png", (nx, ny), color)

    if strength >= 85:
        draw.ellipse((cx - 18, cy - 18, cx + 18, cy + 18), outline=lock, width=2)

    paste_sprite(img, RES / "reticle.png", (cx, cy), color)


def render_title(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f18 = font_bold(max(14, w // 10))
    f14 = font(max(11, w // 13))
    dim = LIGHT_GRAY if color else WHITE

    title = RES / "title.png"
    if title.is_file():
        spr = Image.open(title).convert("RGBA")
        tw = min(spr.width, w - 8)
        th = spr.height
        if spr.width > tw:
            ratio = tw / spr.width
            th = max(1, int(spr.height * ratio))
            spr = spr.resize((tw, th), Image.NEAREST)
        if not color:
            px = spr.load()
            for y in range(spr.height):
                for x in range(spr.width):
                    r, g, b, a = px[x, y]
                    if a < 32:
                        continue
                    lum = (r + g + b) // 3
                    px[x, y] = (255, 255, 255, a) if lum >= 20 else (0, 0, 0, 0)
        x = (w - spr.width) // 2
        img.paste(spr, (x, 8), spr)
        text_y = 8 + spr.height + 2
    else:
        text_y = 36
        text_center(draw, "SONAR", 28, w, font_bold(max(20, w // 7)), WHITE)
        text_center(draw, "SUB HUNT", 56, w, font_bold(max(20, w // 7)), WHITE)

    text_center(draw, "SONAR SUB HUNT", text_y, w, f18, WHITE)
    text_center(draw, "Sweep arm to hunt", h - 58, w, f14, dim)
    text_center(draw, "SELECT to start", h - 36, w, f18, WHITE)
    return img


def render_hunting(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f14 = font_bold(max(11, w // 13))
    f14r = font(max(11, w // 13))
    draw.text((4, 0), "W1  120pts", font=f14, fill=WHITE)
    bbox = draw.textbbox((0, 0), "T:6", font=f14)
    draw.text((w - 4 - (bbox[2] - bbox[0]), 0), "T:6", font=f14, fill=WHITE)
    draw_radar(img, draw, w, h, color, strength=55, bearing_deg=40, show_sub=True)
    text_center(draw, "Hdg 218  El +12", h - 40, w, f14r, WHITE)
    return img


def render_lock(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f14 = font_bold(max(11, w // 13))
    f18 = font_bold(max(14, w // 10))
    lock = RED if color else WHITE
    draw.text((4, 0), "W2  480pts", font=f14, fill=WHITE)
    bbox = draw.textbbox((0, 0), "T:5", font=f14)
    draw.text((w - 4 - (bbox[2] - bbox[0]), 0), "T:5", font=f14, fill=WHITE)
    draw_radar(img, draw, w, h, color, strength=92, bearing_deg=15, show_sub=True)
    text_center(draw, "LOCK — FIRE!", h - 42, w, f18, lock)
    return img


def render_hit(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f14 = font_bold(max(11, w // 13))
    f18 = font_bold(max(14, w // 10))
    draw.text((4, 0), "W2  680pts", font=f14, fill=WHITE)
    bbox = draw.textbbox((0, 0), "T:4", font=f14)
    draw.text((w - 4 - (bbox[2] - bbox[0]), 0), "T:4", font=f14, fill=WHITE)
    paste_sprite(img, RES / "sub.png", (w // 2, h // 2 - 8), color)
    text_center(draw, "HIT! +200", h - 42, w, f18, WHITE)
    return img


def render_wave_clear(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f14 = font_bold(max(11, w // 13))
    f14r = font(max(11, w // 13))
    f18 = font_bold(max(14, w // 10))
    dim = LIGHT_GRAY if color else WHITE
    draw.text((4, 0), "W3  1240pts", font=f14, fill=WHITE)
    bbox = draw.textbbox((0, 0), "T:3", font=f14)
    draw.text((w - 4 - (bbox[2] - bbox[0]), 0), "T:3", font=f14, fill=WHITE)
    draw_radar(img, draw, w, h, color, strength=0, bearing_deg=0, show_sub=False)
    text_center(draw, "WAVE CLEAR", h - 42, w, f18, WHITE)
    text_center(draw, "SELECT: next wave", h - 22, w, f14r, dim)
    return img


SCENES = [
    ("01-title.png", render_title),
    ("02-hunting.png", render_hunting),
    ("03-lock.png", render_lock),
    ("04-hit.png", render_hit),
    ("05-wave-clear.png", render_wave_clear),
]


def main() -> None:
    for plat, (w, h, color) in PLATFORMS.items():
        dest = OUT / plat
        dest.mkdir(parents=True, exist_ok=True)
        for filename, fn in SCENES:
            img = fn(w, h, color)
            path = dest / filename
            img.save(path, "PNG", optimize=True)
            print(f"wrote {path.relative_to(ROOT)} ({w}x{h})")
    print("Done:", OUT)


if __name__ == "__main__":
    main()
