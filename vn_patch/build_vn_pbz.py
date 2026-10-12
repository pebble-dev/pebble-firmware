#!/usr/bin/env python3
"""Build Vietnamese-capable Rebble 4.4.x firmware bundles.

Takes a Rebble 4.4.x-rbl .pbz plus a Vietnamese language pack (.pbl) whose extended
Gothic fonts hold the missing glyphs, merges those glyphs straight into the base
Gothic 14/18/24/28 (regular + bold) system font resources, and writes a new .pbz.

The firmware binary is untouched apart from the version tag, the resource-pack CRC
and the in-image timestamp (same fix-ups legacy-firmware-patcher/patchpbz.py makes).

Needs a checkout of https://github.com/pebble-dev/legacy-firmware-patcher
(for stm32_crc / mkpack / verifpack), given with --patcher-dir.
"""
import argparse, binascii, contextlib, io, json, os, struct, sys, time, zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pbfmerge import Pbf, decode_glyph, encode_glyph  # noqa: E402

FIRMWARE, RESPACK = "tintin_fw.bin", "system_resources.pbpack"
LANG_BASE = 0x100C                    # language packs: 256-entry table, not the system pack's 512
# Codepoints that exist in the stock Gothic fonts but are wrong there (e.g. Đ drawn as D + macron)
# or that the pack redraws to match its stacked marks: take the pack's version.
PREFER_PACK = {0x110, 0x111, 0x128, 0x129, 0x168, 0x169}
SIZES = (14, 18, 24, 28)
BANK_SIZE = 0x80000                   # SYSTEM_RESOURCES_BANK_0/1 on snowy, spalding and silk (512 KiB)


def read_lang_pack(path, crc32):
    d = open(path, "rb").read()
    n, crc = struct.unpack("<II", d[:8])
    if crc32(d[LANG_BASE:]) != crc:
        sys.exit("language pack CRC mismatch")
    res = []
    for i in range(n):
        idx, off, size, c = struct.unpack("<iiiI", d[12 + 16 * i: 28 + 16 * i])
        data = d[LANG_BASE + off: LANG_BASE + off + size]
        if crc32(data) != c:
            sys.exit("language pack resource %d CRC mismatch" % idx)
        res.append(data)
    return res


def load_font(data):
    try:
        return Pbf(data)
    except ValueError:
        return None


glyph_pixels = decode_glyph


def repair_dot_below_y(font):
    """The pack's regular-weight U+1EF5 (y + dot below) is truncated in some sizes: the top of the
    letter is blank and its tail is lost.  Rebuild it from the stock `y` plus the pack's dot."""
    y, adv = decode_glyph(font.glyphs[ord("y")])
    cur, cadv = decode_glyph(font.glyphs[0x1EF5])
    if y <= cur:
        return False
    dot = {p for p in cur if p[1] > max(py for _, py in y)}
    if not dot or cadv != adv:
        sys.exit("cannot repair U+1EF5: unexpected glyph shape")
    font.glyphs[0x1EF5] = encode_glyph(y | dot, adv)
    return True


# base letter -> precomposed "letter + dot below" in the extension; the extension glyph must
# be the base glyph with extra pixels only underneath it
DOT_BELOW = {"a": 0x1EA1, "A": 0x1EA0, "e": 0x1EB9, "E": 0x1EB8, "o": 0x1ECD,
             "O": 0x1ECC, "u": 0x1EE5, "U": 0x1EE4}   # y/Y are left out: the pack redraws the tail


def matches(base, ext):
    for ch, cp in DOT_BELOW.items():
        if ord(ch) not in base.glyphs or cp not in ext.glyphs:
            return False
        bpx, badv = glyph_pixels(base.glyphs[ord(ch)])
        epx, eadv = glyph_pixels(ext.glyphs[cp])
        extra = epx - bpx
        if badv != eadv or not bpx <= epx or not extra or min(y for _, y in extra) <= max(y for _, y in bpx):
            return False
    return True


