from PIL import Image, ImageDraw, ImageFont
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(SCRIPT_DIR, "..", "assets", "gifs")
OUT_DIR = os.path.normpath(OUT_DIR)
os.makedirs(OUT_DIR, exist_ok=True)

W, H = 600, 360  # Mantıklı ölçüde, büyük olmasın
BG = (255, 255, 255)
BLACK = (15, 15, 15)

FONT_PATHS = [
    r"C:/Windows/Fonts/arial.ttf",
    r"C:/Windows/Fonts/tahoma.ttf",
    r"C:/Windows/Fonts/seguisym.ttf",
]


def get_font(size=18):
    for p in FONT_PATHS:
        try:
            return ImageFont.truetype(p, size=size)
        except Exception:
            pass
    return ImageFont.load_default()


F_REG = get_font(20)
F_SMALL = get_font(16)
F_TINY = get_font(13)


def wrap(text: str, max_chars: int):
    lines = []
    for raw in (text or "").split("\n"):
        if raw == "":
            lines.append("")
            continue
        while len(raw) > max_chars:
            cut = raw.rfind(" ", 0, max_chars + 1)
            if cut <= 0:
                cut = max_chars
            lines.append(raw[:cut])
            raw = raw[cut:].lstrip()
        lines.append(raw)
    return lines


def draw_arrow(draw, x1, y1, x2, y2, color=(70, 70, 70), width=4, head=18):
    draw.line((x1, y1, x2, y2), fill=color, width=width)
    import math

    ang = math.atan2(y2 - y1, x2 - x1)
    hx1 = x2 - head * math.cos(ang) + head * 0.45 * math.sin(ang)
    hy1 = y2 - head * math.sin(ang) - head * 0.45 * math.cos(ang)
    hx2 = x2 - head * math.cos(ang) - head * 0.45 * math.sin(ang)
    hy2 = y2 - head * math.sin(ang) + head * 0.45 * math.cos(ang)
    draw.polygon([(x2, y2), (hx1, hy1), (hx2, hy2)], fill=color)


