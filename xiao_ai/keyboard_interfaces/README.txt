RELEASE NOTE: xiao_local full build passes; local serial and user-guided touch tests passed.
See ../../docs/BUILD.md for safe app/cache packaging instructions.

XIAO ESP32S3 + Seeed Round Display: keyboard interfaces

Two separate projects. Both support letters, 123/ABC, symbols, space,
backspace and GO. You can also enter questions using Serial Monitor.

1. xiao_local: PocketGPT runs directly on XIAO.
   Open the folder in PlatformIO. Its source includes the actual local
   inference engine, touchscreen UI and current tied30m validation hashes.
   This is not an Arduino .ino-only project.
   Keep the current tied30m flash model partition and matching SD files
   from the separately reviewed XIAO runtime assets. No weights are copied into this folder.
   The SD directory remains /pocketgpt_xiao_v7b50_final_v1/.
   Build with the project's GCC 16 toolchain package:
   set POCKETGPT_GCC16_PACKAGE to that package's directory, then run pio run.
   Code must also end before the K10 cache at physical 0x7F000.
   The app partition limit alone does not protect the cached weights.
   Do not replace the model partition with the generic mixed weight pack.

2. xiao_network/xiao_network.ino: choose Gemini or Ollama.
   Open this file in Arduino IDE. Select XIAO_ESP32S3 and enable USB CDC on boot.
   Dependencies: ESP32 Arduino core (verified with 2.0.17), ArduinoJson 7.4.3.
   The display library is included and configured for the Seeed round display.
   Serial Monitor: 115200 baud and Newline.
   Enter the requested settings, one line at a time:
   - gemini or ollama
   - Wi-Fi SSID
   - Wi-Fi password
   - Gemini API key OR Ollama base URL
   - Exact model ID, e.g. an available Gemini ID or installed qwen3:4b
   Then use the touchscreen or enter a question over Serial.
   /setup switches backend or changes settings. Reset requires setup again.
   The sketch does not echo setup values or store them in source/NVS.
   Your Serial Monitor input field may still show what you type: do not
   record it while entering private settings. Clear it before recording.

Ollama runs on your computer, not XIAO. Start your chosen model on the
computer first. Use its reachable LAN address, not localhost. Configure
Ollama to accept LAN requests (OLLAMA_HOST) and permit the connection in
your firewall. Use a trusted LAN: the HTTP endpoint is unencrypted.
Official configuration: https://docs.ollama.com/faq
API: https://docs.ollama.com/api/generate
Gemini: https://ai.google.dev/api/generate-content
Gemini uses verified HTTPS and requires a synchronized clock and internet.

These are alternative firmwares: installing the network sketch over the
local deployment with a different partition map can overwrite model data.
Keep the existing deployment archive for restoring local PocketGPT.
The offline UI was tested separately; network firmware was not flashed or tested for this release.

Source comments were removed. Third-party notices remain in the separate
THIRD_PARTY_NOTICES.txt file and supplied license files.

Validation: network sketch compiled and linked using Arduino CLI. Local UI
completed a full PlatformIO library/source build: 450,672-byte firmware,
3,984 bytes before K10. Local inference passed three USB questions; the user confirmed touchscreen
digits, deletion, letters and GO. Network code is supplied compile-checked, without hardware validation.
Detailed connection guide: ../../docs/NETWORK_SETUP.md. Use the offline packager for a fresh install; bare firmware does
not include both model caches.