def find_pairs(sys_res, lang_res):
    """Match every extended Gothic font in the pack to its base font in the system pack."""
    bases = [(j, load_font(r)) for j, r in enumerate(sys_res)]
    bases = [(j, f) for j, f in bases if f and len(f.glyphs) > 300 and 0x1E00 not in f.glyphs]
    pairs, seen = [], set()
    for i, r in enumerate(lang_res):
        ext = load_font(r)
        if not ext or 0x1EA1 not in ext.glyphs:
            continue
        if r in seen:                 # the pack repeats the 28px fonts in the Bitham slots
            continue
        seen.add(r)
        pairs.append((i, ext, [j for j, base in bases if base.max_height == ext.max_height and matches(base, ext)]))
    return pairs


def read_emoji_list(path):
    """-> ({'emoji': [cp, ...], 'symbol': [cp, ...]}, names)"""
    groups, names, cur = {"emoji": [], "symbol": []}, {}, None
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("["):
            cur = line.strip("[]")
            continue
        cp, _, name = line.partition(" ")
        groups[cur].append(int(cp, 16)); names[int(cp, 16)] = name
    return groups, names


def load_donors(groups, names, src_dir):
    donors = {}
    for size in SIZES:
        donors[size] = Pbf(open(os.path.join(src_dir, "EMOJI_%d.pbf" % size), "rb").read())
        for cp in groups["emoji"] + groups["symbol"]:
            if cp not in donors[size].glyphs:
                sys.exit("U+%X (%s) is not in the %dpx donor font" % (cp, names[cp], size))
    return donors


def emoji_resources(sys_res):
    res = {}
    for j, r in enumerate(sys_res):
        f = load_font(r)
        if f and f.cp_bytes == 4 and 0x1F600 in f.glyphs and f.max_height in SIZES:
            if f.max_height in res:
                sys.exit("more than one %dpx emoji font in the resource pack" % f.max_height)
            res[f.max_height] = j
    if sorted(res) != list(SIZES):
        sys.exit("could not find the four system emoji fonts")
    return res


def apply_emoji(sys_res, plan, donors, emoji_cps, symbol_cps):
    """Merge extra emoji into the system EMOJI_nn fonts (the firmware routes U+E000-E537 and
    U+1F300-1F6FF there) and symbols (U+2000-2BFF, drawn from the base text font) into the
    Gothic fonts of the same size, regular and bold.  Modifies sys_res in place."""
    if emoji_cps:
        for size, j in sorted(emoji_resources(sys_res).items()):
            f = Pbf(sys_res[j])
            f.merge(donors[size], only=set(emoji_cps))
            sys_res[j] = f.serialize()
    if symbol_cps:
        for j, ext in sorted(plan.items()):
            f = Pbf(sys_res[j])
            f.merge(donors[ext.max_height], only=set(symbol_cps))
            sys_res[j] = f.serialize()


def large_fonts(sys_res):
    """The stock Gothic 09 / 36 / 36 bold text fonts -> {'09': j, '36': j, '36b': j}."""
    found = {}
    for j, r in enumerate(sys_res):
        f = load_font(r)
        if not f or f.max_height not in (9, 36) or len(f.glyphs) < 200 or ord("a") not in f.glyphs \
                or ord("\u0131") not in f.glyphs:
            continue
        if f.max_height == 9:
            kind = "09"
        else:
            px, _ = decode_glyph(f.glyphs[0x131])               # stem width of the dotless i
            kind = "36b" if max(x for x, _ in px) - min(x for x, _ in px) >= 3 else "36"
        if kind in found:
            sys.exit("more than one candidate for the %s font" % kind)
        found[kind] = j
    if sorted(found) != ["09", "36", "36b"]:
        sys.exit("could not find the Gothic 09 / 36 / 36 bold fonts (found %s)" % sorted(found))
    return found


