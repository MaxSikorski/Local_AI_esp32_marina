# Privacy and release boundaries

Never publish original whole-board flash backups. ESP32 NVS can retain Wi-Fi credentials, including deleted entries. The public source tree contains no NVS images or original backup/rollback folders.

API setup is performed at runtime. Do not record the Serial Monitor input field while entering passwords or keys. Existing NVS from another firmware is not securely erased by uploading this sketch. Ollama HTTP is unencrypted; use only a trusted network. The ESP32 needs the computer's reachable LAN address, not localhost.

Do not commit `.env`, credentials, local provenance, build logs, checkpoints, ELF/map files, or hardware capture logs without reviewing them. `.gitignore` does not protect previously tracked files or ZIP uploads.

The release check scans known token signatures and account paths. Separate local checks compared prepared files with Wi-Fi values recovered from the private backups without exposing those values. Pattern scans cannot prove that every possible unknown secret is absent.

If the original credential-bearing backups were already shared, change the exposed Wi-Fi password. Publishing a cleaned copy cannot remove someone else's copy of an earlier upload.
