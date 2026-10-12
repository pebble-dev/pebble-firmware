#!/usr/bin/env python3
"""Build an updated Vietnamese language pack (.pbl) from an existing one.

  * keeps the strings and every slot as it is,
  * repairs the truncated U+1EF5 in the 14-28px extended Gothic fonts,
  * replaces slots 10/11 (GOTHIC_36_EXTENDED / GOTHIC_36_BOLD_EXTENDED in Core Devices' 21-slot
    layout; the old pack has 28px fonts there) with 36px fonts drawn by compose_vn.py.

A language pack cannot carry the 09px font: no slot exists for it in any firmware.
Language packs use a 256-entry resource table (content starts at 0x100C), unlike system packs.
"""
import argparse, io, os, struct, sys, zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pbfmerge import Pbf, decode_glyph, encode_glyph
from build_vn_pbz import find_pairs, repair_dot_below_y, load_font

BASE = 0x100C
SLOT_36, SLOT_36B = 10, 11                       # 1-based resource numbers


def read_pack(path, crc32):
    d = open(path, "rb").read()
    n, crc, extra = struct.unpack("<III", d[:12])
    if crc32(d[BASE:]) != crc:
        sys.exit("language pack CRC mismatch")
    res = []
    for i in range(n):
        idx, off, size, c = struct.unpack("<iiiI", d[12 + 16 * i: 28 + 16 * i])
        res.append(d[BASE + off: BASE + off + size])
    return extra, res


def write_pack(path, extra, res, crc32):
    table, data, seen = b"", b"", {}
    for i, r in enumerate(res):
        if r and r in seen:
            off = seen[r]
        else:
            off = len(data)
            if r:
                seen[r] = off
            data += r
        table += struct.pack("<iiiI", i + 1, off, len(r), crc32(r))
    blob = struct.pack("<III", len(res), crc32(data), extra) + table
    blob += b"\0" * (BASE - len(blob)) + data
    open(path, "wb").write(blob)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--patcher-dir", required=True)
    ap.add_argument("--pbl", required=True, help="existing Vietnamese language pack (v16)")
    ap.add_argument("--stock-pbz", required=True, help="a Rebble 4.4.x .pbz, for the stock Gothic fonts")
    ap.add_argument("out")
    a = ap.parse_args()
    sys.path.insert(0, a.patcher_dir)
    from stm32_crc import crc32
    from verifpack import verif_pbpack
    from compose_vn import compose

    extra, res = read_pack(a.pbl, crc32)
    sys_res = verif_pbpack(io.BytesIO(zipfile.ZipFile(a.stock_pbz).read("system_resources.pbpack")), quiet=True)

    # 1) 14-28px: repair U+1EF5 using the stock `y` as the reference
    for i, ext, cands in find_pairs(sys_res, res):
        if len(cands) != 1:
            sys.exit("cannot pair pack resource %d with a stock font" % (i + 1))
        base = Pbf(sys_res[cands[0]])
        font = Pbf(res[i])
        font.glyphs[ord("y")] = base.glyphs[ord("y")]            # reference only, removed again below
        fixed = repair_dot_below_y(font)
        del font.glyphs[ord("y")]
        if fixed:
            for j in range(len(res)):                           # the pack repeats some fonts in other slots
                if res[j] == res[i]:
                    res[j] = font.serialize()
            print("slot %2d (h%2d): U+1EF5 repaired" % (i + 1, font.max_height))

    # 2) slots 10/11: 36px extension fonts
    template = Pbf(res[7])                                       # GOTHIC_28_EXTENDED: same flavour
    wanted = sorted(cp for cp in template.glyphs if cp != template.wildcard)
    larges = {}
    for j, r in enumerate(sys_res):
        f = load_font(r)
        if f and f.max_height == 36 and len(f.glyphs) > 200 and ord("a") in f.glyphs:
            px, _ = decode_glyph(f.glyphs[0x131])
            larges["36b" if max(x for x, _ in px) - min(x for x, _ in px) >= 3 else "36"] = f
    for slot, kind in ((SLOT_36, "36"), (SLOT_36B, "36b")):
        stock = larges[kind]
        composed, _ = compose(stock, kind)
        font = Pbf(res[7])
        font.max_height = 36
        font.glyphs = {template.wildcard: stock.glyphs[template.wildcard]}
        for cp in wanted:
            if cp in composed:
                px, adv = composed[cp]
                font.glyphs[cp] = encode_glyph(px, adv)
            elif cp in stock.glyphs:
                font.glyphs[cp] = stock.glyphs[cp]
            else:
                sys.exit("U+%X missing for the 36px font" % cp)
        res[slot - 1] = font.serialize()
        print("slot %2d: GOTHIC_36%s_EXTENDED, %d glyphs, %d bytes" % (
            slot, "_BOLD" if kind == "36b" else "", len(font.glyphs), len(res[slot - 1])))

    write_pack(a.out, extra, res, crc32)
    print("wrote", a.out)


if __name__ == "__main__":
    main()
