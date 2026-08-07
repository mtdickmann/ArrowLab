# Phase 2 - Tare and Calibration

## Date
2026-08-06

## Status
Calibration is implemented and under hardware validation. Both HX711/load-cell channels have repeatedly calibrated independently and produced consistent converted gram readings.

Spine calculations must not be added until calibration, repeatability, drift and measurement timing are considered reliable.

## Measurement architecture

- `main.cpp` is the top-level coordinator. It does not own HX711 acquisition, tare maths or calibration maths.
- `src/measurement/LoadCellChannel.*` owns the state and behaviour of one HX711/load-cell channel.
- Left and Right are independent channels with independent tare state and calibration factors.
- The UI presents state and raises user actions; measurement maths does not belong in the UI.
- Future measurement modes such as spine, whole-arrow mass and FOC will consume calibrated Left/Right loads rather than accessing HX711 hardware directly.

## Tare conventions

Two different zero concepts are intentionally distinguished:

1. Startup automatic zero
   - Used internally to establish usable zero-adjusted raw readings.
   - Does not mean the user has established the correct physical measurement setup.
   - Must not produce `TARE OK`.

2. Deliberate user tare
   - Requires an explicit TARE action and confirmation for the selected side.
   - The calibration platform must be fitted to the selected side when preparing a calibration tare.
   - Only successful completion of this deliberate tare produces `TARE OK`.

Left and Right may be tared in either order.

A new tare changes the temporary zero offset but does not erase a valid calibration factor. This is required because later measurement modes will have different physical configurations. For example, whole-arrow mass/FOC operation will tare the normal jig without calibration platforms or calibration weights while continuing to use the established counts-to-grams factors.

## Calibration conventions

- Calibration is independent for Left and Right.
- The current development reference mass is 999.8 g.
- 999.8 g is not a firmware-defined universal calibration mass. It is the current user's measured reference.
- The measurement API already accepts a supplied reference mass.
- A future calibration screen will provide numeric keypad entry for the user's actual reference mass.
- A practical minimum reference mass of approximately 500 g is currently planned, subject to further validation.
- Calibration uses 20 fresh samples to determine average zero-adjusted counts for the supplied reference mass.
- Calibration factor is stored as signed counts per gram so either load-cell signal direction is supported.
- After successful calibration the channel displays grams.
- Re-taring does not invalidate the calibration factor.
- Calibration factors are persisted independently for Left and Right and restored after a normal power cycle when the firmware version matches.

### Persistence rule

Implemented behaviour:

- Left and Right calibration factors are stored independently.
- Calibration survives normal power off/on.
- Normal tare operations do not invalidate calibration.
- Calibration remains valid until the user deliberately recalibrates or the firmware is updated.
- Stored calibration must be associated with the firmware version from `Version.h`.
- A firmware-version mismatch invalidates the stored calibration and requires recalibration.
- Home and the Calibration screen must visibly show whether calibration is valid or required.
- ArrowLab must not assume a factory calibration.

## Calibration load detection and settling

Calibration must not become available merely because an arbitrary amount of time has passed since tare. The reference mass must actually be present.

Current calibration gating:

1. The selected side must have a deliberate `TARE OK`.
2. The first CAL action records the user's actual reference mass and arms load
   detection.
3. A significant zero-adjusted load of at least 250,000 raw counts must be
   detected for 5 consecutive fresh HX711 samples.
4. CAL then becomes available for the explicit second action.
5. The second CAL action starts one fixed 30-second settling timer. Leaving the
   weight in place does not restart it.
6. After the full interval, filtered calibration sampling and persistent save
   run automatically.
7. The shared calibration controller enforces the rule independently of the
   disabled/enabled state of either normal or diagnostic UI buttons.

### Why 250,000 counts?

Initial hardware observation is approximately one million counts for a mass near 1 kg. A 500,000-count trigger would therefore be too close to the expected signal from a roughly 500 g minimum calibration mass and could fail on a less-sensitive channel.

250,000 counts is still orders of magnitude above the observed near-zero raw noise while leaving useful margin for channel-to-channel sensitivity differences.

This raw threshold is necessary before first calibration because a trustworthy counts-to-grams factor does not yet exist.

## Status and colour conventions

Status uses text and colour together. Colour is a quick visual cue, not the only source of state information.

