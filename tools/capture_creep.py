#!/usr/bin/env python3
"""Capture ArrowLab creep diagnostic records from USB serial to CSV."""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path
import sys

try:
    import serial
except ImportError:
    print(
        "pyserial is required. Install with: py -m pip install pyserial",
        file=sys.stderr,
    )
    raise SystemExit(2)


CSV_HEADER = [
    "run_id",
    "side",
    "test_mass_g",
    "elapsed_ms",
    "raw_count",
    "zeroed_count",
    "calculated_g",
    "calibration_factor",
    "host_timestamp_utc",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture ArrowLab AL_DIAG serial records to CSV."
    )
    parser.add_argument("port", help="Serial port, for example COM7")
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


def default_output() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return (
        Path("calibration")
        / "diagnostics"
        / f"creep_{stamp}.csv"
    )


def main() -> int:
    args = parse_args()
    output = args.output or default_output()
    output.parent.mkdir(parents=True, exist_ok=True)

    print(f"ArrowLab diagnostic capture: {args.port} @ {args.baud}")
    print(f"CSV: {output}")
    print("Leave this running for all diagnostic runs. Ctrl+C stops capture.")

    row_count = 0

    try:
        # Configure modem-control lines before opening the port.
        # The ESP32 upload/reset circuit may react to DTR/RTS changes;
        # diagnostic capture must not deliberately request a reset.
        device = serial.Serial()
        device.port = args.port
        device.baudrate = args.baud
        device.timeout = 1
        device.dtr = False
        device.rts = False
        device.open()

        with device, output.open(
            "w",
            newline="",
            encoding="utf-8",
        ) as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(CSV_HEADER)
            csv_file.flush()

            while True:
                raw_line = device.readline()
                if not raw_line:
                    continue

                line = raw_line.decode(
                    "utf-8",
                    errors="replace",
                ).strip()

                if line.startswith("AL_DIAG,EVENT,"):
                    print(line)

                    if line == "AL_DIAG,EVENT,SESSION_COMPLETE":
                        print(
                            f"Diagnostic session complete. "
                            f"{row_count} data rows saved."
                        )
                        print(f"CSV: {output}")
                        return 0

                    continue

                if not line.startswith("AL_DIAG,DATA,"):
                    continue

                parts = line.split(",")

                if len(parts) != 10:
                    print(
                        f"Ignored malformed diagnostic line: {line}",
                        file=sys.stderr,
                    )
                    continue

                writer.writerow(
                    parts[2:]
                    + [
                        datetime.now(timezone.utc)
                        .isoformat(timespec="milliseconds")
                    ]
                )
                csv_file.flush()
                row_count += 1

                run_id = parts[2]
                side = parts[3]
                mass = parts[4]
                elapsed_s = int(parts[5]) / 1000.0
                grams = parts[8]

                print(
                    f"#{row_count:04d}  run={run_id}  "
                    f"{side:5s}  {mass} g  t={elapsed_s:7.1f}s  "
                    f"reading={grams} g"
                )

    except KeyboardInterrupt:
        print(f"\nCapture stopped cleanly. {row_count} data rows saved.")
        print(f"CSV: {output}")
        return 0
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
