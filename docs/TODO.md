# ArrowLab TODO

## Firmware update paths

- [x] Emergency direct USB update/recovery for VIEWE and WROOM.
- [x] Wi-Fi OTA updates for VIEWE and WROOM.
- [ ] Add firmware updates from the onboard SD card.
  - Keep the SD interface pins reserved for SD use only.
  - Define package naming, target-board identification and version checks.
  - Verify firmware integrity before installation.
  - Prevent a VIEWE image from being installed on the WROOM, and vice versa.
  - Preserve calibration and Wi-Fi settings through an SD update.
  - Provide clear touchscreen progress, success and failure states.
  - Retain direct USB as the final recovery path.
