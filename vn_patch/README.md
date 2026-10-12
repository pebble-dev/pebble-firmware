# Vietnamese build of the Rebble 4.4.x firmware

Rebble's `4.4.x-rbl` firmware for Pebble Time, Time Steel and Time Round is a binary patch of
Pebble's closed 4.3 firmware (see `pebble-dev/legacy-firmware-patcher`), so there is no source to
merge the Core Devices PRs into. What those PRs mean for Vietnamese is font data, and that can be
applied to the resource pack. `build_vn_pbz.py` does this:

1. reads a Vietnamese language pack (`.pbl`) whose extended Gothic fonts hold the missing glyphs,
2. pairs each extended font with its base `GOTHIC_14/18/24/28` (regular and bold) system font,
3. merges the glyphs into the base font records (no re-encoding; layouts and widths already match),
4. rebuilds `system_resources.pbpack`, fixes the CRCs/manifest/version tag, writes a new `.pbz`.

```
git clone https://github.com/pebble-dev/legacy-firmware-patcher
python3 build_vn_pbz.py --patcher-dir legacy-firmware-patcher --pbl vi_VN.pbl \
    --version v4.4.2-rbl-vn  Pebble-4.4.2-rbl-spalding.pbz  dist/Pebble-4.4.2-rbl-vn-spalding.pbz
python3 verify_pbz.py --patcher-dir legacy-firmware-patcher  Pebble-4.4.2-rbl-spalding.pbz  dist/...
```

Stock inputs: `https://github.com/pebble-dev/legacy-firmware-patcher/releases/download/rbl2-rc1/Pebble-4.4.2-rbl-{snowy_dvt,snowy_s3,spalding}.pbz`

## Notes

- Glyphs present in both (Đ đ Ĩ ĩ Ũ ũ) take the pack's version. Stock Gothic 18/28 draw Đ as D with a bar above.
- The pack's U+1EF5 (ỵ) is truncated in the 18, 24 and 28 (regular and bold 28) fonts; the builder repairs it
  from the stock `y` plus the pack's dot, and says so when it does. Worth fixing in the pack itself.
- Sizes 09 and 36 are not touched (the pack has no fonts for them; stock 36 lacks Ă ă Đ đ).
- Silk is not built: the grown pack (535,157 bytes) does not fit its 512 KiB resource bank. The builder refuses.
- The pack contains no translated strings; the UI stays English.
- Checked on the files only (CRCs, resource diffs, hash lookup of every Vietnamese letter). Not yet run on a watch.
