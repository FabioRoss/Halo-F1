# Release Checklist

## Fonts and Localization

- [ ] `assets/fonts/Montserrat-Regular.ttf` exists.
- [ ] `assets/fonts/Font Awesome 7 Free-Solid-900.otf` exists.
- [ ] Run `python scripts/generate_fonts.py`.
- [ ] Build succeeds after font generation.
- [ ] Spot-check special characters on device in all supported languages.

## License and Notices

- [ ] `THIRD_PARTY_LICENSES.md` is up to date with actual dependencies/assets used.
- [ ] `licenses/` folder is present and contains all referenced license files.
- [ ] `ESP_I2S.cpp` and `ESP_I2S.h` keep adaptation/license attribution comments.
- [ ] If shipping bundled third-party assets, include license notices in release artifacts.

## Functional Smoke Tests

- [ ] Language dropdown updates all visible UI labels correctly.
- [ ] Race/session screens render without missing glyph placeholders.
- [ ] News tab and weather icons render correctly.
- [ ] No crashes on settings navigation / dropdown interactions.

## Final Packaging

- [ ] README reflects current setup/build steps.
- [ ] Firmware version string is updated as intended.
- [ ] Changelog/release notes mention localization/font/license updates.
