#!/usr/bin/env python3
"""Render review sheets for the composed 09 / 36 / 36 bold Vietnamese glyphs."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image, ImageDraw, ImageFont
from pbfmerge import Pbf, decode_glyph
from compose_vn import compose, FAMILIES

SAMPLES = ["Tiếng Việt: Người Hà Nội ăn phở, uống cà phê.",
           "Đường Trường Sơn, Nguyễn Huệ, Hồ Chí Minh",
           "ĂÂÊÔƠƯ ẮẰẲẴẶ ẤẦẨẪẬ ẾỀỂỄỆ ỐỒỔỖỘ ỚỜỞỠỢ ỨỪỬỮỰ",
           "ạ ả ẹ ẻ ẽ ị ỉ ĩ ọ ỏ ụ ủ ũ ỳ ỵ ỷ ỹ Đ đ"]


def sheet(font, kind, out_path, scale):
    new, notes = compose(font, kind)
    glyphs = {cp: decode_glyph(r) for cp, r in font.glyphs.items()}
    for cp, (px, adv) in new.items():
        glyphs[cp] = (px, adv)
    lh = font.max_height
    label = ImageFont.truetype("DejaVuSans.ttf", 15)
    cw = max(a for _, a in (new[c] for c in new)) + 6
    cw = max(cw, 12) * scale
    ch = (lh + 8) * scale
    order = [cp for _, t in FAMILIES for cp in [*t.values()] if cp in new]
    order = sorted(set(order) | {c for c in new}, key=lambda c: (c not in (0x102, 0x103, 0x110, 0x111, 0x1A0, 0x1A1, 0x1AF, 0x1B0), c))
    cols = 12
    rows = (len(order) + cols - 1) // cols
    sample_h = (lh + 6) * scale * len(SAMPLES)
    W = max(cols * cw, 60 * (lh // 3 + 8) * scale // 4 * 4)
    need = max(sum(glyphs[ord(c)][1] if ord(c) in glyphs else 6 for c in t) + 8 for t in SAMPLES) * scale + 20
    im = Image.new("RGB", (max(cols * cw + 20, need), rows * (ch + 20) + sample_h + 60), "white")
    d = ImageDraw.Draw(im)
    for i, cp in enumerate(order):
        ox, oy = 10 + (i % cols) * cw, 10 + (i // cols) * (ch + 20)
        d.text((ox, oy), chr(cp), fill=(200, 0, 0), font=label)
        for x, y in new[cp][0]:
            d.rectangle([ox + (x + 2) * scale, oy + 20 + y * scale, ox + (x + 3) * scale - 1, oy + 20 + (y + 1) * scale - 1], fill="black")
    y0 = rows * (ch + 20) + 30
    for s_i, text in enumerate(SAMPLES):
        pen = 4
        for c in text:
            g = glyphs.get(ord(c))
            if g is None:
                pen += 6; continue
            px, adv = g
            for x, y in px:
                X, Y = (pen + x) * scale + 10, y0 + s_i * (lh + 6) * scale + y * scale
                if X + scale < im.width:
                    d.rectangle([X, Y, X + scale - 1, Y + scale - 1], fill="black")
            pen += adv
    im.save(out_path)
    return len(new), notes


if __name__ == "__main__":
    S = sys.argv[1]; outdir = sys.argv[2]
    for kind, res, scale in (("09", "032", 7), ("36", "044", 3), ("36b", "045", 3)):
        F = Pbf(open(os.path.join(S, "sp/res", res), "rb").read())
        n, notes = sheet(F, kind, os.path.join(outdir, "vn_%s.png" % kind), scale)
        print(kind, n, "glyphs", "squeezed:", notes.count("squeezed"))
