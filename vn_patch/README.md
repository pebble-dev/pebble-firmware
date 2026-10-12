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

## Which file for which watch

| Watch | File | `hwrev` in the bundle |
|---|---|---|
| Pebble Time | `dist/Pebble-4.4.2-rbl-vn-snowy_dvt.pbz` | `snowy_dvt` |
| Pebble Time Steel | `dist/Pebble-4.4.2-rbl-vn-snowy_s3.pbz` | `snowy_s3` |
| Pebble Time Round | `dist/Pebble-4.4.2-rbl-vn-spalding.pbz` | `spalding` |

The Time / Time Steel mapping follows Rebble's board names (the stock bundles are named the same way).
Check the hardware revision on the watch (Settings > System > Information) before flashing; the
bundles differ only in the firmware image, the fonts are identical.

## Extra emoji

`emoji_list.txt` names the emoji merged from Core Devices PR #1725's expanded emoji fonts
(`emoji_src/`, with their Apache-2.0 / OFL notices): 45 emoji (frog, noodles, scooter, extra faces...)
go into the four system EMOJI fonts, and U+2615 (coffee) into the eight Gothic fonts, because the
firmware draws U+2000-2BFF from the text font, not the emoji font. Edit the list and rebuild
(`--emoji-list none` skips the step). Each emoji costs about 170 bytes across the four sizes.

Not possible without changing firmware code: anything outside U+E000-E537 / U+1F300-1F6FF
(for example 🤔 🤣 🥰 🥺 🤗), flags and keycaps. That is what the C part of #1725 adds.

## Notes

- Glyphs present in both (Đ đ Ĩ ĩ Ũ ũ) take the pack's version. Stock Gothic 18/28 draw Đ as D with a bar above.
- The pack's U+1EF5 (ỵ) is truncated in the 18, 24 and 28 (regular and bold 28) fonts; the builder repairs it
  from the stock `y` plus the pack's dot, and says so when it does. Worth fixing in the pack itself.
- Sizes 09 and 36 are not touched (the pack has no fonts for them; stock 36 lacks Ă ă Đ đ).
- Silk is not built: the grown pack (535,157 bytes) does not fit its 512 KiB resource bank. The builder refuses.
- The pack contains no translated strings; the UI stays English.
- Checked on the files only (CRCs, resource diffs, hash lookup of every Vietnamese letter). Not yet run on a watch.
- Space left in the resource bank: Spalding 4,385 bytes, Snowy 7,269. `build_vn_pbz.py` refuses to build if the pack no longer fits.
