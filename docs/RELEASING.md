# Releasing Yui

How to cut a release, plus the manual step for getting it into M5Burner.

## Cut a release

1. Make sure `main` is green: `pio test -e native` passes locally and the
   `ci` workflow on the latest commit is green.
2. Pick a version. Tags follow `vMAJOR.MINOR[.PATCH]` (e.g. `v0.3`,
   `v1.0`, `v1.0.1`).
3. Tag and push:

   ```bash
   git tag -a v0.3 -m "v0.3 — short description"
   git push origin v0.3
   git push github v0.3   # if both remotes
   ```

4. The `release` workflow fires on the tag push. It will:
   - Run native tests.
   - Build `cardputer_adv`.
   - Run `tools/pack-release.sh <tag>` to produce
     `release/yui-<tag>-cardputer_adv.bin` (~1.9 MB merged image at
     offset `0x0`) plus a matching `*.json` manifest.
   - Compute SHA256 sums.
   - Refresh `docs/web-installer/` so the GitHub Pages flasher serves
     the new build.
   - Attach the binary, manifest, and `SHA256SUMS` to the GitHub
     Release for the tag.
5. Visit `https://github.com/leingangzj/yui/releases/tag/<tag>` and
   write release notes (or pre-create the release before tagging — the
   CI just attaches assets to whatever release exists).

If the workflow fails partway, rerun it with **Re-run failed jobs** in
the Actions UI. The `pack-release.sh` step is idempotent.

## Web flasher updates automatically

The release workflow rewrites `docs/web-installer/manifest.json` and
swaps the `.bin` to the new tag. Pages picks up the change as soon as
the commit lands on `main`. No manual step needed.

If you skipped CI (e.g. tagged manually and force-uploaded a binary),
you still need to update `docs/web-installer/` by hand and commit:

```bash
cp release/yui-<tag>-cardputer_adv.bin docs/web-installer/
# then edit docs/web-installer/manifest.json — bump "version" and "path"
git add docs/web-installer
git commit -m "release <tag>: refresh web-installer firmware"
```

Old `.bin` files in `docs/web-installer/` get deleted by the CI step;
keeping them around just bloats the repo without serving anything.

## Submitting to M5Burner

This is a **manual** process — M5Stack reviews submissions by hand, and
there's no API. Plan for ~1–2 weeks of review.

1. Make sure the release is on GitHub with a public download URL for the
   `.bin`. Use the `latest` permalink if you want a stable URL across
   versions: `https://github.com/leingangzj/yui/releases/latest/download/yui-<tag>-cardputer_adv.bin`.
2. Open <https://docs.m5stack.com/en/m5burner/firmware_submission> (URL
   may change; the canonical entry point is the M5Burner docs hub).
   Click the firmware-submission form.
3. Fields to fill in:

   | Field          | Value                                                                                      |
   | -------------- | ------------------------------------------------------------------------------------------ |
   | Firmware name  | Yui                                                                                        |
   | Author         | <your handle>                                                                              |
   | Description    | Pocket comms + pentest companion for the Cardputer ADV. (Use the GitHub repo description.) |
   | License        | MIT                                                                                        |
   | Source repo    | https://github.com/leingangzj/yui                                                          |
   | Device         | M5Stack Cardputer ADV                                                                      |
   | Chip           | ESP32-S3                                                                                   |
   | Flash size     | 8 MB                                                                                       |
   | Flash mode     | qio                                                                                        |
   | Flash freq     | 80 MHz                                                                                     |
   | Address / file | `0x0` / the merged `.bin`                                                                  |
   | Category       | Development tools / Productivity / something close — they'll pick.                         |
   | Image          | A 256×256 logo (the koi works; export from `assets/koi.brl` to PNG).                       |

4. They'll email back with either an approval, a request for changes,
   or a rejection. Common rejection reasons:
   - Single-image bundle missing the bootloader at offset `0x0`. Ours
     bundles correctly via `esptool merge_bin`; verify with
     `python3 -m esptool --chip esp32s3 image_info release/yui-<tag>-cardputer_adv.bin`.
   - Vague description. Lead with what the firmware does, not the name.
   - License unclear. Point at `LICENSE` in the repo.
5. Once approved, future releases need to be re-submitted (M5 doesn't
   auto-pull from GitHub). Keep a checklist of which versions are
   live in M5Burner.

## Checklist

- [ ] `pio test -e native` green locally
- [ ] CHANGELOG entry (when one exists)
- [ ] Tag pushed (`git push origin <tag>` and `git push github <tag>`)
- [ ] CI `release` workflow green
- [ ] GitHub Release notes written
- [ ] Web flasher loads at <https://leingangzj.github.io/yui> and shows
      the new version in the "What gets flashed" section
- [ ] (Optional) M5Burner submission emailed