- Orange: action/attention required or operation not yet valid.
- Green: operation/state currently valid or successfully completed.
- `TARE OK` / `TARE REQ` remains visible in text.
- `CAL OK` / calibration-required state remains visible in text.
- TARE and CAL buttons adopt the same orange/green status convention.

Green CAL means the stored or newly completed calibration is valid for the running firmware version.

## Units

- Grams are the canonical measurement unit.
- Planned quick display choices are `g`, `gr` (grain) and `oz`.
- Unit conversion is display-only; calibration and internal measurement remain based on grams.
- Kilograms are not currently planned as a display unit because ArrowLab's useful measurement range does not justify them.

## Current hardware observations

The following are engineering observations, not yet final specifications:

- Both channels repeatedly calibrate consistently.
- The Right channel currently appears somewhat more stable than the Left channel.
- Fast displayed jitter is approximately 0.2 g peak-to-peak in typical testing.
- A time-under-load drift/creep effect has been observed.
- Under prolonged load the indicated mass has moved by roughly +0.4 to +0.5 g after many minutes in some tests.
- A tared channel carrying only its calibration platform remained much closer to zero while the loaded channel continued to drift.
- A temporary approximately 132 g household reference produced readings close to its expected value on both channels, but it is not a calibrated reference standard.
- The calibrated 113.8 g secondary verification weight remains the intended independent verification reference.

## Drift and filtering policy

Jitter and drift must not be treated as the same problem.

- Short-term noise/jitter may later be reduced by statistically defensible filtering or averaging.
- Slow under-load drift/creep must not be hidden by display smoothing.
- No mathematical creep compensation will be added until the behaviour has been measured repeatedly at multiple known loads and over controlled time intervals.
- A fixed measurement timing protocol is preferred before adding model-based compensation.
- Future testing should record readings at controlled intervals such as 0 s, 10 s, 30 s, 60 s, 2 min, 5 min and 10 min.
- If creep proves repeatable per channel, a bounded per-channel correction may be considered.
- Calibration should eventually have a limited valid action window after settling so a reference mass cannot remain loaded indefinitely before calibration is accepted.
- Ordinary arrow mass/FOC measurements should use a controlled settle/sample/freeze window rather than indefinite live readings when a final measurement is required.

## Next validation steps

1. Verify the two-action CAL workflow and fixed 30-second stabilization on
   hardware.
2. Verify leaving the reference weight in place cannot restart stabilization.
3. Characterise zero stability and under-load creep independently for Left and Right.
4. Verify both calibrated channels using the 113.8 g secondary reference.
5. Decide the measurement averaging/stability method from observed data.
6. Implement persistent calibration storage and firmware-version invalidation.
7. Only after scale behaviour is reliable, proceed toward combined load and later spine calculations.


## Navigation and guided calibration UI

The development UI is being moved from a single calibration/test screen into a normal instrument navigation structure.

Current convention:

- Home is the normal startup screen.
- Home contains a disabled SPINE TEST placeholder and SETTINGS.
- Home visibly reports CALIBRATION REQUIRED or CALIBRATION OK.
- Settings owns Calibration because calibration is an instrument setup operation, not a normal measurement action.
- The top-right header position is reserved for a future Help action.
- Firmware version/build information will move to a future About screen instead of permanently consuming header space.
- The existing independent Left/Right calibration controls remain the proven calibration implementation underneath the navigation layer.
- Once a qualifying reference load is detected, the 30-second stabilization interval is shown with a progress bar and remaining-second indication. A disabled CAL button must not leave the operator guessing why it is unavailable.

### Developer-mode convention

Developer and diagnostic facilities are intentionally absent from the normal user interface.

- A long press on the ArrowLab title/logo reveals Developer mode.
- Developer mode exposes DIAGNOSTICS under Settings for the current powered session.
- The reveal mechanism is independent of firmware-version placement so a future About screen does not break service access.
- Diagnostics remains a maintained engineering tool rather than a temporary calibration hack.

## Creep diagnostic rationale

The purpose of the diagnostic is to measure creep before any compensation model is considered.

The original unattended-acquisition requirements were:

