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

## What each build contains

Space in the 512 KiB resource bank is the limit, so `build_vn_pbz.py` fits things in priority order
and keeps `--margin` (default 1 KiB) free: Vietnamese 14-28px (always) > Gothic 36 bold > Gothic 09 >
Gothic 36 > coffee > extra emoji in the order of `emoji_list.txt`. `dist/*.pbz.json` records what each
build got. The Time / Time Steel bundles were built with `--skip-large 36` to leave room for more emoji.

| Build | Vietnamese sizes | Extra emoji |
|---|---|---|
| Time / Time Steel | 09, 14, 18, 24, 28, 36 bold (36 regular skipped with `--skip-large 36`) | 37 (of 45) |
| Time Round | 09, 14, 18, 24, 28, 36 bold (36 regular does not fit) | 19 |

The glyphs for 09 / 36 / 36 bold are drawn by `compose_vn.py` from the font's own letters and marks
(`preview_vn.py` renders review sheets). The stock bold 36 Đ/đ and Ĩ/ĩ were wrong and are replaced.

## Language pack v17

`build_lang_pack.py` turns the v16 pack into `dist/vi_VN-v17.pbl`: it repairs the truncated U+1EF5 in the
14-28px fonts and puts real 36px fonts into slots 10/11 (GOTHIC_36_EXTENDED / _BOLD_EXTENDED in Core
Devices' 21-slot layout; v16 had 28px fonts there). A pack cannot carry 09px, there is no slot for it.
The slot order differs on older firmware (4.4.2's slots 10/11 are Bitham fonts), and the 4.4.2-rbl-vn builds
do not need the pack because the glyphs are in the base fonts. `verify_lang_pack.py` checks it.

## Extra emoji

`emoji_list.txt` names the emoji merged from Core Devices PR #1725's expanded emoji fonts
(`emoji_src/`, with their Apache-2.0 / OFL notices): up to 45 emoji (frog, noodles, scooter, extra faces...)
go into the four system EMOJI fonts, and U+2615 (coffee) into the eight Gothic fonts, because the
firmware draws U+2000-2BFF from the text font, not the emoji font. Edit the list and rebuild
(`--emoji-list none` skips the step). Each emoji costs about 170 bytes across the four sizes; the list is
in priority order and the builder drops from the end when the bank is full.

Not possible without changing firmware code: anything outside U+E000-E537 / U+1F300-1F6FF
(for example 🤔 🤣 🥰 🥺 🤗), flags and keycaps. That is what the C part of #1725 adds.

## Notes

- Glyphs present in both (Đ đ Ĩ ĩ Ũ ũ) take the pack's version. Stock Gothic 18/28 draw Đ as D with a bar above.
- The pack's U+1EF5 (ỵ) is truncated in the 18, 24 and 28 (regular and bold 28) fonts; the builder repairs it
  from the stock `y` plus the pack's dot, and says so when it does. Worth fixing in the pack itself.
- Silk is not built: the grown pack (535,157 bytes) does not fit its 512 KiB resource bank. The builder refuses.
- The pack contains no translated strings; the UI stays English.
- Checked on the files only (CRCs, resource diffs, hash lookup of every Vietnamese letter). Not yet run on a watch.
- Space left in the resource bank: about 1.1-1.2 KB on every build (by design, see above); `build_vn_pbz.py` refuses to build if the pack does not fit.
