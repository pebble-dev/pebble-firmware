#!/usr/bin/env python3
"""Merge the glyphs of one PBF font (v3, 16-bit offsets, no RLE) into another.

Glyph records are copied verbatim, so no re-encoding of bitmaps takes place.
"""
import struct

HASH_ENTRY = "<BBH"


def decode_glyph(rec):
    """-> (set of (x, y) pixels, advance); x is relative to the pen, y to the line top."""
    w, h, l, t, a = struct.unpack("<BBbbb", rec[:5])
    return {(l + k % w, t + k // w) for k in range(w * h) if rec[5 + k // 8] >> (k % 8) & 1}, a


def encode_glyph(pixels, advance):
    """Inverse of decode_glyph (bitmap is row-major, LSB first, padded to 32 bits)."""
    x0, y0 = min(p[0] for p in pixels), min(p[1] for p in pixels)
    w, h = max(p[0] for p in pixels) - x0 + 1, max(p[1] for p in pixels) - y0 + 1
    bits = bytearray(((w * h + 31) // 32) * 4)
    for x, y in pixels:
        k = (y - y0) * w + (x - x0)
        bits[k // 8] |= 1 << (k % 8)
    return struct.pack("<BBbbb", w, h, x0, y0, advance) + bytes(bits)


class Pbf:
    """A PBF v3 font without RLE: 2- or 4-byte codepoints, 16- or 32-bit glyph offsets."""

    def __init__(self, data):
        if len(data) < 10:
            raise ValueError("too short to be a PBF font")
        (self.version, self.max_height, n, self.wildcard, self.table_size,
         self.cp_bytes, self.struct_size, self.features) = struct.unpack("<BBHHBBBB", data[:10])
        if self.version != 3 or self.features & ~0x01 or self.cp_bytes not in (2, 4):
            raise ValueError("unsupported PBF flavour: v%d features=%#x cp_bytes=%d" % (
                self.version, self.features, self.cp_bytes))
        self.entry = "<" + ("I" if self.cp_bytes == 4 else "H") + ("H" if self.features & 1 else "I")
        es = struct.calcsize(self.entry)
        ms = 10
        ot = ms + 4 * self.table_size
        gt = ot + es * n
        self.glyphs, offsets = {}, set()
        for i in range(self.table_size):
            _, cnt, off = struct.unpack(HASH_ENTRY, data[ms + 4 * i: ms + 4 * i + 4])
            for j in range(cnt):
                cp, goff = struct.unpack(self.entry, data[ot + off + es * j: ot + off + es * (j + 1)])
                offsets.add(goff)
                w, h = data[gt + goff], data[gt + goff + 1]
                size = 5 + ((w * h + 31) // 32) * 4
                self.glyphs[cp] = data[gt + goff: gt + goff + size]
        assert len(self.glyphs) == n, (len(self.glyphs), n)
        self.dedup = len(offsets) < n         # the stock emoji fonts share identical bitmaps

    def serialize(self):
        es = struct.calcsize(self.entry)
        order = [self.wildcard] + sorted(c for c in self.glyphs if c != self.wildcard)
        table = b"\0\0\0\0"
        offs, seen = {}, {}
        for cp in order:                      # identical bitmaps are stored once, as in the stock fonts
            rec = self.glyphs[cp]
            if self.dedup and rec in seen:
                offs[cp] = seen[rec]
                continue
            seen[rec] = offs[cp] = len(table)
            table += rec
        buckets = {}
        for cp in sorted(self.glyphs, key=lambda c: (c % self.table_size, c)):
            buckets.setdefault(cp % self.table_size, []).append(cp)
        offtab = b"".join(struct.pack(self.entry, cp, offs[cp])
                          for h in sorted(buckets) for cp in buckets[h])
        # the firmware indexes the hash table by (cp % table_size): one entry per bucket, empty ones included
        hashtab, run = b"", 0
        for h in range(self.table_size):
            n = len(buckets.get(h, ()))
            hashtab += struct.pack(HASH_ENTRY, h, n, run)
            run += es * n
        limit = 0x10000 if self.features & 1 else 1 << 32
        assert len(offtab) < 0x10000 and len(table) < limit, "font too large for its offset width"
        hdr = struct.pack("<BBHHBBBB", self.version, self.max_height, len(self.glyphs), self.wildcard,
                          self.table_size, self.cp_bytes, self.struct_size, self.features)
        return hdr + hashtab + offtab + table

    def merge(self, other, prefer_other=(), only=None):
        """Add glyphs from `other` (all of them, or just `only`); replace ours for codepoints in prefer_other."""
        added = replaced = 0
        for cp, rec in other.glyphs.items():
            if cp == other.wildcard or (only is not None and cp not in only):
                continue
            if cp not in self.glyphs:
                self.glyphs[cp] = rec; added += 1
            elif cp in prefer_other and self.glyphs[cp] != rec:
                self.glyphs[cp] = rec; replaced += 1
        return added, replaced
