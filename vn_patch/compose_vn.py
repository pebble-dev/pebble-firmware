#!/usr/bin/env python3
"""Draw the Vietnamese glyphs missing from the stock Gothic 09 / 36 / 36 bold fonts.

Glyphs are composed from the font's own letters and marks, so strokes match the font:
x=0 is the pen origin, y=0 the line top, y grows downward.  Marks that exist in the font
(grave, acute, tilde, circumflex) are taken from the accented glyphs, the rest (breve, hook,
dot, horn, stroke) are drawn per size from the font's stroke width.
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pbfmerge import Pbf, decode_glyph, encode_glyph

TONES = ("grave", "acute", "hook", "tilde", "dot")
# (stem letter, {tone: codepoint}); the stem may itself carry a circumflex/breve/horn
FAMILIES = [
    ("a", {"dot": 0x1EA1, "hook": 0x1EA3}), ("A", {"dot": 0x1EA0, "hook": 0x1EA2}),
    ("â", {"acute": 0x1EA5, "grave": 0x1EA7, "hook": 0x1EA9, "tilde": 0x1EAB, "dot": 0x1EAD}),
    ("Â", {"acute": 0x1EA4, "grave": 0x1EA6, "hook": 0x1EA8, "tilde": 0x1EAA, "dot": 0x1EAC}),
    ("ă", {"acute": 0x1EAF, "grave": 0x1EB1, "hook": 0x1EB3, "tilde": 0x1EB5, "dot": 0x1EB7}),
    ("Ă", {"acute": 0x1EAE, "grave": 0x1EB0, "hook": 0x1EB2, "tilde": 0x1EB4, "dot": 0x1EB6}),
    ("e", {"dot": 0x1EB9, "hook": 0x1EBB, "tilde": 0x1EBD}), ("E", {"dot": 0x1EB8, "hook": 0x1EBA, "tilde": 0x1EBC}),
    ("ê", {"acute": 0x1EBF, "grave": 0x1EC1, "hook": 0x1EC3, "tilde": 0x1EC5, "dot": 0x1EC7}),
    ("Ê", {"acute": 0x1EBE, "grave": 0x1EC0, "hook": 0x1EC2, "tilde": 0x1EC4, "dot": 0x1EC6}),
    ("ı", {"hook": 0x1EC9, "dot": 0x1ECB, "tilde": 0x129}), ("I", {"hook": 0x1EC8, "dot": 0x1ECA, "tilde": 0x128}),
    ("o", {"dot": 0x1ECD, "hook": 0x1ECF}), ("O", {"dot": 0x1ECC, "hook": 0x1ECE}),
    ("ô", {"acute": 0x1ED1, "grave": 0x1ED3, "hook": 0x1ED5, "tilde": 0x1ED7, "dot": 0x1ED9}),
    ("Ô", {"acute": 0x1ED0, "grave": 0x1ED2, "hook": 0x1ED4, "tilde": 0x1ED6, "dot": 0x1ED8}),
    ("ơ", {"acute": 0x1EDB, "grave": 0x1EDD, "hook": 0x1EDF, "tilde": 0x1EE1, "dot": 0x1EE3}),
    ("Ơ", {"acute": 0x1EDA, "grave": 0x1EDC, "hook": 0x1EDE, "tilde": 0x1EE0, "dot": 0x1EE2}),
    ("u", {"dot": 0x1EE5, "hook": 0x1EE7, "tilde": 0x169}), ("U", {"dot": 0x1EE4, "hook": 0x1EE6, "tilde": 0x168}),
    ("ư", {"acute": 0x1EE9, "grave": 0x1EEB, "hook": 0x1EED, "tilde": 0x1EEF, "dot": 0x1EF1}),
    ("Ư", {"acute": 0x1EE8, "grave": 0x1EEA, "hook": 0x1EEC, "tilde": 0x1EEE, "dot": 0x1EF0}),
    ("y", {"grave": 0x1EF3, "dot": 0x1EF5, "hook": 0x1EF7, "tilde": 0x1EF9}),
    ("Y", {"grave": 0x1EF2, "dot": 0x1EF4, "hook": 0x1EF6, "tilde": 0x1EF8}),
]
# every codepoint that Vietnamese needs, whatever font it is in
WANTED = {cp for _, t in FAMILIES for cp in t.values()} | {
    0x102, 0x103, 0x110, 0x111, 0x1A0, 0x1A1, 0x1AF, 0x1B0, 0xC2, 0xE2, 0xCA, 0xEA, 0xD4, 0xF4}


def bitmap(rows):
    return {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == "#"}


def bbox(px):
    xs, ys = [p[0] for p in px], [p[1] for p in px]
    return min(xs), min(ys), max(xs), max(ys)


def shift(px, dx, dy):
    return {(x + dx, y + dy) for x, y in px}


# hand-drawn marks, one set per font; the stroke width t follows the font ('ı' width)
DRAWN = {
    "09": dict(
        breve=bitmap(["#.#", ".#."]),
        hook=bitmap(["##", ".#"]), dot=bitmap(["#"])),
    "36": dict(
        breve=bitmap(["##......##", ".##....##.", "..######.."]),
        hook=bitmap([".#####.", "##...##", ".....##", "...###.", "..##...", "..##..."]),
        dot=bitmap(["##", "##", "##"])),
    "36b": dict(
        breve=bitmap(["####....####", ".####..####.", "..########.."]),
        hook=bitmap(["..#####..", ".###.###.", "......###", "....####.", "...###...", "...###...", "...###..."]),
        dot=bitmap(["####", "####", "####", "####"])),
}


class Composer:
    def __init__(self, font, kind):
        self.f, self.kind = font, kind
        self.g = lambda c: decode_glyph(font.glyphs[ord(c)] if isinstance(c, str) else font.glyphs[c])
        self.has = lambda c: (ord(c) if isinstance(c, str) else c) in font.glyphs
        self.px = lambda c: self.g(c)[0]
        self.adv = lambda c: self.g(c)[1]
        self.lh = font.max_height
        self.t = bbox(self.px("ı"))[2] - bbox(self.px("ı"))[0] + 1       # stroke width
        self.drawn = DRAWN[kind]
        self.notes = []
        # marks that exist in the font, relative to the host letter's centre
        lc, uc = self.centre(self.px("a")), self.centre(self.px("A"))
        above = lambda acc, base: {p for p in self.px(acc) if p[1] < bbox(self.px(base))[1]}
        self.marks = {}
        for tone, lo, up in (("grave", "à", "À"), ("acute", "á", "Á"), ("tilde", "ã", "Ã")):
            self.marks[tone, False] = shift(above(lo, "a"), -lc, 0)
            self.marks[tone, True] = shift(above(up, "A"), -uc, 0)

    @staticmethod
    def centre(px):
        x0, _, x1, _ = bbox(px); return (x0 + x1) // 2

    def mark(self, tone, upper):
        if tone == "hook":
            m = self.drawn["hook"]; return shift(m, -(bbox(m)[2] // 2), 0)
        m = self.marks[tone, upper]
        return shift(m, 0, -bbox(m)[1])

    def stacked(self, base, tone, upper, own_mark):
        """base pixels + tone mark above (stacked when there is room, beside the base's own mark otherwise)."""
        m = self.mark(tone, upper)
        mh = bbox(m)[3] + 1
        x0, y0, x1, y1 = bbox(base)
        cx = self.centre(base)
        gap = 1 if self.kind == "09" else 2 if self.kind == "36b" else 1
        if y0 - mh - gap >= 0:
            return base | shift(m, cx, y0 - mh - gap)
        if y0 - mh >= 0 and not own_mark:
            return base | shift(m, cx, y0 - mh)
        if not own_mark:                         # plain letter, no room: overlap the top rows (squeezed)
            self.notes.append("squeezed")
            return base | shift(m, cx, 0)
        # own mark (circumflex/breve/horn) in the way: tuck the tone mark beside it, top-aligned
        rows = {p for p in base if p[1] <= y0 + 2}
        tx0, _, tx1, _ = bbox(rows)
        mm = bbox(m)
        top = max(0, y0 - 1) if y0 else 0
        if tone == "grave":
            return base | shift(m, tx0 - 1 - mm[2], top)
        return base | shift(m, tx1 + 1 - mm[0], top)

    def dot_below(self, base, lower_y):
        d = self.drawn["dot"]
        x0, y0, x1, y1 = bbox(base)
        cx = self.centre(base)
        return base | shift(d, cx - bbox(d)[2] // 2, y1 + (2 if self.kind != "09" else 2 - 0) - (y1 - lower_y))

    def build(self):
        out = {}
        T = self.t
        has_base = lambda c: self.has(c)
        # --- letters that need drawing: breve, horn, stroke ---
        breve = self.drawn["breve"]
        for up, ch in ((False, "ă"), (True, "Ă")):
            cp = ord(ch)
            if cp in self.f.glyphs: continue
            base = self.px("A" if up else "a")
            b = shift(breve, -(bbox(breve)[2] // 2), -bbox(breve)[1])
            y0 = bbox(base)[1]; gap = 1
            px = base | shift(b, self.centre(base), y0 - (bbox(b)[3] + 1) - gap)
            out[cp] = (px, self.adv("A" if up else "a"))
        for base_c, cp in (("o", 0x1A1), ("O", 0x1A0), ("u", 0x1B0), ("U", 0x1AF)):
            px = self.px(base_c); x0, y0, x1, y1 = bbox(px)
            if self.kind == "09":
                h = {(x1 + 1, y0)}
            elif base_c in "uU":
                h = {(x, y) for x in range(x1 + 1, x1 + 1 + T) for y in range(y0, y0 + T + 2)}
            else:
                h = {(x, y) for x in range(x1, x1 + 1 + T) for y in range(y0 + 1, y0 + 1 + T)}
            out[cp] = (px | h, self.adv(base_c) + (1 if self.kind == "09" else T))
        for base_c, cp, mid in (("D", 0x110, True), ("d", 0x111, False)):
            px = self.px(base_c); x0, y0, x1, y1 = bbox(px)
            if self.kind == "09":
                y = y0 + (2 if mid else 1)
                bar = {(x, y) for x in range(x0 - (1 if mid else 0), x0 + 2 + (0 if mid else 1))} if mid else {(x1 - 2, y), (x1 - 1, y), (x1, y)}
            elif mid:
                yy = (y0 + y1) // 2
                bar = {(x, y) for x in range(x0 - T - 1, x0 + T + 1) for y in range(yy, yy + max(2, T // 2 + 1))}
            else:
                yy = y0 + T + 1
                bar = {(x, y) for x in range(x1 - T - 2, x1 + 3) for y in range(yy, yy + max(2, T // 2 + 1))}
            out[cp] = (px | bar, self.adv(base_c))
        lookup = lambda c: out[c][0] if (c if isinstance(c, int) else ord(c)) in out else self.px(c)
        lookup_adv = lambda c: out[c][1] if (c if isinstance(c, int) else ord(c)) in out else self.adv(c)
        # make sure the horned/breve letters are reachable by name
        named = {"ă": 0x103, "Ă": 0x102, "ơ": 0x1A1, "Ơ": 0x1A0, "ư": 0x1B0, "Ư": 0x1AF}
        def stem(ch):
            cp = named.get(ch, ord(ch))
            return (out[cp][0], out[cp][1]) if cp in out else (self.px(ch), self.adv(ch))
        # --- tone marks ---
        for ch, tones in FAMILIES:
            base, adv = stem(ch)
            upper = ch.isupper()
            own = ch in "âêôơưăÂÊÔƠƯĂ"
            lower_y = bbox(self.px("A" if upper else "x"))[3]
            for tone, cp in tones.items():
                if cp in self.f.glyphs: continue
                if tone == "dot":
                    d = self.drawn["dot"]
                    y_base = bbox(base)[3] if ch in "yY" else lower_y
                    gap = 2 if self.kind != "09" else 1
                    px = base | shift(d, self.centre(base) - bbox(d)[2] // 2, y_base + gap)
                else:
                    px = self.stacked(base, tone, upper, own)
                out[cp] = (px, adv)
        return out


# stock glyphs that exist but are wrong: the stock bold 36 draws U+0110 as D with a bar above it
REPLACE = {"36b": {0x110, 0x111}}


def compose(font, kind):
    """-> {codepoint: (pixels, advance)} for every Vietnamese codepoint the font lacks (or draws wrongly)."""
    c = Composer(font, kind)
    built = c.build()
    keep = lambda cp: cp in WANTED and (cp not in font.glyphs or cp in REPLACE.get(kind, ()))
    return {cp: v for cp, v in built.items() if keep(cp)}, c.notes