def apply_large(sys_res, kind, j):
    """Draw and merge the Vietnamese glyphs the stock font lacks (see compose_vn.py)."""
    from compose_vn import compose
    f = Pbf(sys_res[j])
    glyphs, _ = compose(f, kind)
    for cp, (px, adv) in glyphs.items():
        f.glyphs[cp] = encode_glyph(px, adv)
    sys_res[j] = f.serialize()
    return len(glyphs)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--patcher-dir", required=True)
    ap.add_argument("--pbl", required=True, help="Vietnamese language pack")
    ap.add_argument("--version", required=True, help="new version tag, max 32 bytes, e.g. v4.4.2-rbl-vn")
    here = os.path.dirname(os.path.abspath(__file__))
    ap.add_argument("--emoji-list", default=os.path.join(here, "emoji_list.txt"),
                    help="extra emoji to merge (default: emoji_list.txt); 'none' to skip")
    ap.add_argument("--emoji-src", default=os.path.join(here, "emoji_src"),
                    help="directory with the expanded EMOJI_14/18/24/28.pbf donor fonts")
    ap.add_argument("--margin", type=int, default=1024,
                    help="bytes of the resource bank to leave free when fitting optional glyphs (default 1024)")
    ap.add_argument("--skip-large", action="append", default=[], choices=["09", "36", "36b"],
                    help="leave this Gothic font alone to save space (repeatable), e.g. --skip-large 36")
    ap.add_argument("--no-large", action="store_true", help="do not add glyphs to the Gothic 09/36/36 bold fonts")
    ap.add_argument("pbzin"); ap.add_argument("pbzout")
    a = ap.parse_args()
    sys.path.insert(0, a.patcher_dir)
    from stm32_crc import crc32
    from mkpack import save_pbpack
    from verifpack import verif_pbpack

    zin = zipfile.ZipFile(a.pbzin)
    manifest = json.loads(zin.read("manifest.json"))
    orig = zin.read(RESPACK)
    sys_res = verif_pbpack(io.BytesIO(orig), quiet=True)
    lang_res = read_lang_pack(a.pbl, crc32)

    pairs = find_pairs(sys_res, lang_res)
    # collapse to base resources: regular and bold both carry U+1EA1, so they match different bases
    plan = {}
    for i, ext, cands in pairs:
        if len(cands) != 1:
            sys.exit("ambiguous/missing base for pack resource %d (height %d): %s" % (i + 1, ext.max_height, cands))
        plan[cands[0]] = ext
    if sorted({e.max_height for e in plan.values()}) != list(SIZES) or len(plan) != 8:
        sys.exit("expected 8 fonts at sizes %s, got %d" % (SIZES, len(plan)))

    for j, ext in sorted(plan.items()):
        base = Pbf(sys_res[j])
        added, replaced = base.merge(ext, PREFER_PACK)
        fixed = repair_dot_below_y(base)
        out = base.serialize()
        assert Pbf(out).glyphs.keys() == base.glyphs.keys()
        print("system resource %3d (h%2d): +%d glyphs, %d replaced%s, %d -> %d bytes"
              % (j + 1, base.max_height, added, replaced, ", U+1EF5 repaired" if fixed else "",
                 len(sys_res[j]), len(out)))
        sys_res[j] = out

    info = {"vietnamese_14_28": True, "large_fonts": [], "skipped_large_fonts": [], "symbols": [], "emoji": [],
            "skipped_emoji": []}
    limit = BANK_SIZE - a.margin

    def size_of(res):
        buf = io.BytesIO()
        with contextlib.redirect_stdout(io.StringIO()):          # save_pbpack reports shared resources
            save_pbpack(buf, res)
        return len(buf.getvalue())

    base_size = size_of(sys_res)
    if base_size > BANK_SIZE:
        sys.exit("resource pack is %d bytes, bigger than the %d byte system resource bank "
                 "(over by %d); this device cannot take the Vietnamese glyphs"
                 % (base_size, BANK_SIZE, base_size - BANK_SIZE))
    print("after Vietnamese 14-28px: %d bytes, %d spare (keeping %d in reserve)" % (
        base_size, BANK_SIZE - base_size, a.margin))

    # tiers, most important first; each is kept only if the pack still fits
    if not a.no_large:
        larges = large_fonts(sys_res)
        for kind in ("36b", "09", "36"):
            if kind in a.skip_large:
                info["skipped_large_fonts"].append(kind)
                print("Gothic %-3s: skipped on request" % kind)
                continue
            trial = list(sys_res)
            n = apply_large(trial, kind, larges[kind])
            if size_of(trial) <= limit:
                sys_res[:] = trial
                info["large_fonts"].append(kind)
                print("Gothic %-3s: +%d Vietnamese glyphs (%d spare)" % (kind, n, BANK_SIZE - size_of(sys_res)))
            else:
                info["skipped_large_fonts"].append(kind)
                print("Gothic %-3s: SKIPPED, would need %d bytes more than available" % (kind, size_of(trial) - limit))
    if a.emoji_list != "none":
        groups, names = read_emoji_list(a.emoji_list)
        donors = load_donors(groups, names, a.emoji_src)
        if groups["symbol"]:
            trial = list(sys_res)
            apply_emoji(trial, plan, donors, [], groups["symbol"])
            if size_of(trial) <= limit:
                sys_res[:] = trial
                info["symbols"] = [names[c] for c in groups["symbol"]]
        lo, hi = 0, len(groups["emoji"])                     # largest prefix of the list that fits
        while lo < hi:
            mid = (lo + hi + 1) // 2
            trial = list(sys_res)
            apply_emoji(trial, plan, donors, groups["emoji"][:mid], [])
            lo, hi = (mid, hi) if size_of(trial) <= limit else (lo, mid - 1)
        keep = groups["emoji"][:lo]
        apply_emoji(sys_res, plan, donors, keep, [])
        info["emoji"] = [names[c] for c in keep]
        info["emoji_codepoints"] = keep
        info["skipped_emoji"] = [names[c] for c in groups["emoji"][lo:]]
        print("extra emoji: %d of %d fit%s" % (lo, len(groups["emoji"]),
              "" if lo == len(groups["emoji"]) else " (dropped: %s)" % ", ".join(info["skipped_emoji"])))

    new = io.BytesIO(); save_pbpack(new, sys_res)
    res_data = new.getvalue()
    if len(res_data) > BANK_SIZE:
        sys.exit("internal error: pack is %d bytes, over the %d byte bank" % (len(res_data), BANK_SIZE))
    info.update(pack_size=len(res_data), spare=BANK_SIZE - len(res_data))
    old_crc, new_crc = orig[4:8], res_data[4:8]
    manifest["resources"].update(crc=crc32(res_data), size=len(res_data), timestamp=int(time.time()))

    fw = zin.read(FIRMWARE)
    fw_ts = int(time.time())
    oldv = fw[-43:-11]
    fw = fw.replace(oldv, a.version.encode().ljust(32, b"\0"))
    n = fw.count(old_crc)
    if n < 1:
        sys.exit("old resource CRC not found in firmware")
    print("replacing resource CRC %s -> %s (%d occurrence(s))" % (
        binascii.hexlify(old_crc).decode(), binascii.hexlify(new_crc).decode(), n))
    fw = fw.replace(old_crc, new_crc)
    fw = fw[:-47] + struct.pack("<I", fw_ts) + fw[-43:]
    manifest["firmware"].update(crc=crc32(fw), size=len(fw), timestamp=fw_ts, versionTag=a.version)
    manifest["generatedBy"] = "Vietnamese font build on top of the Rebble Alliance 4.4.x patch"

    with zipfile.ZipFile(a.pbzout, "w", zipfile.ZIP_STORED) as z:
        for name in zin.namelist():
            if name not in ("manifest.json", FIRMWARE, RESPACK):
                z.writestr(name, zin.read(name))
        z.writestr(RESPACK, res_data)
        z.writestr(FIRMWARE, fw)
        z.writestr("manifest.json", json.dumps(manifest, indent=4, separators=(",", ": ")))
    with open(a.pbzout + ".json", "w") as f:
        json.dump(info, f, indent=2, ensure_ascii=False)
    print("wrote", a.pbzout, "(%d bytes of the bank spare)" % info["spare"])


if __name__ == "__main__":
    main()
