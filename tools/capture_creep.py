#!/usr/bin/env python3
"""Reliable ArrowLab creep-test capture with handshake and replay recovery."""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path
import sys
import time

try:
    import serial
except ImportError:
    print(
        "pyserial is required. Install with: python -m pip install pyserial",
        file=sys.stderr,
    )
    raise SystemExit(2)


PROTOCOL_VERSION = 3
HEARTBEAT_INTERVAL_S = 1.0
RECONNECT_DELAY_S = 2.0

CSV_HEADER = [
    "boot_id",
    "run_id",
    "sample_index",
    "side",
    "run_type",
    "test_mass_g",
    "elapsed_ms",
    "raw_count",
    "run_reference_raw",
    "delta_count",
    "host_timestamp_utc",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture ArrowLab creep-test protocol v3 raw records."
    )
    parser.add_argument("port", help="Serial port, for example COM3")
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Serial baud rate (default: 115200)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional output CSV path",
    )
    return parser.parse_args()


def default_output(side: str, stamp: str) -> Path:
    side_name = "LHS" if side == "LEFT" else "RHS"
    return (
        Path("calibration")
        / "diagnostics"
        / f"creep_{side_name}_{stamp}.csv"
    )


def send_line(device: serial.Serial, text: str) -> None:
    device.write((text + "\n").encode("ascii"))
    device.flush()


def open_device(port: str, baud: int) -> serial.Serial:
    device = serial.Serial()
    device.port = port
    device.baudrate = baud
    device.timeout = 0.25
    device.dtr = False
    device.rts = False
    device.open()
    send_line(device, f"AL_HOST,HELLO,{PROTOCOL_VERSION}")
    return device


def main() -> int:
    args = parse_args()
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output: Path | None = args.output

    print(f"ArrowLab creep logger v2: {args.port} @ {args.baud}")
    if output is None:
        print("CSV: filename will be assigned from the first LHS/RHS run")
    else:
        print(f"CSV: {output}")
    print("Waiting for ArrowLab handshake. Ctrl+C closes the logger.")

    row_count = 0
    seen_samples: set[tuple[int, int, int]] = set()
    device: serial.Serial | None = None
    last_heartbeat = 0.0
    csv_file = None
    writer = None
    active_side: str | None = None

    try:
        while True:
            if device is None:
                try:
                    device = open_device(args.port, args.baud)
                    last_heartbeat = time.monotonic()
                    print("Serial connected; handshake sent.")
                except serial.SerialException as exc:
                    print(
                        f"Serial unavailable ({exc}); retrying in "
                        f"{RECONNECT_DELAY_S:.0f}s...",
                        file=sys.stderr,
                    )
                    time.sleep(RECONNECT_DELAY_S)
                    continue

            try:
                now = time.monotonic()
                if now - last_heartbeat >= HEARTBEAT_INTERVAL_S:
                    send_line(
                        device,
                        f"AL_HOST,HEARTBEAT,{PROTOCOL_VERSION}",
                    )
                    last_heartbeat = now

                raw_line = device.readline()
                if not raw_line:
                    continue

                line = raw_line.decode(
                    "utf-8",
                    errors="replace",
                ).strip()

                if line == f"AL_DIAG,EVENT,HOST_READY,{PROTOCOL_VERSION}":
                    print("ArrowLab confirms PC LOGGER CONNECTED.")
                    continue

                if line.startswith("AL_DIAG,EVENT,"):
                    print(line)

                    if line.startswith("AL_DIAG,EVENT,REPLAY_END,"):
                        parts = line.split(",")
                        if len(parts) == 6:
                            boot_id = int(parts[3])
                            run_id = int(parts[4])
                            expected = int(parts[5])
                            captured = sum(
                                1
                                for key in seen_samples
                                if key[0] == boot_id and key[1] == run_id
                            )

                            if captured == expected:
                                csv_file.flush()
                                send_line(
                                    device,
                                    "AL_HOST,ACK,"
                                    f"{boot_id},{run_id},{expected}",
                                )
                                print(
                                    f"Run {run_id} verified and acknowledged: "
                                    f"{expected} samples saved."
                                )
                            else:
                                send_line(
                                    device,
                                    f"AL_HOST,REPLAY,{boot_id},{run_id}",
                                )
                                print(
                                    f"Run {run_id} incomplete on PC "
                                    f"({captured}/{expected}); replay requested."
                                )

                    if line == "AL_DIAG,EVENT,SESSION_COMPLETE":
                        print(
                            f"Diagnostic session complete. "
                            f"{row_count} unique data rows saved."
                        )
                        if output is not None:
                            print(f"CSV: {output}")
                        return 0

                    continue

                if not line.startswith("AL_DIAG,DATA,"):
                    continue

                parts = line.split(",")
                if len(parts) != 12:
                    print(
                        f"Ignored malformed diagnostic line: {line}",
                        file=sys.stderr,
                    )
                    continue

                boot_id = int(parts[2])
                run_id = int(parts[3])
                sample_index = int(parts[4])
                sample_key = (boot_id, run_id, sample_index)
                side = parts[5]

                if side not in {"LEFT", "RIGHT"}:
                    print(
                        f"Ignored unknown diagnostic side: {side}",
                        file=sys.stderr,
                    )
                    continue

                if active_side is None:
                    active_side = side
                    if output is None:
                        output = default_output(side, stamp)
                    output.parent.mkdir(parents=True, exist_ok=True)
                    csv_file = output.open(
                        "w",
                        newline="",
                        encoding="utf-8",
                    )
                    writer = csv.writer(csv_file)
                    writer.writerow(CSV_HEADER)
                    csv_file.flush()
                    print(f"CSV: {output}")
                elif side != active_side:
                    print(
                        "Ignored opposite-side data. Close this logger "
                        f"before starting {side}; this file is "
                        f"{active_side}-only.",
                        file=sys.stderr,
                    )
                    continue

                if sample_key in seen_samples:
                    continue

                assert writer is not None
                assert csv_file is not None
                writer.writerow(
                    parts[2:]
                    + [
                        datetime.now(timezone.utc).isoformat(
                            timespec="milliseconds"
                        )
                    ]
                )
                csv_file.flush()
                seen_samples.add(sample_key)
                row_count += 1

                run_type = parts[6]
                mass = parts[7]
                elapsed_s = int(parts[8]) / 1000.0
                raw_count = parts[9]
                reference_raw = parts[10]
                delta_count = parts[11]

                print(
                    f"#{row_count:04d} run={run_id} {side:5s} "
                    f"{run_type:4s} {mass} g t={elapsed_s:7.1f}s "
                    f"raw={raw_count} reference={reference_raw} "
                    f"delta={delta_count}"
                )

            except serial.SerialException as exc:
                print(
                    f"Serial connection lost ({exc}). Run data remains "
                    "buffered on ArrowLab; reconnecting...",
                    file=sys.stderr,
                )
                try:
                    device.close()
                except serial.SerialException:
                    pass
                device = None

    except KeyboardInterrupt:
        print(
            f"\nLogger closed cleanly. {row_count} unique data rows saved."
        )
        if output is not None:
            print(f"CSV: {output}")
        return 0
    finally:
        if csv_file is not None:
            csv_file.close()
        if device is not None:
            device.close()


if __name__ == "__main__":
    raise SystemExit(main())