- Run a zero-load baseline with only the normal fixed arrow-rest hardware.
- Run five applied-mass datasets, with actual mass entered by the operator for every weight.
- Initial suggested mass spread is approximately 20 g, 100 g, 250 g, 500 g and 1000 g; recorded values are the operator-entered actual masses.
- Record at t=0, t=10 seconds and every 30 seconds from t=30 seconds through 30 minutes per applied mass.
- Test Left and Right independently so channel behaviour is not averaged together.
- Primary evidence is raw and zero-adjusted HX711 counts.
- Converted grams and the active calibration factor are recorded alongside the raw evidence when calibration is available.
- Do not recalibrate for every diagnostic weight. Doing so would normalize away part of the behaviour being investigated.
- Initial export target is CSV over USB serial with a small host-side capture tool; SD-card logging is deferred until the display board's SD hardware is independently proven.

Candidate CSV fields:

side,test_mass_g,elapsed_s,raw_count,zeroed_count,calculated_g,calibration_factor

The full two-channel campaign is intentionally long (roughly 5.5 to 6 hours including handling/zero runs). Acquisition must therefore be automatic; operator timing is not part of the measurement method.

The resulting datasets will be used to determine:

- creep magnitude versus applied mass,
- time dependence and possible asymptotic behaviour,
- Left/Right channel differences,
- repeatability between runs,
- unloading recovery and hysteresis,
- whether a fixed measurement timing window is sufficient,
- and only then whether a bounded creep correction is technically defensible.


## Instrument health versus calibration validity

Calibration validity and hardware health are independent states and must never be conflated.

- CALIBRATION OK means that both channels currently have valid calibration factors under the applicable persistence/version rules.
- It does not mean the two HX711/load-cell signal paths are presently healthy.
- Home therefore reports live load-cell health separately from calibration status.
- Loss of Left, Right or both HX711 channels produces a red fault indication on Home after the normal live-data timeout.
- A hardware fault does not silently erase a valid calibration factor; repairing/reconnecting the signal path and recalibration policy are separate decisions.
- Future measurement actions such as Spine, mass and FOC must refuse or invalidate a measurement when required hardware is not live.

## Long-term navigation direction

The current Home/Settings pages are functional scaffolding, not the final visual layout.

The intended polished UI follows an instrument-style layout similar in concept to the QIDI Q2 interface:

- persistent navigation tabs/icons down the left side,
- contextual menu or working content in the larger area to the right,
- Help available from normal UI chrome,
- About owning firmware/build information,
- dark/light theme support later,
- graphics and iconography added after workflow and measurement behaviour are proven.

The navigation/state API should therefore remain independent of the exact temporary button layout so screens can be restyled without moving measurement logic.


### Persistent fault presentation

Load-cell health is an instrument-level condition, not a page-local condition.

- A Left/Right HX711 loss is shown in a persistent root-level status strip that overlays the current page.
- The fault strip remains visible while navigating Home, Settings, Calibration and future measurement/diagnostic screens.
- The presentation uses a restrained dark strip with high-contrast red text rather than a blocking modal dialog.
- Modal fault popups are intentionally avoided because expected service/diagnostic work may deliberately disconnect hardware.
- The strip clears automatically after both channels return live.
- Calibration validity is not erased merely because a live signal path was temporarily lost.

Developer reveal is also explicitly timed by ArrowLab rather than relying on the touch stack's long-press event classification. Press duration is measured from press to release and must reach 2000 ms. Normal short taps have no developer-mode action.


## Implemented creep diagnostic facility

A hidden developer creep logger is now part of the maintained firmware rather than a disposable test build.

Implementation boundaries:

- src/diagnostics/CreepDiagnostic.* owns acquisition timing and serial diagnostic records.
- The normal measurement channel continues to own HX711 acquisition, tare and calibration maths.
- The UI owns diagnostic navigation, side selection, numeric mass entry, confirmations and progress presentation.
- tools/capture_creep.py owns host-side serial filtering and CSV creation.
- docs/Creep_Diagnostic_Procedure.md owns the repeatable operator procedure; test/creep_diagnostic/README.md records design rationale.
- calibration/diagnostics/ is the repository location for retained raw investigation datasets.

Loaded diagnostic runs perform a fresh operational tare with the selected calibration platform fitted and the test mass removed. The entered mass is then applied. Five consecutive fresh samples above 2,000 zero-adjusted counts confirm the load and automatically start the 30-minute acquisition clock.

Zero-baseline runs use only the normal fixed arrow-rest hardware and begin automatically after their fresh tare completes.

