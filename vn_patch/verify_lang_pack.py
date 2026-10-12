#!/usr/bin/env python3
"""Check a language pack built by build_lang_pack.py against the pack it came from."""
import argparse, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pbfmerge import Pbf, decode_glyph
from verify_pbz import lookup, VN
from build_lang_pack import read_pack, BASE


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--patcher-dir", required=True)
    ap.add_argument("old"); ap.add_argument("new")
    a = ap.parse_args()
    sys.path.insert(0, a.patcher_dir)
    from stm32_crc import crc32
    ok = True

    def check(c, m):
        nonlocal ok
        print(("PASS " if c else "FAIL ") + m); ok &= bool(c)

    eo, old = read_pack(a.old, crc32)                 # exits on a bad pack CRC
    en, new = read_pack(a.new, crc32)
    d = open(a.new, "rb").read()
    for i in range(len(new)):
        idx, off, size, c = struct.unpack("<iiiI", d[12 + 16 * i: 28 + 16 * i])
        assert idx == i + 1 and crc32(new[i]) == c
    check(True, "pack CRC and all %d resource CRCs are valid" % len(new))
    check(len(old) == len(new), "same number of slots (%d)" % len(new))
    check(old[0] == new[0], "strings (slot 1) unchanged")
    for i in range(1, len(new)):
        if old[i] == new[i]:
            continue
        o, n = Pbf(old[i]), Pbf(new[i])
        lost = [hex(cp) for cp in o.glyphs if cp not in n.glyphs]
        h_ok = o.max_height == n.max_height
        what = "repaired U+1EF5" if h_ok else "36px font (was %dpx)" % o.max_height
        check(True, "slot %d changed: %s" % (i + 1, what))
        if h_ok:
            check(not lost, "slot %d: no glyph lost" % (i + 1))
            y, _ = decode_glyph(lookup(new[i], 0x1EF5))
            check(sum(1 for _ in y) > 8, "slot %d: U+1EF5 has a full body" % (i + 1))
            diff = [cp for cp in n.glyphs if n.glyphs[cp] != o.glyphs.get(cp)]
            check(set(diff) <= {0x1EF5, 0x128}, "slot %d: only U+1EF5 / U+0128 differ (%s)" % (i + 1, [hex(c) for c in diff]))
    for slot in (10, 11):
        n = Pbf(new[slot - 1])
        need = [c for c in VN if ord(c) > 0x2AF]            # the extension is only used above U+02AF
        miss = [c for c in need if lookup(new[slot - 1], ord(c)) is None]
        check(n.max_height == 36 and not miss and n.wildcard == 0x25AF,
              "slot %d: height %d, all %d Vietnamese letters above U+02AF present, wildcard ok" % (slot, n.max_height, len(need)))
    for i in range(11, len(new)):
        check(new[i] == old[i], "slot %d unchanged" % (i + 1))
    print("\nALL CHECKS PASSED" if ok else "\nFAILURES"); sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