def box(draw, x, y, w, h, tag, title, fill=(242, 242, 242), outline=(40, 40, 40), highlight=False):
    linew = 5 if highlight else 2
    rad = min(14, w // 8, h // 6)
    if highlight:
        draw.rounded_rectangle((x - 4, y - 4, x + w + 4, y + h + 4), radius=rad + 2, fill=(255, 245, 200),
                               outline=(255, 190, 0), width=2)
    draw.rounded_rectangle((x, y, x + w, y + h), radius=rad, fill=fill, outline=outline, width=linew)
    title_lines = wrap(title.replace("\\n", "\n"), 22)[:3]
    yy = y + 10
    draw.text((x + 10, yy), tag, font=get_font(20), fill=BLACK)
    yy = y + 45
    for ln in title_lines:
        draw.text((x + 10, yy), ln, font=F_TINY, fill=(60, 60, 60))
        yy += 18


def make_gif(path, frames, duration=700):
    frames = [f.convert("P", palette=Image.Palette.ADAPTIVE, colors=256) for f in frames]
    frames[0].save(
        path,
        save_all=True,
        append_images=frames[1:],
        duration=duration,
        loop=0,
        optimize=True,
    )


def frame_include(step):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    d.text((20, 12), "Include Zinciri (Otomatik Oluşturulan)", font=get_font(20), fill=BLACK)

    boxes_specs = [
        (30, 55, 150, 90, "A", "fs_port.h"),
        (200, 55, 180, 90, "B", "FS_Port_Config.h"),
        (395, 55, 195, 90, "C", "cus_fs_port_config.h"),
    ]

    for i, (x, y, bw, bh, tag, txt) in enumerate(boxes_specs):
        highlight = i == step
        fill = (255, 240, 200) if highlight else (240, 246, 255)
        box(d, x, y, bw, bh, tag, txt, fill=fill, outline=(0, 90, 180) if highlight else (40, 40, 40), highlight=highlight)

    d.text((20, 155), "fs_port.h -> FS_Port_Config -> cus_fs_port_config -> fs_port_*.h", font=F_TINY, fill=(70, 70, 70))
    draw_arrow(d, 180, 100, 200, 100, color=(70, 70, 70))
    draw_arrow(d, 380, 100, 395, 100, color=(70, 70, 70))

    bottom = [
        (30, 195, 170, 95, "D", "fs_port_flashLink\\n(littlefs/flashFS)"),
        (210, 195, 180, 95, "E", "FS_HARDWARE_INIT\\n+ fsInit"),
        (400, 195, 170, 95, "F", "fs* API hazır"),
    ]
    for j, (x, y, bw, bh, tag, txt) in enumerate(bottom):
        highlight = (step == 2 and j == 0) or (step == 1 and j == 1) or (step == 0 and j == 2)
        fill = (255, 240, 200) if highlight else (245, 245, 245)
        box(d, x, y, bw, bh, tag, txt, fill=fill, outline=(0, 90, 180) if highlight else (40, 40, 40), highlight=highlight)

    d.text((20, 300), "Uygulama sadece fs_port.h include eder; geri kalan otomatik.", font=F_TINY, fill=(60, 60, 60))
    return img


def frame_generator(step):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    d.text((20, 12), "generate_fs_port_config.py pipeline", font=get_font(20), fill=BLACK)

    nodes = [
        (30, 55, 165, 95, "1", "Input JSON\\nfs_config.json"),
        (210, 55, 175, 95, "2", "activeFsName\\nfsLib block"),
        (400, 55, 170, 95, "3", "Makrolar +\\nFS_HARDWARE_INIT"),
        (210, 175, 175, 95, "4", "cus_fs_port_config.h"),
        (30, 175, 165, 95, "5", "FS_Port_Config.h"),
        (400, 175, 170, 95, "6", "fs_port.h include"),
    ]

    for i, (x, y, bw, bh, tag, txt) in enumerate(nodes):
        highlight = i == step
        fill = (255, 240, 200) if highlight else (240, 246, 255)
        box(d, x, y, bw, bh, tag, txt, fill=fill, outline=(0, 90, 180) if highlight else (40, 40, 40), highlight=highlight)

    draw_arrow(d, 195, 102, 210, 102, color=(90, 90, 90))
    draw_arrow(d, 385, 102, 400, 102, color=(90, 90, 90))
    draw_arrow(d, 297, 150, 297, 175, color=(90, 90, 90))
    draw_arrow(d, 117, 150, 117, 175, color=(90, 90, 90))
    draw_arrow(d, 485, 150, 485, 175, color=(90, 90, 90))

    footer = [
        "1: JSON oku -> activeFsName",
        "2: USE_* makro seç",
        "3: geometry/cells/HW callback makroları",
        "4: fs_port_*.h include",
        "5: FS_Port_Config.h üret",
        "6: Sabit fs* API",
    ][min(step, 5)]
    d.text((20, 285), footer, font=F_TINY, fill=(60, 60, 60))
    return img


def frame_flashlink(step):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    d.text((20, 10), "FlashLink: bağlı hücre depolama", font=get_font(18), fill=BLACK)

    base_x, base_y = 40, 95
    cell_w, cell_h, gap = 82, 50, 14

    states_sets = [
        ["empty"] * 6,
        ["head"] + ["empty"] * 5,
        ["head", "mid"] + ["empty"] * 4,
        ["head", "mid", "mid"] + ["empty"] * 3,
        ["empty"] * 6,
    ]
    states = states_sets[min(step, 4)]
    colors = {"empty": (245, 245, 245), "head": (255, 220, 120), "mid": (255, 200, 90)}

    for i in range(6):
        sx = base_x + i * (cell_w + gap)
        sy = base_y
        fill = colors.get(states[i], (240, 240, 240))
        d.rounded_rectangle((sx, sy, sx + cell_w, sy + cell_h), radius=10, fill=fill, outline=(30, 30, 30), width=2)
        d.text((sx + cell_w // 2 - 6, sy + cell_h // 2 - 10), str(i + 1), font=get_font(14), fill=BLACK)

    last = max([j for j, s in enumerate(states) if s in ("head", "mid")], default=-1)
    for i in range(5):
        x1 = base_x + i * (cell_w + gap) + cell_w
        y1 = base_y + cell_h / 2
        x2 = base_x + (i + 1) * (cell_w + gap)
        col = (180, 70, 70) if i < last else (70, 70, 70)
        draw_arrow(d, x1, y1, x2, y1, color=col, width=3, head=10)

    info = ["0) Boş hücreler 0xFF", "1) Dosya oluştur -> head hücre", "2) Yaz -> zincir ekle", "3) Oku -> nextIndex=0'a kadar", "4) Sil -> tombstone"][min(step, 4)]
    d.text((20, 260), info, font=F_TINY, fill=(40, 40, 40))

    d.text((380, 85), "Hücre: name, nextIdx, dataLen", font=get_font(12), fill=BLACK)
    for i, ln in enumerate(["- name: 64B", "- nextIndex: ~val", "- dataLen: ~val"]):
        d.text((380, 105 + i * 18), ln, font=F_TINY, fill=(60, 60, 60))
    return img


def frame_flashfs(step):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    d.text((20, 10), "FlashFS: append-only log", font=get_font(18), fill=BLACK)

    super_x, super_y = 40, 85
    log_x, log_y = 230, 75

    super_fill = (255, 220, 120) if step >= 1 else (245, 245, 245)
    d.rounded_rectangle((super_x, super_y, super_x + 180, super_y + 55), radius=12, fill=super_fill, outline=(40, 40, 40), width=2)
    d.text((super_x + 14, super_y + 16), "Superblock", font=get_font(14), fill=BLACK)
    d.text((super_x + 14, super_y + 36), "magic/version/geom", font=F_TINY, fill=(60, 60, 60))

    d.rounded_rectangle((log_x, log_y, log_x + 350, log_y + 195), radius=14, fill=(245, 250, 255), outline=(40, 40, 40), width=2)
    d.text((log_x + 14, log_y + 10), "Append-only log", font=get_font(14), fill=BLACK)

    record_labels = ["FORMAT", "FILE+data", "DELETE", "RENAME"]
    show_count = [1, 2, 3, 3, 4][min(step, 4)]
    slot_y = log_y + 45
    for i in range(4):
        y = slot_y + i * 38
        active = i < show_count
        fill = (255, 240, 200) if active else (235, 235, 235)
        d.rounded_rectangle((log_x + 14, y, log_x + 336, y + 30), radius=8, fill=fill, outline=(40, 40, 40), width=2)
        d.text((log_x + 24, y + 8), record_labels[i], font=get_font(12), fill=BLACK)

    hx = log_x + 50 + int([0.0, 0.35, 0.65, 1.0, 1.35][min(step, 4)] * 240)
    hy = slot_y
    d.polygon([(hx, hy - 8), (hx - 10, hy + 4), (hx + 10, hy + 4)], fill=(200, 70, 70))

    expl = ["0) configure + fsInit", "1) flashFsFormat", "2) FILE + data append", "3) DELETE append", "4) RENAME, mount rebuilds"][min(step, 4)]
    d.text((20, 285), expl, font=F_TINY, fill=(40, 40, 40))
    d.text((400, 55), "GC yok, dolunca format", font=F_TINY, fill=(60, 60, 60))
    return img


def frame_generator_flashlink(step):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    d.text((20, 10), "generate_fs_port_config.py --customer ZD_2622", font=get_font(16), fill=BLACK)

    d.rounded_rectangle((20, 45, 270, 305), radius=12, fill=(245, 245, 245), outline=(40, 40, 40), width=2)
    d.text((35, 60), "Konsol", font=get_font(14), fill=BLACK)
    console_lines = [
        "SELECTED FS: flashlink",
        "USE_FLASHLINK=1",
        "Driver + callbacks",
        "cus_fs_port_config.h",
        "FS_Port_Config.h",
    ]
    for i, ln in enumerate(console_lines):
        col = (0, 100, 200) if i == step else (70, 70, 70)
        d.text((40, 95 + i * 38), ln, font=F_TINY, fill=col)

    d.rounded_rectangle((295, 45, 585, 305), radius=12, fill=(255, 255, 255), outline=(40, 40, 40), width=2)
    d.text((310, 60), "cus_fs_port_config.h", font=get_font(12), fill=BLACK)

    snippets = [
        ("#define USE_FLASHLINK 1", "activeFsName -> macro"),
        ('#include "fs_port_flashLink.h"', "USE_FLASHLINK -> header"),
        ('#include "Driver/...McuInternal...h"', "flashHwFunc.drvSrcPath"),
        ("STORAGE_DRV_FUNC_READ internal...", "read/prog/erase/sync"),
        ("FS_HARDWARE_INIT(error_out)...", "flashLinkInit+Format"),
    ]
    a, b = snippets[min(step, 4)]
    d.rounded_rectangle((310, 95, 570, 175), radius=8, fill=(255, 240, 200), outline=(255, 180, 0), width=2)
    d.text((320, 105), a, font=get_font(11), fill=BLACK)
    d.text((320, 135), b, font=F_TINY, fill=(60, 60, 60))

    d.text((20, 325), "Derleme: fs_port.h -> sabit fs* API", font=F_TINY, fill=(60, 60, 60))
    return img


def frame_generator_littlefs(step):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    d.text((20, 10), "activeFsName='littlefs' (simülasyon)", font=get_font(16), fill=BLACK)

    d.rounded_rectangle((20, 45, 270, 305), radius=12, fill=(245, 245, 245), outline=(40, 40, 40), width=2)
    d.text((35, 60), "Konsol", font=get_font(14), fill=BLACK)
    console_lines = [
        "SELECTED FS: littlefs",
        "USE_LITTLEFS=1",
        "LITTLEFS_GEOMETRY_*",
        "fs_littlefs_helper_func.h",
        "FS_HARDWARE_INIT (init+mutex)",
    ]
    for i, ln in enumerate(console_lines):
        col = (0, 100, 200) if i == step else (70, 70, 70)
        d.text((40, 95 + i * 38), ln, font=F_TINY, fill=col)

    d.rounded_rectangle((295, 45, 585, 305), radius=12, fill=(255, 255, 255), outline=(40, 40, 40), width=2)
    d.text((310, 60), "cus_fs_port_config.h", font=get_font(12), fill=BLACK)

    snippets = [
        ('#include "fs_port_littlefs.h"', "USE_LITTLEFS -> port"),
        ('#define FS_NAME "littlefs"', "littlefs.name"),
        ("LITTLEFS_GEOMETRY_BLOCK_COUNT 128", "flash.geometry"),
        ('#include "fs_littlefs_helper_func.h"', "osHelper.path"),
        ("FS_HARDWARE_INIT: init + mutex", "initFunc + mutexCreate"),
    ]
    a, b = snippets[min(step, 4)]
    d.rounded_rectangle((310, 95, 570, 175), radius=8, fill=(255, 240, 200), outline=(255, 180, 0), width=2)
    d.text((320, 105), a, font=get_font(11), fill=BLACK)
    d.text((320, 135), b, font=F_TINY, fill=(60, 60, 60))

    d.text((20, 325), "littlefs: geometry + HW + mutex", font=F_TINY, fill=(60, 60, 60))
    return img


def main():
    make_gif(os.path.join(OUT_DIR, "include_chain.gif"), [frame_include(i) for i in range(3)], duration=850)
    make_gif(os.path.join(OUT_DIR, "generator_flow.gif"), [frame_generator(i) for i in range(6)], duration=650)
    make_gif(os.path.join(OUT_DIR, "flashlink_layout.gif"), [frame_flashlink(i) for i in range(5)], duration=760)
    make_gif(os.path.join(OUT_DIR, "flashfs_log_layout.gif"), [frame_flashfs(i) for i in range(5)], duration=780)
    make_gif(os.path.join(OUT_DIR, "generator_flashlink_ZD_2622.gif"), [frame_generator_flashlink(i) for i in range(5)], duration=780)
    make_gif(os.path.join(OUT_DIR, "generator_littlefs_simulated.gif"), [frame_generator_littlefs(i) for i in range(5)], duration=780)
    print("GIFs generated in:", OUT_DIR)
    print(sorted(os.listdir(OUT_DIR)))


if __name__ == "__main__":
    main()