Each completed run emits samples at t=0 and t=10 seconds, followed by one sample every 30 seconds from t=30 seconds through 30 minutes, for 62 expected rows. The dedicated 10-second point captures the important initial creep reference without changing the established 30-second cadence for the remainder of the campaign.

The CSV record preserves raw counts, zero-adjusted counts, calculated grams and the active calibration factor. The diagnostic never recalibrates itself for each mass.

## Firmware update requirement

Network-delivered firmware update support is a project requirement, not an optional future experiment.

The current 16 MB flash partitioning provides two 6.25 MiB OTA application slots. Future update architecture must preserve safe dual-slot update/validation and rollback capability. Wi-Fi is the expected primary transport, including ordinary phone-hotspot connectivity when appropriate. Bluetooth/BLE may support provisioning or service workflows, but must not be assumed to provide general Internet tethering without a separately proven implementation.

Stored calibration validity must remain tied to firmware version compatibility as documented above; a firmware update that changes the calibration compatibility version must force recalibration.

## Diagnostic workflow and serial recovery revision (v0.1.1)

Field testing exposed two design weaknesses in the first creep logger: the two channels were unnecessarily coupled by a both-baselines gate, and a PC/USB interruption could leave the operator unsure whether the final samples reached the CSV.

The revised design treats Left and Right as independent instruments until a later measurement mode deliberately combines them:

- Settings -> Diagnostics is a submenu so further developer tools can be added without overloading one screen.
- The operator selects a channel before entering the creep workflow.
- Baseline, calibration and reset state are stored per channel.
- Replacing or resetting one channel does not invalidate the other.
- The selected channel follows baseline -> deliberate platform tare -> calibration -> arbitrary loaded runs.
- Existing calibration/tare routines are reused; diagnostics does not maintain competing measurement maths.

The USB protocol now uses a host handshake and heartbeat. A run cannot start merely because the operator claims the logger is open. Firmware buffers all 62 samples in RAM, replays the full run at completion and waits for a PC acknowledgement containing boot ID, run ID and expected row count. Duplicate replay rows are removed by the PC using sample identity. A zero baseline becomes valid only after this acknowledgement.

This recovery is intentionally bounded. Serial disconnection is recoverable while ArrowLab remains powered; a power loss destroys the RAM buffer. Another run cannot replace an unacknowledged buffer.

Routine high-frequency raw serial output was removed because it competed with diagnostic protocol traffic and had no value in a disciplined CSV acquisition.

Persistent storage now implements the previously planned rules. Counts-per-gram factor, reference mass and firmware version are stored independently for each channel. Version mismatch invalidates calibration. Baseline evidence is stored separately because it records a completed historical diagnostic rather than defining measurement conversion.

Legacy PC CSV files remain evidence but are not instrument state. The earlier
`USE CSV` declaration was removed because the firmware cannot parse or verify a
PC file and the control therefore implied authority it did not possess. A new
or reset channel earns `BASE OK` only after a complete run is acknowledged by
the logger.

## Creep evidence decision (v0.1.2)

The retained campaign data shows short-term residual noise near 0.03-0.05 g,
approximately 0.16-0.29 g movement over some 30-minute loaded runs, and drift
which reverses direction between runs and sessions. One Left 999.8 g run also
contains a single multi-million-count transient. These facts rule out a single
honest counts-per-minute compensation curve.

The adopted boundary is therefore:

- diagnostic CSV output remains raw and unsmoothed;
- normal displayed measurements use a 15-sample trimmed-mean window (three
  samples removed from each tail once full);
- a tare is taken in the actual physical configuration immediately before a
  measurement sequence;
- future mass and spine workflows accept a short stable window rather than
  leaving a result under load for many minutes;
- automatic zero tracking is not used because it could erase real small masses
  such as protector rings;
- no empirical creep correction is applied until repeatable temperature-aware
  evidence proves one is valid.

The filter rejects isolated electrical spikes and calms the display. It does
not pretend that filtering removes mechanical or thermal creep. Calibration
also uses the robust window after the existing 30-second settling gate.

Normal Calibration and Creep Diagnostic remain separate workflows. Both call
the same channel tare/calibration primitives, but diagnostic baseline state
does not block a normal recalibration and normal calibration does not fabricate
baseline evidence.

## Diagnostic UI convention (v0.1.2)

