# Changelog

## v0.1.0 DEV

### Added
- Dual HX711 support
- LVGL interface
- Version framework
- GitHub repository
- Independent deliberate tare controls for Left and Right
- Per-channel reference-mass calibration and gram conversion
- Load-triggered calibration settling gate
- Tare/calibration status colour cues
- Structured LoadCellChannel measurement module
- Home / Settings navigation with calibration status
- Hidden developer-mode reveal and Diagnostics entry
- Calibration stabilization countdown and progress feedback
- Home load-cell health/fault status separate from calibration validity
- Enlarged long-press-only developer reveal touch target
- Persistent cross-screen load-cell fault strip
- Explicit 2-second developer reveal hold timing
- Hidden 30-minute creep diagnostic logger with side/mass workflow
- Numeric diagnostic mass keypad and automatic load-triggered timing
- USB serial diagnostic CSV protocol and host capture tool
- Repeatable creep diagnostic test procedure
- Mandatory per-channel zero baseline before loaded diagnostic runs
- Explicit diagnostic FINISH / CSV session completion
- Dedicated 10-second creep reference sample in every diagnostic run
- Local-by-default diagnostic CSV ignore convention

### Fixed
- GT911 initialization issue
- Right HX711 DT/SCK wiring
- Raw unit label alignment
- Left HX711 DT GPIO documentation
