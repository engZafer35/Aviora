#!/usr/bin/env python3
"""Generate GIFs for App MQTT Connection Service documentation (Pillow required)."""

from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit("Install Pillow: pip install pillow")

OUT = Path(__file__).resolve().parent / "media"
FRAMES = 28
W, H = 760, 220


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
    fnt_s = font(12)
    frames = []
    for i in range(FRAMES):
        im = Image.new("RGB", (W, H), (14, 16, 22))
        dr = ImageDraw.Draw(im)
        draw_fn(dr, fnt, fnt_s, i)
        frames.append(im)
    out = OUT / name
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
    OUT.mkdir(parents=True, exist_ok=True)

    def poll_loop(dr, fnt, fs, i):
        phase = (i // 7) % 4
        dr.text((20, 12), "MQTT service task: loop + mqttClientTask (poll)", fill=(180, 200, 255), font=fnt)
        # Broker
        dr.rounded_rectangle([30, 50, 150, 110], radius=8, outline=(100, 180, 120), width=2)
        dr.text((48, 72), "Broker", fill=(160, 230, 180), font=fs)
        # Arrow blink
        pulse = 120 + (i % 8) * 12
        dr.line([160, 78, 240, 78], fill=(pulse, pulse, 255), width=3)
        dr.polygon([(245, 78), (235, 72), (235, 84)], fill=(pulse, pulse, 255))
        # Task box
        dr.rounded_rectangle([250, 45, 420, 115], radius=8, outline=(80, 140, 220), width=2)
        dr.text((265, 52), "Service task", fill=(150, 190, 255), font=fs)
        labels = ["connect", "SUBSCRIBE", "process()", "sleep 100ms"]
        dr.text((265, 72), labels[phase], fill=(220, 230, 255), font=fnt)
        dr.text((265, 92), "mqttClientTask -> PUBLISH / PING", fill=(140, 170, 210), font=fs)
        # App callback
        dr.rounded_rectangle([440, 50, 720, 110], radius=8, outline=(200, 140, 80), width=2)
        dr.text((455, 58), "Application", fill=(255, 200, 150), font=fs)
        if phase == 2:
            dr.text((455, 78), "IncomingCb(topic, data, len)", fill=(255, 230, 200), font=fs)
        else:
            dr.text((455, 78), "LinkCb(1) / LinkCb(0)", fill=(200, 180, 150), font=fs)
        dr.text((20, 165), "CycloneTCP: no separate MQTT thread; process() must be called periodically.", fill=(120, 130, 150), font=fs)

    def reconnect_flow(dr, fnt, fs, i):
        t = i / (FRAMES - 1)
        dr.text((20, 10), "Drop -> CONNECTION_FAIL -> reconnect", fill=(255, 180, 140), font=fnt)
        x = 80 + t * (W - 280)
        dr.rounded_rectangle([40, 55, 200, 115], radius=8, outline=(180, 80, 80), width=2)
        dr.text((75, 78), "TCP/MQTT", fill=(255, 150, 150), font=fs)
        dr.rounded_rectangle([x, 60, x + 140, 110], radius=6, fill=(60, 50, 90))
        dr.text((x + 15, 78), "loss", fill=(255, 255, 255), font=fs)
        dr.rounded_rectangle([W - 200, 55, W - 40, 115], radius=8, outline=(80, 180, 120), width=2)
        dr.text((W - 175, 78), "taskConnect", fill=(150, 255, 180), font=fs)
        dr.text((20, 165), "process() error -> link down notify -> retry after 2s.", fill=(130, 140, 160), font=fs)

    save_gif("mqtt_service_poll.gif", poll_loop)
    save_gif("mqtt_service_reconnect.gif", reconnect_flow)


if __name__ == "__main__":
    main()
