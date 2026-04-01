#!/usr/bin/env python3
"""Generate simple packet-flow GIFs for OrionTLV documentation (Pillow required)."""

from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit("Install Pillow: pip install pillow")

OUT = Path(__file__).resolve().parent / "media"
FRAMES = 60
W, H = 720, 200


def font(size=16):
    try:
        return ImageFont.truetype("consola.ttf", size)
    except OSError:
        try:
            return ImageFont.truetype("arial.ttf", size)
        except OSError:
            return ImageFont.load_default()


def save_gif(name, draw_fn):
    fnt = font(15)
    fnt_small = font(13)
    frames = []
    for i in range(FRAMES):
        im = Image.new("RGB", (W, H), (18, 18, 24))
        dr = ImageDraw.Draw(im)
        draw_fn(dr, fnt, fnt_small, i)
        frames.append(im)
    out = OUT / name
    frames[0].save(
        out,
        save_all=True,
        append_images=frames[1:],
        duration=80,
        loop=0,
        optimize=True,
    )
    print("Wrote", out)


def main():
    OUT.mkdir(parents=True, exist_ok=True)

    def ident_flow(dr, fnt, fs, i):
        t = i / (FRAMES - 1)
        x = 50 + t * (W - 200)
        dr.rectangle([20, 70, 200, 130], outline=(80, 120, 200), width=2)
        dr.text((45, 45), "Cihaz (Gateway)", fill=(180, 200, 255), font=fnt)
        dr.text((55, 95), "IDENT", fill=(200, 220, 255), font=fs)
        dr.rectangle([W - 220, 70, W - 40, 130], outline=(120, 180, 120), width=2)
        dr.text((W - 195, 45), "Sunucu", fill=(180, 255, 200), font=fnt)
        dr.text((W - 175, 95), "IDENT reply", fill=(200, 255, 220), font=fs)
        pkt = [x, 88, x + 100, 112]
        dr.rounded_rectangle(pkt, radius=6, fill=(40, 90, 140))
        dr.text((x + 8, 96), "$ TLV... #", fill=(255, 255, 255), font=fs)
        dr.text((20, 155), "Push TCP: cihaz sunucuya baglanir, IDENT gonderilir.", fill=(150, 150, 170), font=fs)

    def pull_flow(dr, fnt, fs, i):
        t = i / (FRAMES - 1)
        x = W - 240 - t * (W - 280)
        dr.rectangle([20, 70, 200, 130], outline=(80, 120, 200), width=2)
        dr.text((45, 45), "Sunucu (Test)", fill=(180, 200, 255), font=fnt)
        dr.text((55, 95), "Pull client", fill=(200, 220, 255), font=fs)
        dr.rectangle([W - 220, 70, W - 40, 130], outline=(120, 180, 120), width=2)
        dr.text((W - 195, 45), "Cihaz", fill=(180, 255, 200), font=fnt)
        dr.text((W - 175, 95), "Pull dinleyici", fill=(200, 255, 220), font=fs)
        pkt = [x, 88, x + 110, 112]
        dr.rounded_rectangle(pkt, radius=6, fill=(90, 60, 120))
        dr.text((x + 6, 96), "Komut $...#", fill=(255, 240, 255), font=fs)
        dr.text((20, 155), "Pull TCP: sunucu cihazin pull portuna baglanir.", fill=(150, 150, 170), font=fs)

    def push_data_flow(dr, fnt, fs, i):
        t = (i % (FRAMES // 2)) / (FRAMES // 2 - 1)
        x = 50 + t * (W - 280)
        dr.rectangle([20, 55, 200, 145], outline=(80, 120, 200), width=2)
        dr.text((40, 28), "Cihaz", fill=(180, 200, 255), font=fnt)
        dr.text((45, 100), "READOUT/\nLOADPROFILE\nchunk", fill=(200, 220, 255), font=fs)
        dr.rectangle([W - 200, 55, W - 30, 145], outline=(120, 180, 120), width=2)
        dr.text((W - 175, 28), "Sunucu", fill=(180, 255, 200), font=fnt)
        dr.text((W - 165, 100), "ACK\n(ayni transNo)", fill=(200, 255, 220), font=fs)
        pkt = [x, 78, x + 95, 102]
        dr.rounded_rectangle(pkt, radius=6, fill=(40, 100, 80))
        dr.text((x + 10, 84), "data", fill=(220, 255, 230), font=fs)
        if i > FRAMES // 2:
            ax = W - 320 - t * 120
            dr.rounded_rectangle([ax, 118, ax + 55, 138], radius=4, fill=(60, 80, 120))
            dr.text((ax + 8, 122), "ACK", fill=(255, 255, 255), font=fs)
        dr.text((20, 168), "Ayni transactionNumber ile gelen ACK mevcut oturuma yonlenir.", fill=(140, 140, 165), font=fs)

    def dispatch_flow(dr, fnt, fs, i):
        dr.text((W // 2 - 180, 12), "dispatchIncomingMessage: once transNo ile session ara", fill=(220, 220, 240), font=fnt)
        y = 55 + (i % 8) * 2
        boxes = [
            ("RX kuyruk", 40, y, 160, y + 50),
            ("parse TLV", 200, y, 340, y + 50),
            ("session?", 380, y, 520, y + 50),
            ("process", 540, y, 680, y + 50),
        ]
        for label, x1, y1, x2, y2 in boxes:
            dr.rounded_rectangle([x1, y1, x2, y2], radius=8, outline=(100, 140, 200), width=2)
            dr.text((x1 + 12, y1 + 14), label, fill=(200, 210, 230), font=fs)
        if i > FRAMES // 3:
            dr.line([200, y + 25, 380, y + 25], fill=(80, 200, 120), width=3)
        if i > 2 * FRAMES // 3:
            dr.line([520, y + 25, 540, y + 25], fill=(200, 120, 80), width=3)
        dr.text((40, 165), "Yeni paket: transNo aktif session ile eslesirse rxReady + processSessionMessage", fill=(130, 130, 155), font=fs)

    save_gif("flow_ident.gif", ident_flow)
    save_gif("flow_pull.gif", pull_flow)
    save_gif("flow_push_data.gif", push_data_flow)
    save_gif("flow_dispatch.gif", dispatch_flow)


if __name__ == "__main__":
    main()
