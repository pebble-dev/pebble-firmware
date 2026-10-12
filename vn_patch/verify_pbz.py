#!/usr/bin/env python3
"""Independent check of a Vietnamese build against the stock .pbz it came from."""
import argparse, io, json, os, struct, sys, zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

VN = "ĂÂÊÔƠƯăâêôơưĐđ" + "".join(map(chr, range(0x1EA0, 0x1EFA))) + "ÀÁÃÈÉÌÍÒÓÕÙÚÝàáãèéìíòóõùúýĨĩŨũ"
BANK = 0x80000


def lookup(d, cp):
    """Find a glyph the way the firmware does: hash bucket -> offset table -> glyph record."""
    ver, mh, n, wc, ts, cpb, ss, ft = struct.unpack("<BBHHBBBB", d[:10])
    assert ver == 3 and cpb in (2, 4) and ft in (0, 1)
    entry = "<" + ("I" if cpb == 4 else "H") + ("H" if ft & 1 else "I")
    es = struct.calcsize(entry)
    h, cnt, off = struct.unpack("<BBH", d[10 + 4 * (cp % ts): 14 + 4 * (cp % ts)])
    assert h == cp % ts
    ot = 10 + 4 * ts
    gt = ot + es * n
    for j in range(cnt):
        c, go = struct.unpack(entry, d[ot + off + es * j: ot + off + es * (j + 1)])
        if c == cp:
            w, hh = d[gt + go], d[gt + go + 1]
            assert gt + go + 5 + ((w * hh + 31) // 32) * 4 <= len(d)
            return d[gt + go: gt + go + 5 + ((w * hh + 31) // 32) * 4]
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--patcher-dir", required=True)
    ap.add_argument("--emoji-list", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "emoji_list.txt"))
    ap.add_argument("stock"); ap.add_argument("built")
    a = ap.parse_args()
    sys.path.insert(0, a.patcher_dir)
    from stm32_crc import crc32
    from verifpack import verif_pbpack
    from pbfmerge import decode_glyph

    zs, zb = zipfile.ZipFile(a.stock), zipfile.ZipFile(a.built)
    ok = True

    def check(cond, msg):
        nonlocal ok
        print(("PASS " if cond else "FAIL ") + msg)
        ok &= bool(cond)

    man = json.loads(zb.read("manifest.json"))
    fw, rp = zb.read("tintin_fw.bin"), zb.read("system_resources.pbpack")
    check(crc32(fw) == man["firmware"]["crc"] and len(fw) == man["firmware"]["size"], "manifest firmware crc/size")
    check(crc32(rp) == man["resources"]["crc"] and len(rp) == man["resources"]["size"], "manifest resources crc/size")
    check(len(rp) <= BANK, "resource pack %d bytes fits the 512 KiB bank (%d spare)" % (len(rp), BANK - len(rp)))
    check(rp[4:8] in fw and rp[4:8] != zs.read("system_resources.pbpack")[4:8], "firmware embeds the new resource CRC")
    check(zs.read("system_resources.pbpack")[4:8] not in fw, "old resource CRC no longer in firmware")
    check(man["firmware"]["versionTag"] == fw[-43:-11].rstrip(b"\0").decode(), "version tag %s matches image" % man["firmware"]["versionTag"])

    old = verif_pbpack(io.BytesIO(zs.read("system_resources.pbpack")), quiet=True)
    new = verif_pbpack(io.BytesIO(rp), quiet=True)       # prints ERR lines on any bad CRC
    check(len(old) == len(new), "resource count unchanged (%d)" % len(new))
    changed = [i + 1 for i in range(len(new)) if old[i] != new[i]]
    print("changed resource ids:", changed)
    info = json.load(open(a.built + ".json"))
    expected = 8 + len(info["large_fonts"]) + (4 if info["emoji"] else 0)
    check(len(changed) == expected, "exactly %d resources changed (14-28px Gothic x8, %s%s)" % (
        expected, "+".join(info["large_fonts"]) or "no 09/36", ", 4 emoji fonts" if info["emoji"] else ""))
    sofar = zs.read("tintin_fw.bin")
    diff = sum(x != y for x, y in zip(sofar, fw))
    check(len(sofar) == len(fw) and diff < 40, "firmware image differs in only %d bytes (version, timestamp, CRCs)" % diff)
    for name in zs.namelist():
        if name not in ("manifest.json", "tintin_fw.bin", "system_resources.pbpack"):
            check(zs.read(name) == zb.read(name), "%s unchanged" % name)

    from build_vn_pbz import read_emoji_list
    groups, names = read_emoji_list(a.emoji_list)
    groups["emoji"] = info.get("emoji_codepoints", [])           # what this build actually kept
    groups["symbol"] = groups["symbol"] if info["symbols"] else []
    for rid in changed:
        d, o = new[rid - 1], old[rid - 1]
        if d[7] == 4 and d[1] in (14, 18, 24, 28):      # system emoji font (Gothic 36 also has 4-byte codepoints)
            miss = [names[c] for c in groups["emoji"] if lookup(d, c) is None]
            lost = [hex(cp) for cp in list(range(0xE000, 0xE540)) + list(range(0x1F300, 0x1F700)) + [0x2192, 0x2605, 0x2764]
                    if lookup(o, cp) is not None and lookup(d, cp) is None]
            check(not miss and not lost and lookup(d, 0x1F438) is not None,
                  "resource %d (emoji h%d): all %d extra emoji resolve (frog included), nothing lost" % (rid, d[1], len(groups["emoji"])))
            continue
        if d[1] in (14, 18, 24, 28):
            check(all(lookup(d, c) is not None for c in groups["symbol"]), "resource %d: coffee/symbol glyphs resolve" % rid)
        missing = [c for c in VN if lookup(d, ord(c)) is None]
        kept = all(lookup(d, c) is not None for c in range(0x20, 0x7F))
        lost = [cp for cp in list(range(0x10000)) + list(range(0x1F300, 0x1F700))
                if lookup(o, cp) is not None and lookup(d, cp) is None]
        y, _ = decode_glyph(lookup(d, ord("y"))); dy, _ = decode_glyph(lookup(d, 0x1EF5))
        check(y <= dy, "resource %d: U+1EF5 contains the full letter y" % rid)
        check(not missing and kept and not lost,
              "resource %d (h%d): all %d Vietnamese chars resolve, nothing lost" % (rid, d[1], len(VN)))
    print("\nALL CHECKS PASSED" if ok else "\nFAILURES")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
