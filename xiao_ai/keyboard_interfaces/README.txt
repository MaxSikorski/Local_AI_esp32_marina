XIAO ESP32-S3 + Seeed Round Display

Choose one firmware:

xiao_local
  Runs PocketGPT on the XIAO with a touchscreen keyboard and microSD.
  Model weights and tokenizer are available in GitHub Releases.
  Follow ../../docs/ASSETS.md for downloads, GCC16 setup, build and installation.
  Install the packaged app_with_tail.bin with model.bin and the SD files;
  bare firmware.bin does not include the required model caches.

xiao_network/xiao_network.ino
  Sends questions to Gemini or to Ollama running on your PC.
  Follow ../../docs/NETWORK_SETUP.md for Arduino setup and connection steps.
  Enter credentials through Serial Monitor, not source files.
  This variant is compile-checked; network hardware connectivity was not tested.

Both interfaces provide letters, 123/ABC, space, backspace and GO.
Switching firmware can overwrite the offline model layout. Use the complete
local installation guide when returning to PocketGPT.

Keep THIRD_PARTY_NOTICES.txt and the supplied license files.
