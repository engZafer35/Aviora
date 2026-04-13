#!/usr/bin/env python3
"""Generate GIFs for Network Service documentation (Pillow required).

Run from repo root or this directory:
  python docs/network/generate_network_docs_gifs.py

Outputs four GIFs under docs/network/media/ (compact size for 2x2 layout in docs).
"""

from __future__ import annotations

from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError as e:
    raise SystemExit("Install Pillow: pip install pillow") from e

OUT = Path(__file__).resolve().parent / "media"
FRAMES = 28
# Compact canvas for sidebar docs (2 GIFs per row)
W, H = 500, 128


def font(size: int = 14):
    for name in ("consola.ttf", "consolas.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def save_gif(name: str, draw_fn):
    fnt = font(13)
    fnt_s = font(11)
    frames = []
    for i in range(FRAMES):
        im = Image.new("RGB", (W, H), (14, 16, 22))
        dr = ImageDraw.Draw(im)
        draw_fn(dr, fnt, fnt_s, i)
        frames.append(im)
    out = OUT / name
    OUT.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        out,
        save_all=True,
        append_images=frames[1:],
        duration=90,
        loop=0,
        optimize=True,
    )
    print("Wrote", out)


def main():
    # 1) Generator pipeline
    def gen_pipeline(dr, fnt, fs, i):
        pulse = 95 + (i % 10) * 12
        dr.text((12, 8), "generate_network_service.py --customer X", fill=(180, 200, 255), font=fnt)
        dr.rounded_rectangle([12, 32, 200, 78], radius=6, outline=(80, 140, 220), width=2)
        dr.text((22, 44), "network_config.json", fill=(150, 190, 255), font=fs)
        dr.text((22, 60), "Customers/X/Network/", fill=(120, 150, 190), font=fs)
        dr.line([208, 54, 248, 54], fill=(pulse, pulse, 255), width=2)
        dr.polygon([(254, 54), (246, 50), (246, 58)], fill=(pulse, pulse, 255))
        dr.rounded_rectangle([258, 30, 488, 80], radius=6, outline=(200, 160, 80), width=2)
        dr.text((268, 38), "Emits .h / .c + root bridge", fill=(255, 210, 150), font=fs)
        dr.text((268, 56), "First stack with use:true wins", fill=(200, 180, 140), font=fs)
        dr.text((12, 90), "Outputs: Cus_Network_Config.* | NetworkService_Config.h", fill=(140, 170, 200), font=fs)
        dr.text((12, 108), "Auto-generated — edit JSON, re-run script.", fill=(120, 130, 150), font=fs)

    save_gif("network_generator_pipeline.gif", gen_pipeline)

    # 2) Runtime startup
    def runtime(dr, fnt, fs, i):
        step = (i // 7) % 4
        dr.text((12, 8), "Runtime: AppNetworkService", fill=(180, 200, 255), font=fnt)
        dr.rounded_rectangle([12, 30, 210, 72], radius=6, outline=(100, 180, 120), width=2)
        dr.text((28, 44), "appNetworkServiceStart()", fill=(160, 230, 180), font=fs)
        dr.line([218, 50, 268, 50], fill=(120, 200, 255), width=2)
        dr.rounded_rectangle([274, 28, 488, 74], radius=6, outline=(80, 140, 220), width=2)
        dr.text((282, 36), "networkServiceInitTCPIPStack()", fill=(180, 210, 255), font=fs)
        dr.text((282, 54), "NET_STACK_STACK_INIT_FUNCTION()", fill=(140, 170, 210), font=fs)
        dr.text((12, 82), "~1s delay then start interfaces", fill=(140, 160, 180), font=fs)
        eth_on = step >= 2
        ppp_on = step >= 3
        dr.rounded_rectangle([12, 96, 488, 122], radius=6, outline=(200, 140, 100), width=2)
        dr.text(
            (22, 102),
            "ETH* start "
            + ("OK" if eth_on else "..")
            + "   |   PPP* start "
            + ("OK" if ppp_on else ".."),
            fill=(200, 230, 200) if (eth_on or ppp_on) else (120, 130, 140),
            font=fs,
        )

    save_gif("network_runtime_startup.gif", runtime)

    # 3) Include chain (compact horizontal)
    def include_chain(dr, fnt, fs, i):
        t = i / max(FRAMES - 1, 1)
        dr.text((12, 6), "Include chain (build)", fill=(180, 200, 255), font=fnt)
        boxes = [
            (12, 28, 158, 88, "AppNetworkService.c", "NetworkService_Config.h"),
            (170, 28, 328, 88, "NetworkService_Config.h", "Cus_Network_Config.h"),
            (338, 28, 488, 88, "Cus_Network_Config.h", "stack + if macros"),
        ]
        for j, (x1, y1, x2, y2, title, sub) in enumerate(boxes):
            alpha = min(1.0, max(0.0, (t * 3 - j * 0.35)))
            outline = (int(80 + 90 * alpha), int(130 + 50 * alpha), 220)
            dr.rounded_rectangle([x1, y1, x2, y2], radius=6, outline=outline, width=2)
            dr.text((x1 + 8, y1 + 10), title[:22], fill=(200, 220, 255), font=fs)
            dr.text((x1 + 8, y1 + 28), sub[:24], fill=(140, 170, 200), font=fs)
        for j in range(2):
            x = 158 + j * 168
            dr.line([x, 56, x + 10, 56], fill=(180, 180, 255), width=2)
            dr.polygon([(x + 14, 56), (x + 6, 52), (x + 6, 60)], fill=(180, 180, 255))
        dr.text((12, 98), "Application pulls customer config via Customers/ bridge header.", fill=(120, 130, 150), font=fs)

    save_gif("network_include_chain.gif", include_chain)

    # 4) Stack + interface selection rules
    def selection(dr, fnt, fs, i):
        phase = (i // 9) % 3
        dr.text((12, 6), "Selection rules", fill=(180, 200, 255), font=fnt)
        dr.text((12, 28), "networkStack[]", fill=(150, 190, 255), font=fs)
        rows = [
            (12, 44, "Stack A  use:false  (skipped)"),
            (12, 60, "Stack B  use:false  (skipped)"),
            (12, 76, "Stack C  use:true   (ACTIVE)"),
        ]
        for idx, (x, y, label) in enumerate(rows):
            hl = (idx == 2) and (phase >= 0)
            col = (120, 200, 150) if (idx == 2 and phase == 2) else ((100, 100, 110) if idx < 2 else (200, 230, 200))
            if idx == 2 and i % 12 < 6:
                col = (255, 220, 150)
            dr.text((x, y), label, fill=col, font=fs)
        dr.text((12, 96), "Then: ethInterfaces[] / gsmInterfaces[] with use:true only.", fill=(140, 170, 200), font=fs)
        dr.text((12, 112), "Order: all active ETH first, then all active PPP.", fill=(120, 130, 150), font=fs)

    save_gif("network_selection_rules.gif", selection)


if __name__ == "__main__":
    main()
