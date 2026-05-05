# Releasing Yui

How to cut a release, plus the manual M5Burner submission step.

## Cut a release

1. **Make sure `main` is green.** `pio test -e native` passes locally and the `ci` workflow on the latest commit is green.
2. **Pick a version.** Tags follow `vMAJOR.MINOR[.PATCH]` (e.g. `v1.1.0`, `v1.2.0`).
3. **Tag and push** to both remotes:

   ```bash
   git tag -a v1.2.0 -m "v1.2.0 — short description"
   git push github v1.2.0
   git push origin v1.2.0   # Gitea mirror
   ```

4. **Release CI fires automatically** on the tag push. It will:
   - Run native tests
   - Build `cardputer_adv`
   - Run `tools/pack-release.sh <tag>` to produce `release/yui-<tag>-cardputer_adv.bin` (~2 MB merged image at offset `0x0`) plus `*.json` manifest
   - Compute SHA256 sums
   - Refresh `docs/web-installer/` so GitHub Pages serves the new build
   - Attach binary, manifest, and `SHA256SUMS` to the GitHub Release for the tag

5. **Write release notes** at `https://github.com/leingangzj/yui/releases/tag/<tag>` (or pre-create the release before tagging — CI just attaches assets to whatever release exists).

If CI fails partway, rerun with **Re-run failed jobs** in the Actions UI. `pack-release.sh` is idempotent.

## Web flasher

The release workflow rewrites `docs/web-installer/manifest.json` and swaps in the new `.bin`. Pages picks it up as soon as the commit lands on `main`. No manual step.

If you skipped CI (e.g. tagged manually and force-uploaded a binary), update `docs/web-installer/` by hand:

```bash
cp release/yui-<tag>-cardputer_adv.bin docs/web-installer/
# edit docs/web-installer/manifest.json — bump "version" and "path"
git add docs/web-installer
git commit -m "release <tag>: refresh web-installer firmware"
```

Old `.bin` files in `docs/web-installer/` get deleted by the CI step. Don't leave them lying around — they bloat the repo without serving anything.

## Submitting to M5Burner

**Manual process.** M5Stack reviews submissions by hand, no API. Plan for ~1–2 weeks of review.

1. Make sure the release is on GitHub with a public download URL for the `.bin`. Use the `latest` permalink for a stable URL across versions:
   `https://github.com/leingangzj/yui/releases/latest/download/yui-<tag>-cardputer_adv.bin`
2. Open <https://docs.m5stack.com/en/m5burner/firmware_submission>. Click the firmware-submission form.
3. Fill in:

   | Field          | Value                                                                        |
   | -------------- | ---------------------------------------------------------------------------- |
   | Firmware name  | Yui                                                                          |
   | Author         | leingangzj                                                                   |
   | Description    | Pen-test workbench for the Cardputer ADV. (Use the GitHub repo description.) |
   | License        | MIT                                                                          |
   | Source repo    | <https://github.com/leingangzj/yui>                                          |
   | Device         | M5Stack Cardputer ADV                                                        |
   | Chip           | ESP32-S3                                                                     |
   | Flash size     | 8 MB                                                                         |
   | Flash mode     | qio                                                                          |
   | Flash freq     | 80 MHz                                                                       |
   | Address / file | `0x0` / the merged `.bin`                                                    |
   | Category       | Development tools / Productivity (their call)                                |
   | Image          | 256×256 logo (the koi from `assets/koi.brl` exports cleanly to PNG)          |

4. They'll email back with approval, a change request, or a rejection. Common rejection reasons:
   - Single-image bundle missing the bootloader at offset `0x0`. Ours bundles correctly via `esptool merge_bin`; verify with
     `python3 -m esptool --chip esp32s3 image_info release/yui-<tag>-cardputer_adv.bin`
   - Vague description. Lead with what the firmware does, not the name.
   - License unclear. Point at `LICENSE` in the repo.
5. Once approved, future releases need to be re-submitted. M5 doesn't auto-pull from GitHub. Keep a checklist of which versions are live in M5Burner.

## Checklist

- [ ] `pio test -e native` green locally
- [ ] Tag pushed to both remotes
- [ ] CI `release` workflow green
- [ ] GitHub Release notes written
- [ ] Web flasher loads at <https://leingangzj.github.io/yui> and shows the new version in the "What gets flashed" section
- [ ] (Optional) M5Burner submission emailed
