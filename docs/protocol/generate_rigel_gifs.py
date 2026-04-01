#!/usr/bin/env python3
"""GIFs for RigelMq MQTT JSON protocol docs (Pillow)."""

from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit("pip install pillow")

OUT = Path(__file__).resolve().parent / "media"
FRAMES = 40
W, H = 800, 260


def font(sz=14):
    try:
        return ImageFont.truetype("segoeui.ttf", sz)
    except OSError:
        try:
            return ImageFont.truetype("arial.ttf", sz)
        except OSError:
            return ImageFont.load_default()


def save_gif(name, draw_fn, frames=FRAMES, size=(W, H)):
    w, h = size
    fnt, fs = font(16), font(13)
    frames_im = []
    for i in range(frames):
        im = Image.new("RGB", (w, h), (14, 16, 22))
        dr = ImageDraw.Draw(im)
        draw_fn(dr, fnt, fs, i, w, h)
        frames_im.append(im)
    p = OUT / name
    frames_im[0].save(
        p,
        save_all=True,
        append_images=frames_im[1:],
        duration=85,
        loop=0,
        optimize=True,
    )
    print("Wrote", p)


def main():
    OUT.mkdir(parents=True, exist_ok=True)

    def mqtt_topics(dr, fnt, fs, i, Wn, Hn):
        phase = (i // (FRAMES // 4)) % 4
        t = i / max(FRAMES - 1, 1)
        dr.text((16, 8), "MQTT topics: who publishes where", fill=(210, 215, 230), font=fnt)
        bx, by, bw, bh = Wn // 2 - 58, 42, 116, 52
        dr.rounded_rectangle([bx, by, bx + bw, by + bh], radius=10, outline=(100, 140, 220), width=2)
        br_fill = (35, 45, 70) if phase in (0, 1) else (28, 32, 42)
        dr.rounded_rectangle([bx + 4, by + 4, bx + bw - 4, by + bh - 4], radius=8, fill=br_fill)
        dr.text((bx + 28, by + 16), "Broker", fill=(170, 195, 255), font=fs)
        # Gateway left
        gw_fill = (28, 42, 34) if phase == 2 else (22, 28, 24)
        dr.rounded_rectangle([36, 118, 200, 198], radius=8, outline=(80, 160, 120), width=2, fill=gw_fill)
        dr.text((52, 126), "Gateway", fill=(190, 235, 205), font=fnt)
        dr.text((44, 152), "Subscribes avi/request", fill=(150, 190, 165), font=fs)
        dr.text((44, 170), "Publishes avi/response", fill=(150, 190, 165), font=fs)
        # Backend right
        be_fill = (38, 30, 48) if phase == 3 else (26, 22, 32)
        dr.rounded_rectangle([Wn - 200, 118, Wn - 36, 198], radius=8, outline=(160, 120, 200), width=2, fill=be_fill)
        dr.text((Wn - 188, 126), "Backend / tool", fill=(230, 210, 255), font=fnt)
        dr.text((Wn - 192, 152), "Publishes avi/request", fill=(190, 170, 210), font=fs)
        dr.text((Wn - 192, 170), "Subscribes avi/response", fill=(190, 170, 210), font=fs)
        # Animated packets
        ax = 210 + t * (Wn // 2 - 230)
        dr.polygon([(ax, 148), (ax + 20, 138), (ax + 20, 158)], fill=(230, 200, 90))
        dr.text((ax - 8, 122), "cmd", fill=(200, 180, 100), font=fs)
        ax2 = Wn - 210 - t * (Wn // 2 - 230)
        dr.polygon([(ax2, 168), (ax2 - 20, 158), (ax2 - 20, 178)], fill=(90, 190, 230))
        dr.text((ax2 - 18, 182), "reply", fill=(120, 190, 220), font=fs)
        hints = [
            "Downlink: commands travel to the gateway on avi/request.",
            "Uplink: gateway sends ident, ack, chunks, logs on avi/response.",
            "Same broker; two topics separate command vs telemetry paths.",
            "QoS 0 is typical; use QoS 1 if you need each publish stored at the broker.",
        ]
        dr.text((16, Hn - 36), hints[phase], fill=(130, 138, 155), font=fs)

    def session_flow(dr, fnt, fs, i, Wn, Hn):
        dr.text(
            (16, 8),
            "One transNumber = one logical operation (session)",
            fill=(210, 215, 230),
            font=fnt,
        )
        labels = [
            "MQTT JSON in",
            "Parse function + transNumber",
            "Find or open session",
            "Continue job / reply",
        ]
        n = len(labels)
        frames_per = max(1, FRAMES // n)
        active = min(n - 1, i // frames_per)
        box_w = (Wn - 80) // n
        x0 = 36
        y0 = 52
        bh = 44
        xs = []
        for j, lab in enumerate(labels):
            x = x0 + j * (box_w + 8)
            xs.append(x)
            done = j < active
            cur = j == active
            if cur:
                fill, outline = (42, 58, 42), (100, 180, 110)
            elif done:
                fill, outline = (32, 42, 36), (70, 110, 80)
            else:
                fill, outline = (28, 30, 36), (55, 62, 75)
            dr.rounded_rectangle([x, y0, x + box_w, y0 + bh], radius=8, fill=fill, outline=outline, width=2)
            dr.text((x + 8, y0 + 13), lab, fill=(210, 218, 225), font=fs)
        mid = y0 + bh // 2
        for j in range(n - 1):
            x_end = xs[j] + box_w + 1
            x_next = xs[j + 1] - 1
            dr.line([x_end, mid, x_next - 6, mid], fill=(90, 110, 140), width=2)
            dr.polygon([(x_next, mid), (x_next - 7, mid - 5), (x_next - 7, mid + 5)], fill=(90, 110, 140))
        dr.text(
            (16, y0 + bh + 18),
            "Matching transNumber ties a later ack or data chunk to the same request — even if several jobs overlap.",
            fill=(130, 138, 155),
            font=fs,
        )

    def readout_seq(dr, fnt, fs, i, Wn, Hn):
        dr.text(
            (16, 6),
            "Readout: request → ack → meter work → result chunks (same transNumber)",
            fill=(210, 215, 230),
            font=fnt,
        )
        steps = [
            ("Step 1", "Backend publishes readout on avi/request"),
            ("Step 2", "Gateway answers ack on avi/response"),
            ("Step 3", "Gateway runs the meter job"),
            ("Step 4", "Gateway streams readout JSON chunks on avi/response"),
        ]
        n = len(steps)
        frames_per = max(1, FRAMES // n)
        active = min(n - 1, i // frames_per)
        y_base = 44
        row_h = 46
        for j, (tag, desc) in enumerate(steps):
            y = y_base + j * row_h
            done = j < active
            cur = j == active
            if cur:
                fill, outline = (48, 85, 58), (120, 200, 140)
                tag_col = (220, 255, 230)
            elif done:
                fill, outline = (38, 55, 42), (85, 130, 95)
                tag_col = (170, 210, 185)
            else:
                fill, outline = (30, 32, 38), (55, 60, 72)
                tag_col = (130, 135, 145)
            dr.rounded_rectangle([36, y, Wn - 36, y + 38], radius=8, fill=fill, outline=outline, width=2)
            dr.text((48, y + 10), tag, fill=tag_col, font=fs)
            dr.text((130, y + 10), desc, fill=(200, 205, 215) if (cur or done) else (110, 115, 125), font=fs)
            if done and not cur:
                dr.text((Wn - 52, y + 10), "OK", fill=(100, 160, 120), font=fs)
        dr.text(
            (16, Hn - 32),
            "Chunks reuse transNumber; packetNum rises until packetStream is false.",
            fill=(120, 128, 145),
            font=fs,
        )

    def json_dispatch(dr, fnt, fs, i, Wn, Hn):
        dr.text(
            (16, 8),
            "Incoming message: valid JSON, then function + transNumber",
            fill=(210, 215, 230),
            font=fnt,
        )
        checks = ["Valid UTF-8 JSON", "function field present", "transNumber usable", "Dispatch to handler"]
        n = len(checks)
        frames_per = max(1, FRAMES // n)
        active = min(n - 1, i // frames_per)
        y0 = 46
        for j, lab in enumerate(checks):
            y = y0 + j * 30
            ok = j < active
            on = j == active
            if on:
                bg, fg = (45, 40, 58), (230, 220, 250)
            elif ok:
                bg, fg = (32, 40, 35), (150, 190, 160)
            else:
                bg, fg = (26, 28, 34), (110, 115, 125)
            dr.rounded_rectangle([40, y, Wn - 40, y + 24], radius=6, fill=bg, outline=(70, 75, 90), width=1)
            dr.text((52, y + 4), lab, fill=fg, font=fs)
            if ok and not on:
                dr.text((Wn - 72, y + 4), "pass", fill=(100, 160, 120), font=fs)
        sample = (
            '{"device":{"flag":"AVI","serialNumber":"…"},"function":"readout","transNumber":1001,…}'
        )
        sample_y = y0 + n * 30 + 10
        show = min(len(sample), 52 + int((i / max(FRAMES - 1, 1)) * (len(sample) - 52)))
        dr.text((40, sample_y), sample[: max(52, show)], fill=(200, 190, 235), font=fs)
        dr.text(
            (40, Hn - 36),
            "Bad JSON or missing fields → nack on avi/response (same correlation rules).",
            fill=(190, 140, 120),
            font=fs,
        )

    save_gif("rigel_mqtt_topics.gif", mqtt_topics)
    save_gif("rigel_session_flow.gif", session_flow)
    save_gif("rigel_readout_sequence.gif", readout_seq)
    save_gif("rigel_json_dispatch.gif", json_dispatch)


if __name__ == "__main__":
    main()
