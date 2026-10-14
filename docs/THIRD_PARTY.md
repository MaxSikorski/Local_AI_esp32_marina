# Third-party inventory

Reviewed 2026-10-13. This inventory distinguishes source distributed here from build dependencies. It is not a blanket license for compiled firmware.

| Component | Where used | Evidence and handling |
|---|---|---|
| Seeed/Bodmer TFT_eSPI | Vendored network display; pinned Git dependency for local UI | Preserve `xiao_ai/keyboard_interfaces/xiao_network/src/display/license.txt`, font license and interface notices. The bundled notice contains Adafruit MIT/BSD and Bodmer FreeBSD terms; do not reduce the whole directory to the project's MIT license. Local UI pins Seeed commit `096f2fa3549475af17f5266f0200ea241e14151f`. |
| Adafruit GFX fonts / TomThumb | Vendored display fonts | Existing font `license.txt` and `THIRD_PARTY_NOTICES.txt` retained. |
| ArduinoJson 7.4.3 | Network sketch build dependency, not vendored | Installed upstream LICENSE.txt states MIT, copyright Benoit Blanchon. [Upstream](https://github.com/bblanchon/ArduinoJson). |
| Arduino ESP32 core | Both firmware builds, downloaded dependency | Inspected Arduino.h carries LGPL-2.1-or-later. Other components have their own notices; review the entire selected framework when distributing linked binaries. [Upstream](https://github.com/espressif/arduino-esp32). |
| Espressif framework libraries / ESP-IDF components | Linked firmware dependencies | Retain upstream component licenses and any required distribution materials for published firmware. This repository's MIT license does not cover those libraries. [Upstream](https://github.com/espressif/esp-idf). |
| Espressif GCC16 toolchain | Local UI build tool, not vendored | Official archive, pinned checksum and reproduction instructions in TOOLCHAIN.md. Preserve bundled per-component licenses. |
| SentencePiece | Tokenizer training/export tooling, not vendored | Separate software and trained-artifact provenance. Tokenizer rights are discussed in DATA_PROVENANCE.md. [Upstream](https://github.com/google/sentencepiece). |

The bundled display source was compared file-by-file with available installed TFT_eSPI/Seeed sources on 2026-10-13: 151 code files match after comment/whitespace and inclusion-suffix normalization; eight contain reviewed include/configuration adaptations, and RoundSetup.h is local board configuration. See DISPLAY_SOURCE_REVIEW.json for filenames and code diffs. Recovered upstream notices are in `../xiao_ai/keyboard_interfaces/DISPLAY_UPSTREAM_NOTICES.txt`, alongside the existing license files and THIRD_PARTY_NOTICES.txt. Preserve all notices. This is a source attribution review, not a license review of a compiled firmware distribution. [Upstream license](https://github.com/Bodmer/TFT_eSPI/blob/master/license.txt).

Before publishing prebuilt firmware, complete the linked-dependency license inventory and any applicable source/relinking requirements. The prepared source snapshot excludes prebuilt firmware. Missing attributions must be resolved rather than assuming that the author's MIT grant covers third-party code.