- the persistent header names the current page/tool;
- the compact state line reports selected side, BASE, TARE and CAL state;
- the main status line gives the exact next action or a deliberate WAIT state;
- the progress bar represents the active acquisition or 30-second calibration
  settling gate;
- modal popups are reserved for prerequisites, confirmation and explicit Help;
- CAL is green for a stored factor, orange while action is required, and may be
  deliberately run again through normal Calibration after a fresh tare;
- diagnostic CSV filenames include LHS/RHS and one capture session contains
  only one selected channel.


### Diagnostic session rules

The creep diagnostic requires an explicit channel selection; there is no implicit/default Left or Right run.

A zero baseline is mandatory independently for each channel before that channel may start any loaded diagnostic run. The firmware diagnostic engine enforces this rule in addition to the UI control state.

After a channel's zero baseline is complete, the operator may record any number of loaded masses. The diagnostic facility does not hard-code a required count because future investigations may need a different mass distribution.

The current first-pass campaign uses anchor loads first:

- 0 g baseline
- approximately 20 g
- approximately 1000 g

Approximately 50 g, 100 g, 250 g and 500 g runs remain available to fill informative gaps after the low/high anchor datasets have been inspected. This prevents an unnecessarily long fixed campaign from being treated as a substitute for targeted experimental design.

Actual measured mass is entered for every loaded run.

DONE explicitly ends the current PC capture session and emits an AL_DIAG SESSION_COMPLETE event. The PC capture tool treats that event as the clean end of the CSV acquisition. A previously acknowledged baseline remains stored for its channel until the operator resets that channel; a new CSV session alone does not erase it.

Generated diagnostic CSV files are ignored by Git by default. A reviewed dataset chosen as permanent evidence may be force-added deliberately; ordinary trial acquisitions remain local and do not clutter source control.

### High-load reference comparison

A later engineering reference run may follow the timing and conditioning structure of OIML R 60 creep testing without claiming accredited type evaluation. For each independent channel, the planned reference uses approximately 1,800 g total physical load on the 2 kg cell, three brief preload/removal conditioning cycles, a one-hour minimum-load rest, and one 30-minute acquisition near 20 C. The calibration platform and all other supported hardware count toward the total physical load; SET MASS still receives only the separately measured added test-piece mass. The explicit 10-second sample, 20-minute sample and 30-minute endpoint are preserved in the CSV.

## Shared calibration workflow controller (v0.1.3)

Calibration workflow ownership is separated from `main.cpp`. `LoadCellChannel`
continues to own HX711 acquisition, tare sampling, filtering and the actual
counts-per-gram calculation. The new `CalibrationController` owns the user
workflow, 30-second stabilization timer and persistent calibration save. Normal
Calibration and the developer creep diagnostic therefore use the same controller
instead of maintaining parallel calibration state machines.

Calibration factor/state and tare state deliberately have different lifetimes:

- a valid Left/Right calibration factor is restored from NVS across normal power
  cycles when its firmware compatibility version still matches;
- tare is a temporary zero for the current physical setup and is never restored
  across power cycles;
- a fresh tare changes the zero offset without deleting an established factor;
- firmware compatibility changes still invalidate the stored factor and require
  a deliberate new calibration.

For each side the normal calibration interaction is:

1. `TARE` is always available unless a calibration is actively stabilizing or
   sampling. Fit the empty calibration platform and deliberately tare that side.
2. A completed tare turns `TARE` green but leaves it actionable.
3. `CAL` becomes actionable immediately after tare. Orange means calibration is
   required; green means a valid factor already exists and CAL may be pressed to
   deliberately recalibrate.
4. The first `CAL` action requests the user's actual reference mass. ArrowLab does
   not treat 999.8 g or any other development weight as a universal constant.
5. ArrowLab instructs the user to place that reference weight. A confirmed
   significant raw-count change arms the second CAL action.
6. The second `CAL` action starts one fixed 30-second stabilization period. The
   weight remaining in place cannot restart that timer. CAL is disabled and the
   progress indicator provides positive feedback while stabilizing.
7. After the timer, the normal filtered calibration samples are collected, the
   new factor is calculated and stored in NVS, and CAL returns green/actionable.

Left and Right remain independent. Completing one side never navigates away from
the Calibration screen; if the other side still needs attention, the next-action
message continues to guide the user there. Global calibration validity becomes OK
only when both stored channel factors are valid.
