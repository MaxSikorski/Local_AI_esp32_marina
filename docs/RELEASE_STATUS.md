# Release status

Updated 2026-10-13. This is a source release. Original development archives and private recovery backups are not included.

## Verified

- DevKit: a fresh PlatformIO build succeeded. Retained hardware measurements for the deduplicated candidate are in `espdev_ai/recorded_results/2026-10-12_tied30m_dedup_devkit/`; these measurements predate source cleanup.
- XIAO local: a fresh full source/library build with the checksum-verified official GCC16 toolchain produced a 450,672-byte firmware. Build/upload layout guards and six packaging tests protect the cache placement.
- XIAO local hardware: application/cache installation was verified on 2026-10-13 on an already provisioned board. Boot/cache checks and three USB inference tests passed. The user confirmed display, digits, deletion, ABC switching and GO. Evidence: `xiao_ai/recorded_results/ui_2026-10-13/`.
- XIAO network: the Gemini/Ollama sketch compiled (892,941 bytes Flash; 48,240 bytes static RAM). Network hardware testing is outside this source-release scope, not a pending required task.
- The DevKit pack verifier passed for the 20,241,616-byte pack held separately. The release privacy checker screens local metadata and basic secret signatures; it does not use a root checksum manifest.

## Included

Firmware source, touchscreen keyboard interfaces, build/configuration instructions, packaging and verification tools, relevant hardware evidence, and retained third-party notices. Historical comparison experiments, recovery/flash logs and superseded XIAO results have been removed from this public copy.

## Limits and separate work

- Deployment weights and tokenizer files are now distributed separately in [GitHub Releases](ASSETS.md); they are not committed to the source tree. They are necessary to run the local model. Original Base V2 training sources are now included under `training/`; the full subsequent repair/export pipeline and training data remain incomplete.
- The author selected MIT for released weights/tokenizer within the author’s rights. Attribution is supplied with the packages; parts of historical data provenance remain unresolved. See [DATA_PROVENANCE.md](DATA_PROVENANCE.md) and [../LICENSE_STATUS.md](../LICENSE_STATUS.md).
- The bundled display source comparison and notice recovery are documented; retained license/notice files must remain. Linked binary dependency obligations still need review if publishing firmware binaries. See [THIRD_PARTY.md](THIRD_PARTY.md).
- Fresh-board provisioning and builds on hosts other than Apple Silicon macOS have not been validated. The local hardware test used an already provisioned board.
- Checksums and privacy scanning do not certify model quality or the absence of every possible secret.

## Source naming cleanup

On 2026-10-13, 30 source/configuration filenames were clarified and remaining ordinary code comments were removed. All three firmware variants compiled after the changes; six layout/packaging tests passed. XIAO local remains 450,672 bytes, DevKit firmware is 431,696 bytes, and the network sketch uses 892,941 bytes Flash and 48,240 bytes static RAM. No hardware was flashed or retested in this cleanup; recorded hardware results describe the preceding build. Third-party notice files and original recorded outputs were preserved.

## Source publication preparation — 2026-10-13

- Included 20 historical training Python sources with AST-equivalent comment cleanup, per-file provenance hashes, saved schedule and readable Base V2 recipe. No training was run. The final runner's `--help` works in the existing development environment; missing historical data prevents a turnkey full run.
- Removed the optional root SHA256SUMS.txt and replaced the release checker with limited privacy/metadata checks. Checksums generated for model packages and historical evidence remain because they serve different purposes.
- Nine tests passed; all 28 Python sources parsed; local Markdown links resolved. Existing firmware source bytes were unchanged, so previous build evidence is retained rather than claiming a new hardware test.
- The source tree contains no model weights, tokenizer, datasets, private backups, or prebuilt firmware. Model/tokenizer deployment assets are supplied separately in Releases. The source release is published at https://github.com/mmm1712/Local_AI_esp32.

## Model asset release — 2026-10-13

- DevKit and XIAO deployment assets are supplied as distinct release ZIPs with license, attribution, file sizes and SHA256 manifests. No firmware executable or whole-board/NVS backup is included.
- DevKit pack verification passed; XIAO Flash/cache hashes and all 84 matrix placements were checked against source constants. Existing hardware evidence remains the validation scope; no board was flashed for this release.
- The asset guide supplies matching download, build, packaging and write/verify commands. Fresh-board provisioning remains untested end to end.
